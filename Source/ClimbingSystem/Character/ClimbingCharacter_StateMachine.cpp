// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingCharacter.h"
// Part of AClimbingCharacter — see ClimbingCharacter.h
#include "Movement/ClimbingMovementComponent.h"
#include "Animation/ClimbingAnimInstance.h"
#include "Animation/ClimbingAnimationSet.h"
#include "Data/ClimbingSurfaceData.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/AssetManager.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/SpringArmComponent.h"
#include "MotionWarpingComponent.h"
#include "TimerManager.h"

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

	PendingStateTransitionTarget = NewState;
	OnStateExit(OldState);

	// Update state in movement component (handles replication)
	ClimbingMovement->SetClimbingState(NewState);

	// Store detection result
	CurrentDetectionResult = DetectionResult;

	// Enter new state
	OnStateEnter(NewState, DetectionResult);
	PendingStateTransitionTarget = EClimbingState::None;

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
	if (UClimbingAnimInstance* AnimInstance = CachedAnimInstance.Get())
	{
		AnimInstance->ResetIKNotifyMask();
	}

	// Handle IMC on first climbing state entry
	if (IsLocallyControlled() && ClimbingMovement && ClimbingMovement->PreviousClimbingState == EClimbingState::None && NewState != EClimbingState::None)
	{
		AddClimbingInputMappingContext();
	}

	const bool bUseAttachedClimbingCapsule =
		NewState == EClimbingState::Hanging ||
		NewState == EClimbingState::Shimmying ||
		NewState == EClimbingState::BracedWall ||
		NewState == EClimbingState::BracedShimmying ||
		NewState == EClimbingState::OnLadder ||
		NewState == EClimbingState::CornerTransition ||
		NewState == EClimbingState::LadderTransition;

	// Update capsule only for states where the pawn should stay attached to geometry.
	if (bUseAttachedClimbingCapsule)
	{
		if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			Capsule->SetCapsuleSize(ClimbingCapsuleRadius, ClimbingCapsuleHalfHeight);
			Capsule->SetCollisionProfileName(ClimbingCollisionProfile);
		}

		// Adjust camera probe radius for climbing
		if (CameraBoom && IsLocallyControlled())
		{
			CameraBoom->ProbeSize = ClimbingCameraProbeRadius;
		}
	}

	if (NewState != EClimbingState::None && NewState != EClimbingState::Ragdoll)
	{
		bUseControllerRotationYaw = false;
		if (DetectionResult.bValid)
		{
			SetActorRotation((-DetectionResult.SurfaceNormal).Rotation());
		}
	}

	const auto PlayStateMontage = [this](UAnimMontage* Montage)
	{
		if (!Montage)
		{
			return;
		}

		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			const float MontageDuration = AnimInstance->Montage_Play(Montage);
			if (MontageDuration <= 0.0f)
			{
				UE_LOG(LogClimbing, Warning, TEXT("Montage_Play failed for slot %s — check ClimbingMontageSlot name matches ABP slot node name"), *ClimbingMontageSlot.ToString());
			}
		}
	};

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
	}

	// State-specific entry logic
	switch (NewState)
	{
	case EClimbingState::Hanging:
		{
			if (ClimbingMovement)
			{
				ClimbingMovement->StopMovementImmediately();
				ClimbingMovement->Velocity = FVector::ZeroVector;
				if (DetectionResult.HitComponent.IsValid())
				{
					// SurfaceNormal points AWAY from the wall (toward player)
					// To position the character in front of the wall, we ADD the surface normal
					const float WallStandOff = ClimbingCapsuleRadius + 8.0f;
					const float HangVerticalOffset = ClimbingCapsuleHalfHeight + 12.0f;
					const FVector HangLocation = DetectionResult.LedgePosition
						+ DetectionResult.SurfaceNormal * WallStandOff
						- FVector::UpVector * HangVerticalOffset
						+ DetectionResult.SurfaceNormal * HangingCharacterOffset.X
						+ FVector::UpVector * HangingCharacterOffset.Z
						+ FVector::CrossProduct(FVector::UpVector, DetectionResult.SurfaceNormal) * HangingCharacterOffset.Y;
					SetActorLocation(HangLocation, false);
					ClimbingMovement->SetAnchor(DetectionResult.HitComponent.Get(), GetActorLocation());
				}
			}

			// Reset shimmy distance tracker
			ContinuousShimmyDistance = 0.0f;
			CommittedShimmyDir = 0.0f;
			IdleTimer = 0.0f;

			// Play grab animation if coming from None
			if (ClimbingMovement && ClimbingMovement->PreviousClimbingState == EClimbingState::None)
			{
				if (UAnimMontage* GrabMontage = GetMontageForSlot(EClimbingAnimationSlot::GrabLedge))
				{
					PlayStateMontage(GrabMontage);

					// After grab montage ends, start looping HangIdle
					if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
					{
						FOnMontageBlendingOutStarted BlendOutDelegate;
						BlendOutDelegate.BindWeakLambda(this, [this](UAnimMontage*, bool bInterrupted)
						{
							if (bInterrupted || !ClimbingMovement || ClimbingMovement->CurrentClimbingState != EClimbingState::Hanging)
							{
								return;
							}
							if (UAnimMontage* IdleMontage = GetMontageForSlot(EClimbingAnimationSlot::HangIdle))
							{
								if (UAnimInstance* AI = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
								{
									AI->Montage_Play(IdleMontage);
									if (!bEnableIdleVariations)
									{
										const FName Sec = IdleMontage->GetSectionName(0);
										AI->Montage_SetNextSection(Sec, Sec, IdleMontage);
									}
								}
							}
						});
						AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, GrabMontage);
					}

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
				
#if !UE_BUILD_SHIPPING
				UE_LOG(LogClimbing, Log, TEXT("Entered Hanging state. Clearance: %s. Press ClimbUp to pull up over ledge."),
					*UEnum::GetValueAsString(DetectionResult.ClearanceType));
#endif
			}
			else
			{
				// Play idle animation
				if (UAnimMontage* IdleMontage = GetMontageForSlot(EClimbingAnimationSlot::HangIdle))
				{
					PlayStateMontage(IdleMontage);

					// Loop the idle montage when variations are disabled
					if (!bEnableIdleVariations)
					{
						if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
						{
							const FName DefaultSection = IdleMontage->GetSectionName(0);
							AnimInstance->Montage_SetNextSection(DefaultSection, DefaultSection, IdleMontage);
						}
					}
				}
			}

			// Start idle variation timer
			if (bEnableIdleVariations && GetWorld())
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
			if (ClimbingMovement && DetectionResult.HitComponent.IsValid())
			{
				ClimbingMovement->StopMovementImmediately();
				ClimbingMovement->Velocity = FVector::ZeroVector;
				ClimbingMovement->SetAnchor(DetectionResult.HitComponent.Get(), GetActorLocation());
				SetBase(DetectionResult.HitComponent.Get());
			}

			// Play enter animation based on entry direction
			// For now, assume bottom entry
			if (UAnimMontage* EnterMontage = GetMontageForSlot(EClimbingAnimationSlot::LadderEnterBottom))
			{
				PlayStateMontage(EnterMontage);

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
			if (ClimbingMovement)
			{
				ClimbingMovement->StopMovementImmediately();
				ClimbingMovement->Velocity = FVector::ZeroVector;
				if (DetectionResult.HitComponent.IsValid())
				{
					ClimbingMovement->SetAnchor(DetectionResult.HitComponent.Get(), GetActorLocation());
					SetBase(DetectionResult.HitComponent.Get());
				}
			}

			ContinuousShimmyDistance = 0.0f;
			CommittedShimmyDir = 0.0f;

			if (UAnimMontage* BracedIdleMontage = GetMontageForSlot(EClimbingAnimationSlot::BracedIdle))
			{
				PlayStateMontage(BracedIdleMontage);
			}

			// SetBase is called in SetAnchor for braced states
		}
		break;

	case EClimbingState::Shimmying:
		{
			// Play shimmy montage based on committed direction
			const EClimbingAnimationSlot ShimmySlot = (CommittedShimmyDir < 0.0f) ?
				EClimbingAnimationSlot::ShimmyLeft : EClimbingAnimationSlot::ShimmyRight;
			if (UAnimMontage* ShimmyMontage = GetMontageForSlot(ShimmySlot))
			{
				PlayStateMontage(ShimmyMontage);
			}
		}
		break;

	case EClimbingState::BracedShimmying:
		{
			// Play braced shimmy montage based on committed direction
			const EClimbingAnimationSlot BracedShimmySlot = (CommittedShimmyDir < 0.0f) ?
				EClimbingAnimationSlot::BracedShimmyLeft : EClimbingAnimationSlot::BracedShimmyRight;
			if (UAnimMontage* BracedShimmyMontage = GetMontageForSlot(BracedShimmySlot))
			{
				PlayStateMontage(BracedShimmyMontage);
			}
		}
		break;

	case EClimbingState::DroppingDown:
		{
			// Calculate safe drop position - push character away from wall surface
			if (ClimbingMovement && ClimbingMovement->PreviousClimbingState != EClimbingState::None)
			{
				// Use the previous detection result for surface normal
				// The SurfaceNormal points AWAY from the wall, so adding it moves us away
				const float DropOffsetDistance = ClimbingCapsuleRadius + 20.0f;
				const FVector DropOffset = CurrentDetectionResult.SurfaceNormal * DropOffsetDistance;
				const FVector NewLocation = GetActorLocation() + DropOffset;
				SetActorLocation(NewLocation, false);
			}

			if (UAnimMontage* DropMontage = GetMontageForSlot(EClimbingAnimationSlot::DropDown))
			{
				PlayStateMontage(DropMontage);
			}
		}
		break;

	case EClimbingState::Lache:
		{
			// Play lache launch montage
			if (UAnimMontage* LacheMontage = GetMontageForSlot(EClimbingAnimationSlot::LacheLaunch))
			{
				PlayStateMontage(LacheMontage);

				// Bind montage completion callback to transition to LacheInAir
				if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
				{
					FOnMontageBlendingOutStarted BlendOutDelegate;
					BlendOutDelegate.BindUObject(this, &AClimbingCharacter::OnLacheLaunchMontageBlendingOut);
					AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, LacheMontage);
				}
			}
			PlayClimbingSound(EClimbSoundType::LacheLaunchGrunt);
		}
		break;

	case EClimbingState::LacheInAir:
		{
			// Play lache flight montage (looping)
			if (UAnimMontage* FlightMontage = GetMontageForSlot(EClimbingAnimationSlot::LacheFlight))
			{
				PlayStateMontage(FlightMontage);
			}
		}
		break;

	case EClimbingState::LacheCatch:
		{
			// Play lache catch montage
			if (UAnimMontage* CatchMontage = GetMontageForSlot(EClimbingAnimationSlot::LacheCatch))
			{
				PlayStateMontage(CatchMontage);

				// Bind montage completion callback to transition to Hanging
				if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
				{
					FOnMontageBlendingOutStarted BlendOutDelegate;
					BlendOutDelegate.BindUObject(this, &AClimbingCharacter::OnLacheCatchMontageBlendingOut);
					AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, CatchMontage);
				}
			}
			PlayClimbingSound(EClimbSoundType::LacheCatchImpact);
		}
		break;

	case EClimbingState::LacheMiss:
		{
			// Play lache miss montage
			if (UAnimMontage* MissMontage = GetMontageForSlot(EClimbingAnimationSlot::LacheMiss))
			{
				PlayStateMontage(MissMontage);
			}
		}
		break;

	case EClimbingState::ClimbingUp:
		{
			// Play climb up montage
			UAnimMontage* ClimbUpMontage = GetMontageForSlot(EClimbingAnimationSlot::ClimbUp);
			if (ClimbUpMontage)
			{
				PlayStateMontage(ClimbUpMontage);

				// Bind montage completion callback to transition to None
				if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
				{
					FOnMontageBlendingOutStarted BlendOutDelegate;
					BlendOutDelegate.BindUObject(this, &AClimbingCharacter::OnClimbUpMontageBlendingOut);
					AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, ClimbUpMontage);
				}

				// Set up motion warp target
				if (MotionWarping && DetectionResult.bValid)
				{
					FMotionWarpingTarget WarpTarget;
					WarpTarget.Name = FName("WarpTarget_ClimbUp");
					WarpTarget.Location = DetectionResult.LedgePosition;
					WarpTarget.Rotation = (-DetectionResult.SurfaceNormal).Rotation();
					MotionWarping->AddOrUpdateWarpTarget(WarpTarget);
				}
			}
			else
			{
				UE_LOG(LogClimbing, Warning, TEXT("ClimbingUp: No montage assigned for ClimbUp slot. Using top-out fallback."));
				// Fallback: place the capsule in a safe standing position on top of the ledge.
				if (DetectionResult.bValid)
				{
					const FVector SafeTopOutLocation =
						DetectionResult.LedgePosition +
						DetectionResult.SurfaceNormal * (OriginalCapsuleRadius + 8.0f) +
						FVector::UpVector * OriginalCapsuleHalfHeight;

					FHitResult SweepHit;
					SetActorLocation(SafeTopOutLocation, true, &SweepHit, ETeleportType::TeleportPhysics);
					SetActorRotation((-DetectionResult.SurfaceNormal).Rotation());
				}

				// Auto-exit after short delay to restore locomotion state.
				if (GetWorld())
				{
					FTimerHandle UnusedHandle;
					GetWorld()->GetTimerManager().SetTimer(UnusedHandle, [this]()
					{
						if (ClimbingMovement && ClimbingMovement->CurrentClimbingState == EClimbingState::ClimbingUp)
						{
							TransitionToState(EClimbingState::None, FClimbingDetectionResult());
						}
					}, 0.05f, false);
				}
			}
		}
		break;

	case EClimbingState::ClimbingUpCrouch:
		{
			// Play climb up crouch montage
			UAnimMontage* ClimbUpCrouchMontage = GetMontageForSlot(EClimbingAnimationSlot::ClimbUpCrouch);
			if (ClimbUpCrouchMontage)
			{
				PlayStateMontage(ClimbUpCrouchMontage);

				// Bind montage completion callback to transition to None
				if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
				{
					FOnMontageBlendingOutStarted BlendOutDelegate;
					BlendOutDelegate.BindUObject(this, &AClimbingCharacter::OnClimbUpMontageBlendingOut);
					AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, ClimbUpCrouchMontage);
				}

				// Set up motion warp target
				if (MotionWarping && DetectionResult.bValid)
				{
					FMotionWarpingTarget WarpTarget;
					WarpTarget.Name = FName("WarpTarget_ClimbUpCrouch");
					WarpTarget.Location = DetectionResult.LedgePosition;
					WarpTarget.Rotation = (-DetectionResult.SurfaceNormal).Rotation();
					MotionWarping->AddOrUpdateWarpTarget(WarpTarget);
				}
			}
			else
			{
				UE_LOG(LogClimbing, Warning, TEXT("ClimbingUpCrouch: No montage assigned for ClimbUpCrouch slot. Using top-out fallback."));
				// Fallback: place the capsule in a safe standing position on top of the ledge.
				if (DetectionResult.bValid)
				{
					const FVector SafeTopOutLocation =
						DetectionResult.LedgePosition +
						DetectionResult.SurfaceNormal * (OriginalCapsuleRadius + 8.0f) +
						FVector::UpVector * (OriginalCapsuleHalfHeight * 0.75f);

					FHitResult SweepHit;
					SetActorLocation(SafeTopOutLocation, true, &SweepHit, ETeleportType::TeleportPhysics);
					SetActorRotation((-DetectionResult.SurfaceNormal).Rotation());
				}

				// Auto-exit after short delay to restore locomotion state.
				if (GetWorld())
				{
					FTimerHandle UnusedHandle;
					GetWorld()->GetTimerManager().SetTimer(UnusedHandle, [this]()
					{
						if (ClimbingMovement && ClimbingMovement->CurrentClimbingState == EClimbingState::ClimbingUpCrouch)
						{
							TransitionToState(EClimbingState::None, FClimbingDetectionResult());
						}
					}, 0.05f, false);
				}
			}
		}
		break;

	case EClimbingState::CornerTransition:
		{
			// Determine inside/outside and left/right for montage selection
			EClimbingAnimationSlot CornerSlot;
			if (bCurrentCornerIsInside)
			{
				CornerSlot = (CommittedShimmyDir < 0.0f) ?
					EClimbingAnimationSlot::CornerInsideLeft : EClimbingAnimationSlot::CornerInsideRight;
			}
			else
			{
				CornerSlot = (CommittedShimmyDir < 0.0f) ?
					EClimbingAnimationSlot::CornerOutsideLeft : EClimbingAnimationSlot::CornerOutsideRight;
			}

			if (UAnimMontage* CornerMontage = GetMontageForSlot(CornerSlot))
			{
				PlayStateMontage(CornerMontage);

				// Bind montage completion callback to transition to Hanging
				if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
				{
					FOnMontageBlendingOutStarted BlendOutDelegate;
					BlendOutDelegate.BindUObject(this, &AClimbingCharacter::OnCornerTransitionMontageBlendingOut);
					AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, CornerMontage);
				}

				// Set up motion warp target for corner destination
				if (MotionWarping && DetectionResult.bValid)
				{
					FMotionWarpingTarget WarpTarget;
					WarpTarget.Name = FName("WarpTarget_Corner");
					WarpTarget.Location = DetectionResult.LedgePosition;
					WarpTarget.Rotation = (-DetectionResult.SurfaceNormal).Rotation();
					MotionWarping->AddOrUpdateWarpTarget(WarpTarget);
				}
			}

			// Update animation instance with corner type
			if (UClimbingAnimInstance* AnimInstance = CachedAnimInstance.Get())
			{
				AnimInstance->bIsInsideCorner = bCurrentCornerIsInside;
			}
		}
		break;

	case EClimbingState::LadderTransition:
		{
			// Determine exit direction - top or bottom
			EClimbingAnimationSlot LadderExitSlot;
			if (DetectionResult.bValid)
			{
				// Valid ledge means exiting at top
				LadderExitSlot = EClimbingAnimationSlot::LadderExitTop;
			}
			else
			{
				// No ledge means exiting at bottom
				LadderExitSlot = EClimbingAnimationSlot::LadderExitBottom;
			}

			if (UAnimMontage* LadderExitMontage = GetMontageForSlot(LadderExitSlot))
			{
				PlayStateMontage(LadderExitMontage);

				// Bind montage completion callback to transition to None
				if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
				{
					FOnMontageBlendingOutStarted BlendOutDelegate;
					BlendOutDelegate.BindUObject(this, &AClimbingCharacter::OnLadderTransitionMontageBlendingOut);
					AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, LadderExitMontage);
				}

				// Set up motion warp target if exiting at top
				if (MotionWarping && DetectionResult.bValid)
				{
					FMotionWarpingTarget WarpTarget;
					WarpTarget.Name = FName("WarpTarget_LadderExit");
					WarpTarget.Location = DetectionResult.LedgePosition;
					WarpTarget.Rotation = (-DetectionResult.SurfaceNormal).Rotation();
					MotionWarping->AddOrUpdateWarpTarget(WarpTarget);
				}
			}
		}
		break;

	case EClimbingState::Mantling:
		{
			// Select mantle animation based on obstacle height
			// Calculate height difference between ledge and character feet
			const float LedgeHeight = DetectionResult.bValid ? DetectionResult.LedgePosition.Z : GetActorLocation().Z;
			const float CharacterFeetZ = GetActorLocation().Z - OriginalCapsuleHalfHeight;
			const float MantleHeight = LedgeHeight - CharacterFeetZ;
			
			EClimbingAnimationSlot MantleSlot = (MantleHeight > MantleLowMaxHeight) ?
				EClimbingAnimationSlot::MantleHigh : EClimbingAnimationSlot::MantleLow;

			UAnimMontage* MantleMontage = GetMontageForSlot(MantleSlot);
			if (MantleMontage)
			{
				PlayStateMontage(MantleMontage);

				// Bind montage completion callback to transition to None
				if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
				{
					FOnMontageBlendingOutStarted BlendOutDelegate;
					BlendOutDelegate.BindUObject(this, &AClimbingCharacter::OnClimbUpMontageBlendingOut);
					AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, MantleMontage);
				}

				// Set up motion warp target for mantle destination
				if (MotionWarping && DetectionResult.bValid)
				{
					FMotionWarpingTarget WarpTarget;
					WarpTarget.Name = FName("WarpTarget_Mantle");
					WarpTarget.Location = DetectionResult.LedgePosition;
					WarpTarget.Rotation = (-DetectionResult.SurfaceNormal).Rotation();
					MotionWarping->AddOrUpdateWarpTarget(WarpTarget);
				}
			}
			else
			{
				UE_LOG(LogClimbing, Error, TEXT("Mantling: No montage assigned for %s slot! Character will be stuck. Assign montage in Animation Set or use auto-transition fallback."),
					(MantleHeight > MantleLowMaxHeight) ? TEXT("MantleHigh") : TEXT("MantleLow"));
				// Fallback: teleport to ledge and exit climbing
				if (DetectionResult.bValid && MotionWarping)
				{
					SetActorLocation(DetectionResult.LedgePosition + DetectionResult.SurfaceNormal * 50.0f);
				}
				// Auto-exit after short delay
				if (GetWorld())
				{
					FTimerHandle UnusedHandle;
					GetWorld()->GetTimerManager().SetTimer(UnusedHandle, [this]()
					{
						if (ClimbingMovement && ClimbingMovement->CurrentClimbingState == EClimbingState::Mantling)
						{
							TransitionToState(EClimbingState::None, FClimbingDetectionResult());
						}
					}, 0.5f, false);
				}
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
	const EClimbingState TargetState = PendingStateTransitionTarget;

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
			const bool bShouldDetachFromSurface =
				TargetState == EClimbingState::None ||
				TargetState == EClimbingState::DroppingDown ||
				TargetState == EClimbingState::ClimbingUp ||
				TargetState == EClimbingState::ClimbingUpCrouch ||
				TargetState == EClimbingState::Mantling ||
				TargetState == EClimbingState::Lache ||
				TargetState == EClimbingState::LacheInAir ||
				TargetState == EClimbingState::LacheMiss ||
				TargetState == EClimbingState::Ragdoll;

			if (bShouldDetachFromSurface)
			{
				SetBase(nullptr);

				if (ClimbingMovement)
				{
					ClimbingMovement->ClearAnchor();
				}
				if (UCapsuleComponent* Capsule = GetCapsuleComponent())
				{
					Capsule->SetCapsuleSize(OriginalCapsuleRadius, OriginalCapsuleHalfHeight);
					Capsule->SetCollisionProfileName(OriginalCollisionProfile);
					// Explicitly re-enable collision to handle edge cases where it may have been disabled
					Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				}
			}
		}
		break;

	case EClimbingState::Ragdoll:
		{
			if (CameraBoom)
			{
				CameraBoom->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			}

			if (UCapsuleComponent* Capsule = GetCapsuleComponent())
			{
				Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				Capsule->SetCapsuleSize(OriginalCapsuleRadius, OriginalCapsuleHalfHeight);
				Capsule->SetCollisionProfileName(OriginalCollisionProfile);
			}
		}
		break;

	default:
		break;
	}

	if (TargetState == EClimbingState::None)
	{
		if (IsLocallyControlled())
		{
			RemoveClimbingInputMappingContext();
		}

		// Keep bUseControllerRotationYaw false - don't orient to camera after exiting climb
		// The character rotation is managed by movement component's orient to movement setting

		if (CameraBoom && IsLocallyControlled())
		{
			CameraBoom->ProbeSize = OriginalCameraProbeSize;
		}

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
	if (IsLocallyControlled() &&
		(CurrentState == EClimbingState::Hanging ||
		CurrentState == EClimbingState::Shimmying ||
		CurrentState == EClimbingState::BracedWall ||
		CurrentState == EClimbingState::BracedShimmying ||
		CurrentState == EClimbingState::OnLadder))
	{
		FClimbingDetectionResult RefreshedDetection = PerformLedgeDetection();
		if (!RefreshedDetection.bValid)
		{
			RefreshedDetection = PerformLadderDetection();
		}

		if (RefreshedDetection.bValid)
		{
			CurrentDetectionResult = RefreshedDetection;
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

	case EClimbingState::DroppingDown:
		// Intentional drops should quickly return to locomotion/falling control
		// so the player can air-control and attempt re-grabs before landing.
		if (ClimbingMovement->IsFalling())
		{
			TransitionToState(EClimbingState::None, FClimbingDetectionResult());
		}
		break;

	case EClimbingState::Ragdoll:
		// Ragdoll is physics-driven, recovery handled by timer
		break;

	default:
		break;
	}

	// Update IK for active climb states. Proxies can recover from failed confirmation traces here.
	if (CurrentState != EClimbingState::None && CurrentState != EClimbingState::Ragdoll)
	{
		UpdateClimbingIK(DeltaTime);
	}

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		// Draw current + previous state text
		if (GEngine && ClimbingMovement)
		{
			const FString StateText = FString::Printf(TEXT("Climbing: %s (prev: %s) | ShimmyDir: %.1f"),
				*UEnum::GetValueAsString(CurrentState),
				*UEnum::GetValueAsString(ClimbingMovement->PreviousClimbingState),
				CommittedShimmyDir);
			GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, StateText);
		}

		// Draw anchor point (cyan)
		if (ClimbingMovement && ClimbingMovement->AnchorComponent)
		{
			const FTransform WorldAnchor = ClimbingMovement->AnchorLocalTransform * ClimbingMovement->AnchorComponent->GetComponentTransform();
			DrawDebugSphere(GetWorld(), WorldAnchor.GetLocation(), 12.0f, 8, FColor::Cyan, false, 0.0f);
			DrawDebugDirectionalArrow(GetWorld(), WorldAnchor.GetLocation(),
				WorldAnchor.GetLocation() + WorldAnchor.GetRotation().GetForwardVector() * 30.0f,
				8.0f, FColor::Cyan, false, 0.0f);
		}

		// Draw climbing capsule bounds (orange)
		if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			const float Radius = Capsule->GetScaledCapsuleRadius();
			DrawDebugCapsule(GetWorld(), Capsule->GetComponentLocation(), HalfHeight, Radius,
				Capsule->GetComponentQuat(), FColor::Orange, false, 0.0f);
		}
	}
#endif
}

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

void AClimbingCharacter::AddClimbingInputMappingContext()
{
	if (!IsLocallyControlled() || !ClimbingInputMappingContext || bClimbingIMCActive)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				Subsystem->AddMappingContext(ClimbingInputMappingContext, ClimbingIMCPriority);
				bClimbingIMCActive = true;
			}
		}
	}
}

void AClimbingCharacter::AddLocomotionInputMappingContext()
{
	if (!IsLocallyControlled() || !LocomotionInputMappingContext || bLocomotionIMCActive)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				Subsystem->AddMappingContext(LocomotionInputMappingContext, 0);
				bLocomotionIMCActive = true;
			}
		}
	}
}

void AClimbingCharacter::RemoveClimbingInputMappingContext()
{
	if (!IsLocallyControlled() || !ClimbingInputMappingContext || !bClimbingIMCActive)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				Subsystem->RemoveMappingContext(ClimbingInputMappingContext);
			}
		}
	}

	bClimbingIMCActive = false;
}

void AClimbingCharacter::RemoveLocomotionInputMappingContext()
{
	if (!IsLocallyControlled() || !LocomotionInputMappingContext || !bLocomotionIMCActive)
	{
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				Subsystem->RemoveMappingContext(LocomotionInputMappingContext);
			}
		}
	}

	bLocomotionIMCActive = false;
}

void AClimbingCharacter::OnClimbUpMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (!ClimbingMovement)
	{
		return;
	}

	// Only transition to None if we're still in a climb up or mantle state
	const EClimbingState CurrentState = ClimbingMovement->CurrentClimbingState;
	if (CurrentState == EClimbingState::ClimbingUp ||
		CurrentState == EClimbingState::ClimbingUpCrouch ||
		CurrentState == EClimbingState::Mantling)
	{
		// If interrupted (e.g., by ragdoll), don't force transition to None
		if (!bInterrupted)
		{
			TransitionToState(EClimbingState::None, FClimbingDetectionResult());
		}
	}
}

void AClimbingCharacter::OnCornerTransitionMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (!ClimbingMovement)
	{
		return;
	}

	// Only transition if we're still in CornerTransition state
	if (ClimbingMovement->CurrentClimbingState == EClimbingState::CornerTransition)
	{
		if (!bInterrupted)
		{
			// After corner transition, go back to Hanging on the new surface
			// Use current detection result which should have been updated during transition
			TransitionToState(EClimbingState::Hanging, CurrentDetectionResult);
		}
	}
}

void AClimbingCharacter::OnLadderTransitionMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (!ClimbingMovement)
	{
		return;
	}

	// Only transition if we're still in LadderTransition state
	if (ClimbingMovement->CurrentClimbingState == EClimbingState::LadderTransition)
	{
		if (!bInterrupted)
		{
			// Ladder transition ends in None (character exits ladder)
			TransitionToState(EClimbingState::None, FClimbingDetectionResult());
		}
	}
}

void AClimbingCharacter::OnLacheLaunchMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (!ClimbingMovement)
	{
		return;
	}

	// Only transition if we're still in Lache state
	if (ClimbingMovement->CurrentClimbingState == EClimbingState::Lache)
	{
		if (!bInterrupted)
		{
			// After launch animation, transition to in-air flight
			TransitionToState(EClimbingState::LacheInAir, LockedLacheTarget);
		}
	}
}

void AClimbingCharacter::OnLacheCatchMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (!ClimbingMovement)
	{
		return;
	}

	// Only transition if we're still in LacheCatch state
	if (ClimbingMovement->CurrentClimbingState == EClimbingState::LacheCatch)
	{
		if (!bInterrupted)
		{
			// After catching, transition to Hanging on the target ledge
			TransitionToState(EClimbingState::Hanging, LockedLacheTarget);
		}
	}
}
