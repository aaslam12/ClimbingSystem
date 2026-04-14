// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingCharacter.h"
/**
 * AClimbingCharacter — Core
 *
 * This file contains only lifecycle functions (constructor, BeginPlay, EndPlay,
 * Tick, SetupPlayerInputComponent, GetLifetimeReplicatedProps).
 *
 * Implementation is split across:
 *   ClimbingCharacter_Detection.cpp     — surface detection and trace logic
 *   ClimbingCharacter_StateMachine.cpp  — state transitions and montage selection
 *   ClimbingCharacter_Actions.cpp       — per-state climbing action logic
 *   ClimbingCharacter_IK.cpp            — IK target computation and manager
 *   ClimbingCharacter_Camera.cpp        — camera assist, cinematic lock, ragdoll camera
 *   ClimbingCharacter_Physics.cpp       — anchor tracking, grab break, ragdoll
 *   ClimbingCharacter_Audio.cpp         — sound loading, caching, playback
 *   ClimbingCharacter_Multiplayer.cpp   — server RPCs, client RPCs, replication
 *   ClimbingCharacter_Debug.cpp         — debug drawing and slot validation
 */
#include "Movement/ClimbingMovementComponent.h"
#include "Animation/ClimbingAnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "MotionWarpingComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

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

	// Ensure the owning local player receives the default locomotion IMC.
	AddLocomotionInputMappingContext();

	// Cache original capsule dimensions for restoration when exiting climbing
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		OriginalCapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
		OriginalCapsuleRadius = Capsule->GetUnscaledCapsuleRadius();
		OriginalCollisionProfile = Capsule->GetCollisionProfileName();
	}

	if (CameraBoom)
	{
		OriginalCameraProbeSize = CameraBoom->ProbeSize;
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
	// Dual-unregister in EndPlay AND Destroyed handles multiplayer edge cases
	// where destruction order varies (seamless travel, client disconnect, etc.)
	PerformCleanup();
	Super::EndPlay(EndPlayReason);
}

void AClimbingCharacter::Destroyed()
{
	// Dual-unregister in EndPlay AND Destroyed handles multiplayer edge cases
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
	RemoveLocomotionInputMappingContext();
	bClimbingIMCActive = false;
	bLocomotionIMCActive = false;

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

	// Update camera systems (local player only)
	if (IsLocallyControlled())
	{
		UpdateCameraLock(DeltaTime);
		UpdateCameraNudge(DeltaTime);
		UpdatePredictionRollback(DeltaTime);
	}

	// Update coyote time
	if (CoyoteTimeRemaining > 0.0f)
	{
		CoyoteTimeRemaining -= DeltaTime;
	}

#if !UE_BUILD_SHIPPING
	// Freefall grab window visualization (shoulder-height sphere)
	if (bDrawDebug && bEnableFallingGrab && ClimbingMovement && ClimbingMovement->IsFalling())
	{
		// Draw the reach window sphere at shoulder height
		const FVector ShoulderPosition = GetActorLocation() + FVector(0.0f, 0.0f, 60.0f); // Approximate shoulder height
		DrawDebugSphere(GetWorld(), ShoulderPosition, FallingGrabReachDistance, 16, FColor::Magenta, false, 0.0f);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Magenta, TEXT("Freefall Grab Window Active"));
		}
	}
#endif

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

void AClimbingCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (!ClimbingMovement)
	{
		return;
	}

	const EClimbingState CurrentState = ClimbingMovement->CurrentClimbingState;
	if (CurrentState == EClimbingState::DroppingDown ||
		CurrentState == EClimbingState::Lache ||
		CurrentState == EClimbingState::LacheInAir ||
		CurrentState == EClimbingState::LacheMiss)
	{
		TransitionToState(EClimbingState::None, FClimbingDetectionResult());
	}
}

void AClimbingCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	// Re-apply input contexts after possession/controller changes on the owning client.
	bLocomotionIMCActive = false;
	bClimbingIMCActive = false;
	AddLocomotionInputMappingContext();

	if (ClimbingMovement && ClimbingMovement->CurrentClimbingState != EClimbingState::None)
	{
		AddClimbingInputMappingContext();
	}
}

void AClimbingCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_Move)
		{
			EnhancedInput->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AClimbingCharacter::Input_Move);
			EnhancedInput->BindAction(IA_Move, ETriggerEvent::Ongoing, this, &AClimbingCharacter::Input_Move);
			EnhancedInput->BindAction(IA_Move, ETriggerEvent::Completed, this, &AClimbingCharacter::Input_Move);
		}
		if (IA_Look)
		{
			EnhancedInput->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AClimbingCharacter::Input_Look);
			EnhancedInput->BindAction(IA_Look, ETriggerEvent::Ongoing, this, &AClimbingCharacter::Input_Look);
		}
		if (IA_Jump)
		{
			EnhancedInput->BindAction(IA_Jump, ETriggerEvent::Started, this, &AClimbingCharacter::Input_JumpStarted);
			EnhancedInput->BindAction(IA_Jump, ETriggerEvent::Completed, this, &AClimbingCharacter::Input_JumpCompleted);
		}

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

	DOREPLIFETIME(AClimbingCharacter, CommittedShimmyDir);
	DOREPLIFETIME(AClimbingCharacter, bCurrentCornerIsInside);
}

void AClimbingCharacter::UpdatePredictionRollback(float DeltaTime)
{
	if (!bPredictionRollbackInProgress || PredictionRollbackBlendOut <= 0.0f)
	{
		return;
	}

	PredictionRollbackElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(PredictionRollbackElapsed / PredictionRollbackBlendOut, 0.0f, 1.0f);
	const FVector NewLocation = FMath::Lerp(PredictionRollbackStart, PredictionRollbackTarget, Alpha);
	SetActorLocation(NewLocation, false);

	if (Alpha >= 1.0f)
	{
		bPredictionRollbackInProgress = false;
		PrePredictionPosition = FVector::ZeroVector;
		PredictionRollbackElapsed = 0.0f;
	}
}
