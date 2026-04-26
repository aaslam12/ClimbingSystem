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

	if (!GetWorld() || !ClimbingMovement)
	{
		return Result;
	}

	const FVector ActorLoc  = GetActorLocation();
	const FVector Origin    = ActorLoc + FVector::UpVector * (OriginalCapsuleHalfHeight * 0.75f);
	const FVector Forward   = GetActorForwardVector();
	const FVector Right     = GetActorRightVector();
	const FVector Up        = FVector::UpVector;
	const float   GravityZ  = ClimbingMovement->GetGravityZ(); // negative

	// Arc velocity: use actual velocity when airborne, otherwise a gentle forward+up probe
	FVector ArcVel = ClimbingMovement->Velocity;
	if (!ClimbingMovement->IsFalling() || ArcVel.IsNearlyZero())
	{
		// Grounded / attached: probe forward and slightly upward so we scan above the character
		ArcVel = Forward * (LedgeDetectionForwardReach / LedgeArcDuration)
		       + Up      * (LedgeDetectionVerticalReach * 0.5f / LedgeArcDuration);
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;

	// Grid step sizes (avoid divide-by-zero for 1-column/row configs)
	const float ColStep = (LedgeGridColumns > 1) ? (LedgeGridHalfWidth  * 2.0f / (LedgeGridColumns - 1)) : 0.0f;
	const float RowStep = (LedgeGridRows    > 1) ? (LedgeGridHalfHeight * 2.0f / (LedgeGridRows    - 1)) : 0.0f;

	// Best candidate across all arc samples
	FClimbingDetectionResult BestResult;
	float BestScore = TNumericLimits<float>::Max();

	const float dt = LedgeArcDuration / LedgeArcSamples;

	for (int32 s = 1; s <= LedgeArcSamples; ++s)
	{
		const float t = s * dt;
		// Kinematic arc position
		const FVector ArcPoint = Origin
			+ ArcVel * t
			+ FVector(0.f, 0.f, 0.5f * GravityZ * t * t);

		// Grid: columns along Right, rows along Up
		for (int32 col = 0; col < LedgeGridColumns; ++col)
		{
			const float ColOffset = (LedgeGridColumns > 1)
				? (-LedgeGridHalfWidth + col * ColStep)
				: 0.0f;

			for (int32 row = 0; row < LedgeGridRows; ++row)
			{
				const float RowOffset = (LedgeGridRows > 1)
					? (-LedgeGridHalfHeight + row * RowStep)
					: 0.0f;

				const FVector CellOrigin = ArcPoint + Right * ColOffset + Up * RowOffset;

				// --- Trace A: downward — finds horizontal ledge tops (beams, platforms) ---
				{
					const FVector TraceEnd = CellOrigin - Up * LedgeGridTraceLength;
					FHitResult Hit;
					const bool bHit = GetWorld()->LineTraceSingleByChannel(
						Hit, CellOrigin, TraceEnd, ECC_WorldStatic, QueryParams);

#if !UE_BUILD_SHIPPING
					if (bDrawDebug)
					{
						DrawDebugLine(GetWorld(), CellOrigin, bHit ? Hit.ImpactPoint : TraceEnd,
							bHit ? FColor::Green : FColor(80,80,80), false, 0.1f, 0, 1.0f);
					}
#endif

					if (bHit && Hit.Component.IsValid()
						&& !Hit.Component->ComponentHasTag(FName("Unclimbable"))
						&& !Hit.Component->ComponentHasTag(FName("LadderOnly")))
					{
						const float NormalAngle = FMath::RadiansToDegrees(
							FMath::Acos(FMath::Clamp(FVector::DotProduct(Hit.ImpactNormal, Up), -1.f, 1.f)));
						const float HeightAboveFeet = Hit.ImpactPoint.Z - (ActorLoc.Z - OriginalCapsuleHalfHeight);

						if (NormalAngle <= MaxClimbableSurfaceAngle
							&& HeightAboveFeet >= 0.0f
							&& HeightAboveFeet <= LedgeDetectionVerticalReach)
						{
							const float HorizDist = FVector::Dist2D(Hit.ImpactPoint, ActorLoc);
							const float Score = static_cast<float>(s) + HorizDist * 0.001f;
							if (Score < BestScore)
							{
								BestScore = Score;
								EClimbSurfaceTier Tier = EClimbSurfaceTier::Untagged;
								if (Hit.Component->ComponentHasTag(FName("Climbable")))         Tier = EClimbSurfaceTier::Climbable;
								else if (Hit.Component->ComponentHasTag(FName("ClimbableOneWay"))) Tier = EClimbSurfaceTier::ClimbableOneWay;
								BestResult.LedgePosition = Hit.ImpactPoint;
								BestResult.SurfaceNormal  = (ActorLoc - Hit.ImpactPoint).GetSafeNormal2D();
								BestResult.SurfaceTier    = Tier;
								BestResult.ClearanceType  = EClimbClearanceType::Full;
								BestResult.HitComponent   = Hit.Component;
								BestResult.bValid         = true;
							}
						}
					}
				}

				// --- Trace B: forward — finds vertical walls, then resolves ledge top ---
				{
					// Use the arc's forward direction (velocity direction projected horizontal)
					const FVector ArcFwd = ArcVel.GetSafeNormal2D().IsNearlyZero()
						? Forward : ArcVel.GetSafeNormal2D();
					const FVector FwdEnd = CellOrigin + ArcFwd * LedgeGridTraceLength;
					FHitResult WallHit;
					const bool bWallHit = GetWorld()->LineTraceSingleByChannel(
						WallHit, CellOrigin, FwdEnd, ECC_WorldStatic, QueryParams);

#if !UE_BUILD_SHIPPING
					if (bDrawDebug)
					{
						DrawDebugLine(GetWorld(), CellOrigin, bWallHit ? WallHit.ImpactPoint : FwdEnd,
							bWallHit ? FColor::Yellow : FColor(60,60,60), false, 0.1f, 0, 1.0f);
					}
#endif

					if (!bWallHit || !WallHit.Component.IsValid())                                    { continue; }
					if (WallHit.Component->ComponentHasTag(FName("Unclimbable")))                     { continue; }
					if (WallHit.Component->ComponentHasTag(FName("LadderOnly")))                      { continue; }

					// Must be roughly vertical
					const float WallAngle = FMath::RadiansToDegrees(
						FMath::Acos(FMath::Clamp(FMath::Abs(FVector::DotProduct(WallHit.ImpactNormal, Up)), 0.f, 1.f)));
					if (WallAngle < (90.0f - MaxClimbableSurfaceAngle))                               { continue; }

					// Resolve ledge top: trace down from just above the wall impact
					const FVector LedgeProbeStart = WallHit.ImpactPoint + Up * LedgeGridTraceLength;
					const FVector LedgeProbeEnd   = WallHit.ImpactPoint - Up * 5.0f;
					FHitResult LedgeHit;
					if (!GetWorld()->LineTraceSingleByChannel(
						LedgeHit, LedgeProbeStart, LedgeProbeEnd, ECC_WorldStatic, QueryParams))      { continue; }

					const float LedgeNormalAngle = FMath::RadiansToDegrees(
						FMath::Acos(FMath::Clamp(FVector::DotProduct(LedgeHit.ImpactNormal, Up), -1.f, 1.f)));
					if (LedgeNormalAngle > MaxClimbableSurfaceAngle)                                  { continue; }

					const float HeightAboveFeet = LedgeHit.ImpactPoint.Z - (ActorLoc.Z - OriginalCapsuleHalfHeight);
					if (HeightAboveFeet < 0.0f || HeightAboveFeet > LedgeDetectionVerticalReach)      { continue; }

					const float HorizDist = FVector::Dist2D(LedgeHit.ImpactPoint, ActorLoc);
					const float Score = static_cast<float>(s) + HorizDist * 0.001f;
					if (Score < BestScore)
					{
						BestScore = Score;
						EClimbSurfaceTier Tier = EClimbSurfaceTier::Untagged;
						if (WallHit.Component->ComponentHasTag(FName("Climbable")))         Tier = EClimbSurfaceTier::Climbable;
						else if (WallHit.Component->ComponentHasTag(FName("ClimbableOneWay"))) Tier = EClimbSurfaceTier::ClimbableOneWay;
						BestResult.LedgePosition = LedgeHit.ImpactPoint;
						BestResult.SurfaceNormal  = WallHit.ImpactNormal;
						BestResult.SurfaceTier    = Tier;
						BestResult.ClearanceType  = EClimbClearanceType::Full;
						BestResult.HitComponent   = WallHit.Component;
						BestResult.bValid         = true;
					}
				}
			}
		}

#if !UE_BUILD_SHIPPING
		if (bDrawDebug)
		{
			DrawDebugSphere(GetWorld(), ArcPoint, 6.0f, 6, FColor::Cyan, false, 0.1f);
		}
#endif
	}

	if (!BestResult.bValid)
	{
		return Result;
	}

	// Clearance check above the best ledge
	{
		const FVector ClearStart = BestResult.LedgePosition + Up * 10.0f;
		FCollisionQueryParams ClearParams = QueryParams;
		if (BestResult.HitComponent.IsValid())
		{
			ClearParams.AddIgnoredComponent(BestResult.HitComponent.Get());
		}

		FHitResult ClearHit;
		const bool bBlocked = GetWorld()->SweepSingleByChannel(
			ClearHit, ClearStart, ClearStart + Up * OriginalCapsuleHalfHeight * 2.0f,
			FQuat::Identity, ECC_WorldStatic,
			FCollisionShape::MakeSphere(OriginalCapsuleRadius * 0.8f), ClearParams);

		if (bBlocked)
		{
			FHitResult CrouchHit;
			const bool bCrouchBlocked = GetWorld()->SweepSingleByChannel(
				CrouchHit, ClearStart, ClearStart + Up * OriginalCapsuleHalfHeight,
				FQuat::Identity, ECC_WorldStatic,
				FCollisionShape::MakeSphere(OriginalCapsuleRadius * 0.8f), ClearParams);
			BestResult.ClearanceType = bCrouchBlocked
				? EClimbClearanceType::None : EClimbClearanceType::CrouchOnly;
		}
	}

	ClassifyHangType(BestResult);

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		DrawDebugSphere(GetWorld(), BestResult.LedgePosition, 12.0f, 8, FColor::Cyan, false, 0.15f);
		DrawDebugDirectionalArrow(GetWorld(), BestResult.LedgePosition,
			BestResult.LedgePosition + BestResult.SurfaceNormal * 40.0f,
			10.0f, FColor::Blue, false, 0.15f);
	}
#endif

	return BestResult;
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

void AClimbingCharacter::ClassifyHangType(FClimbingDetectionResult& Result) const
{
	if (!Result.bValid || !GetWorld())
	{
		return;
	}

	// Trace from the ledge position backward (into the wall) to check for a backing surface.
	// SurfaceNormal points away from the wall, so -SurfaceNormal points into it.
	const FVector TraceStart = Result.LedgePosition + FVector::UpVector * 5.0f;
	const FVector TraceEnd   = TraceStart - Result.SurfaceNormal * BracedWallCheckDepth;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = false;

	FHitResult WallHit;
	const bool bWallFound = GetWorld()->LineTraceSingleByChannel(
		WallHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams);

	Result.bIsFreeHang = !bWallFound;

#if !UE_BUILD_SHIPPING
	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd,
			bWallFound ? FColor::Blue : FColor::Magenta, false, 0.1f);
	}
#endif
}


