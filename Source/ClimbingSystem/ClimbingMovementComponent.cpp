// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingMovementComponent.h"
#include "ClimbingCharacter.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"

UClimbingMovementComponent::UClimbingMovementComponent()
{
	// Initialize state configs map with defaults for all states
	InitializeStateConfigs();
}

void UClimbingMovementComponent::InitializeStateConfigs()
{
	// Freely interruptible states
	StateConfigs.Add(EClimbingState::None, FClimbingStateConfig(true));
	StateConfigs.Add(EClimbingState::Hanging, FClimbingStateConfig(true));
	StateConfigs.Add(EClimbingState::Shimmying, FClimbingStateConfig(true));
	StateConfigs.Add(EClimbingState::BracedWall, FClimbingStateConfig(true));
	StateConfigs.Add(EClimbingState::BracedShimmying, FClimbingStateConfig(true));
	StateConfigs.Add(EClimbingState::OnLadder, FClimbingStateConfig(true));
	StateConfigs.Add(EClimbingState::DroppingDown, FClimbingStateConfig(true));

	// Committed states (not interruptible)
	StateConfigs.Add(EClimbingState::CornerTransition, FClimbingStateConfig(false, 1.0f));
	StateConfigs.Add(EClimbingState::LadderTransition, FClimbingStateConfig(false, 1.0f));
	StateConfigs.Add(EClimbingState::Mantling, FClimbingStateConfig(false, 1.0f));
	StateConfigs.Add(EClimbingState::LacheCatch, FClimbingStateConfig(false, 1.0f));
	StateConfigs.Add(EClimbingState::Ragdoll, FClimbingStateConfig(false, 1.0f));
	StateConfigs.Add(EClimbingState::Lache, FClimbingStateConfig(false, 1.0f));

	// Conditionally interruptible states
	StateConfigs.Add(EClimbingState::ClimbingUp, FClimbingStateConfig(false, 0.7f));
	StateConfigs.Add(EClimbingState::ClimbingUpCrouch, FClimbingStateConfig(false, 0.7f));
	StateConfigs.Add(EClimbingState::LacheInAir, FClimbingStateConfig(false, 0.0f)); // Can be interrupted by miss
	StateConfigs.Add(EClimbingState::LacheMiss, FClimbingStateConfig(false, 0.5f));
}

void UClimbingMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UClimbingMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update character position to follow moving anchor
	if (AnchorComponent && CurrentClimbingState != EClimbingState::None)
	{
		UpdateAnchorFollowing();
	}
}

void UClimbingMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UClimbingMovementComponent, CurrentClimbingState);
	DOREPLIFETIME(UClimbingMovementComponent, LastValidatedDetectionResult);
	DOREPLIFETIME(UClimbingMovementComponent, AnchorComponent);
	DOREPLIFETIME(UClimbingMovementComponent, AnchorLocalTransform);
}

float UClimbingMovementComponent::GetMaxSpeed() const
{
	switch (CurrentClimbingState)
	{
	case EClimbingState::Shimmying:
	case EClimbingState::BracedShimmying:
		return BaseShimmySpeed;

	case EClimbingState::OnLadder:
		return BaseLadderClimbSpeed;

	case EClimbingState::None:
	default:
		return Super::GetMaxSpeed();
	}
}

bool UClimbingMovementComponent::CanAttemptJump() const
{
	// Disable normal jump during climbing states
	if (CurrentClimbingState != EClimbingState::None)
	{
		return false;
	}
	return Super::CanAttemptJump();
}

bool UClimbingMovementComponent::DoJump(bool bReplayingMoves)
{
	// Disable normal jump during climbing states
	if (CurrentClimbingState != EClimbingState::None)
	{
		return false;
	}
	return Super::DoJump(bReplayingMoves);
}

bool UClimbingMovementComponent::ShouldUsePackedMovementRPCs() const
{
	// During in-place climbing animations, don't use packed movement RPCs
	// to prevent base class interference
	switch (CurrentClimbingState)
	{
	case EClimbingState::Hanging:
	case EClimbingState::Shimmying:
	case EClimbingState::BracedWall:
	case EClimbingState::BracedShimmying:
	case EClimbingState::OnLadder:
		return false;

	default:
		return Super::ShouldUsePackedMovementRPCs();
	}
}

float UClimbingMovementComponent::CalculateEffectiveShimmySpeed(const FVector& SurfaceNormal, float ClimbSpeedMultiplier) const
{
	const float OverhangPenalty = CalculateOverhangPenalty(SurfaceNormal);
	return BaseShimmySpeed * ClimbSpeedMultiplier * OverhangPenalty;
}

float UClimbingMovementComponent::CalculateEffectiveLadderSpeed(bool bSprinting, bool bFastDescending, float ClimbSpeedMultiplier) const
{
	float Speed = BaseLadderClimbSpeed * ClimbSpeedMultiplier;

	if (bSprinting)
	{
		Speed *= LadderSprintMultiplier;
	}
	else if (bFastDescending)
	{
		Speed *= LadderFastDescentMultiplier;
	}

	return Speed;
}

float UClimbingMovementComponent::CalculateOverhangPenalty(const FVector& SurfaceNormal) const
{
	// Calculate angle from vertical (world up)
	// Dot product with up vector gives cos(angle from up)
	// angle = acos(dot) gives angle from up
	// We want angle from vertical wall (90 degrees from up)
	
	const float DotUp = FVector::DotProduct(SurfaceNormal, FVector::UpVector);
	const float AngleFromUp = FMath::RadiansToDegrees(FMath::Acos(FMath::Abs(DotUp)));
	
	// Angle from vertical = 90 - AngleFromUp
	// Overhang angle = how far past vertical the surface tilts toward player
	// For a vertical wall, DotUp = 0, AngleFromUp = 90, OverhangAngle = 0
	// For an overhang, DotUp < 0 (normal points somewhat down), meaning surface faces up
	
	// Calculate overhang: negative DotUp means the surface is an overhang
	const float OverhangAngleDeg = (DotUp < 0.0f) ? -FMath::RadiansToDegrees(FMath::Asin(DotUp)) : 0.0f;

	if (OverhangAngleDeg <= OverhangPenaltyStartAngle)
	{
		return 1.0f;
	}

	const float PenaltyProgress = FMath::Clamp(
		(OverhangAngleDeg - OverhangPenaltyStartAngle) / OverhangPenaltyRangeAngle,
		0.0f,
		1.0f
	);

	return FMath::Lerp(1.0f, OverhangMaxPenaltyScalar, PenaltyProgress);
}

void UClimbingMovementComponent::SetClimbingState(EClimbingState NewState)
{
	if (CurrentClimbingState == NewState)
	{
		return;
	}

	PreviousClimbingState = CurrentClimbingState;
	CurrentClimbingState = NewState;
	CurrentMontageCompletion = 0.0f;

	// Clear anchor when exiting climbing
	if (NewState == EClimbingState::None)
	{
		ClearAnchor();
	}
}

bool UClimbingMovementComponent::CanInterruptCurrentState() const
{
	const FClimbingStateConfig* Config = StateConfigs.Find(CurrentClimbingState);
	if (!Config)
	{
		// Missing config — default to interruptible for safety
		UE_LOG(LogClimbing, Warning, TEXT("ClimbingMovementComponent: Missing StateConfig for state %d"), 
			static_cast<int32>(CurrentClimbingState));
		return true;
	}

	if (Config->bInterruptible)
	{
		return true;
	}

	// Check completion threshold for conditionally interruptible states
	return CurrentMontageCompletion >= Config->MinCompletionBeforeCancel;
}

bool UClimbingMovementComponent::IsValidStateTransition(EClimbingState NewState) const
{
	// Always allow transitioning to None (drop/fail)
	if (NewState == EClimbingState::None)
	{
		return true;
	}

	// Check if current state can be interrupted
	if (!CanInterruptCurrentState())
	{
		return false;
	}

	// Specific transition rules
	switch (CurrentClimbingState)
	{
	case EClimbingState::None:
		// From None, can enter: Hanging, OnLadder, Mantling (via grab)
		return NewState == EClimbingState::Hanging || 
			   NewState == EClimbingState::OnLadder ||
			   NewState == EClimbingState::Mantling;

	case EClimbingState::Hanging:
		// From Hanging: Shimmying, ClimbingUp, CornerTransition, Lache, DroppingDown, BracedWall
		return NewState == EClimbingState::Shimmying ||
			   NewState == EClimbingState::ClimbingUp ||
			   NewState == EClimbingState::ClimbingUpCrouch ||
			   NewState == EClimbingState::CornerTransition ||
			   NewState == EClimbingState::Lache ||
			   NewState == EClimbingState::DroppingDown ||
			   NewState == EClimbingState::BracedWall ||
			   NewState == EClimbingState::Ragdoll;

	case EClimbingState::Shimmying:
		// From Shimmying: Hanging, CornerTransition, DroppingDown
		return NewState == EClimbingState::Hanging ||
			   NewState == EClimbingState::CornerTransition ||
			   NewState == EClimbingState::DroppingDown ||
			   NewState == EClimbingState::Ragdoll;

	case EClimbingState::BracedWall:
		// From BracedWall: BracedShimmying, Hanging, DroppingDown
		return NewState == EClimbingState::BracedShimmying ||
			   NewState == EClimbingState::Hanging ||
			   NewState == EClimbingState::DroppingDown ||
			   NewState == EClimbingState::Ragdoll;

	case EClimbingState::BracedShimmying:
		// From BracedShimmying: BracedWall, DroppingDown
		return NewState == EClimbingState::BracedWall ||
			   NewState == EClimbingState::DroppingDown ||
			   NewState == EClimbingState::Ragdoll;

	case EClimbingState::OnLadder:
		// From OnLadder: LadderTransition, DroppingDown
		return NewState == EClimbingState::LadderTransition ||
			   NewState == EClimbingState::DroppingDown ||
			   NewState == EClimbingState::Ragdoll;

	case EClimbingState::CornerTransition:
		// From Corner: Hanging (on completion)
		return NewState == EClimbingState::Hanging ||
			   NewState == EClimbingState::Ragdoll;

	case EClimbingState::LadderTransition:
		// From LadderTransition: OnLadder, None (on completion)
		return NewState == EClimbingState::OnLadder ||
			   NewState == EClimbingState::Ragdoll;

	case EClimbingState::ClimbingUp:
	case EClimbingState::ClimbingUpCrouch:
		// From ClimbingUp: None (on completion)
		return NewState == EClimbingState::Ragdoll;

	case EClimbingState::Mantling:
		// From Mantling: None (on completion)
		return NewState == EClimbingState::Ragdoll;

	case EClimbingState::Lache:
		// From Lache: LacheInAir
		return NewState == EClimbingState::LacheInAir ||
			   NewState == EClimbingState::Ragdoll;

	case EClimbingState::LacheInAir:
		// From LacheInAir: LacheCatch, LacheMiss
		return NewState == EClimbingState::LacheCatch ||
			   NewState == EClimbingState::LacheMiss ||
			   NewState == EClimbingState::Ragdoll;

	case EClimbingState::LacheCatch:
		// From LacheCatch: Hanging (on completion)
		return NewState == EClimbingState::Hanging ||
			   NewState == EClimbingState::Ragdoll;

	case EClimbingState::LacheMiss:
		// From LacheMiss: None (falling)
		return NewState == EClimbingState::Ragdoll;

	case EClimbingState::DroppingDown:
		// From DroppingDown: None
		return NewState == EClimbingState::Ragdoll;

	case EClimbingState::Ragdoll:
		// From Ragdoll: None (on recovery)
		return true; // Allow exit to None after recovery

	default:
		return false;
	}
}

void UClimbingMovementComponent::SetAnchor(UPrimitiveComponent* NewAnchor, const FVector& WorldGrabPoint)
{
	AnchorComponent = NewAnchor;

	if (NewAnchor)
	{
		// Calculate local transform relative to anchor
		const FTransform AnchorWorldTransform = NewAnchor->GetComponentTransform();
		const FVector LocalPosition = AnchorWorldTransform.InverseTransformPosition(WorldGrabPoint);
		AnchorLocalTransform = FTransform(FQuat::Identity, LocalPosition);

		// Set the character's movement base to the anchor for physics sync
		if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
		{
			Character->SetBase(NewAnchor);
		}
	}
	else
	{
		AnchorLocalTransform = FTransform::Identity;
	}
}

void UClimbingMovementComponent::ClearAnchor()
{
	AnchorComponent = nullptr;
	AnchorLocalTransform = FTransform::Identity;

	// Clear movement base
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->SetBase(nullptr);
	}
}

FVector UClimbingMovementComponent::GetWorldGrabPosition() const
{
	if (!AnchorComponent)
	{
		return FVector::ZeroVector;
	}

	const FTransform AnchorWorldTransform = AnchorComponent->GetComponentTransform();
	return AnchorWorldTransform.TransformPosition(AnchorLocalTransform.GetLocation());
}

void UClimbingMovementComponent::UpdateAnchorFollowing()
{
	if (!AnchorComponent)
	{
		return;
	}

	// Calculate where the character should be based on anchor movement
	const FVector TargetGrabPosition = GetWorldGrabPosition();
	
	// The character's grab point is typically at the ledge, which is above the character's location
	// We maintain the relative offset established when the grab was initiated
	
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		// The anchor following is handled by SetBase in most cases
		// This function is here for additional custom anchor logic if needed
		
		// Validate anchor is still valid
		if (!AnchorComponent->IsValidLowLevel() || AnchorComponent->IsPendingKill())
		{
			UE_LOG(LogClimbing, Warning, TEXT("ClimbingMovementComponent: Anchor invalidated, dropping"));
			
			// Notify character to handle dropped grab
			if (AClimbingCharacter* ClimbingChar = Cast<AClimbingCharacter>(Character))
			{
				// Character will handle state transition
				ClearAnchor();
			}
		}
	}
}

void UClimbingMovementComponent::OnRep_ClimbingState()
{
	// This is called on clients when the state is replicated
	// Simulated proxies need to play appropriate animations and resolve HitComponent
	
	if (AClimbingCharacter* ClimbingChar = Cast<AClimbingCharacter>(GetOwner()))
	{
		// Notify character of replicated state change
		ClimbingChar->OnClimbingStateReplicated(PreviousClimbingState, CurrentClimbingState);
	}
}
