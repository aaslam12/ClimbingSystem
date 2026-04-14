// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingCharacter.h"
// Part of AClimbingCharacter — see ClimbingCharacter.h
#include "Movement/ClimbingMovementComponent.h"
#include "Animation/ClimbingAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

void AClimbingCharacter::OnClimbingStateReplicated(EClimbingState OldState, EClimbingState NewState)
{
	// This is called on simulated proxies when state is replicated
	// Owning client and server handle state transitions differently

	if (IsLocallyControlled())
	{
		// Owning client already predicted and handled the state transition
		// Only need to correct if there's a mismatch (handled by confirm/reject RPCs)
		return;
	}

	// Simulated proxy: play entry montage and resolve HitComponent

	// Run confirmation trace immediately to resolve HitComponent for IK
	if (ClimbingMovement && ClimbingMovement->LastValidatedDetectionResult.bValid)
	{
		UPrimitiveComponent* ResolvedComponent = ResolveHitComponentFromNet(
			ClimbingMovement->LastValidatedDetectionResult
		);

		// Update local detection result with resolved component
		CurrentDetectionResult.LedgePosition = FVector(ClimbingMovement->LastValidatedDetectionResult.LedgePosition);
		CurrentDetectionResult.SurfaceNormal = FVector(ClimbingMovement->LastValidatedDetectionResult.SurfaceNormal);
		CurrentDetectionResult.SurfaceTier = ClimbingMovement->LastValidatedDetectionResult.SurfaceTier;
		CurrentDetectionResult.ClearanceType = ClimbingMovement->LastValidatedDetectionResult.ClearanceType;
		CurrentDetectionResult.HitComponent = ResolvedComponent;
		CurrentDetectionResult.bValid = ResolvedComponent != nullptr;

		if (!ResolvedComponent)
		{
			// Confirmation trace failed - IK will be disabled for this frame
			// Will retry on next IK update
			UE_LOG(LogClimbing, Verbose, TEXT("OnClimbingStateReplicated: Confirmation trace failed, IK disabled for %s"),
				*GetName());
		}
	}

	// Update animation instance state
	if (UClimbingAnimInstance* AnimInstance = CachedAnimInstance.Get())
	{
		AnimInstance->PreviousClimbingState = OldState;
		AnimInstance->CurrentClimbingState = NewState;
		AnimInstance->bIsClimbing = (NewState != EClimbingState::None);

		// Update surface data for animation
		if (CurrentDetectionResult.bValid)
		{
			AnimInstance->CurrentSurfaceNormal = CurrentDetectionResult.SurfaceNormal;
			AnimInstance->CurrentLedgePosition = CurrentDetectionResult.LedgePosition;
		}
	}

	// Play entry montage based on state
	EClimbingAnimationSlot EntrySlot = EClimbingAnimationSlot::HangIdle;

	switch (NewState)
	{
	case EClimbingState::Hanging:
		EntrySlot = EClimbingAnimationSlot::GrabLedge;
		break;
	case EClimbingState::Shimmying:
		EntrySlot = (CommittedShimmyDir < 0) ? EClimbingAnimationSlot::ShimmyLeft : EClimbingAnimationSlot::ShimmyRight;
		break;
	case EClimbingState::BracedWall:
		EntrySlot = EClimbingAnimationSlot::BracedIdle;
		break;
	case EClimbingState::BracedShimmying:
		EntrySlot = (CommittedShimmyDir < 0) ? EClimbingAnimationSlot::BracedShimmyLeft : EClimbingAnimationSlot::BracedShimmyRight;
		break;
	case EClimbingState::OnLadder:
		EntrySlot = EClimbingAnimationSlot::LadderIdle;
		break;
	case EClimbingState::CornerTransition:
		if (bCurrentCornerIsInside)
		{
			EntrySlot = (CommittedShimmyDir < 0) ? EClimbingAnimationSlot::CornerInsideLeft : EClimbingAnimationSlot::CornerInsideRight;
		}
		else
		{
			EntrySlot = (CommittedShimmyDir < 0) ? EClimbingAnimationSlot::CornerOutsideLeft : EClimbingAnimationSlot::CornerOutsideRight;
		}
		break;
	case EClimbingState::ClimbingUp:
		EntrySlot = EClimbingAnimationSlot::ClimbUp;
		break;
	case EClimbingState::ClimbingUpCrouch:
		EntrySlot = EClimbingAnimationSlot::ClimbUpCrouch;
		break;
	case EClimbingState::DroppingDown:
		EntrySlot = EClimbingAnimationSlot::DropDown;
		break;
	case EClimbingState::LadderTransition:
		// Determine exit direction based on replicated detection result validity
		EntrySlot = ClimbingMovement && ClimbingMovement->LastValidatedDetectionResult.bValid ?
			EClimbingAnimationSlot::LadderExitTop : EClimbingAnimationSlot::LadderExitBottom;
		break;
	case EClimbingState::Lache:
		EntrySlot = EClimbingAnimationSlot::LacheLaunch;
		break;
	case EClimbingState::LacheInAir:
		EntrySlot = EClimbingAnimationSlot::LacheFlight;
		break;
	case EClimbingState::LacheCatch:
		EntrySlot = EClimbingAnimationSlot::LacheCatch;
		break;
	case EClimbingState::LacheMiss:
		EntrySlot = EClimbingAnimationSlot::LacheMiss;
		break;
	case EClimbingState::Mantling:
		// Height-based selection would require additional replicated data
		EntrySlot = EClimbingAnimationSlot::MantleLow;
		break;
	case EClimbingState::Ragdoll:
		// No entry montage for ragdoll - physics takes over
		return;
	case EClimbingState::None:
		// Exiting climbing - no entry animation needed
		return;
	default:
		break;
	}

	if (UAnimMontage* EntryMontage = GetMontageForSlot(EntrySlot))
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			const float MontageDuration = AnimInstance->Montage_Play(EntryMontage);
			if (MontageDuration <= 0.0f)
			{
				UE_LOG(LogClimbing, Warning, TEXT("Montage_Play failed for slot %s — check ClimbingMontageSlot name matches ABP slot node name"), *ClimbingMontageSlot.ToString());
			}
		}
	}
}

// ============================================================================
// Input Mapping Context Management
// ============================================================================

UPrimitiveComponent* AClimbingCharacter::ResolveHitComponentFromNet(const FClimbingDetectionResultNet& NetResult) const
{
	if (!NetResult.bValid || !GetWorld())
	{
		return nullptr;
	}

	// Cast a short sphere trace from LedgePosition inward along the negated SurfaceNormal
	// to resolve the HitComponent locally
	const FVector TraceStart = NetResult.LedgePosition;
	const FVector TraceEnd = TraceStart - FVector(NetResult.SurfaceNormal) * ConfirmationTraceRadius * 2.0f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;

	// Use object type query for WorldStatic and WorldDynamic
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FHitResult HitResult;
	const bool bHit = GetWorld()->SweepSingleByObjectType(
		HitResult,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ObjectParams,
		FCollisionShape::MakeSphere(ConfirmationTraceRadius),
		QueryParams
	);

	if (bHit && HitResult.GetComponent())
	{
		return HitResult.GetComponent();
	}

	// Log warning on miss (once per miss)
	UE_LOG(LogClimbing, Warning, TEXT("ResolveHitComponentFromNet: Confirmation trace failed from %s toward %s on %s"),
		*NetResult.LedgePosition.ToString(),
		*FVector(NetResult.SurfaceNormal).ToString(),
		*GetName());

	return nullptr;
}

// ============================================================================
// Detection Helpers
// ============================================================================

void AClimbingCharacter::Server_AttemptGrab_Implementation(FClimbingDetectionResultNet ClientResult)
{
	if (!HasAuthority())
	{
		return;
	}

	// Server re-runs detection to validate client's claim
	FClimbingDetectionResult ServerDetection = PerformLedgeDetection();

	if (!ServerDetection.bValid)
	{
		// Try ladder detection as fallback
		ServerDetection = PerformLadderDetection();
	}

	if (!ServerDetection.bValid)
	{
		// No valid surface found - reject
		Client_RejectStateTransition();
		return;
	}

	// Validate client position within tolerance
	const float PositionDifference = FVector::Dist(
		FVector(ClientResult.LedgePosition),
		ServerDetection.LedgePosition
	);

	if (PositionDifference > ServerValidationPositionTolerance)
	{
		UE_LOG(LogClimbing, Warning, TEXT("Server_AttemptGrab: Position mismatch (%.1fcm > %.1fcm tolerance) on %s"),
			PositionDifference, ServerValidationPositionTolerance, *GetName());
		Client_RejectStateTransition();
		return;
	}

	// Verify anchor is not in streaming sublevel
	if (ServerDetection.HitComponent.IsValid())
	{
		if (AActor* HitOwner = ServerDetection.HitComponent->GetOwner())
		{
			ULevel* Level = HitOwner->GetLevel();
			if (Level && Level != GetWorld()->PersistentLevel)
			{
				UE_LOG(LogClimbing, Warning, TEXT("ClimbingSystem: Grab rejected — '%s' is in a streaming sublevel. Climbable surfaces must be in the persistent level."),
					*HitOwner->GetName());
				Client_RejectStateTransition();
				return;
			}
		}
	}

	// Determine target state based on surface type
	EClimbingState TargetState = EClimbingState::Hanging;
	if (ServerDetection.SurfaceTier == EClimbSurfaceTier::LadderOnly)
	{
		TargetState = EClimbingState::OnLadder;
	}

	// Update replicated state
	if (ClimbingMovement)
	{
		ClimbingMovement->LastValidatedDetectionResult = ServerDetection.ToNetResult();

		// Set up anchor
		if (ServerDetection.HitComponent.IsValid())
		{
			ClimbingMovement->SetAnchor(ServerDetection.HitComponent.Get(), GetActorLocation());
		}
	}

	// Transition to climbing state
	TransitionToState(TargetState, ServerDetection);

	// Confirm to client
	Client_ConfirmStateTransition(TargetState);
}

void AClimbingCharacter::Server_Drop_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	// Validate that we're in a droppable state
	if (!ClimbingMovement || ClimbingMovement->CurrentClimbingState == EClimbingState::None)
	{
		return;
	}

	// Check if current state allows drop
	if (!ClimbingMovement->CanInterruptCurrentState())
	{
		return;
	}

	// Transition to dropping down
	FClimbingDetectionResult EmptyResult;
	TransitionToState(EClimbingState::DroppingDown, EmptyResult);
}

void AClimbingCharacter::Server_AttemptLache_Implementation(FVector ClientArcTarget)
{
	if (!HasAuthority())
	{
		return;
	}

	// Validate we're in a state that allows Lache
	if (!ClimbingMovement || ClimbingMovement->CurrentClimbingState != EClimbingState::Hanging)
	{
		Client_RejectStateTransition();
		return;
	}

	// Server calculates arc and validates target
	FClimbingDetectionResult LacheTarget = CalculateLacheArc();

	if (!LacheTarget.bValid)
	{
		UE_LOG(LogClimbing, Verbose, TEXT("Server_AttemptLache: No valid target found on %s"), *GetName());
		Client_RejectStateTransition();
		return;
	}

	// Validate client's target within tolerance
	const float TargetDifference = FVector::Dist(ClientArcTarget, LacheTarget.LedgePosition);
	if (TargetDifference > ServerValidationPositionTolerance * 2.0f) // Larger tolerance for Lache
	{
		UE_LOG(LogClimbing, Warning, TEXT("Server_AttemptLache: Target mismatch (%.1fcm) on %s"),
			TargetDifference, *GetName());
		Client_RejectStateTransition();
		return;
	}

	// Lock the Lache target
	LockedLacheTarget = LacheTarget;
	LacheLaunchPosition = GetActorLocation();
	LacheLaunchDirection = GetActorForwardVector();
	LacheFlightTime = 0.0f;

	// Update replicated state
	if (ClimbingMovement)
	{
		ClimbingMovement->LastValidatedDetectionResult = LacheTarget.ToNetResult();
	}

	// Transition to Lache state
	TransitionToState(EClimbingState::Lache, LacheTarget);

	// Confirm to client
	Client_ConfirmStateTransition(EClimbingState::Lache);
}

void AClimbingCharacter::Server_AttemptClimbUp_Implementation()
{
	if (!HasAuthority())
	{
		return;
	}

	// Validate we're in a state that allows climb up
	if (!ClimbingMovement ||
		(ClimbingMovement->CurrentClimbingState != EClimbingState::Hanging &&
		 ClimbingMovement->CurrentClimbingState != EClimbingState::BracedWall))
	{
		Client_RejectStateTransition();
		return;
	}

	// Check clearance
	const EClimbClearanceType Clearance = CurrentDetectionResult.bValid ?
		CurrentDetectionResult.ClearanceType : EClimbClearanceType::None;

	if (Clearance == EClimbClearanceType::None)
	{
		UE_LOG(LogClimbing, Verbose, TEXT("Server_AttemptClimbUp: No clearance on %s"), *GetName());
		Client_RejectStateTransition();
		return;
	}

	// Determine target state based on clearance
	const EClimbingState TargetState = (Clearance == EClimbClearanceType::Full) ?
		EClimbingState::ClimbingUp : EClimbingState::ClimbingUpCrouch;

	// Transition
	TransitionToState(TargetState, CurrentDetectionResult);

	// Confirm to client
	Client_ConfirmStateTransition(TargetState);
}

void AClimbingCharacter::Server_UpdateShimmyDirection_Implementation(FVector2D Direction)
{
	if (!HasAuthority())
	{
		return;
	}

	// Update shimmy input on server
	CurrentClimbMoveInput = Direction;

	// Update committed direction with hysteresis
	if (FMath::Abs(Direction.X) > ShimmyDirectionDeadzone)
	{
		CommittedShimmyDir = FMath::Sign(Direction.X);
	}
}

// ============================================================================
// Client RPCs
// ============================================================================

void AClimbingCharacter::Client_RejectStateTransition_Implementation()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	// Stop any currently playing montage
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
		{
			AnimInst->Montage_Stop(PredictionRollbackBlendOut);
		}
	}

	// Play grab fail animation
	if (UAnimMontage* FailMontage = GetMontageForSlot(EClimbingAnimationSlot::GrabFail))
	{
		PlayAnimMontage(FailMontage);
	}

	// Play grab fail sound
	PlayClimbingSound(EClimbSoundType::GrabFail);

	// Lerp back to pre-prediction position
	if (!PrePredictionPosition.IsNearlyZero())
	{
		if (PredictionRollbackBlendOut > 0.0f)
		{
			bPredictionRollbackInProgress = true;
			PredictionRollbackStart = GetActorLocation();
			PredictionRollbackTarget = PrePredictionPosition;
			PredictionRollbackElapsed = 0.0f;
		}
		else
		{
			SetActorLocation(PrePredictionPosition, false);
			PrePredictionPosition = FVector::ZeroVector;
		}
	}

	// Ensure we're in None state
	if (ClimbingMovement && ClimbingMovement->CurrentClimbingState != EClimbingState::None)
	{
		FClimbingDetectionResult EmptyResult;
		TransitionToState(EClimbingState::None, EmptyResult);
	}

	UE_LOG(LogClimbing, Verbose, TEXT("Client_RejectStateTransition: Rolled back grab attempt on %s"), *GetName());
}

void AClimbingCharacter::Client_ConfirmStateTransition_Implementation(EClimbingState ConfirmedState)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	// Client already predicted this state - just validate
	if (ClimbingMovement && ClimbingMovement->CurrentClimbingState != ConfirmedState)
	{
		UE_LOG(LogClimbing, Verbose, TEXT("Client_ConfirmStateTransition: State mismatch - local %d, confirmed %d on %s"),
			static_cast<int32>(ClimbingMovement->CurrentClimbingState),
			static_cast<int32>(ConfirmedState),
			*GetName());

		// Force to confirmed state
		FClimbingDetectionResult EmptyResult;
		TransitionToState(ConfirmedState, EmptyResult);
	}

	// Clear pre-prediction position
	PrePredictionPosition = FVector::ZeroVector;
	bPredictionRollbackInProgress = false;
	PredictionRollbackElapsed = 0.0f;
}
