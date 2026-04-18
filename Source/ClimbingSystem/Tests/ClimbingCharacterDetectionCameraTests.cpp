// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Misc/AutomationTest.h"

// WHAT: Validates detection-related default property values match PROMPT.md spec
// WHY: Incorrect detection defaults cause grab failures or phantom ledge detection
// EDGE CASES: Default construction only — no world required

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingDetectionSettingsDefaultsTest,
	"ClimbingSystem.Character.Detection.DefaultValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingDetectionSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("Character defaults should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("Detection: LedgeDetectionForwardReach should be 75"),
		FMath::IsNearlyEqual(Defaults->LedgeDetectionForwardReach, 75.0f));
	TestTrue(TEXT("Detection: LedgeDetectionVerticalReach should be 150"),
		FMath::IsNearlyEqual(Defaults->LedgeDetectionVerticalReach, 150.0f));
	TestTrue(TEXT("Detection: LedgeDetectionRadius should be 12"),
		FMath::IsNearlyEqual(Defaults->LedgeDetectionRadius, 12.0f));
	TestTrue(TEXT("Detection: MinLedgeDepth should be 15"),
		FMath::IsNearlyEqual(Defaults->MinLedgeDepth, 15.0f));
	TestTrue(TEXT("Detection: MaxClimbableSurfaceAngle should be 30"),
		FMath::IsNearlyEqual(Defaults->MaxClimbableSurfaceAngle, 30.0f));

	return true;
}

// WHAT: Validates camera-related default property values match PROMPT.md spec
// WHY: Incorrect camera defaults cause jarring camera behavior during climbing
// EDGE CASES: ClimbingCameraProbeRadius already tested in FClimbingCharacterDefaultsTest — skipped here

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingCameraSettingsDefaultsTest,
	"ClimbingSystem.Character.Camera.DefaultValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingCameraSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("Character defaults should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("Camera: CameraNudgeStrength should be 0.5"),
		FMath::IsNearlyEqual(Defaults->CameraNudgeStrength, 0.5f));
	TestTrue(TEXT("Camera: CameraNudgeActivationAngle should be 45"),
		FMath::IsNearlyEqual(Defaults->CameraNudgeActivationAngle, 45.0f));
	TestTrue(TEXT("Camera: CameraNudgeBlendSpeed should be 3"),
		FMath::IsNearlyEqual(Defaults->CameraNudgeBlendSpeed, 3.0f));

	return true;
}

// WHAT: Validates Lache-related default property values match PROMPT.md spec
// WHY: Incorrect Lache defaults cause broken arc trajectories or missed catches
// EDGE CASES: All 6 Lache properties checked

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingLacheSettingsDefaultsTest,
	"ClimbingSystem.Character.Lache.DefaultValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingLacheSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("Character defaults should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("Lache: LacheLaunchSpeed should be 1200"),
		FMath::IsNearlyEqual(Defaults->LacheLaunchSpeed, 1200.0f));
	TestTrue(TEXT("Lache: LacheTotalArcTime should be 1.2"),
		FMath::IsNearlyEqual(Defaults->LacheTotalArcTime, 1.2f));
	TestEqual(TEXT("Lache: LacheArcTraceSteps should be 12"),
		Defaults->LacheArcTraceSteps, 12);
	TestTrue(TEXT("Lache: LacheArcTraceRadius should be 24"),
		FMath::IsNearlyEqual(Defaults->LacheArcTraceRadius, 24.0f));
	TestTrue(TEXT("Lache: bAutoLacheCinematic should be true"),
		Defaults->bAutoLacheCinematic);
	TestTrue(TEXT("Lache: LacheCinematicDistanceThreshold should be 300"),
		FMath::IsNearlyEqual(Defaults->LacheCinematicDistanceThreshold, 300.0f));

	return true;
}

// WHAT: Validates mantle height bracket defaults and ascending invariant
// WHY: Incorrect mantle thresholds cause wrong animation or no mantle at all
// EDGE CASES: Ascending bracket invariant (Step < Low < High)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMantleSettingsDefaultsTest,
	"ClimbingSystem.Character.Mantle.DefaultValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMantleSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("Character defaults should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("Mantle: MantleStepMaxHeight should be 50"),
		FMath::IsNearlyEqual(Defaults->MantleStepMaxHeight, 50.0f));
	TestTrue(TEXT("Mantle: MantleLowMaxHeight should be 100"),
		FMath::IsNearlyEqual(Defaults->MantleLowMaxHeight, 100.0f));
	TestTrue(TEXT("Mantle: MantleHighMaxHeight should be 180"),
		FMath::IsNearlyEqual(Defaults->MantleHighMaxHeight, 180.0f));
	TestTrue(TEXT("Mantle: height brackets must be ascending (Step < Low < High)"),
		Defaults->MantleStepMaxHeight < Defaults->MantleLowMaxHeight &&
		Defaults->MantleLowMaxHeight < Defaults->MantleHighMaxHeight);

	return true;
}

// WHAT: Validates freefall re-grab default property values
// WHY: Incorrect freefall defaults disable coyote time or falling grab silently
// EDGE CASES: Both enable flags and their associated distance/time values

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingFreefallSettingsDefaultsTest,
	"ClimbingSystem.Character.Freefall.DefaultValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingFreefallSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("Character defaults should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("Freefall: bEnableCoyoteTime should be true"),
		Defaults->bEnableCoyoteTime);
	TestTrue(TEXT("Freefall: CoyoteTimeWindow should be 0.15"),
		FMath::IsNearlyEqual(Defaults->CoyoteTimeWindow, 0.15f));
	TestTrue(TEXT("Freefall: bEnableFallingGrab should be true"),
		Defaults->bEnableFallingGrab);
	TestTrue(TEXT("Freefall: FallingGrabReachDistance should be 80"),
		FMath::IsNearlyEqual(Defaults->FallingGrabReachDistance, 80.0f));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
