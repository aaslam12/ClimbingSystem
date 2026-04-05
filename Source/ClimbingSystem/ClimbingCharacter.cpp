// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingCharacter.h"
#include "ClimbingMovementComponent.h"
#include "ClimbingAnimInstance.h"
#include "ClimbingAnimationSet.h"
#include "ClimbingSurfaceData.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "MotionWarpingComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

// Static IK manager array - game thread access only
TArray<TWeakObjectPtr<AClimbingCharacter>> AClimbingCharacter::ActiveClimbingCharacters;

AClimbingCharacter::AClimbingCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UClimbingMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Get reference to our custom movement component
	ClimbingMovement = Cast<UClimbingMovementComponent>(GetCharacterMovement());

	// Create motion warping component
	MotionWarping = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));

	// Create camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Configure character rotation
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void AClimbingCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Cache original capsule dimensions for restoration when exiting climbing
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		OriginalCapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
		OriginalCapsuleRadius = Capsule->GetUnscaledCapsuleRadius();
		OriginalCollisionProfile = Capsule->GetCollisionProfileName();
	}

	// Cache animation instance
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		CachedAnimInstance = Cast<UClimbingAnimInstance>(MeshComp->GetAnimInstance());
	}

	// Register with IK manager (game thread access only - this is in BeginPlay which is game thread)
	RegisterWithIKManager();

	// Validate animation slots
	ValidateAnimationSlots();

	// Motion Warping component validation - warp windows are curve-driven via MotionWarp float curves
	if (!MotionWarping)
	{
		UE_LOG(LogClimbing, Warning, TEXT("ClimbingCharacter '%s': MotionWarpingComponent not found. Motion warping animations will not work correctly."), *GetName());
	}

	// Start detection timer for ground locomotion
	// bIgnorePause = false (default) - climbing system fully pauses with game
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			DetectionTimerHandle,
			[this]() 
			{ 
				// Only run detection during ground locomotion (not while climbing)
				if (ClimbingMovement && ClimbingMovement->CurrentClimbingState == EClimbingState::None)
				{
					// Cache detection result for potential grab input
					CurrentDetectionResult = PerformLedgeDetection();
					
					// If ledge detection failed, try ladder detection
					if (!CurrentDetectionResult.bValid)
					{
						CurrentDetectionResult = PerformLadderDetection();
					}
				}
			},
			DetectionScanInterval,
			true  // bLoop
		);
	}
}

void AClimbingCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PerformCleanup();
	Super::EndPlay(EndPlayReason);
}

void AClimbingCharacter::Destroyed()
{
	PerformCleanup();
	Super::Destroyed();
}

void AClimbingCharacter::PerformCleanup()
{
	// 1. Remove self from ActiveClimbingCharacters (game thread access only)
	ActiveClimbingCharacters.RemoveAll([this](const TWeakObjectPtr<AClimbingCharacter>& Ptr)
	{
		return !Ptr.IsValid() || Ptr.Get() == this;
	});

	// 2. SetBase(nullptr)
	SetBase(nullptr);

	// 3. Montage_Stop(0.0f)
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
		{
			AnimInstance->Montage_Stop(0.0f);
		}
	}

	// 4. Clear LockedLacheTarget; if LacheInAir -> DroppingDown
	LockedLacheTarget.Reset();
	if (ClimbingMovement && ClimbingMovement->CurrentClimbingState == EClimbingState::LacheInAir)
	{
		ClimbingMovement->SetClimbingState(EClimbingState::DroppingDown);
	}

	// 5. If mesh simulating physics -> SetSimulatePhysics(false)
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (MeshComp->IsSimulatingPhysics())
		{
			MeshComp->SetSimulatePhysics(false);
		}
	}

	// 6. Remove ClimbingInputMappingContext from subsystem if present
	RemoveClimbingInputMappingContext();

	// Clear timers
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DetectionTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(FallingGrabTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(RagdollRecoveryTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(IdleVariationTimerHandle);
	}
}

void AClimbingCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Tick climbing state logic
	if (ClimbingMovement && ClimbingMovement->CurrentClimbingState != EClimbingState::None)
	{
		TickClimbingState(DeltaTime);
	}

	// Update coyote time
	if (CoyoteTimeRemaining > 0.0f)
	{
		CoyoteTimeRemaining -= DeltaTime;
	}

#if WITH_EDITOR
	// Editor-only Lache arc preview
	if (bDrawDebug && IsSelected() && GetWorld() && !GetWorld()->IsGameWorld())
	{
		// Draw Lache arc preview using parabola formula
		// GravityZ is negative - use directly, do not Abs or negate
		if (ClimbingMovement)
		{
			const FVector LaunchOrigin = GetActorLocation();
			const FVector ArcVelocity = GetActorForwardVector() * LacheLaunchSpeed;
			const float GravityZ = ClimbingMovement->GetGravityZ();

			FVector PrevPos = LaunchOrigin;
			for (int32 i = 1; i <= LacheArcTraceSteps; ++i)
			{
				const float t = i * (LacheTotalArcTime / LacheArcTraceSteps);
				const FVector StepPos = LaunchOrigin
					+ ArcVelocity * t
					+ FVector(0.f, 0.f, 0.5f * GravityZ * t * t);

				DrawDebugLine(GetWorld(), PrevPos, StepPos, FColor::Cyan, false, -1.0f, 0, 2.0f);
				DrawDebugSphere(GetWorld(), StepPos, LacheArcTraceRadius, 8, FColor::Cyan, false, -1.0f);
				PrevPos = StepPos;
			}
		}
	}
#endif
}

void AClimbingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Bind climbing inputs - all using Enhanced Input, zero legacy BindAxis/BindAction calls
		if (IA_Grab)
		{
			EnhancedInput->BindAction(IA_Grab, ETriggerEvent::Triggered, this, &AClimbingCharacter::Input_Grab);
		}
		if (IA_Drop)
		{
			EnhancedInput->BindAction(IA_Drop, ETriggerEvent::Triggered, this, &AClimbingCharacter::Input_Drop);
		}
		if (IA_Lache)
		{
			EnhancedInput->BindAction(IA_Lache, ETriggerEvent::Triggered, this, &AClimbingCharacter::Input_Lache);
		}
		if (IA_ClimbUp)
		{
			EnhancedInput->BindAction(IA_ClimbUp, ETriggerEvent::Triggered, this, &AClimbingCharacter::Input_ClimbUp);
		}
		if (IA_ClimbMove)
		{
			EnhancedInput->BindAction(IA_ClimbMove, ETriggerEvent::Ongoing, this, &AClimbingCharacter::Input_ClimbMove);
			EnhancedInput->BindAction(IA_ClimbMove, ETriggerEvent::Completed, this, &AClimbingCharacter::Input_ClimbMoveCompleted);
		}
		if (IA_Sprint)
		{
			EnhancedInput->BindAction(IA_Sprint, ETriggerEvent::Ongoing, this, &AClimbingCharacter::Input_Sprint);
			EnhancedInput->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &AClimbingCharacter::Input_SprintCompleted);
		}
		if (IA_Crouch)
		{
			EnhancedInput->BindAction(IA_Crouch, ETriggerEvent::Ongoing, this, &AClimbingCharacter::Input_Crouch);
			EnhancedInput->BindAction(IA_Crouch, ETriggerEvent::Completed, this, &AClimbingCharacter::Input_CrouchCompleted);
		}
	}
}

void AClimbingCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Note: Most climbing replication is handled by UClimbingMovementComponent
	// Character-level replication could be added here if needed
}

// ============================================================================
// Input Handlers
// ============================================================================

void AClimbingCharacter::Input_Grab(const FInputActionValue& Value)
{
	if (!ClimbingMovement)
	{
		return;
	}

	// Handle coyote time re-grab
	if (bEnableCoyoteTime && CoyoteTimeRemaining > 0.0f && ClimbingMovement->CurrentClimbingState == EClimbingState::None)
	{
		// Re-run detection
		FClimbingDetectionResult DetectionResult = PerformLedgeDetection();
		if (!DetectionResult.bValid)
		{
			DetectionResult = PerformLadderDetection();
		}
		
		if (DetectionResult.bValid)
		{
			// Initiate grab
			if (HasAuthority())
			{
				TransitionToState(DetectionResult.SurfaceTier == EClimbSurfaceTier::LadderOnly ? 
					EClimbingState::OnLadder : EClimbingState::Hanging, DetectionResult);
			}
			else
			{
				// Client prediction + server RPC
				PrePredictionPosition = GetActorLocation();
				TransitionToState(DetectionResult.SurfaceTier == EClimbSurfaceTier::LadderOnly ? 
					EClimbingState::OnLadder : EClimbingState::Hanging, DetectionResult);
				Server_AttemptGrab(DetectionResult.ToNetResult());
			}
			CoyoteTimeRemaining = 0.0f;
			return;
		}
	}

	// Normal grab from ground/falling
	if (ClimbingMovement->CurrentClimbingState == EClimbingState::None)
	{
		// Use cached detection result or re-run if falling
		FClimbingDetectionResult DetectionResult = CurrentDetectionResult;
		
		if (ClimbingMovement->IsFalling())
		{
			// Re-run detection for falling grab
			if (bEnableFallingGrab)
			{
				DetectionResult = PerformLedgeDetection();
				if (!DetectionResult.bValid)
				{
					DetectionResult = PerformLadderDetection();
				}
			}
		}
		
		if (DetectionResult.bValid)
		{
			// Initiate grab
			if (HasAuthority())
			{
				TransitionToState(DetectionResult.SurfaceTier == EClimbSurfaceTier::LadderOnly ? 
					EClimbingState::OnLadder : EClimbingState::Hanging, DetectionResult);
			}
			else
			{
				// Client prediction + server RPC
				PrePredictionPosition = GetActorLocation();
				TransitionToState(DetectionResult.SurfaceTier == EClimbSurfaceTier::LadderOnly ? 
					EClimbingState::OnLadder : EClimbingState::Hanging, DetectionResult);
				Server_AttemptGrab(DetectionResult.ToNetResult());
			}
		}
	}
}

void AClimbingCharacter::Input_Drop(const FInputActionValue& Value)
{
	if (!ClimbingMovement)
	{
		return;
	}

	const EClimbingState CurrentState = ClimbingMovement->CurrentClimbingState;
	
	// Drop is valid from: Hanging, Shimmying, BracedWall, BracedShimmying, OnLadder
	if (CurrentState == EClimbingState::Hanging ||
		CurrentState == EClimbingState::Shimmying ||
		CurrentState == EClimbingState::BracedWall ||
		CurrentState == EClimbingState::BracedShimmying ||
		CurrentState == EClimbingState::OnLadder)
	{
		if (!ClimbingMovement->CanInterruptCurrentState())
		{
			return;
		}

		// Enable coyote time
		if (bEnableCoyoteTime)
		{
			CoyoteTimeRemaining = CoyoteTimeWindow;
		}

		if (HasAuthority())
		{
			TransitionToState(EClimbingState::DroppingDown, FClimbingDetectionResult());
		}
		else
		{
			TransitionToState(EClimbingState::DroppingDown, FClimbingDetectionResult());
			Server_Drop();
		}
	}
}

void AClimbingCharacter::Input_Lache(const FInputActionValue& Value)
{
	if (!ClimbingMovement)
	{
		return;
	}

	const EClimbingState CurrentState = ClimbingMovement->CurrentClimbingState;

	// Lache is valid from Hanging only
	if (CurrentState != EClimbingState::Hanging)
	{
		return;
	}

	if (!ClimbingMovement->CanInterruptCurrentState())
	{
		return;
	}

	// Calculate Lache arc and check for valid target
	FClimbingDetectionResult LacheTarget = CalculateLacheArc();
	
	if (!LacheTarget.bValid)
	{
		// No valid target - play grunt but don't launch
		PlayClimbingSound(EClimbSoundType::LacheLaunchGrunt);
		
#if !UE_BUILD_SHIPPING
		if (bDrawDebug)
		{
			UE_LOG(LogClimbing, Log, TEXT("Lache: No valid target found"));
		}
#endif
		return;
	}

	// Lock target and initiate Lache
	LockedLacheTarget = LacheTarget;
	LacheLaunchPosition = GetActorLocation();
	LacheLaunchDirection = GetActorForwardVector();
	LacheFlightTime = 0.0f;

	// Auto-cinematic camera if enabled and target far enough
	if (bAutoLacheCinematic)
	{
		const float DistanceToTarget = FVector::Dist(GetActorLocation(), LacheTarget.LedgePosition);
		if (DistanceToTarget >= LacheCinematicDistanceThreshold)
		{
			// Calculate cinematic camera position
			const FVector CameraLocation = (GetActorLocation() + LacheTarget.LedgePosition) * 0.5f + FVector(0.f, 0.f, 200.f);
			const FRotator CameraRotation = (LacheTarget.LedgePosition - CameraLocation).Rotation();
			LockCameraToFrame(CameraLocation, CameraRotation, 0.3f);
		}
	}

	if (HasAuthority())
	{
		TransitionToState(EClimbingState::Lache, LacheTarget);
	}
	else
	{
		PrePredictionPosition = GetActorLocation();
		TransitionToState(EClimbingState::Lache, LacheTarget);
		Server_AttemptLache(LacheTarget.LedgePosition);
	}
}

FClimbingDetectionResult AClimbingCharacter::CalculateLacheArc() const
{
	FClimbingDetectionResult Result;

	if (!GetWorld() || !ClimbingMovement)
	{
		return Result;
	}

	const FVector LaunchOrigin = GetActorLocation();
	const FVector ArcVelocity = GetActorForwardVector() * LacheLaunchSpeed;
	const float GravityZ = ClimbingMovement->GetGravityZ(); // Negative value - use directly, do not Abs

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;

	FVector PrevPos = LaunchOrigin;

	// Trace along arc
	for (int32 i = 1; i <= LacheArcTraceSteps; ++i)
	{
		const float t = i * (LacheTotalArcTime / LacheArcTraceSteps);
		const FVector StepPos = LaunchOrigin
			+ ArcVelocity * t
			+ FVector(0.f, 0.f, 0.5f * GravityZ * t * t);

		FHitResult Hit;
		bool bHit = GetWorld()->SweepSingleByChannel(
			Hit,
			PrevPos,
			StepPos,
			FQuat::Identity,
			ECC_WorldStatic,
			FCollisionShape::MakeSphere(LacheArcTraceRadius),
			QueryParams
		);

#if !UE_BUILD_SHIPPING
		if (bDrawDebug)
		{
			DrawDebugLine(GetWorld(), PrevPos, StepPos, bHit ? FColor::Yellow : FColor::Cyan, false, 0.5f);
		}
#endif

		if (bHit && Hit.Component.IsValid())
		{
			// Check if this is a climbable surface
			const UClimbingSurfaceData* SurfaceData = GetSurfaceDataFromComponent(Hit.Component.Get());
			
			// Determine surface tier
			EClimbSurfaceTier Tier = EClimbSurfaceTier::Untagged;
			if (SurfaceData)
			{
				Tier = SurfaceData->SurfaceTier;
			}
			else if (Hit.Component->ComponentHasTag(FName("Climbable")))
			{
				Tier = EClimbSurfaceTier::Climbable;
			}
			else if (Hit.Component->ComponentHasTag(FName("Unclimbable")))
			{
				// Obstacle in path - cannot Lache
				return Result;
			}

			// Final step or climbable surface hit
			if (i == LacheArcTraceSteps || Tier == EClimbSurfaceTier::Climbable || Tier == EClimbSurfaceTier::Untagged)
			{
				// Check for ledge near impact point
				FClimbingDetectionResult LedgeCheck = PerformLedgeDetectionAtLocation(Hit.ImpactPoint);
				if (LedgeCheck.bValid)
				{
					return LedgeCheck;
				}
			}

			// Blocked by non-climbable - fail
			if (Tier == EClimbSurfaceTier::Unclimbable)
			{
				return Result;
			}
		}

		PrevPos = StepPos;
	}

	return Result;
}

FClimbingDetectionResult AClimbingCharacter::PerformLedgeDetectionAtLocation(const FVector& Location) const
{
	FClimbingDetectionResult Result;

	if (!GetWorld())
	{
		return Result;
	}

	// Similar to PerformLedgeDetection but centered at the specified location
	const FVector UpVector = FVector::UpVector;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = true;

	// Trace upward from location
	const FVector UpTraceStart = Location;
	const FVector UpTraceEnd = Location + UpVector * LedgeDetectionVerticalReach;

	FHitResult UpHit;
	bool bUpBlocked = GetWorld()->SweepSingleByChannel(
		UpHit,
		UpTraceStart,
		UpTraceEnd,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(LedgeDetectionRadius),
		QueryParams
	);

	if (bUpBlocked)
	{
		// Blocked above - use the hit point as reference
		const FVector DownTraceStart = UpHit.ImpactPoint + UpVector * 10.0f;
		const FVector DownTraceEnd = DownTraceStart - UpVector * (LedgeDetectionVerticalReach + 20.0f);

		FHitResult DownHit;
		bool bFoundLedge = GetWorld()->SweepSingleByChannel(
			DownHit,
			DownTraceStart,
			DownTraceEnd,
			FQuat::Identity,
			ECC_WorldStatic,
			FCollisionShape::MakeSphere(LedgeDetectionRadius),
			QueryParams
		);

		if (bFoundLedge && DownHit.Component.IsValid())
		{
			Result.LedgePosition = DownHit.ImpactPoint;
			Result.SurfaceNormal = DownHit.ImpactNormal;
			Result.HitComponent = DownHit.Component;
			Result.bValid = true;

			// Determine surface tier
			const UClimbingSurfaceData* SurfaceData = GetSurfaceDataFromComponent(DownHit.Component.Get());
			if (SurfaceData)
			{
				Result.SurfaceTier = SurfaceData->SurfaceTier;
			}
			else
			{
				Result.SurfaceTier = EClimbSurfaceTier::Untagged;
			}

			Result.ClearanceType = EClimbClearanceType::Full; // Simplified for Lache
		}
	}
	else
	{
		// Not blocked - trace down from above
		const FVector DownTraceStart = UpTraceEnd;
		const FVector DownTraceEnd = Location;

		FHitResult DownHit;
		bool bFoundLedge = GetWorld()->SweepSingleByChannel(
			DownHit,
			DownTraceStart,
			DownTraceEnd,
			FQuat::Identity,
			ECC_WorldStatic,
			FCollisionShape::MakeSphere(LedgeDetectionRadius),
			QueryParams
		);

		if (bFoundLedge && DownHit.Component.IsValid())
		{
			Result.LedgePosition = DownHit.ImpactPoint;
			Result.SurfaceNormal = DownHit.ImpactNormal;
			Result.HitComponent = DownHit.Component;
			Result.bValid = true;

			const UClimbingSurfaceData* SurfaceData = GetSurfaceDataFromComponent(DownHit.Component.Get());
			if (SurfaceData)
			{
				Result.SurfaceTier = SurfaceData->SurfaceTier;
			}
			else
			{
				Result.SurfaceTier = EClimbSurfaceTier::Untagged;
			}

			Result.ClearanceType = EClimbClearanceType::Full;
		}
	}

	return Result;
}

void AClimbingCharacter::Input_ClimbUp(const FInputActionValue& Value)
{
	if (!ClimbingMovement)
	{
		return;
	}

	const EClimbingState CurrentState = ClimbingMovement->CurrentClimbingState;
	
	// Climb up is valid from Hanging
	if (CurrentState != EClimbingState::Hanging)
	{
		return;
	}

	if (!ClimbingMovement->CanInterruptCurrentState())
	{
		return;
	}

	// Check clearance from current detection result
	const EClimbClearanceType Clearance = CurrentDetectionResult.ClearanceType;
	
	if (Clearance == EClimbClearanceType::None)
	{
		// Cannot climb up - no clearance
		return;
	}

	const EClimbingState ClimbUpState = (Clearance == EClimbClearanceType::Full) ? 
		EClimbingState::ClimbingUp : EClimbingState::ClimbingUpCrouch;

	if (HasAuthority())
	{
		TransitionToState(ClimbUpState, CurrentDetectionResult);
	}
	else
	{
		PrePredictionPosition = GetActorLocation();
		TransitionToState(ClimbUpState, CurrentDetectionResult);
		Server_AttemptClimbUp();
	}
}

void AClimbingCharacter::Input_ClimbMove(const FInputActionValue& Value)
{
	CurrentClimbMoveInput = Value.Get<FVector2D>();
	
	// Update committed shimmy direction with hysteresis
	if (FMath::Abs(CurrentClimbMoveInput.X) > ShimmyDirectionDeadzone)
	{
		CommittedShimmyDir = FMath::Sign(CurrentClimbMoveInput.X);
	}
	// else: CommittedShimmyDir unchanged — last direction maintained
}

void AClimbingCharacter::Input_ClimbMoveCompleted(const FInputActionValue& Value)
{
	CurrentClimbMoveInput = FVector2D::ZeroVector;
}

void AClimbingCharacter::Input_Sprint(const FInputActionValue& Value)
{
	bSprintModifierActive = true;
}

void AClimbingCharacter::Input_SprintCompleted(const FInputActionValue& Value)
{
	bSprintModifierActive = false;
}

void AClimbingCharacter::Input_Crouch(const FInputActionValue& Value)
{
	bCrouchModifierActive = true;
}

void AClimbingCharacter::Input_CrouchCompleted(const FInputActionValue& Value)
{
	bCrouchModifierActive = false;
}

// ============================================================================
// State Management
// ============================================================================

void AClimbingCharacter::TransitionToState(EClimbingState NewState, const FClimbingDetectionResult& DetectionResult)
{
	if (!ClimbingMovement)
	{
		return;
	}

	const EClimbingState OldState = ClimbingMovement->CurrentClimbingState;

	// Validate transition
	if (!ClimbingMovement->IsValidStateTransition(NewState))
	{
		UE_LOG(LogClimbing, Verbose, TEXT("ClimbingCharacter '%s': Invalid transition from %s to %s"),
			*GetName(),
			*UEnum::GetValueAsString(OldState),
			*UEnum::GetValueAsString(NewState));
		return;
	}

	// Exit old state
	OnStateExit(OldState);

	// Update state in movement component (handles replication)
	ClimbingMovement->SetClimbingState(NewState);

	// Store detection result
	CurrentDetectionResult = DetectionResult;

	// Enter new state
	OnStateEnter(NewState, DetectionResult);

	// Update animation instance
	if (UClimbingAnimInstance* AnimInstance = CachedAnimInstance.Get())
	{
		AnimInstance->PreviousClimbingState = OldState;
		AnimInstance->CurrentClimbingState = NewState;
	}

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan,
			FString::Printf(TEXT("State: %s -> %s"),
				*UEnum::GetValueAsString(OldState),
				*UEnum::GetValueAsString(NewState)));
	}
#endif
}

void AClimbingCharacter::OnStateEnter(EClimbingState NewState, const FClimbingDetectionResult& DetectionResult)
{
	// Handle IMC on first climbing state entry
	if (ClimbingMovement && ClimbingMovement->PreviousClimbingState == EClimbingState::None && NewState != EClimbingState::None)
	{
		AddClimbingInputMappingContext();
	}

	// Update capsule for climbing states
	if (NewState != EClimbingState::None && NewState != EClimbingState::Ragdoll)
	{
		if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			Capsule->SetCapsuleSize(ClimbingCapsuleRadius, ClimbingCapsuleHalfHeight);
			Capsule->SetCollisionProfileName(ClimbingCollisionProfile);
		}
	}

	// Update surface data and animation override
	if (DetectionResult.bValid && DetectionResult.HitComponent.IsValid())
	{
		CurrentSurfaceData = GetSurfaceDataFromComponent(DetectionResult.HitComponent.Get());
		
		// Load animation set override if present
		if (CurrentSurfaceData.IsValid() && CurrentSurfaceData->AnimationSetOverride.ToSoftObjectPath().IsValid())
		{
			// Async load the animation set
			FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
			StreamableManager.RequestAsyncLoad(
				CurrentSurfaceData->AnimationSetOverride.ToSoftObjectPath(),
				FStreamableDelegate::CreateWeakLambda(this, [this]()
				{
					if (CurrentSurfaceData.IsValid())
					{
						CurrentAnimationSetOverride = CurrentSurfaceData->AnimationSetOverride.Get();
					}
				})
			);
		}
		else
		{
			CurrentAnimationSetOverride = nullptr;
		}

		// Set up anchor
		if (ClimbingMovement)
		{
			ClimbingMovement->SetAnchor(DetectionResult.HitComponent.Get(), DetectionResult.LedgePosition);
		}
	}

	// State-specific entry logic
	switch (NewState)
	{
	case EClimbingState::Hanging:
		{
			// Reset shimmy distance tracker
			ContinuousShimmyDistance = 0.0f;
			CommittedShimmyDir = 0.0f;
			IdleTimer = 0.0f;
			
			// Play grab animation if coming from None
			if (ClimbingMovement && ClimbingMovement->PreviousClimbingState == EClimbingState::None)
			{
				if (UAnimMontage* GrabMontage = GetMontageForSlot(EClimbingAnimationSlot::GrabLedge))
				{
					PlayAnimMontage(GrabMontage);
					
					// Set up motion warp target
					if (MotionWarping && DetectionResult.bValid)
					{
						FMotionWarpingTarget WarpTarget;
						WarpTarget.Name = FName("WarpTarget_LedgeGrab");
						WarpTarget.Location = DetectionResult.LedgePosition;
						WarpTarget.Rotation = (-DetectionResult.SurfaceNormal).Rotation();
						MotionWarping->AddOrUpdateWarpTarget(WarpTarget);
					}
				}
			}
			else
			{
				// Play idle animation
				if (UAnimMontage* IdleMontage = GetMontageForSlot(EClimbingAnimationSlot::HangIdle))
				{
					PlayAnimMontage(IdleMontage);
				}
			}

			// Start idle variation timer
			if (GetWorld())
			{
				GetWorld()->GetTimerManager().SetTimer(
					IdleVariationTimerHandle,
					[this]()
					{
						PlayIdleVariation();
					},
					IdleVariationDelay,
					false  // No loop - reset in variation callback
				);
			}
		}
		break;

	case EClimbingState::OnLadder:
		{
			// Play enter animation based on entry direction
			// For now, assume bottom entry
			if (UAnimMontage* EnterMontage = GetMontageForSlot(EClimbingAnimationSlot::LadderEnterBottom))
			{
				PlayAnimMontage(EnterMontage);
				
				if (MotionWarping && DetectionResult.bValid)
				{
					FMotionWarpingTarget WarpTarget;
					WarpTarget.Name = FName("WarpTarget_LadderEnterBottom");
					WarpTarget.Location = DetectionResult.LedgePosition;
					WarpTarget.Rotation = (-DetectionResult.SurfaceNormal).Rotation();
					MotionWarping->AddOrUpdateWarpTarget(WarpTarget);
				}
			}
		}
		break;

	case EClimbingState::BracedWall:
		{
			ContinuousShimmyDistance = 0.0f;
			CommittedShimmyDir = 0.0f;
			
			if (UAnimMontage* BracedIdleMontage = GetMontageForSlot(EClimbingAnimationSlot::BracedIdle))
			{
				PlayAnimMontage(BracedIdleMontage);
			}

			// SetBase is called in SetAnchor for braced states
		}
		break;

	case EClimbingState::DroppingDown:
		{
			if (UAnimMontage* DropMontage = GetMontageForSlot(EClimbingAnimationSlot::DropDown))
			{
				PlayAnimMontage(DropMontage);
			}
		}
		break;

	case EClimbingState::Ragdoll:
		{
			// Enable ragdoll physics
			if (USkeletalMeshComponent* MeshComp = GetMesh())
			{
				MeshComp->SetSimulatePhysics(true);
				MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}

			// Disable capsule collision during ragdoll
			if (UCapsuleComponent* Capsule = GetCapsuleComponent())
			{
				Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}

			// Release camera lock and attach to pelvis
			ReleaseCameraLock(0.1f);
			if (CameraBoom)
			{
				CameraBoom->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, RagdollCameraTargetSocket);
			}

			// Set up recovery timer
			// bIgnorePause = false (default) - climbing system fully pauses with game
			if (GetWorld())
			{
				GetWorld()->GetTimerManager().SetTimer(
					RagdollRecoveryTimerHandle,
					[this]()
					{
						RecoverFromRagdoll();
					},
					RagdollRecoveryTime,
					false  // No loop
				);
			}
		}
		break;

	default:
		break;
	}

	// Update animation instance with surface data
	if (UClimbingAnimInstance* AnimInstance = CachedAnimInstance.Get())
	{
		if (DetectionResult.bValid)
		{
			AnimInstance->CurrentSurfaceNormal = DetectionResult.SurfaceNormal;
			AnimInstance->CurrentLedgePosition = DetectionResult.LedgePosition;
		}
	}

	// Play appropriate sound
	if (NewState == EClimbingState::Hanging || NewState == EClimbingState::BracedWall)
	{
		PlayClimbingSound(EClimbSoundType::HandGrab);
	}
}

void AClimbingCharacter::OnStateExit(EClimbingState OldState)
{
	// Handle IMC on climbing exit
	if (ClimbingMovement && ClimbingMovement->CurrentClimbingState != EClimbingState::None && OldState == EClimbingState::None)
	{
		// This case shouldn't happen - we're entering climbing, not exiting
	}

	// Clear timers
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(IdleVariationTimerHandle);
	}

	// State-specific exit logic
	switch (OldState)
	{
	case EClimbingState::Hanging:
	case EClimbingState::Shimmying:
	case EClimbingState::BracedWall:
	case EClimbingState::BracedShimmying:
	case EClimbingState::OnLadder:
		{
			// Clear SetBase if transitioning to non-climbing state
			EClimbingState NewState = ClimbingMovement ? ClimbingMovement->CurrentClimbingState : EClimbingState::None;
			if (NewState == EClimbingState::None || NewState == EClimbingState::DroppingDown)
			{
				SetBase(nullptr);
				
				// Clear anchor
				if (ClimbingMovement)
				{
					ClimbingMovement->ClearAnchor();
				}
			}
		}
		break;

	case EClimbingState::Ragdoll:
		{
			// Re-attach camera to root
			if (CameraBoom)
			{
				CameraBoom->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			}

			// Re-enable capsule
			if (UCapsuleComponent* Capsule = GetCapsuleComponent())
			{
				Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}
		}
		break;

	default:
		break;
	}

	// If exiting all climbing, restore capsule
	EClimbingState NewState = ClimbingMovement ? ClimbingMovement->CurrentClimbingState : EClimbingState::None;
	if (NewState == EClimbingState::None)
	{
		RemoveClimbingInputMappingContext();

		if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			Capsule->SetCapsuleSize(OriginalCapsuleRadius, OriginalCapsuleHalfHeight);
			Capsule->SetCollisionProfileName(OriginalCollisionProfile);
		}

		// Clear surface data
		CurrentSurfaceData.Reset();
		CurrentAnimationSetOverride = nullptr;
	}
}

void AClimbingCharacter::TickClimbingState(float DeltaTime)
{
	if (!ClimbingMovement)
	{
		return;
	}

	const EClimbingState CurrentState = ClimbingMovement->CurrentClimbingState;

	// Update detection during active climbing states (every tick)
	if (CurrentState == EClimbingState::Hanging ||
		CurrentState == EClimbingState::Shimmying ||
		CurrentState == EClimbingState::BracedWall ||
		CurrentState == EClimbingState::BracedShimmying ||
		CurrentState == EClimbingState::OnLadder)
	{
		CurrentDetectionResult = PerformLedgeDetection();
		if (!CurrentDetectionResult.bValid)
		{
			CurrentDetectionResult = PerformLadderDetection();
		}

		// Owning client during active climbing: use local scan HitComponent directly
		// Skip confirmation trace - it's only needed for simulated proxies
	}

	// State-specific tick logic
	switch (CurrentState)
	{
	case EClimbingState::Hanging:
		TickHangingState(DeltaTime);
		break;

	case EClimbingState::Shimmying:
		TickShimmyingState(DeltaTime);
		break;

	case EClimbingState::BracedWall:
		TickBracedWallState(DeltaTime);
		break;

	case EClimbingState::BracedShimmying:
		TickBracedShimmyingState(DeltaTime);
		break;

	case EClimbingState::CornerTransition:
		// Corner transition is montage-driven, no tick logic needed
		break;

	case EClimbingState::OnLadder:
		TickLadderState(DeltaTime);
		break;

	case EClimbingState::LacheInAir:
		TickLacheInAirState(DeltaTime);
		break;

	case EClimbingState::Ragdoll:
		// Ragdoll is physics-driven, recovery handled by timer
		break;

	default:
		break;
	}

	// Update IK if we have a valid detection result
	if (CurrentDetectionResult.bValid)
	{
		UpdateClimbingIK(DeltaTime);
	}

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		// Draw current state text
		if (GEngine)
		{
			const FString StateText = FString::Printf(TEXT("Climbing State: %s | CommittedShimmyDir: %.1f"), 
				*UEnum::GetValueAsString(CurrentState), CommittedShimmyDir);
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, StateText);
		}
	}
#endif
}

void AClimbingCharacter::TickHangingState(float DeltaTime)
{
	// Check if player wants to shimmy (has movement input)
	if (!FMath::IsNearlyZero(CurrentClimbMoveInput.X))
	{
		// Update committed shimmy direction with hysteresis
		if (FMath::Abs(CurrentClimbMoveInput.X) > ShimmyDirectionDeadzone)
		{
			CommittedShimmyDir = FMath::Sign(CurrentClimbMoveInput.X);
		}

		// Check for corner in shimmy direction
		FClimbingDetectionResult CornerResult = PerformCornerDetection(CommittedShimmyDir);
		if (CornerResult.bValid)
		{
			// Corner detected - determine inside/outside
			const float Dot = FVector::DotProduct(CurrentDetectionResult.SurfaceNormal, CornerResult.SurfaceNormal);
			bCurrentCornerIsInside = Dot > 0.0f;

			// Transition to corner state
			TransitionToState(EClimbingState::CornerTransition, CornerResult);
			return;
		}

		// No corner - check if we can shimmy
		if (CurrentDetectionResult.bValid)
		{
			TransitionToState(EClimbingState::Shimmying, CurrentDetectionResult);
		}
	}
}

void AClimbingCharacter::TickShimmyingState(float DeltaTime)
{
	// Get surface speed multiplier
	float ClimbSpeedMultiplier = 1.0f;
	if (CurrentSurfaceData.IsValid())
	{
		ClimbSpeedMultiplier = CurrentSurfaceData->ClimbSpeedMultiplier;
	}

	// Calculate effective shimmy speed
	const float EffectiveSpeed = ClimbingMovement->CalculateEffectiveShimmySpeed(
		CurrentDetectionResult.SurfaceNormal, ClimbSpeedMultiplier);

	// Update committed direction with hysteresis
	if (FMath::Abs(CurrentClimbMoveInput.X) > ShimmyDirectionDeadzone)
	{
		CommittedShimmyDir = FMath::Sign(CurrentClimbMoveInput.X);
	}

	// Check if player released input (return to hanging)
	if (FMath::IsNearlyZero(CurrentClimbMoveInput.X))
	{
		TransitionToState(EClimbingState::Hanging, CurrentDetectionResult);
		return;
	}

	// Check for corner
	FClimbingDetectionResult CornerResult = PerformCornerDetection(CommittedShimmyDir);
	if (CornerResult.bValid)
	{
		const float Dot = FVector::DotProduct(CurrentDetectionResult.SurfaceNormal, CornerResult.SurfaceNormal);
		bCurrentCornerIsInside = Dot > 0.0f;
		TransitionToState(EClimbingState::CornerTransition, CornerResult);
		return;
	}

	// Update continuous shimmy distance
	ContinuousShimmyDistance += EffectiveSpeed * DeltaTime;

	// Check shimmy reposition
	if (MaxContinuousShimmyDistance > 0.0f && ContinuousShimmyDistance >= MaxContinuousShimmyDistance)
	{
		// Play reposition animation
		if (UAnimMontage* RepositionMontage = GetMontageForSlot(EClimbingAnimationSlot::ShimmyReposition))
		{
			PlayAnimMontage(RepositionMontage);
		}
		ContinuousShimmyDistance = 0.0f;
	}

	// Apply movement
	if (ClimbingMovement && EffectiveSpeed >= ShimmySpeedDeadzone)
	{
		// Calculate shimmy direction (perpendicular to wall normal, projected onto wall plane)
		const FVector WallRight = FVector::CrossProduct(FVector::UpVector, CurrentDetectionResult.SurfaceNormal).GetSafeNormal();
		const FVector ShimmyVelocity = WallRight * CommittedShimmyDir * EffectiveSpeed;

		// Apply movement (in-place animation, movement component drives lateral)
		FVector NewLocation = GetActorLocation() + ShimmyVelocity * DeltaTime;
		SetActorLocation(NewLocation);

		// Update montage playback rate based on speed
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			const float NormalizedSpeed = FMath::Clamp(EffectiveSpeed / ClimbingMovement->BaseShimmySpeed, 0.0f, 1.0f);
			const float PlaybackRate = FMath::Lerp(ShimmyPlaybackRateMin, ShimmyPlaybackRateMax, NormalizedSpeed);
			AnimInstance->Montage_SetPlayRate(GetCurrentMontage(), PlaybackRate);
		}
	}
	else
	{
		// Speed below deadzone - pause montage
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->Montage_SetPlayRate(GetCurrentMontage(), 0.0f);
		}
	}

	// Update animation instance
	if (UClimbingAnimInstance* AnimInst = CachedAnimInstance.Get())
	{
		AnimInst->NormalizedShimmySpeed = FMath::Clamp(EffectiveSpeed / ClimbingMovement->BaseShimmySpeed, 0.0f, 1.0f);
		AnimInst->CommittedShimmyDir = CommittedShimmyDir;
	}

#if !UE_BUILD_SHIPPING
	if (bDrawDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Yellow, 
			FString::Printf(TEXT("Shimmy: Speed=%.1f Dir=%.0f Dist=%.1f"), 
				EffectiveSpeed, CommittedShimmyDir, ContinuousShimmyDistance));
	}
#endif
}

void AClimbingCharacter::TickBracedWallState(float DeltaTime)
{
	// Check for lip/ledge above to transition to hanging
	FClimbingDetectionResult LedgeResult;
	if (CheckForLipAbove(LedgeResult))
	{
		// Found ledge above - transition to braced-to-hang then hanging
		TransitionToState(EClimbingState::Hanging, LedgeResult);
		return;
	}

	// Check if player wants to shimmy
	if (!FMath::IsNearlyZero(CurrentClimbMoveInput.X))
	{
		if (FMath::Abs(CurrentClimbMoveInput.X) > ShimmyDirectionDeadzone)
		{
			CommittedShimmyDir = FMath::Sign(CurrentClimbMoveInput.X);
		}

		// Re-run braced wall detection for new position
		FClimbingDetectionResult BracedResult = PerformBracedWallDetection();
		if (BracedResult.bValid)
		{
			TransitionToState(EClimbingState::BracedShimmying, BracedResult);
		}
	}
}

void AClimbingCharacter::TickBracedShimmyingState(float DeltaTime)
{
	// Get surface speed multiplier
	float ClimbSpeedMultiplier = 1.0f;
	if (CurrentSurfaceData.IsValid())
	{
		ClimbSpeedMultiplier = CurrentSurfaceData->ClimbSpeedMultiplier;
	}

	// Calculate effective shimmy speed (same as ledge shimmy)
	const float EffectiveSpeed = ClimbingMovement->CalculateEffectiveShimmySpeed(
		CurrentDetectionResult.SurfaceNormal, ClimbSpeedMultiplier);

	// Update committed direction with hysteresis
	if (FMath::Abs(CurrentClimbMoveInput.X) > ShimmyDirectionDeadzone)
	{
		CommittedShimmyDir = FMath::Sign(CurrentClimbMoveInput.X);
	}

	// Check if player released input
	if (FMath::IsNearlyZero(CurrentClimbMoveInput.X))
	{
		// Re-detect braced position
		FClimbingDetectionResult BracedResult = PerformBracedWallDetection();
		TransitionToState(EClimbingState::BracedWall, BracedResult.bValid ? BracedResult : CurrentDetectionResult);
		return;
	}

	// Check for lip above (can transition to hanging while shimmying)
	FClimbingDetectionResult LedgeResult;
	if (CheckForLipAbove(LedgeResult))
	{
		TransitionToState(EClimbingState::Hanging, LedgeResult);
		return;
	}

	// Apply movement
	if (ClimbingMovement && EffectiveSpeed >= ShimmySpeedDeadzone)
	{
		const FVector WallRight = FVector::CrossProduct(FVector::UpVector, CurrentDetectionResult.SurfaceNormal).GetSafeNormal();
		const FVector ShimmyVelocity = WallRight * CommittedShimmyDir * EffectiveSpeed;

		FVector NewLocation = GetActorLocation() + ShimmyVelocity * DeltaTime;
		SetActorLocation(NewLocation);

		// Update playback rate
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			const float NormalizedSpeed = FMath::Clamp(EffectiveSpeed / ClimbingMovement->BaseShimmySpeed, 0.0f, 1.0f);
			const float PlaybackRate = FMath::Lerp(ShimmyPlaybackRateMin, ShimmyPlaybackRateMax, NormalizedSpeed);
			AnimInstance->Montage_SetPlayRate(GetCurrentMontage(), PlaybackRate);
		}
	}
	else
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			AnimInstance->Montage_SetPlayRate(GetCurrentMontage(), 0.0f);
		}
	}

	// Update animation instance
	if (UClimbingAnimInstance* AnimInst = CachedAnimInstance.Get())
	{
		AnimInst->NormalizedShimmySpeed = FMath::Clamp(EffectiveSpeed / ClimbingMovement->BaseShimmySpeed, 0.0f, 1.0f);
		AnimInst->CommittedShimmyDir = CommittedShimmyDir;
	}
}

void AClimbingCharacter::TickLadderState(float DeltaTime)
{
	if (!ClimbingMovement)
	{
		return;
	}

	// Get climb direction from input (Y axis)
	const float ClimbInput = CurrentClimbMoveInput.Y;

	if (FMath::IsNearlyZero(ClimbInput))
	{
		// No input - idle on ladder
		return;
	}

	// Determine if fast ascent/descent
	const bool bFastAscent = bSprintModifierActive && ClimbInput > 0.0f;
	const bool bFastDescent = bCrouchModifierActive && ClimbInput < 0.0f;

	// Get surface speed multiplier
	float ClimbSpeedMultiplier = 1.0f;
	if (CurrentSurfaceData.IsValid())
	{
		ClimbSpeedMultiplier = CurrentSurfaceData->ClimbSpeedMultiplier;
	}

	// Calculate effective speed
	const float EffectiveSpeed = ClimbingMovement->CalculateEffectiveLadderSpeed(
		bFastAscent, bFastDescent, ClimbSpeedMultiplier);

	// Apply vertical movement
	const float ClimbDirection = FMath::Sign(ClimbInput);
	const FVector ClimbVelocity = FVector::UpVector * ClimbDirection * EffectiveSpeed;
	
	FVector NewLocation = GetActorLocation() + ClimbVelocity * DeltaTime;

	// Check for ladder bounds (top/bottom exit)
	FClimbingDetectionResult LadderResult = PerformLadderDetection();
	if (!LadderResult.bValid)
	{
		// Lost ladder - check if we should exit
		if (ClimbDirection > 0.0f)
		{
			// Moving up - check for ledge at top
			FClimbingDetectionResult LedgeResult = PerformLedgeDetection();
			if (LedgeResult.bValid)
			{
				TransitionToState(EClimbingState::LadderTransition, LedgeResult);
				return;
			}
		}
		else
		{
			// Moving down - exit at bottom
			TransitionToState(EClimbingState::LadderTransition, FClimbingDetectionResult());
			return;
		}
	}

	SetActorLocation(NewLocation);

	// Update animation montage based on movement
	EClimbingAnimationSlot TargetSlot = EClimbingAnimationSlot::LadderIdle;
	if (ClimbDirection > 0.0f)
	{
		TargetSlot = bFastAscent ? EClimbingAnimationSlot::LadderFastAscend : EClimbingAnimationSlot::LadderClimbUp;
	}
	else if (ClimbDirection < 0.0f)
	{
		TargetSlot = bFastDescent ? EClimbingAnimationSlot::LadderFastDescend : EClimbingAnimationSlot::LadderClimbDown;
	}

	// Update animation instance
	if (UClimbingAnimInstance* AnimInst = CachedAnimInstance.Get())
	{
		AnimInst->LadderClimbDir = ClimbDirection;
		AnimInst->bIsLadderSprinting = bFastAscent;
		AnimInst->bIsLadderFastDescending = bFastDescent;
	}

#if !UE_BUILD_SHIPPING
	if (bDrawDebug && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green,
			FString::Printf(TEXT("Ladder: Speed=%.1f Dir=%.0f Fast=%s"),
				EffectiveSpeed, ClimbDirection, 
				bFastAscent ? TEXT("Ascent") : (bFastDescent ? TEXT("Descent") : TEXT("None"))));
	}
#endif
}

void AClimbingCharacter::TickLacheInAirState(float DeltaTime)
{
	if (!ClimbingMovement || !LockedLacheTarget.bValid)
	{
		// No target - miss
		TransitionToState(EClimbingState::LacheMiss, FClimbingDetectionResult());
		return;
	}

	// Update flight time
	LacheFlightTime += DeltaTime;

	// Calculate expected position on arc
	const FVector LaunchOrigin = LacheLaunchPosition;
	const FVector ArcVelocity = LacheLaunchDirection * LacheLaunchSpeed;
	const float GravityZ = ClimbingMovement->GetGravityZ(); // Negative value - use directly

	const FVector ExpectedPosition = LaunchOrigin
		+ ArcVelocity * LacheFlightTime
		+ FVector(0.f, 0.f, 0.5f * GravityZ * LacheFlightTime * LacheFlightTime);

	// Check if we've reached the target
	const float DistanceToTarget = FVector::Dist(ExpectedPosition, LockedLacheTarget.LedgePosition);
	
	if (DistanceToTarget <= LacheArcTraceRadius * 2.0f)
	{
		// Close enough - catch
		TransitionToState(EClimbingState::LacheCatch, LockedLacheTarget);
		return;
	}

	// Check if we've exceeded flight time (missed)
	if (LacheFlightTime >= LacheTotalArcTime)
	{
		TransitionToState(EClimbingState::LacheMiss, FClimbingDetectionResult());
		return;
	}

	// Move character along arc
	SetActorLocation(ExpectedPosition);

	// Rotate to face target
	const FVector DirectionToTarget = (LockedLacheTarget.LedgePosition - GetActorLocation()).GetSafeNormal();
	if (!DirectionToTarget.IsNearlyZero())
	{
		const FRotator TargetRotation = DirectionToTarget.Rotation();
		SetActorRotation(FRotator(0.0f, TargetRotation.Yaw, 0.0f));
	}

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		// Draw arc and target
		DrawDebugLine(GetWorld(), GetActorLocation(), LockedLacheTarget.LedgePosition, FColor::Yellow, false, 0.0f);
		DrawDebugSphere(GetWorld(), LockedLacheTarget.LedgePosition, LacheArcTraceRadius, 8, FColor::Green, false, 0.0f);
		
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Cyan,
				FString::Printf(TEXT("Lache: Time=%.2f/%.2f Dist=%.1f"),
					LacheFlightTime, LacheTotalArcTime, DistanceToTarget));
		}
	}
#endif
}

void AClimbingCharacter::UpdateClimbingIK(float DeltaTime)
{
	// Implementation in Milestone 8
}

void AClimbingCharacter::OnClimbingStateReplicated(EClimbingState OldState, EClimbingState NewState)
{
	// Implementation in Milestone 13
}

// ============================================================================
// Input Mapping Context Management
// ============================================================================

void AClimbingCharacter::AddClimbingInputMappingContext()
{
	if (!ClimbingInputMappingContext)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(ClimbingInputMappingContext, ClimbingIMCPriority);
		}
	}
}

void AClimbingCharacter::RemoveClimbingInputMappingContext()
{
	if (!ClimbingInputMappingContext)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(ClimbingInputMappingContext);
		}
	}
}

// ============================================================================
// Detection (stubs - implementation in Milestone 6)
// ============================================================================

FClimbingDetectionResult AClimbingCharacter::PerformLedgeDetection() const
{
	FClimbingDetectionResult Result;
	
	if (!GetWorld())
	{
		return Result;
	}

	const FVector CharacterLocation = GetActorLocation();
	const FVector ForwardVector = GetActorForwardVector();
	const FVector UpVector = FVector::UpVector;
	
	// Collision query parameters
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;
	QueryParams.bReturnPhysicalMaterial = true;
	
	// Step 1: Forward trace to find wall
	const FVector ForwardTraceStart = CharacterLocation + UpVector * LedgeDetectionVerticalReach * 0.5f;
	const FVector ForwardTraceEnd = ForwardTraceStart + ForwardVector * LedgeDetectionForwardReach;
	
	FHitResult ForwardHit;
	bool bForwardHit = GetWorld()->SweepSingleByChannel(
		ForwardHit,
		ForwardTraceStart,
		ForwardTraceEnd,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(LedgeDetectionRadius),
		QueryParams
	);

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), ForwardTraceStart, ForwardTraceEnd, bForwardHit ? FColor::Green : FColor::Red, false, 0.1f);
	}
#endif

	if (!bForwardHit)
	{
		return Result;
	}

	// Check surface angle
	const float WallAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Abs(FVector::DotProduct(ForwardHit.ImpactNormal, UpVector))));
	if (WallAngle < (90.0f - MaxClimbableSurfaceAngle))
	{
		return Result; // Surface too horizontal (floor-like)
	}

	// Check component tag for surface tier
	EClimbSurfaceTier SurfaceTier = EClimbSurfaceTier::Untagged;
	if (ForwardHit.Component.IsValid())
	{
		if (ForwardHit.Component->ComponentHasTag(FName("Unclimbable")))
		{
			return Result; // Explicitly unclimbable
		}
		else if (ForwardHit.Component->ComponentHasTag(FName("Climbable")))
		{
			SurfaceTier = EClimbSurfaceTier::Climbable;
		}
		else if (ForwardHit.Component->ComponentHasTag(FName("ClimbableOneWay")))
		{
			SurfaceTier = EClimbSurfaceTier::ClimbableOneWay;
			// Validate one-way approach
			const UClimbingSurfaceData* SurfaceData = GetSurfaceDataFromComponent(ForwardHit.Component.Get());
			if (SurfaceData)
			{
				if (!ValidateOneWayApproach(ForwardHit.ImpactNormal, SurfaceData->OneWayApproachDirection, SurfaceData->ApproachAngleTolerance))
				{
					return Result; // Wrong approach direction
				}
			}
		}
		else if (ForwardHit.Component->ComponentHasTag(FName("LadderOnly")))
		{
			return Result; // Ledge detection doesn't handle ladders
		}
	}

	// Step 2: Trace downward from above the wall to find the ledge top
	const FVector DownTraceStart = ForwardHit.ImpactPoint + ForwardVector * MinLedgeDepth + UpVector * LedgeDetectionVerticalReach;
	const FVector DownTraceEnd = DownTraceStart - UpVector * LedgeDetectionVerticalReach * 1.5f;
	
	FHitResult DownHit;
	bool bDownHit = GetWorld()->SweepSingleByChannel(
		DownHit,
		DownTraceStart,
		DownTraceEnd,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(LedgeDetectionRadius),
		QueryParams
	);

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), DownTraceStart, DownTraceEnd, bDownHit ? FColor::Green : FColor::Yellow, false, 0.1f);
	}
#endif

	if (!bDownHit)
	{
		return Result;
	}

	// Verify this is approximately horizontal (a ledge top)
	const float LedgeTopAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(DownHit.ImpactNormal, UpVector)));
	if (LedgeTopAngle > MaxClimbableSurfaceAngle)
	{
		return Result; // Not a flat enough ledge top
	}

	// Step 3: Check clearance above ledge
	const FVector ClearanceCheckStart = DownHit.ImpactPoint + UpVector * 10.0f;
	const FVector ClearanceCheckEnd = ClearanceCheckStart + UpVector * OriginalCapsuleHalfHeight * 2.0f;
	
	FHitResult ClearanceHit;
	bool bClearanceBlocked = GetWorld()->SweepSingleByChannel(
		ClearanceHit,
		ClearanceCheckStart,
		ClearanceCheckEnd,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(OriginalCapsuleRadius * 0.8f),
		QueryParams
	);

	EClimbClearanceType ClearanceType = EClimbClearanceType::Full;
	if (bClearanceBlocked)
	{
		// Check if crouch clearance is available
		const FVector CrouchClearanceEnd = ClearanceCheckStart + UpVector * OriginalCapsuleHalfHeight;
		FHitResult CrouchClearanceHit;
		bool bCrouchBlocked = GetWorld()->SweepSingleByChannel(
			CrouchClearanceHit,
			ClearanceCheckStart,
			CrouchClearanceEnd,
			FQuat::Identity,
			ECC_WorldStatic,
			FCollisionShape::MakeSphere(OriginalCapsuleRadius * 0.8f),
			QueryParams
		);
		
		ClearanceType = bCrouchBlocked ? EClimbClearanceType::None : EClimbClearanceType::CrouchOnly;
	}

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		FColor ClearanceColor = (ClearanceType == EClimbClearanceType::Full) ? FColor::Green : 
								(ClearanceType == EClimbClearanceType::CrouchOnly) ? FColor::Yellow : FColor::Red;
		DrawDebugLine(GetWorld(), ClearanceCheckStart, ClearanceCheckEnd, ClearanceColor, false, 0.1f);
	}
#endif

	// Build result
	Result.LedgePosition = DownHit.ImpactPoint;
	Result.SurfaceNormal = ForwardHit.ImpactNormal;
	Result.SurfaceTier = SurfaceTier;
	Result.ClearanceType = ClearanceType;
	Result.HitComponent = ForwardHit.Component;
	Result.bValid = true;

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), Result.LedgePosition, 10.0f, 8, FColor::Cyan, false, 0.1f);
		DrawDebugDirectionalArrow(GetWorld(), Result.LedgePosition, Result.LedgePosition + Result.SurfaceNormal * 50.0f, 10.0f, FColor::Blue, false, 0.1f);
	}
#endif

	return Result;
}

FClimbingDetectionResult AClimbingCharacter::PerformLadderDetection() const
{
	FClimbingDetectionResult Result;
	
	if (!GetWorld())
	{
		return Result;
	}

	const FVector CharacterLocation = GetActorLocation();
	const FVector ForwardVector = GetActorForwardVector();
	const FVector UpVector = FVector::UpVector;
	
	// Collision query parameters
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;
	
	// Forward trace to find ladder
	const FVector ForwardTraceStart = CharacterLocation;
	const FVector ForwardTraceEnd = ForwardTraceStart + ForwardVector * LedgeDetectionForwardReach;
	
	FHitResult ForwardHit;
	bool bForwardHit = GetWorld()->SweepSingleByChannel(
		ForwardHit,
		ForwardTraceStart,
		ForwardTraceEnd,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(LedgeDetectionRadius),
		QueryParams
	);

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), ForwardTraceStart, ForwardTraceEnd, bForwardHit ? FColor::Orange : FColor::Red, false, 0.1f);
	}
#endif

	if (!bForwardHit || !ForwardHit.Component.IsValid())
	{
		return Result;
	}

	// Check for LadderOnly tag
	if (!ForwardHit.Component->ComponentHasTag(FName("LadderOnly")))
	{
		return Result; // Not a ladder
	}

	// Validate surface is approximately vertical
	const float WallAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Abs(FVector::DotProduct(ForwardHit.ImpactNormal, UpVector))));
	if (WallAngle < (90.0f - MaxClimbableSurfaceAngle))
	{
		return Result; // Surface not vertical enough for ladder
	}

	// Get surface data for ladder properties
	const UClimbingSurfaceData* SurfaceData = GetSurfaceDataFromComponent(ForwardHit.Component.Get());
	
	// Build result - for ladders, LedgePosition is the contact point on the ladder
	Result.LedgePosition = ForwardHit.ImpactPoint;
	Result.SurfaceNormal = ForwardHit.ImpactNormal;
	Result.SurfaceTier = EClimbSurfaceTier::LadderOnly;
	Result.ClearanceType = EClimbClearanceType::Full; // Ladders always have clearance
	Result.HitComponent = ForwardHit.Component;
	Result.bValid = true;

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), Result.LedgePosition, 10.0f, 8, FColor::Orange, false, 0.1f);
	}
#endif

	return Result;
}

FClimbingDetectionResult AClimbingCharacter::PerformBracedWallDetection() const
{
	FClimbingDetectionResult Result;
	
	if (!GetWorld())
	{
		return Result;
	}

	const FVector CharacterLocation = GetActorLocation();
	const FVector ForwardVector = GetActorForwardVector();
	const FVector UpVector = FVector::UpVector;
	
	// Collision query parameters
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;
	
	// For braced wall, we need:
	// 1. A wall in front for hands
	// 2. A surface below for feet
	
	// Trace forward at chest height for hand contact
	const FVector HandTraceStart = CharacterLocation + UpVector * OriginalCapsuleHalfHeight * 0.5f;
	const FVector HandTraceEnd = HandTraceStart + ForwardVector * LedgeDetectionForwardReach * 0.8f;
	
	FHitResult HandHit;
	bool bHandHit = GetWorld()->SweepSingleByChannel(
		HandHit,
		HandTraceStart,
		HandTraceEnd,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(LedgeDetectionRadius),
		QueryParams
	);

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), HandTraceStart, HandTraceEnd, bHandHit ? FColor::Purple : FColor::Red, false, 0.1f);
	}
#endif

	if (!bHandHit || !HandHit.Component.IsValid())
	{
		return Result;
	}

	// Check for Unclimbable tag
	if (HandHit.Component->ComponentHasTag(FName("Unclimbable")))
	{
		return Result;
	}

	// Check if this is ladder only (braced wall doesn't use ladders)
	if (HandHit.Component->ComponentHasTag(FName("LadderOnly")))
	{
		return Result;
	}

	// Validate wall surface is approximately vertical
	const float WallAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Abs(FVector::DotProduct(HandHit.ImpactNormal, UpVector))));
	if (WallAngle < (90.0f - MaxClimbableSurfaceAngle))
	{
		return Result; // Surface not vertical enough
	}

	// Trace for foot placement below
	const FVector FootTraceStart = CharacterLocation + ForwardVector * (LedgeDetectionForwardReach * 0.3f);
	const FVector FootTraceEnd = FootTraceStart - UpVector * OriginalCapsuleHalfHeight;
	
	FHitResult FootHit;
	bool bFootHit = GetWorld()->SweepSingleByChannel(
		FootHit,
		FootTraceStart,
		FootTraceEnd,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(LedgeDetectionRadius * 0.5f),
		QueryParams
	);

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), FootTraceStart, FootTraceEnd, bFootHit ? FColor::Purple : FColor::Yellow, false, 0.1f);
	}
#endif

	// Braced wall requires foot contact OR being in a position where feet can reach wall
	// For now, we validate we have wall contact at hand level
	
	// Determine surface tier
	EClimbSurfaceTier SurfaceTier = EClimbSurfaceTier::Untagged;
	if (HandHit.Component->ComponentHasTag(FName("Climbable")))
	{
		SurfaceTier = EClimbSurfaceTier::Climbable;
	}
	else if (HandHit.Component->ComponentHasTag(FName("ClimbableOneWay")))
	{
		SurfaceTier = EClimbSurfaceTier::ClimbableOneWay;
		const UClimbingSurfaceData* SurfaceData = GetSurfaceDataFromComponent(HandHit.Component.Get());
		if (SurfaceData)
		{
			if (!ValidateOneWayApproach(HandHit.ImpactNormal, SurfaceData->OneWayApproachDirection, SurfaceData->ApproachAngleTolerance))
			{
				return Result;
			}
		}
	}

	// Build result
	Result.LedgePosition = HandHit.ImpactPoint;
	Result.SurfaceNormal = HandHit.ImpactNormal;
	Result.SurfaceTier = SurfaceTier;
	Result.ClearanceType = EClimbClearanceType::Full; // Braced wall doesn't use clearance the same way
	Result.HitComponent = HandHit.Component;
	Result.bValid = true;

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), Result.LedgePosition, 10.0f, 8, FColor::Purple, false, 0.1f);
	}
#endif

	return Result;
}

UPrimitiveComponent* AClimbingCharacter::ResolveHitComponentFromNet(const FClimbingDetectionResultNet& NetResult) const
{
	// Implementation in Milestone 13
	return nullptr;
}

// ============================================================================
// Detection Helpers
// ============================================================================

bool AClimbingCharacter::IsSurfaceClimbable(EClimbSurfaceTier Tier) const
{
	switch (Tier)
	{
	case EClimbSurfaceTier::Unclimbable:
		return false;
	case EClimbSurfaceTier::Climbable:
	case EClimbSurfaceTier::ClimbableOneWay:
	case EClimbSurfaceTier::Untagged:
		return true;
	case EClimbSurfaceTier::LadderOnly:
		// Only climbable when in ladder detection context
		return false;
	default:
		return false;
	}
}

bool AClimbingCharacter::ValidateOneWayApproach(const FVector& SurfaceNormal, const FVector& ApproachDirection, float Tolerance) const
{
	const float DotProduct = FVector::DotProduct(SurfaceNormal.GetSafeNormal(), ApproachDirection.GetSafeNormal());
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Abs(DotProduct)));
	return AngleDeg <= Tolerance;
}

const UClimbingSurfaceData* AClimbingCharacter::GetSurfaceDataFromComponent(UPrimitiveComponent* Component) const
{
	if (!Component)
	{
		return nullptr;
	}

	// Check for direct reference via interface or component tag containing asset path
	// For now, we look for a tag that starts with "SurfaceData:" followed by the asset path
	for (const FName& Tag : Component->ComponentTags)
	{
		const FString TagString = Tag.ToString();
		if (TagString.StartsWith(TEXT("SurfaceData:")))
		{
			// Extract asset path
			const FString AssetPath = TagString.RightChop(12); // Length of "SurfaceData:"
			
			// Try to load the asset synchronously (should already be loaded if properly set up)
			if (UClimbingSurfaceData* SurfaceData = Cast<UClimbingSurfaceData>(StaticLoadObject(UClimbingSurfaceData::StaticClass(), nullptr, *AssetPath)))
			{
				return SurfaceData;
			}
		}
	}

	// Could also check owner actor for surface data component/reference
	if (AActor* Owner = Component->GetOwner())
	{
		// Check for surface data in actor tags
		for (const FName& Tag : Owner->Tags)
		{
			const FString TagString = Tag.ToString();
			if (TagString.StartsWith(TEXT("SurfaceData:")))
			{
				const FString AssetPath = TagString.RightChop(12);
				if (UClimbingSurfaceData* SurfaceData = Cast<UClimbingSurfaceData>(StaticLoadObject(UClimbingSurfaceData::StaticClass(), nullptr, *AssetPath)))
				{
					return SurfaceData;
				}
			}
		}
	}

	return nullptr;
}

// ============================================================================
// Montage Access
// ============================================================================

UAnimMontage* AClimbingCharacter::GetMontageForSlot(EClimbingAnimationSlot Slot) const
{
	// Priority: current surface AnimationSetOverride → character defaults
	// Per-slot fallback: null fields in the override fall back to character defaults individually
	
	// First check override
	if (CurrentAnimationSetOverride)
	{
		if (UAnimMontage* OverrideMontage = CurrentAnimationSetOverride->GetMontageForSlot(Slot))
		{
			return OverrideMontage;
		}
	}

	// Fall back to character defaults
	switch (Slot)
	{
	// Ledge animations
	case EClimbingAnimationSlot::HangIdle: return HangIdle;
	case EClimbingAnimationSlot::HangIdleLeft: return HangIdleLeft;
	case EClimbingAnimationSlot::HangIdleRight: return HangIdleRight;
	case EClimbingAnimationSlot::ShimmyLeft: return ShimmyLeft;
	case EClimbingAnimationSlot::ShimmyRight: return ShimmyRight;
	case EClimbingAnimationSlot::CornerInsideLeft: return CornerInsideLeft;
	case EClimbingAnimationSlot::CornerInsideRight: return CornerInsideRight;
	case EClimbingAnimationSlot::CornerOutsideLeft: return CornerOutsideLeft;
	case EClimbingAnimationSlot::CornerOutsideRight: return CornerOutsideRight;
	case EClimbingAnimationSlot::ClimbUp: return ClimbUp;
	case EClimbingAnimationSlot::ClimbUpCrouch: return ClimbUpCrouch;
	case EClimbingAnimationSlot::DropDown: return DropDown;
	case EClimbingAnimationSlot::GrabLedge: return GrabLedge;
	case EClimbingAnimationSlot::GrabFail: return GrabFail;
	case EClimbingAnimationSlot::ShimmyReposition: return ShimmyReposition;

	// Lache animations
	case EClimbingAnimationSlot::LacheLaunch: return LacheLaunch;
	case EClimbingAnimationSlot::LacheFlight: return LacheFlight;
	case EClimbingAnimationSlot::LacheCatch: return LacheCatch;
	case EClimbingAnimationSlot::LacheMiss: return LacheMiss;

	// Mantle animations
	case EClimbingAnimationSlot::MantleLow: return MantleLow;
	case EClimbingAnimationSlot::MantleHigh: return MantleHigh;

	// Ragdoll animations
	case EClimbingAnimationSlot::RagdollGetUpFaceDown: return RagdollGetUpFaceDown;
	case EClimbingAnimationSlot::RagdollGetUpFaceUp: return RagdollGetUpFaceUp;

	// Braced wall animations
	case EClimbingAnimationSlot::BracedIdle: return BracedIdle;
	case EClimbingAnimationSlot::BracedShimmyLeft: return BracedShimmyLeft;
	case EClimbingAnimationSlot::BracedShimmyRight: return BracedShimmyRight;
	case EClimbingAnimationSlot::BracedToHang: return BracedToHang;

	// Ladder animations
	case EClimbingAnimationSlot::LadderIdle: return LadderIdle;
	case EClimbingAnimationSlot::LadderClimbUp: return LadderClimbUp;
	case EClimbingAnimationSlot::LadderClimbDown: return LadderClimbDown;
	case EClimbingAnimationSlot::LadderFastAscend: return LadderFastAscend;
	case EClimbingAnimationSlot::LadderFastDescend: return LadderFastDescend;
	case EClimbingAnimationSlot::LadderEnterBottom: return LadderEnterBottom;
	case EClimbingAnimationSlot::LadderEnterTop: return LadderEnterTop;
	case EClimbingAnimationSlot::LadderExitBottom: return LadderExitBottom;
	case EClimbingAnimationSlot::LadderExitTop: return LadderExitTop;
	case EClimbingAnimationSlot::LadderExitSide: return LadderExitSide;

	default:
		return nullptr;
	}
}

// ============================================================================
// Camera
// ============================================================================

void AClimbingCharacter::LockCameraToFrame(FVector Location, FRotator Rotation, float BlendTime)
{
	// Implementation in Milestone 9
	bCameraLocked = true;
}

void AClimbingCharacter::ReleaseCameraLock(float BlendTime)
{
	// Implementation in Milestone 9
	bCameraLocked = false;
}

// ============================================================================
// IK Helpers (stubs - implementation in Milestone 8)
// ============================================================================

void AClimbingCharacter::UpdateIKTargets()
{
	// Implementation in Milestone 8
}

void AClimbingCharacter::RegisterWithIKManager()
{
	// Game thread access only - called from BeginPlay which is game thread
	ActiveClimbingCharacters.Add(this);
}

void AClimbingCharacter::UnregisterFromIKManager()
{
	// Called from PerformCleanup
	ActiveClimbingCharacters.RemoveAll([this](const TWeakObjectPtr<AClimbingCharacter>& Ptr)
	{
		return !Ptr.IsValid() || Ptr.Get() == this;
	});
}

// ============================================================================
// Audio Helpers (stubs - implementation in Milestone 11)
// ============================================================================

void AClimbingCharacter::PlayClimbingSound(EClimbSoundType SoundType)
{
	// Implementation in Milestone 11
}

USoundBase* AClimbingCharacter::GetResolvedSound(EClimbSoundType SoundType)
{
	// Implementation in Milestone 11
	return nullptr;
}

// ============================================================================
// Validation
// ============================================================================

void AClimbingCharacter::ValidateAnimationSlots()
{
	// Iterate every EClimbingAnimationSlot value and call GetMontageForSlot
	// For each null result, log warning
	for (uint8 SlotIndex = 0; SlotIndex < static_cast<uint8>(EClimbingAnimationSlot::MAX); ++SlotIndex)
	{
		const EClimbingAnimationSlot Slot = static_cast<EClimbingAnimationSlot>(SlotIndex);
		if (!GetMontageForSlot(Slot))
		{
			UE_LOG(LogClimbing, Warning, TEXT("ClimbingSystem: Slot '%s' unassigned on '%s'. The action requiring this animation will fail silently at runtime."),
				*UEnum::GetValueAsString(Slot), *GetName());
		}
	}
}

// ============================================================================
// Server RPCs (stubs - implementation in Milestone 13)
// ============================================================================

void AClimbingCharacter::Server_AttemptGrab_Implementation(FClimbingDetectionResultNet ClientResult)
{
	// Implementation in Milestone 13
}

void AClimbingCharacter::Server_Drop_Implementation()
{
	// Implementation in Milestone 13
}

void AClimbingCharacter::Server_AttemptLache_Implementation(FVector ClientArcTarget)
{
	// Implementation in Milestone 13
}

void AClimbingCharacter::Server_AttemptClimbUp_Implementation()
{
	// Implementation in Milestone 13
}

void AClimbingCharacter::Server_UpdateShimmyDirection_Implementation(FVector2D Direction)
{
	// Implementation in Milestone 13
}

// ============================================================================
// Client RPCs (stubs - implementation in Milestone 13)
// ============================================================================

void AClimbingCharacter::Client_RejectStateTransition_Implementation()
{
	// Implementation in Milestone 13
}

void AClimbingCharacter::Client_ConfirmStateTransition_Implementation(EClimbingState ConfirmedState)
{
	// Implementation in Milestone 13
}

// ============================================================================
// Helper Functions
// ============================================================================

void AClimbingCharacter::PlayIdleVariation()
{
	if (!ClimbingMovement || ClimbingMovement->CurrentClimbingState != EClimbingState::Hanging)
	{
		return;
	}

	// Build pool of available variations
	TArray<int32> AvailableIndices;
	
	// Add HangIdleLeft (index 0) and HangIdleRight (index 1) if they exist
	if (HangIdleLeft) AvailableIndices.Add(0);
	if (HangIdleRight) AvailableIndices.Add(1);
	
	// Add custom variations
	for (int32 i = 0; i < HangIdleVariations.Num(); ++i)
	{
		if (HangIdleVariations[i])
		{
			AvailableIndices.Add(i + 2); // Offset by 2 for Left/Right
		}
	}

	if (AvailableIndices.Num() == 0)
	{
		// No variations available, reschedule timer
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimer(
				IdleVariationTimerHandle,
				[this]() { PlayIdleVariation(); },
				IdleVariationDelay,
				false
			);
		}
		return;
	}

	// Remove last played if preventing repeats
	if (bPreventConsecutiveVariationRepeat && LastIdleVariationIndex >= 0)
	{
		AvailableIndices.Remove(LastIdleVariationIndex);
	}

	if (AvailableIndices.Num() == 0)
	{
		// Only one variation and we can't repeat - play default
		if (UAnimMontage* IdleMontage = GetMontageForSlot(EClimbingAnimationSlot::HangIdle))
		{
			PlayAnimMontage(IdleMontage, 1.0f, NAME_None);
		}
		LastIdleVariationIndex = -1;
	}
	else
	{
		// Select random variation
		const int32 RandomIndex = FMath::RandRange(0, AvailableIndices.Num() - 1);
		const int32 VariationIndex = AvailableIndices[RandomIndex];
		LastIdleVariationIndex = VariationIndex;

		UAnimMontage* VariationMontage = nullptr;
		if (VariationIndex == 0)
		{
			VariationMontage = HangIdleLeft;
		}
		else if (VariationIndex == 1)
		{
			VariationMontage = HangIdleRight;
		}
		else if (VariationIndex - 2 < HangIdleVariations.Num())
		{
			VariationMontage = HangIdleVariations[VariationIndex - 2];
		}

		if (VariationMontage)
		{
			PlayAnimMontage(VariationMontage, 1.0f, NAME_None);
		}
	}

	// Reschedule timer
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			IdleVariationTimerHandle,
			[this]() { PlayIdleVariation(); },
			IdleVariationDelay,
			false
		);
	}
}

void AClimbingCharacter::RecoverFromRagdoll()
{
	if (!ClimbingMovement || ClimbingMovement->CurrentClimbingState != EClimbingState::Ragdoll)
	{
		return;
	}

	// Determine face up or face down using pelvis up-vector dot world up
	// Do NOT use pelvis Z rotation - ambiguous on skeletons with non-trivial reference pose
	bool bFaceUp = false;
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		const FQuat PelvisQuat = MeshComp->GetBoneQuaternion(PelvisBoneName, EBoneSpaces::WorldSpace);
		const FVector PelvisUp = PelvisQuat.GetUpVector();
		bFaceUp = FVector::DotProduct(PelvisUp, FVector::UpVector) > 0.0f;
	}

	// Disable physics
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		// Get pelvis location before disabling physics for positioning
		const FVector PelvisLocation = MeshComp->GetBoneLocation(PelvisBoneName);
		
		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		
		// Position character at pelvis location
		SetActorLocation(PelvisLocation + FVector::UpVector * OriginalCapsuleHalfHeight);
	}

	// Select and play get-up animation
	const EClimbingAnimationSlot GetUpSlot = bFaceUp ? 
		EClimbingAnimationSlot::RagdollGetUpFaceUp : 
		EClimbingAnimationSlot::RagdollGetUpFaceDown;
	
	if (UAnimMontage* GetUpMontage = GetMontageForSlot(GetUpSlot))
	{
		PlayAnimMontage(GetUpMontage);
	}

	// Update animation instance
	if (UClimbingAnimInstance* AnimInstance = CachedAnimInstance.Get())
	{
		AnimInstance->bRagdollFaceUp = bFaceUp;
	}

	// Transition to None state
	FClimbingDetectionResult EmptyResult;
	TransitionToState(EClimbingState::None, EmptyResult);
}

FClimbingDetectionResult AClimbingCharacter::PerformCornerDetection(float ShimmyDirection) const
{
	FClimbingDetectionResult Result;

	if (!GetWorld() || FMath::IsNearlyZero(ShimmyDirection))
	{
		return Result;
	}

	const FVector CharacterLocation = GetActorLocation();
	const FVector ForwardVector = GetActorForwardVector();
	const FVector RightVector = GetActorRightVector();
	const FVector UpVector = FVector::UpVector;

	// Direction to trace (left or right)
	const FVector TraceDirection = RightVector * FMath::Sign(ShimmyDirection);

	// Collision query parameters
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;

	// Trace to the side to find corner
	const FVector SideTraceStart = CharacterLocation + UpVector * OriginalCapsuleHalfHeight * 0.5f;
	const FVector SideTraceEnd = SideTraceStart + TraceDirection * LedgeDetectionForwardReach;

	FHitResult SideHit;
	bool bSideHit = GetWorld()->SweepSingleByChannel(
		SideHit,
		SideTraceStart,
		SideTraceEnd,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(LedgeDetectionRadius),
		QueryParams
	);

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), SideTraceStart, SideTraceEnd, bSideHit ? FColor::Blue : FColor::Red, false, 0.1f);
	}
#endif

	if (!bSideHit)
	{
		// No wall to the side - check if this is an outside corner
		// Trace forward from the side position
		const FVector OutsideCornerStart = SideTraceEnd;
		const FVector OutsideCornerEnd = OutsideCornerStart + ForwardVector * LedgeDetectionForwardReach;

		FHitResult OutsideHit;
		bool bOutsideHit = GetWorld()->SweepSingleByChannel(
			OutsideHit,
			OutsideCornerStart,
			OutsideCornerEnd,
			FQuat::Identity,
			ECC_WorldStatic,
			FCollisionShape::MakeSphere(LedgeDetectionRadius),
			QueryParams
		);

#if !UE_BUILD_SHIPPING
		if (bDrawDebug)
		{
			DrawDebugLine(GetWorld(), OutsideCornerStart, OutsideCornerEnd, bOutsideHit ? FColor::Blue : FColor::Yellow, false, 0.1f);
		}
#endif

		if (bOutsideHit && OutsideHit.Component.IsValid())
		{
			// Found outside corner
			// Check angle between current normal and new normal
			const float Dot = FVector::DotProduct(CurrentDetectionResult.SurfaceNormal, OutsideHit.ImpactNormal);
			const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));

			if (AngleDeg >= CornerAngleThreshold)
			{
				// Valid corner transition
				Result.LedgePosition = OutsideHit.ImpactPoint;
				Result.SurfaceNormal = OutsideHit.ImpactNormal;
				Result.SurfaceTier = CurrentDetectionResult.SurfaceTier; // Maintain tier
				Result.ClearanceType = CurrentDetectionResult.ClearanceType;
				Result.HitComponent = OutsideHit.Component;
				Result.bValid = true;
			}
		}
	}
	else
	{
		// Wall to the side - check if this is an inside corner
		// Check angle between current normal and side wall normal
		const float Dot = FVector::DotProduct(CurrentDetectionResult.SurfaceNormal, SideHit.ImpactNormal);
		const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));

		if (AngleDeg >= CornerAngleThreshold)
		{
			// Valid inside corner
			Result.LedgePosition = SideHit.ImpactPoint;
			Result.SurfaceNormal = SideHit.ImpactNormal;
			Result.SurfaceTier = CurrentDetectionResult.SurfaceTier;
			Result.ClearanceType = CurrentDetectionResult.ClearanceType;
			Result.HitComponent = SideHit.Component;
			Result.bValid = true;
		}
	}

	return Result;
}

bool AClimbingCharacter::CheckForLipAbove(FClimbingDetectionResult& OutLedgeResult) const
{
	if (!GetWorld())
	{
		return false;
	}

	const FVector CharacterLocation = GetActorLocation();
	const FVector ForwardVector = GetActorForwardVector();
	const FVector UpVector = FVector::UpVector;

	// Collision query parameters
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;

	// Trace upward from current position
	const FVector UpTraceStart = CharacterLocation + ForwardVector * MinLedgeDepth;
	const FVector UpTraceEnd = UpTraceStart + UpVector * LedgeDetectionVerticalReach;

	FHitResult UpHit;
	bool bUpHit = GetWorld()->SweepSingleByChannel(
		UpHit,
		UpTraceStart,
		UpTraceEnd,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(LedgeDetectionRadius),
		QueryParams
	);

	if (bUpHit)
	{
		// Blocked above - no lip
		return false;
	}

	// Trace down from above to find ledge top
	const FVector DownTraceStart = UpTraceEnd;
	const FVector DownTraceEnd = UpTraceStart;

	FHitResult DownHit;
	bool bDownHit = GetWorld()->SweepSingleByChannel(
		DownHit,
		DownTraceStart,
		DownTraceEnd,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(LedgeDetectionRadius),
		QueryParams
	);

	if (bDownHit)
	{
		// Check if it's a horizontal surface (ledge top)
		const float LedgeTopAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(DownHit.ImpactNormal, UpVector)));
		if (LedgeTopAngle <= MaxClimbableSurfaceAngle)
		{
			OutLedgeResult.LedgePosition = DownHit.ImpactPoint;
			OutLedgeResult.SurfaceNormal = CurrentDetectionResult.SurfaceNormal; // Use wall normal
			OutLedgeResult.SurfaceTier = CurrentDetectionResult.SurfaceTier;
			OutLedgeResult.ClearanceType = EClimbClearanceType::Full; // Check clearance properly if needed
			OutLedgeResult.HitComponent = DownHit.Component.IsValid() ? DownHit.Component : CurrentDetectionResult.HitComponent;
			OutLedgeResult.bValid = true;
			return true;
		}
	}

	return false;
}
