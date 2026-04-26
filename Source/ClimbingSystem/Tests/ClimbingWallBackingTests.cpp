// Copyright Epic Games, Inc. All Rights Reserved.
// Category 4 — ClimbingSystem.Detection.WallBacking
// Tests braced vs free hang classification based on backing wall presence.

#if WITH_DEV_AUTOMATION_TESTS

#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

// ============================================================================
// Test: Classifies as braced hang when wall is present behind ledge
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWallBackingClassifiesBracedHang,
	"ClimbingSystem.Detection.WallBacking.ClassifiesAsBracedHang_WhenWallPresent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWallBackingClassifiesBracedHang::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("Character spawned"), Char);
	if (!Char) { Env.Teardown(); return false; }

	// Ledge at grab height with a wall directly behind it
	const float FeetZ = Char->GetActorLocation().Z - Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float LedgeZ = FeetZ + 150.0f;
	Env.SpawnBoxAt(FVector(60, 0, LedgeZ), FVector(30, 50, 5));       // ledge top
	Env.SpawnBoxAt(FVector(60, 0, LedgeZ - 50.0f), FVector(5, 50, 50)); // wall behind

	FClimbingDetectionResult Result = Char->PerformLedgeDetection();
	TestTrue(TEXT("Should detect the ledge"), Result.bValid);
	if (Result.bValid)
	{
		TestFalse(TEXT("Braced hang: bIsFreeHang should be false when wall is behind ledge"),
			Result.bIsFreeHang);
	}

	Env.Teardown();
	return true;
}

// ============================================================================
// Test: Classifies as free hang when no wall is present
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWallBackingClassifiesFreeHang,
	"ClimbingSystem.Detection.WallBacking.ClassifiesAsFreeHang_WhenNoWallPresent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWallBackingClassifiesFreeHang::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("Character spawned"), Char);
	if (!Char) { Env.Teardown(); return false; }

	// Freestanding ledge — no wall behind, just a thin platform at 130cm above feet
	const float FeetZ = Char->GetActorLocation().Z - Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float LedgeZ = FeetZ + 130.0f;
	Env.SpawnBoxAt(FVector(60, 0, LedgeZ), FVector(10, 50, 3));

	FClimbingDetectionResult Result = Char->PerformLedgeDetection();
	TestTrue(TEXT("Should detect the freestanding ledge"), Result.bValid);
	if (Result.bValid)
	{
		TestTrue(TEXT("Free hang: bIsFreeHang should be true when no wall is behind ledge"),
			Result.bIsFreeHang);
	}

	Env.Teardown();
	return true;
}

// ============================================================================
// Test: WallBackingTraceDepth parameter is respected
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWallBackingTraceDepthRespected,
	"ClimbingSystem.Detection.WallBacking.TraceDepth_RespondsToParameter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWallBackingTraceDepthRespected::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("Character spawned"), Char);
	if (!Char) { Env.Teardown(); return false; }

	// Ledge with a wall placed far behind (100cm back)
	const float FeetZ = Char->GetActorLocation().Z - Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float LedgeZ = FeetZ + 130.0f;
	Env.SpawnBoxAt(FVector(60, 0, LedgeZ), FVector(10, 50, 3));
	// Wall 100cm behind the ledge (in the -SurfaceNormal direction, which is +X from char perspective)
	Env.SpawnBoxAt(FVector(170, 0, LedgeZ - 50.0f), FVector(5, 50, 50));

	// Set trace depth to only 30cm — should NOT reach the wall at 100cm
	Char->BracedWallCheckDepth = 30.0f;

	FClimbingDetectionResult Result = Char->PerformLedgeDetection();
	TestTrue(TEXT("Should detect the ledge"), Result.bValid);
	if (Result.bValid)
	{
		TestTrue(TEXT("Short trace depth should not reach distant wall — should classify as free hang"),
			Result.bIsFreeHang);
	}

	Env.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
