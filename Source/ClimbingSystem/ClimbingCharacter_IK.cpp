// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingCharacter.h"
// Part of AClimbingCharacter — see ClimbingCharacter.h
#include "ClimbingMovementComponent.h"
#include "ClimbingAnimInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"

TArray<TWeakObjectPtr<AClimbingCharacter>> AClimbingCharacter::ActiveClimbingCharacters;

void AClimbingCharacter::UpdateClimbingIK(float DeltaTime)
{
	UClimbingAnimInstance* AnimInst = CachedAnimInstance.Get();
	if (!AnimInst)
	{
		return;
	}

	if (!ClimbingMovement)
	{
		AnimInst->ResetAllIKWeights();
		return;
	}

	// Check if we're within IK budget
	if (!IsWithinIKBudget())
	{
		AnimInst->ResetAllIKWeights();
		return;
	}

	const EClimbingState CurrentState = ClimbingMovement->CurrentClimbingState;

	// State-specific IK handling
	switch (CurrentState)
	{
	case EClimbingState::Hanging:
	case EClimbingState::Shimmying:
		UpdateLedgeHangIK(DeltaTime, AnimInst);
		break;

	case EClimbingState::BracedWall:
	case EClimbingState::BracedShimmying:
		UpdateBracedWallIK(DeltaTime, AnimInst);
		break;

	case EClimbingState::OnLadder:
		UpdateLadderIK(DeltaTime, AnimInst);
		break;

	case EClimbingState::CornerTransition:
		UpdateCornerIK(DeltaTime, AnimInst);
		break;

	default:
		// Disable IK for other states - set all target weights to zero
		AnimInst->TargetIKWeightHandLeft = 0.0f;
		AnimInst->TargetIKWeightHandRight = 0.0f;
		AnimInst->TargetIKWeightFootLeft = 0.0f;
		AnimInst->TargetIKWeightFootRight = 0.0f;
		break;
	}

	// Update surface data in animation instance
	AnimInst->CurrentSurfaceNormal = CurrentDetectionResult.SurfaceNormal;
	AnimInst->CurrentLedgePosition = CurrentDetectionResult.LedgePosition;

	// Call internal blend update
	AnimInst->UpdateIKBlending(DeltaTime);
}

void AClimbingCharacter::UpdateLedgeHangIK(float DeltaTime, UClimbingAnimInstance* AnimInst)
{
	if (!AnimInst || !CurrentDetectionResult.bValid)
	{
		return;
	}

	// Calculate hand positions on ledge
	const FVector LedgePos = CurrentDetectionResult.LedgePosition;
	const FVector WallNormal = CurrentDetectionResult.SurfaceNormal;
	const FVector WallRight = FVector::CrossProduct(FVector::UpVector, WallNormal).GetSafeNormal();

	// Hand offset from center
	const float HandSpacing = 30.0f; // cm between hands

	// Left hand target (ledge position offset left)
	const FVector LeftHandTarget = LedgePos - WallRight * HandSpacing;

	// Right hand target (ledge position offset right)
	const FVector RightHandTarget = LedgePos + WallRight * HandSpacing;

	// Check reach distances
	const FVector CharLocation = GetActorLocation();
	const float LeftHandDist = FVector::Dist(CharLocation, LeftHandTarget);
	const float RightHandDist = FVector::Dist(CharLocation, RightHandTarget);

	// Calculate weights based on reach (use MaxReachDistance property)
	float LeftWeight = 1.0f;
	float RightWeight = 1.0f;

	if (MaxReachDistance > 0.0f)
	{
		if (LeftHandDist > MaxReachDistance)
		{
			LeftWeight = FMath::Max(0.0f, 1.0f - (LeftHandDist - MaxReachDistance) / (IKFadeOutBlendTime * 100.0f));
		}
		if (RightHandDist > MaxReachDistance)
		{
			RightWeight = FMath::Max(0.0f, 1.0f - (RightHandDist - MaxReachDistance) / (IKFadeOutBlendTime * 100.0f));
		}
	}

	// Set targets
	AnimInst->IKTargetHandLeft = LeftHandTarget;
	AnimInst->IKTargetHandRight = RightHandTarget;

	// Set target weights (hands only for hanging, feet off)
	AnimInst->TargetIKWeightHandLeft = LeftWeight;
	AnimInst->TargetIKWeightHandRight = RightWeight;
	AnimInst->TargetIKWeightFootLeft = 0.0f;
	AnimInst->TargetIKWeightFootRight = 0.0f;

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), LeftHandTarget, 5.0f, 8, FColor::White, false, 0.0f);
		DrawDebugSphere(GetWorld(), RightHandTarget, 5.0f, 8, FColor::White, false, 0.0f);
	}
#endif
}

void AClimbingCharacter::UpdateBracedWallIK(float DeltaTime, UClimbingAnimInstance* AnimInst)
{
	if (!AnimInst || !CurrentDetectionResult.bValid)
	{
		return;
	}

	// Calculate all four limb positions for braced wall
	const FVector WallPos = CurrentDetectionResult.LedgePosition;
	const FVector WallNormal = CurrentDetectionResult.SurfaceNormal;
	const FVector WallRight = FVector::CrossProduct(FVector::UpVector, WallNormal).GetSafeNormal();
	const FVector WallUp = FVector::UpVector;

	const float HandSpacing = 30.0f;
	const float FootSpacing = 25.0f;
	const float HandHeight = 80.0f;  // Hands higher on wall
	const float FootHeight = -60.0f; // Feet lower on wall

	// Calculate targets
	const FVector LeftHandTarget = WallPos - WallRight * HandSpacing + WallUp * HandHeight;
	const FVector RightHandTarget = WallPos + WallRight * HandSpacing + WallUp * HandHeight;
	const FVector LeftFootTarget = WallPos - WallRight * FootSpacing + WallUp * FootHeight;
	const FVector RightFootTarget = WallPos + WallRight * FootSpacing + WallUp * FootHeight;

	// Trace to find actual wall contact points
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	// Trace each limb to wall
	auto TraceToWall = [this, &QueryParams, &WallNormal](const FVector& Start) -> FVector
	{
		FHitResult Hit;
		const FVector End = Start - WallNormal * 50.0f;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, QueryParams))
		{
			return Hit.ImpactPoint + WallNormal * 5.0f; // Slight offset from wall
		}
		return Start - WallNormal * 20.0f;
	};

	AnimInst->IKTargetHandLeft = TraceToWall(LeftHandTarget);
	AnimInst->IKTargetHandRight = TraceToWall(RightHandTarget);
	AnimInst->IKTargetFootLeft = TraceToWall(LeftFootTarget);
	AnimInst->IKTargetFootRight = TraceToWall(RightFootTarget);

	// All limbs active for braced
	AnimInst->TargetIKWeightHandLeft = 1.0f;
	AnimInst->TargetIKWeightHandRight = 1.0f;
	AnimInst->TargetIKWeightFootLeft = 1.0f;
	AnimInst->TargetIKWeightFootRight = 1.0f;

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), AnimInst->IKTargetHandLeft, 5.0f, 8, FColor::White, false, 0.0f);
		DrawDebugSphere(GetWorld(), AnimInst->IKTargetHandRight, 5.0f, 8, FColor::White, false, 0.0f);
		DrawDebugSphere(GetWorld(), AnimInst->IKTargetFootLeft, 5.0f, 8, FColor::Blue, false, 0.0f);
		DrawDebugSphere(GetWorld(), AnimInst->IKTargetFootRight, 5.0f, 8, FColor::Blue, false, 0.0f);
	}
#endif
}

void AClimbingCharacter::UpdateLadderIK(float DeltaTime, UClimbingAnimInstance* AnimInst)
{
	if (!AnimInst || !CurrentDetectionResult.bValid)
	{
		return;
	}

	// Calculate rung positions based on character height and rung spacing
	const FVector LadderPos = CurrentDetectionResult.LedgePosition;
	const FVector LadderNormal = CurrentDetectionResult.SurfaceNormal;
	const FVector LadderRight = FVector::CrossProduct(FVector::UpVector, LadderNormal).GetSafeNormal();

	// Use default rung spacing (expose as UPROPERTY if needed)
	const float RungSpacing = 30.0f; // cm between rungs

	// Calculate which rung the character is at
	const float CharZ = GetActorLocation().Z;
	const float RungIndex = FMath::RoundToFloat(CharZ / RungSpacing);
	const float CurrentRungZ = RungIndex * RungSpacing;

	// Hand positions on current rung (or slightly above)
	const float HandRungOffset = RungSpacing * 0.8f;
	const float HandZ = CurrentRungZ + HandRungOffset;

	// Foot positions on rung below
	const float FootZ = CurrentRungZ - RungSpacing * 0.5f;

	const float HandSpacing = 20.0f;
	const float FootSpacing = 15.0f;

	// Calculate targets relative to ladder
	AnimInst->IKTargetHandLeft = LadderPos + LadderRight * (-HandSpacing) + FVector(0, 0, HandZ - LadderPos.Z);
	AnimInst->IKTargetHandRight = LadderPos + LadderRight * HandSpacing + FVector(0, 0, HandZ - LadderPos.Z);
	AnimInst->IKTargetFootLeft = LadderPos + LadderRight * (-FootSpacing) + FVector(0, 0, FootZ - LadderPos.Z);
	AnimInst->IKTargetFootRight = LadderPos + LadderRight * FootSpacing + FVector(0, 0, FootZ - LadderPos.Z);

	// All limbs active for ladder
	AnimInst->TargetIKWeightHandLeft = 1.0f;
	AnimInst->TargetIKWeightHandRight = 1.0f;
	AnimInst->TargetIKWeightFootLeft = 1.0f;
	AnimInst->TargetIKWeightFootRight = 1.0f;

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), AnimInst->IKTargetHandLeft, 5.0f, 8, FColor::Green, false, 0.0f);
		DrawDebugSphere(GetWorld(), AnimInst->IKTargetHandRight, 5.0f, 8, FColor::Green, false, 0.0f);
		DrawDebugSphere(GetWorld(), AnimInst->IKTargetFootLeft, 5.0f, 8, FColor::Green, false, 0.0f);
		DrawDebugSphere(GetWorld(), AnimInst->IKTargetFootRight, 5.0f, 8, FColor::Green, false, 0.0f);
	}
#endif
}

void AClimbingCharacter::UpdateCornerIK(float DeltaTime, UClimbingAnimInstance* AnimInst)
{
	if (!AnimInst)
	{
		return;
	}

	// During corner transition, blend out IK (montage handles positioning)
	AnimInst->TargetIKWeightHandLeft = 0.0f;
	AnimInst->TargetIKWeightHandRight = 0.0f;
	AnimInst->TargetIKWeightFootLeft = 0.0f;
	AnimInst->TargetIKWeightFootRight = 0.0f;
}

bool AClimbingCharacter::IsWithinIKBudget() const
{
	// Game thread access only for ActiveClimbingCharacters
	// Count how many characters have active IK
	int32 ActiveIKCount = 0;

	// Get local player controller for distance calculation
	APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	FVector CameraLocation = FVector::ZeroVector;

	if (LocalPC && LocalPC->PlayerCameraManager)
	{
		CameraLocation = LocalPC->PlayerCameraManager->GetCameraLocation();
	}

	// Find our position in the sorted list
	const float OurDistance = FVector::Dist(GetActorLocation(), CameraLocation);

	for (const TWeakObjectPtr<AClimbingCharacter>& WeakChar : ActiveClimbingCharacters)
	{
		if (AClimbingCharacter* OtherChar = WeakChar.Get())
		{
			if (OtherChar == this)
			{
				continue;
			}

			// Check if other character has active IK
			if (OtherChar->ClimbingMovement &&
				OtherChar->ClimbingMovement->CurrentClimbingState != EClimbingState::None)
			{
				const float OtherDistance = FVector::Dist(OtherChar->GetActorLocation(), CameraLocation);

				// Only count characters closer than us
				if (OtherDistance < OurDistance)
				{
					ActiveIKCount++;
				}
			}
		}
	}

	// Check budget
	if (ActiveIKCount >= MaxSimultaneousIKCharacters)
	{
		return false;
	}

	// Check if we're the owning client (never culled) or within cull distance
	if (IsLocallyControlled())
	{
		return true;
	}

	// Simulated proxy - check distance
	return OurDistance <= SimulatedProxyIKCullDistance;
}

void AClimbingCharacter::RegisterWithIKManager()
{
	// Game thread access only
	check(IsInGameThread());

	// Check if already registered
	for (const TWeakObjectPtr<AClimbingCharacter>& WeakChar : ActiveClimbingCharacters)
	{
		if (WeakChar.Get() == this)
		{
			return;
		}
	}

	// Add to list
	ActiveClimbingCharacters.Add(TWeakObjectPtr<AClimbingCharacter>(this));

	// Sort by distance to camera (done when state changes, not every add)
	SortIKManagerByDistance();
}

void AClimbingCharacter::UnregisterFromIKManager()
{
	// Game thread access only
	check(IsInGameThread());

	ActiveClimbingCharacters.RemoveAll([this](const TWeakObjectPtr<AClimbingCharacter>& WeakChar)
	{
		return WeakChar.Get() == this || !WeakChar.IsValid();
	});
}

void AClimbingCharacter::SortIKManagerByDistance()
{
	// Get world from first valid character in the list
	UWorld* World = nullptr;
	for (const TWeakObjectPtr<AClimbingCharacter>& WeakChar : ActiveClimbingCharacters)
	{
		if (AClimbingCharacter* Char = WeakChar.Get())
		{
			World = Char->GetWorld();
			if (World)
			{
				break;
			}
		}
	}

	if (!World)
	{
		return;
	}

	APlayerController* LocalPC = World->GetFirstPlayerController();
	if (!LocalPC || !LocalPC->PlayerCameraManager)
	{
		return;
	}

	const FVector CameraLocation = LocalPC->PlayerCameraManager->GetCameraLocation();

	// Sort by distance (closest first)
	ActiveClimbingCharacters.Sort([&CameraLocation](const TWeakObjectPtr<AClimbingCharacter>& A, const TWeakObjectPtr<AClimbingCharacter>& B)
	{
		const AClimbingCharacter* CharA = A.Get();
		const AClimbingCharacter* CharB = B.Get();

		if (!CharA) return false;
		if (!CharB) return true;

		const float DistA = FVector::Dist(CharA->GetActorLocation(), CameraLocation);
		const float DistB = FVector::Dist(CharB->GetActorLocation(), CameraLocation);

		return DistA < DistB;
	});
}

void AClimbingCharacter::UpdateIKTargets()
{
	// IK targets now updated in UpdateClimbingIK and state-specific IK functions
	// This function is kept for compatibility with animation notifies if needed
}
