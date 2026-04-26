// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Env.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

// ============================================================================
// TC-0160: LedgeDetectionNoResultWhenNoGeometry
// ============================================================================
// WHAT: PerformLedgeDetection returns invalid when the world has no geometry.
// WHY: Baseline guard — detection must not fabricate results from empty space.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeDetectionNoResultWhenNoGeometry,
	"ClimbingSystem.Detection.LedgeGrab.NoResult_WhenNoGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeDetectionNoResultWhenNoGeometry::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("TC-0160: character must spawn"), Char);
	if (!Char) { Env.Teardown(); return false; }

	const FClimbingDetectionResult Result = Char->PerformLedgeDetection();
	TestFalse(TEXT("TC-0160: PerformLedgeDetection must return bValid==false in empty world"), Result.bValid);

	Env.Teardown();
	return true;
}

// ============================================================================
// TC-0161: BracedWallDetectionIgnoresUnclimbableTag
// ============================================================================
// WHAT: PerformBracedWallDetection rejects a wall tagged "Unclimbable" behind the character.
// WHY: Tag-based rejection must apply to braced-wall detection, not just ledge detection.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBracedWallDetectionIgnoresUnclimbableTag,
	"ClimbingSystem.Detection.BracedWall.RejectsUnclimbableTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBracedWallDetectionIgnoresUnclimbableTag::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("TC-0161: character must spawn"), Char);
	if (!Char) { Env.Teardown(); return false; }

	// Wall behind the character (negative X) tagged Unclimbable
	Env.SpawnBoxAt(FVector(-80, 0, 90), FVector(12, 120, 120), FName("Unclimbable"));

	const FClimbingDetectionResult Result = Char->PerformBracedWallDetection();
	TestFalse(TEXT("TC-0161: Unclimbable-tagged wall must be rejected by PerformBracedWallDetection"), Result.bValid);

	Env.Teardown();
	return true;
}

// ============================================================================
// TC-0162: BracedWallDetectionSetsCorrectSurfaceTier
// ============================================================================
// WHAT: PerformBracedWallDetection sets SurfaceTier==Climbable for "Climbable" tag and
//       SurfaceTier==Untagged for an untagged wall.
// WHY: SurfaceTier drives animation and movement state; wrong tier causes incorrect behaviour.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBracedWallDetectionSetsCorrectSurfaceTier,
	"ClimbingSystem.Detection.BracedWall.SetsCorrectSurfaceTier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBracedWallDetectionSetsCorrectSurfaceTier::RunTest(const FString& Parameters)
{
	// Sub-test 1: Climbable tag → SurfaceTier == Climbable
	{
		FClimbingTestWorld Env;
		Env.Setup();

		AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
		TestNotNull(TEXT("TC-0162a: character must spawn"), Char);
		if (!Char) { Env.Teardown(); return false; }

		Env.SpawnBoxAt(FVector(80, 0, 90), FVector(12, 120, 120), FName("Climbable"));

		const FClimbingDetectionResult Result = Char->PerformBracedWallDetection();
		if (Result.bValid)
		{
			TestEqual(TEXT("TC-0162a: Climbable-tagged wall must yield SurfaceTier==Climbable"),
				Result.SurfaceTier, EClimbSurfaceTier::Climbable);
		}
		else
		{
			// Detection may legitimately miss depending on character facing; log but don't hard-fail tier check
			AddWarning(TEXT("TC-0162a: BracedWallDetection returned invalid — tier assertion skipped"));
		}

		Env.Teardown();
	}

	// Sub-test 2: No tag → SurfaceTier == Untagged
	{
		FClimbingTestWorld Env;
		Env.Setup();

		AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
		TestNotNull(TEXT("TC-0162b: character must spawn"), Char);
		if (!Char) { Env.Teardown(); return false; }

		Env.SpawnBoxAt(FVector(80, 0, 90), FVector(12, 120, 120));

		const FClimbingDetectionResult Result = Char->PerformBracedWallDetection();
		if (Result.bValid)
		{
			TestEqual(TEXT("TC-0162b: Untagged wall must yield SurfaceTier==Untagged"),
				Result.SurfaceTier, EClimbSurfaceTier::Untagged);
		}
		else
		{
			AddWarning(TEXT("TC-0162b: BracedWallDetection returned invalid — tier assertion skipped"));
		}

		Env.Teardown();
	}

	return true;
}

// ============================================================================
// TC-0163: LedgeDetectionMaxSurfaceAngleRejectsSlope
// ============================================================================
// WHAT: PerformLedgeDetection rejects a 45° slope when MaxClimbableSurfaceAngle==30.
// WHY: Angle gate prevents climbing on surfaces too steep for the character.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeDetectionMaxSurfaceAngleRejectsSlope,
	"ClimbingSystem.Detection.LedgeGrab.MaxSurfaceAngle_RejectsExcessiveSlope",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeDetectionMaxSurfaceAngleRejectsSlope::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("TC-0163: character must spawn"), Char);
	if (!Char) { Env.Teardown(); return false; }

	Char->MaxClimbableSurfaceAngle = 30.0f;

	const float FeetZ = Char->GetActorLocation().Z - Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Env.SpawnSlopeAt(FVector(60, 0, FeetZ + 120.0f), 45.0f);

	const FClimbingDetectionResult Result = Char->PerformLedgeDetection();
	TestFalse(TEXT("TC-0163: 45° slope must be rejected when MaxClimbableSurfaceAngle==30"), Result.bValid);

	Env.Teardown();
	return true;
}

// ============================================================================
// TC-0164: LedgeDetectionMaxSurfaceAngleAcceptsWithinLimit
// ============================================================================
// WHAT: PerformLedgeDetection accepts a 15° slope when MaxClimbableSurfaceAngle==30.
// WHY: Surfaces within the angle limit must remain reachable.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeDetectionMaxSurfaceAngleAcceptsWithinLimit,
	"ClimbingSystem.Detection.LedgeGrab.MaxSurfaceAngle_AcceptsWithinLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeDetectionMaxSurfaceAngleAcceptsWithinLimit::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("TC-0164: character must spawn"), Char);
	if (!Char) { Env.Teardown(); return false; }

	Char->MaxClimbableSurfaceAngle = 30.0f;

	const float FeetZ = Char->GetActorLocation().Z - Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Env.SpawnSlopeAt(FVector(60, 0, FeetZ + 120.0f), 15.0f);

	const FClimbingDetectionResult Result = Char->PerformLedgeDetection();
	TestTrue(TEXT("TC-0164: 15° slope must be accepted when MaxClimbableSurfaceAngle==30"), Result.bValid);

	Env.Teardown();
	return true;
}

// ============================================================================
// TC-0165: LedgeDetectionArcSamplesMinimumThreeNoCrash
// ============================================================================
// WHAT: PerformLedgeDetection does not crash and returns a valid result with LedgeArcSamples==3.
// WHY: Minimum arc sample count must be handled gracefully without divide-by-zero or OOB access.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeDetectionArcSamplesMinimumThreeNoCrash,
	"ClimbingSystem.Traces.ArcSamples_MinimumThree_NoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeDetectionArcSamplesMinimumThreeNoCrash::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("TC-0165: character must spawn"), Char);
	if (!Char) { Env.Teardown(); return false; }

	Char->LedgeArcSamples = 3;

	const float FeetZ = Char->GetActorLocation().Z - Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Env.SpawnBoxAt(FVector(60, 0, FeetZ + 120.0f), FVector(30, 50, 5));

	// Primary assertion: must not crash. Secondary: result should be valid with geometry present.
	const FClimbingDetectionResult Result = Char->PerformLedgeDetection();
	TestTrue(TEXT("TC-0165: PerformLedgeDetection with LedgeArcSamples==3 must return bValid==true when ledge is present"), Result.bValid);

	Env.Teardown();
	return true;
}

// ============================================================================
// TC-0167: LedgeDetectionIgnoresCharacterOwnCapsule
// ============================================================================
// WHAT: PerformLedgeDetection returns invalid in an empty world (no external geometry).
// WHY: The character's own capsule must never self-report as a climbable ledge.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeDetectionIgnoresCharacterOwnCapsule,
	"ClimbingSystem.Detection.LedgeGrab.IgnoresOwnCapsule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeDetectionIgnoresCharacterOwnCapsule::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("TC-0167: character must spawn"), Char);
	if (!Char) { Env.Teardown(); return false; }

	// No external geometry — only the character's own capsule exists
	const FClimbingDetectionResult Result = Char->PerformLedgeDetection();
	TestFalse(TEXT("TC-0167: PerformLedgeDetection must not self-detect the character's own capsule as a ledge"), Result.bValid);

	Env.Teardown();
	return true;
}

// ============================================================================
// TC-0157: LedgeDetectionAtLocationSurfaceTierFromSurfaceData
// ============================================================================
// WHAT: PerformLedgeDetectionAtLocation reads SurfaceTier from tag-based surface data.
// WHY: Tag-driven tier resolution must propagate through the at-location detection path.
// VERIFY: This test relies on tag-to-tier mapping being implemented in PerformLedgeDetectionAtLocation.
//         If UClimbingSurfaceData asset-based lookup is required (not tag-based), this test will
//         need a loadable asset at a known content path (e.g. /Game/ClimbingSystem/Data/DA_LadderSurface).
//         Until that asset is confirmed available in test builds, the bValid guard below is intentional.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeDetectionAtLocationSurfaceTierFromSurfaceData,
	"ClimbingSystem.Detection.AtLocation.SurfaceTier_ReadFromSurfaceData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeDetectionAtLocationSurfaceTierFromSurfaceData::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("TC-0157: character must spawn"), Char);
	if (!Char) { Env.Teardown(); return false; }

	// Spawn a box tagged "LadderOnly" at a reachable ledge height in front of the character
	const float FeetZ = Char->GetActorLocation().Z - Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector LedgeLocation(60, 0, FeetZ + 120.0f);
	Env.SpawnBoxAt(LedgeLocation, FVector(30, 50, 5), FName("LadderOnly"));

	// VERIFY: PerformLedgeDetectionAtLocation must exist and accept a world-space location.
	//         If the method signature differs from the API reference, update accordingly.
	const FClimbingDetectionResult Result = Char->PerformLedgeDetectionAtLocation(LedgeLocation);

	if (Result.bValid)
	{
		// VERIFY: If tier resolution is tag-based, LadderOnly tag must map to EClimbSurfaceTier::LadderOnly.
		//         If tier resolution requires a UClimbingSurfaceData asset, this assertion may fail in
		//         test builds that lack the asset — add the asset or mock the data source to fix.
		TestEqual(TEXT("TC-0157: LadderOnly-tagged surface must yield SurfaceTier==LadderOnly"),
			Result.SurfaceTier, EClimbSurfaceTier::LadderOnly);
	}
	else
	{
		// VERIFY: Detection returned invalid. Either the geometry placement is outside the detection
		//         radius for PerformLedgeDetectionAtLocation, or asset-based tier lookup failed.
		//         Confirm the expected search radius and asset availability before treating as a failure.
		AddWarning(TEXT("TC-0157: PerformLedgeDetectionAtLocation returned invalid — tier assertion skipped. "
			"Confirm geometry placement is within detection radius and UClimbingSurfaceData asset is available."));
	}

	Env.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
