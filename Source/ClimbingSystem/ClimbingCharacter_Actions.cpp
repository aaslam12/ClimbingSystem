// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingCharacter.h"
// Part of AClimbingCharacter — see ClimbingCharacter.h
#include "ClimbingMovementComponent.h"
#include "ClimbingAnimInstance.h"
#include "ClimbingSurfaceData.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Controller.h"

void AClimbingCharacter::Input_Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}

	// If climbing, update climb move input instead of using AddMovementInput
	if (ClimbingMovement && ClimbingMovement->CurrentClimbingState != EClimbingState::None)
	{
		CurrentClimbMoveInput = MovementVector;
		
		// Update committed shimmy direction with hysteresis
		if (FMath::Abs(CurrentClimbMoveInput.X) > ShimmyDirectionDeadzone)
		{
			CommittedShimmyDir = FMath::Sign(CurrentClimbMoveInput.X);
		}
		return;
	}

	// Normal locomotion movement
	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void AClimbingCharacter::Input_Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (!Controller)
	{
		return;
	}

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AClimbingCharacter::Input_JumpStarted(const FInputActionValue& Value)
{
	// Handle jump during climbing
	if (ClimbingMovement && ClimbingMovement->CurrentClimbingState != EClimbingState::None)
	{
		const EClimbingState CurrentState = ClimbingMovement->CurrentClimbingState;
		
		// Jump from hanging - trigger lache (directional jump to next ledge)
		if (CurrentState == EClimbingState::Hanging)
		{
			Input_Lache(Value);
		}
		// Other climbing states - ignore jump
		return;
	}

	// Normal locomotion jump
	Jump();
}

void AClimbingCharacter::Input_JumpCompleted(const FInputActionValue& Value)
{
	// Only stop jumping if not climbing
	if (ClimbingMovement && ClimbingMovement->CurrentClimbingState != EClimbingState::None)
	{
		return;
	}

	StopJumping();
}

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

		// If normal detection failed and we're on ground, try simplified mantle detection
		if (!DetectionResult.bValid && !ClimbingMovement->IsFalling())
		{
			// Simplified forward trace for low obstacles
			const FVector CharacterLocation = GetActorLocation();
			const FVector ForwardVector = GetActorForwardVector();
			const FVector ForwardTraceStart = CharacterLocation + FVector::UpVector * 50.0f; // Waist height
			const FVector ForwardTraceEnd = ForwardTraceStart + ForwardVector * LedgeDetectionForwardReach;

			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this);
			QueryParams.bTraceComplex = false;

			FHitResult ForwardHit;
			if (GetWorld() && GetWorld()->SweepSingleByChannel(
				ForwardHit,
				ForwardTraceStart,
				ForwardTraceEnd,
				FQuat::Identity,
				ECC_WorldStatic,
				FCollisionShape::MakeSphere(LedgeDetectionRadius),
				QueryParams))
			{
				// Check if it's climbable
				const bool bHasClimbableTag = ForwardHit.Component.IsValid() && 
					(ForwardHit.Component->ComponentHasTag(FName("Climbable")) || 
					 !ForwardHit.Component->ComponentHasTag(FName("Unclimbable")));

				if (bHasClimbableTag)
				{
					// Use the top of the hit as the ledge position
					FVector LedgePos = ForwardHit.ImpactPoint;
					LedgePos.Z = ForwardHit.Component->Bounds.GetBox().Max.Z;

					const float CharacterFeetZ = CharacterLocation.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
					const float ObstacleHeight = LedgePos.Z - CharacterFeetZ;

					// Check if it's in mantle height range
					if (ObstacleHeight > MantleStepMaxHeight && ObstacleHeight <= MantleHighMaxHeight)
					{
						DetectionResult.bValid = true;
						DetectionResult.LedgePosition = LedgePos;
						DetectionResult.SurfaceNormal = ForwardHit.ImpactNormal;
						DetectionResult.HitComponent = ForwardHit.Component;
						DetectionResult.ClearanceType = EClimbClearanceType::Full; // Assume full for mantles
						DetectionResult.SurfaceTier = EClimbSurfaceTier::Climbable;

						UE_LOG(LogClimbing, Log, TEXT("Mantle detection: Found obstacle at height %.1f cm"), ObstacleHeight);
					}
				}
			}
		}

		if (DetectionResult.bValid)
		{
			// Determine target state based on surface type and ledge height
			EClimbingState TargetState = EClimbingState::Hanging;

			if (DetectionResult.SurfaceTier == EClimbSurfaceTier::LadderOnly)
			{
				TargetState = EClimbingState::OnLadder;
			}
			else
			{
				// Check if this is a low ledge that should be mantled instead of hung from
				// Only mantle from ground, not while falling (mantling needs stable footing)
				if (!ClimbingMovement->IsFalling() && DetectionResult.ClearanceType != EClimbClearanceType::None)
				{
					const float CharacterFeetZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
					const float LedgeHeight = DetectionResult.LedgePosition.Z - CharacterFeetZ;

					// If ledge is within mantle range and has clearance, mantle over it
					if (LedgeHeight > MantleStepMaxHeight && LedgeHeight <= MantleHighMaxHeight)
					{
						TargetState = EClimbingState::Mantling;
					}
				}
			}

			// Initiate grab/mantle
			if (HasAuthority())
			{
				TransitionToState(TargetState, DetectionResult);
			}
			else
			{
				// Client prediction + server RPC
				PrePredictionPosition = GetActorLocation();
				TransitionToState(TargetState, DetectionResult);
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
	
	// Calculate launch direction based on movement input
	// If no input, launch forward. If has input, blend forward with shimmy direction
	FVector LaunchDir = GetActorForwardVector();
	if (!FMath::IsNearlyZero(CurrentClimbMoveInput.X))
	{
		// Add horizontal shimmy component to launch direction
		const FVector ShimmyDir = GetActorRightVector() * CurrentClimbMoveInput.X;
		LaunchDir = (LaunchDir + ShimmyDir * 0.5f).GetSafeNormal(); // Blend 2:1 forward to shimmy
	}
	LacheLaunchDirection = LaunchDir;
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
	
	// Calculate launch direction based on movement input
	// If no input, launch forward. If has input, blend forward with shimmy direction
	FVector LaunchDir = GetActorForwardVector();
	
	if (!FMath::IsNearlyZero(CurrentClimbMoveInput.X))
	{
		// Add horizontal shimmy component to launch direction
		const FVector ShimmyDir = GetActorRightVector() * CurrentClimbMoveInput.X;
		LaunchDir = (LaunchDir + ShimmyDir * 0.5f).GetSafeNormal(); // Blend 2:1 forward to shimmy
	}
	
	const FVector ArcVelocity = LaunchDir * LacheLaunchSpeed;
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

void AClimbingCharacter::Input_ClimbUp(const FInputActionValue& Value)
{
	if (!ClimbingMovement)
	{
		UE_LOG(LogClimbing, Warning, TEXT("Input_ClimbUp: ClimbingMovement is null!"));
		return;
	}

	const EClimbingState CurrentState = ClimbingMovement->CurrentClimbingState;

	// Climb up is valid from Hanging
	if (CurrentState != EClimbingState::Hanging)
	{
		UE_LOG(LogClimbing, Log, TEXT("Input_ClimbUp: Not in Hanging state (current: %s)"), *UEnum::GetValueAsString(CurrentState));
		return;
	}

	if (!ClimbingMovement->CanInterruptCurrentState())
	{
		UE_LOG(LogClimbing, Log, TEXT("Input_ClimbUp: Cannot interrupt current state"));
		return;
	}

	// Check clearance from current detection result
	const EClimbClearanceType Clearance = CurrentDetectionResult.ClearanceType;

	if (Clearance == EClimbClearanceType::None)
	{
		// Cannot climb up - no clearance
		UE_LOG(LogClimbing, Warning, TEXT("Input_ClimbUp: No clearance above ledge! Cannot climb up. Move to a location with overhead clearance."));
		return;
	}

	const EClimbingState ClimbUpState = (Clearance == EClimbClearanceType::Full) ?
		EClimbingState::ClimbingUp : EClimbingState::ClimbingUpCrouch;

	UE_LOG(LogClimbing, Log, TEXT("Input_ClimbUp: Transitioning to %s"), *UEnum::GetValueAsString(ClimbUpState));

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

	// Send direction update to server (unreliable, can be called frequently)
	if (!HasAuthority() && IsLocallyControlled())
	{
		Server_UpdateShimmyDirection(CurrentClimbMoveInput);
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
		if (ClimbingMovement->AnchorComponent)
		{
			const FTransform AnchorWorldTransform = ClimbingMovement->AnchorComponent->GetComponentTransform();
			ClimbingMovement->AnchorLocalTransform = FTransform(FQuat::Identity, AnchorWorldTransform.InverseTransformPosition(NewLocation));
		}
		SetActorLocation(NewLocation, false);

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

	// Send direction update to server (unreliable, can be called frequently)
	if (!HasAuthority() && IsLocallyControlled())
	{
		Server_UpdateShimmyDirection(CurrentClimbMoveInput);
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
		if (ClimbingMovement->AnchorComponent)
		{
			const FTransform AnchorWorldTransform = ClimbingMovement->AnchorComponent->GetComponentTransform();
			ClimbingMovement->AnchorLocalTransform = FTransform(FQuat::Identity, AnchorWorldTransform.InverseTransformPosition(NewLocation));
		}
		SetActorLocation(NewLocation, false);

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

	if (ClimbingMovement->AnchorComponent)
	{
		const FTransform AnchorWorldTransform = ClimbingMovement->AnchorComponent->GetComponentTransform();
		ClimbingMovement->AnchorLocalTransform = FTransform(FQuat::Identity, AnchorWorldTransform.InverseTransformPosition(NewLocation));
	}
	SetActorLocation(NewLocation, false);

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

	// Play the appropriate ladder animation montage
	if (UAnimMontage* LadderMontage = GetMontageForSlot(TargetSlot))
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			// Only play if not already playing this montage
			if (AnimInstance->GetCurrentActiveMontage() != LadderMontage)
			{
				AnimInstance->Montage_Play(LadderMontage);
			}
		}
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
