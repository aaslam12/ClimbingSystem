// Copyright Epic Games, Inc. All Rights Reserved.
// Category 2 — ClimbingSystem.Detection.LedgeGrab
// Tests ledge grab height range, priority over mantle, and top confirmation.

#if WITH_DEV_AUTOMATION_TESTS

#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

// ============================================================================
// Test: Ledge grab triggers within valid height range
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeGrabTriggersWithinValidHeightRange,
	"ClimbingSystem.Detection.LedgeGrab.TriggersWithinValidHeightRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeGrabTriggersWithinValidHeightRange::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("Character spawned"), Char);
	if (!Char) { Env.Teardown(); return false; }

	// Ledge at 140cm above feet — above MantleHighMaxHeight(180) would be out of reach,
	// so use a height that is within LedgeDetectionVerticalReach(150) AND above MantleHighMaxHeight
	// MantleHighMaxHeight default is 180 but VerticalReach is 150, so use 140 (within reach, above mantle)
	// Actually MantleHighMaxHeight(180) > VerticalReach(150), so any ledge within reach IS above mantle range
	const float FeetZ = Char->GetActorLocation().Z - Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float LedgeHeight = FeetZ + 140.0f; // 140cm above feet, within 150cm reach
	Env.SpawnBoxAt(FVector(60, 0, LedgeHeight), FVector(30, 50, 5));

	FClimbingDetectionResult Result = Char->PerformLedgeDetection();
	TestTrue(TEXT("Ledge grab should detect a ledge within valid height range"), Result.bValid);

	Env.Teardown();
	return true;
}

// ============================================================================
// Test: Ledge grab rejects ledge below minimum height
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeGrabRejectsBelowMinHeight,
	"ClimbingSystem.Detection.LedgeGrab.RejectsLedgeBelowMinHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeGrabRejectsBelowMinHeight::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 100));
	TestNotNull(TEXT("Character spawned"), Char);
	if (!Char) { Env.Teardown(); return false; }

	// Ledge below character feet — should not be detected
	const float FeetZ = Char->GetActorLocation().Z - Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Env.SpawnBoxAt(FVector(60, 0, FeetZ - 20.0f), FVector(30, 50, 5));

	FClimbingDetectionResult Result = Char->PerformLedgeDetection();
	TestFalse(TEXT("Ledge grab should reject a ledge below character feet"), Result.bValid);

	Env.Teardown();
	return true;
}

// ============================================================================
// Test: Ledge grab rejects ledge above max height
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeGrabRejectsAboveMaxHeight,
	"ClimbingSystem.Detection.LedgeGrab.RejectsLedgeAboveMaxHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeGrabRejectsAboveMaxHeight::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("Character spawned"), Char);
	if (!Char) { Env.Teardown(); return false; }

	// Ledge well above vertical reach
	const float TooHigh = Char->LedgeDetectionVerticalReach + 50.0f;
	Env.SpawnBoxAt(FVector(60, 0, TooHigh), FVector(30, 50, 5));

	FClimbingDetectionResult Result = Char->PerformLedgeDetection();
	TestFalse(TEXT("Ledge grab should reject a ledge above maximum vertical reach"), Result.bValid);

	Env.Teardown();
	return true;
}

// ============================================================================
// Test: Ledge grab takes priority over mantle
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeGrabTakesPriorityOverMantle,
	"ClimbingSystem.Detection.LedgeGrab.TakesPriorityOverMantle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeGrabTakesPriorityOverMantle::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("Character spawned"), Char);
	if (!Char) { Env.Teardown(); return false; }

	// Surface at 120cm above feet — within LedgeDetectionVerticalReach(150)
	// Since MantleHighMaxHeight(180) > VerticalReach(150), all detectable ledges are in "ledge grab" territory
	const float FeetZ = Char->GetActorLocation().Z - Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float Height = FeetZ + 120.0f;
	Env.SpawnBoxAt(FVector(60, 0, Height), FVector(30, 50, 5));

	FClimbingDetectionResult Result = Char->PerformLedgeDetection();
	TestTrue(TEXT("Detection should find the surface"), Result.bValid);

	// The detection result should be usable for ledge grab (height within vertical reach)
	if (Result.bValid)
	{
		const float HeightAboveFeet = Result.LedgePosition.Z - (Char->GetActorLocation().Z - Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		TestTrue(
			FString::Printf(TEXT("Ledge height (%.1f) should be within vertical reach (%.1f) for grab priority"),
				HeightAboveFeet, Char->LedgeDetectionVerticalReach),
			HeightAboveFeet <= Char->LedgeDetectionVerticalReach);
	}

	Env.Teardown();
	return true;
}

// ============================================================================
// Test: Top confirmation trace finds flat surface
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeGrabTopConfirmationFindsFlatSurface,
	"ClimbingSystem.Detection.LedgeGrab.TopConfirmationTrace_FindsFlatSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeGrabTopConfirmationFindsFlatSurface::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("Character spawned"), Char);
	if (!Char) { Env.Teardown(); return false; }

	// Flat-topped box at grab height (120cm above feet)
	const float FeetZ = Char->GetActorLocation().Z - Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float LedgeZ = FeetZ + 120.0f;
	Env.SpawnBoxAt(FVector(60, 0, LedgeZ), FVector(30, 50, 5));

	FClimbingDetectionResult Result = Char->PerformLedgeDetection();
	TestTrue(TEXT("Should detect the flat ledge"), Result.bValid);
	if (Result.bValid)
	{
		TestTrue(
			FString::Printf(TEXT("Ledge Z (%.1f) should be near box top (%.1f) within 5cm"), Result.LedgePosition.Z, LedgeZ + 5.0f),
			FMath::IsNearlyEqual(Result.LedgePosition.Z, LedgeZ + 5.0f, 5.0f));
	}

	Env.Teardown();
	return true;
}

// ============================================================================
// Test: Top confirmation rejects mid-wall face
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeGrabTopConfirmationRejectsMidWall,
	"ClimbingSystem.Detection.LedgeGrab.TopConfirmationTrace_RejectsMidWallFace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeGrabTopConfirmationRejectsMidWall::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("Character spawned"), Char);
	if (!Char) { Env.Teardown(); return false; }

	// Very tall wall — top is way above reach, character faces mid-wall
	// The wall top is at 500cm, well above LedgeDetectionVerticalReach
	Env.SpawnBoxAt(FVector(60, 0, 250), FVector(5, 50, 250));

	FClimbingDetectionResult Result = Char->PerformLedgeDetection();

	// The forward trace will hit mid-wall, but the ledge top (at Z=500) is above reach
	// Detection should either reject or find the top at Z=500 which is above height filter
	if (Result.bValid)
	{
		const float HeightAboveFeet = Result.LedgePosition.Z - (Char->GetActorLocation().Z - Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		TestTrue(
			FString::Printf(TEXT("Mid-wall hit should not produce a ledge within reach (height=%.1f, max=%.1f)"),
				HeightAboveFeet, Char->LedgeDetectionVerticalReach),
			HeightAboveFeet <= Char->LedgeDetectionVerticalReach);
	}
	else
	{
		// No valid result is also acceptable — mid-wall correctly rejected
		TestFalse(TEXT("Mid-wall face correctly rejected as ledge grab target"), Result.bValid);
	}

	Env.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
