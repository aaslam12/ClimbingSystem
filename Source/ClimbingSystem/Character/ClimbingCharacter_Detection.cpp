// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingCharacter.h"
// Part of AClimbingCharacter — see ClimbingCharacter.h
#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingSurfaceData.h"
#include "DrawDebugHelpers.h"

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

	EClimbSurfaceTier SurfaceTier = EClimbSurfaceTier::Untagged;
	const bool bHasValidForwardComponent = ForwardHit.Component.IsValid();
	const bool bHasUnclimbableTag = bHasValidForwardComponent && ForwardHit.Component->ComponentHasTag(FName("Unclimbable"));
	const bool bHasLadderTag = bHasValidForwardComponent && ForwardHit.Component->ComponentHasTag(FName("LadderOnly"));
	const bool bHasOneWayTag = bHasValidForwardComponent && ForwardHit.Component->ComponentHasTag(FName("ClimbableOneWay"));
	const bool bHasClimbableTag = bHasValidForwardComponent && ForwardHit.Component->ComponentHasTag(FName("Climbable"));

	if (bHasUnclimbableTag)
	{
#if !UE_BUILD_SHIPPING
		if (bDrawDebug)
		{
			DrawDebugLine(GetWorld(), ForwardTraceStart, ForwardTraceEnd, FColor::Yellow, false, 0.1f);
		}
#endif
		return Result;
	}

	if (bHasLadderTag)
	{
		return Result;
	}

	if (bHasOneWayTag)
	{
		SurfaceTier = EClimbSurfaceTier::ClimbableOneWay;
		const UClimbingSurfaceData* SurfaceData = GetSurfaceDataFromComponent(ForwardHit.Component.Get());
		if (SurfaceData && !ValidateOneWayApproach(ForwardHit.ImpactNormal, SurfaceData->OneWayApproachDirection, SurfaceData->ApproachAngleTolerance))
		{
			return Result;
		}
	}

	if (bHasClimbableTag)
	{
		SurfaceTier = EClimbSurfaceTier::Climbable;
	}
	else
	{
		const float WallAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Abs(FVector::DotProduct(ForwardHit.ImpactNormal, UpVector))));
		if (WallAngle < (90.0f - MaxClimbableSurfaceAngle))
		{
			return Result;
		}
	}

	// Step 2: Trace downward from above the wall to find the ledge top
	// Use multi-trace to handle stacked ledges - we want the topmost valid ledge
	const FVector DownTraceStart = ForwardHit.ImpactPoint + ForwardVector * MinLedgeDepth + UpVector * LedgeDetectionVerticalReach;
	const FVector DownTraceEnd = DownTraceStart - UpVector * LedgeDetectionVerticalReach * 1.5f;

	TArray<FHitResult> DownHits;
	GetWorld()->SweepMultiByChannel(
		DownHits,
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
		DrawDebugLine(GetWorld(), DownTraceStart, DownTraceEnd, DownHits.Num() > 0 ? FColor::Green : FColor::Yellow, false, 0.1f);
	}
#endif

	if (DownHits.Num() == 0)
	{
		return Result;
	}

	// Find the best ledge hit - prefer the one closest to character's current height when climbing
	// This prevents jumping between stacked ledges during shimmy
	const bool bIsCurrentlyClimbing = ClimbingMovement && ClimbingMovement->CurrentClimbingState != EClimbingState::None;
	const float CurrentCharacterZ = CharacterLocation.Z;

	FHitResult* BestDownHit = nullptr;
	float BestScore = TNumericLimits<float>::Max();

	for (FHitResult& DownHit : DownHits)
	{
		// Verify this is approximately horizontal (a ledge top)
		const float LedgeTopAngle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(DownHit.ImpactNormal, UpVector)));
		if (LedgeTopAngle > MaxClimbableSurfaceAngle)
		{
			continue; // Not a flat enough ledge top
		}

		// Calculate preference score
		float Score;
		if (bIsCurrentlyClimbing)
		{
			// When climbing, prefer ledges at similar height to current position
			Score = FMath::Abs(DownHit.ImpactPoint.Z - CurrentCharacterZ);
		}
		else
		{
			// When not climbing (initial grab), prefer the highest reachable ledge
			// Lower Z value in trace direction = hit earlier = higher ledge
			Score = -DownHit.ImpactPoint.Z;
		}

		if (Score < BestScore)
		{
			BestScore = Score;
			BestDownHit = &DownHit;
		}
	}

	if (!BestDownHit)
	{
		return Result;
	}

	const FHitResult& DownHit = *BestDownHit;

	// Step 3: Check clearance above ledge
	const FVector ClearanceCheckStart = DownHit.ImpactPoint + UpVector * 10.0f;
	const FVector ClearanceCheckEnd = ClearanceCheckStart + UpVector * OriginalCapsuleHalfHeight * 2.0f;

	// Add the ledge components to ignore list for clearance check
	// Both the wall (ForwardHit) and ledge top (DownHit) should be ignored
	FCollisionQueryParams ClearanceQueryParams = QueryParams;
	if (DownHit.Component.IsValid())
	{
		ClearanceQueryParams.AddIgnoredComponent(DownHit.Component.Get());
	}
	if (ForwardHit.Component.IsValid() && ForwardHit.Component != DownHit.Component)
	{
		ClearanceQueryParams.AddIgnoredComponent(ForwardHit.Component.Get());
	}

	FHitResult ClearanceHit;
	bool bClearanceBlocked = GetWorld()->SweepSingleByChannel(
		ClearanceHit,
		ClearanceCheckStart,
		ClearanceCheckEnd,
		FQuat::Identity,
		ECC_WorldStatic,
		FCollisionShape::MakeSphere(OriginalCapsuleRadius * 0.8f),
		ClearanceQueryParams
	);

#if !UE_BUILD_SHIPPING
	if (bDrawDebug && bClearanceBlocked)
	{
		UE_LOG(LogClimbing, Log, TEXT("Clearance blocked by: %s at distance %.1f"), 
			ClearanceHit.Component.IsValid() ? *ClearanceHit.Component->GetName() : TEXT("Unknown"),
			ClearanceHit.Distance);
	}
#endif

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
			ClearanceQueryParams
		);

		ClearanceType = bCrouchBlocked ? EClimbClearanceType::None : EClimbClearanceType::CrouchOnly;
		
#if !UE_BUILD_SHIPPING
		if (bDrawDebug)
		{
			UE_LOG(LogClimbing, Log, TEXT("Crouch clearance: %s"), bCrouchBlocked ? TEXT("BLOCKED") : TEXT("Available"));
		}
#endif
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
