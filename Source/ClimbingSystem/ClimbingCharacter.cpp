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
			[this]() { /* Detection tick will be implemented in Milestone 6 */ },
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
// Input Handlers (stubs - implementation in later milestones)
// ============================================================================

void AClimbingCharacter::Input_Grab(const FInputActionValue& Value)
{
	// Implementation in Milestone 7
}

void AClimbingCharacter::Input_Drop(const FInputActionValue& Value)
{
	// Implementation in Milestone 7
}

void AClimbingCharacter::Input_Lache(const FInputActionValue& Value)
{
	// Implementation in Milestone 7c
}

void AClimbingCharacter::Input_ClimbUp(const FInputActionValue& Value)
{
	// Implementation in Milestone 7
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
// State Management (stubs - implementation in later milestones)
// ============================================================================

void AClimbingCharacter::TransitionToState(EClimbingState NewState, const FClimbingDetectionResult& DetectionResult)
{
	// Implementation in Milestone 7a
}

void AClimbingCharacter::OnStateEnter(EClimbingState NewState, const FClimbingDetectionResult& DetectionResult)
{
	// Implementation in Milestone 7a
}

void AClimbingCharacter::OnStateExit(EClimbingState OldState)
{
	// Implementation in Milestone 7a
}

void AClimbingCharacter::TickClimbingState(float DeltaTime)
{
	// Implementation in Milestone 7b/7c
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
	// Implementation in Milestone 6
	return Result;
}

FClimbingDetectionResult AClimbingCharacter::PerformLadderDetection() const
{
	FClimbingDetectionResult Result;
	// Implementation in Milestone 6
	return Result;
}

FClimbingDetectionResult AClimbingCharacter::PerformBracedWallDetection() const
{
	FClimbingDetectionResult Result;
	// Implementation in Milestone 6
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
	// Implementation in Milestone 6
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
