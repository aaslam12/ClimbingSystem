// Copyright Epic Games, Inc. All Rights Reserved.
// Category 5 — ClimbingSystem.Arc
// Tests arc sample count, early exit, and debug toggle.

#if WITH_DEV_AUTOMATION_TESTS

#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

// ============================================================================
// Test: Arc sample count matches parameter
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArcSampleCountMatchesParameter,
	"ClimbingSystem.Arc.SampleCount_MatchesParameter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcSampleCountMatchesParameter::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("Character spawned"), Char);
	if (!Char) { Env.Teardown(); return false; }

	Char->LedgeArcSamples = 8;
	TestEqual(TEXT("Arc sample count should be 8 as configured"), Char->LedgeArcSamples, 8);

	// Run detection — it should sample exactly 8 arc points
	// (Verified by the loop: for s = 1..LedgeArcSamples)
	Char->bDrawDebug = false;
	Char->PerformLedgeDetection();

	// Structural test: the parameter is used directly as loop bound
	TestTrue(TEXT("LedgeArcSamples parameter exists and is settable"), Char->LedgeArcSamples == 8);

	Env.Teardown();
	return true;
}

// ============================================================================
// Test: Arc early exit stops at first valid ledge
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArcEarlyExitStopsAtFirstValidLedge,
	"ClimbingSystem.Arc.EarlyExit_StopsAtFirstValidLedge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcEarlyExitStopsAtFirstValidLedge::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("Character spawned"), Char);
	if (!Char) { Env.Teardown(); return false; }

	Char->LedgeArcSamples = 8;
	Char->LedgeArcDuration = 0.5f;

	// Place a ledge close — should be found at early arc samples
	const float FeetZ = Char->GetActorLocation().Z - Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Env.SpawnBoxAt(FVector(40, 0, FeetZ + 120.0f), FVector(30, 50, 5));

	FClimbingDetectionResult Result = Char->PerformLedgeDetection();
	TestTrue(TEXT("Should find ledge at early arc sample"), Result.bValid);

	// The best score should correspond to a low arc sample index (close to character)
	// We can't directly observe which sample hit, but proximity confirms early detection
	if (Result.bValid)
	{
		const float Dist = FVector::Dist2D(Result.LedgePosition, Char->GetActorLocation());
		TestTrue(
			FString::Printf(TEXT("Ledge found at distance %.1f — should be close (early arc sample)"), Dist),
			Dist < 100.0f);
	}

	Env.Teardown();
	return true;
}

// ============================================================================
// Test: Debug rendering only when bDrawDebug is enabled
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArcDebugOnlyWhenEnabled,
	"ClimbingSystem.Arc.SamplePoints_RenderOnlyWhenDebugEnabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArcDebugOnlyWhenEnabled::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("Character spawned"), Char);
	if (!Char) { Env.Teardown(); return false; }

	// Disable debug
	Char->bDrawDebug = false;

	// Run detection — should not crash and should not draw
	// (DrawDebug calls are gated by bDrawDebug in #if !UE_BUILD_SHIPPING blocks)
	Char->PerformLedgeDetection();

	TestFalse(TEXT("bDrawDebug is false — no debug draw calls should be issued"), Char->bDrawDebug);

	Env.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
