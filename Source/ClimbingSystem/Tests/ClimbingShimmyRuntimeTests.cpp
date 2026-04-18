// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

// Test-only access for input handlers and per-state tick functions.
#define protected public
#include "Character/ClimbingCharacter.h"
#undef protected

#include "InputActionValue.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Helpers/SharedTestHelpers.h"
#include "Misc/AutomationTest.h"

namespace
{
static FClimbingDetectionResult MakeShimmyDetection()
{
	FClimbingDetectionResult Detection;
	Detection.bValid = true;
	Detection.LedgePosition = FVector(100.0f, 0.0f, 120.0f);
	Detection.SurfaceNormal = FVector(-1.0f, 0.0f, 0.0f);
	Detection.SurfaceTier = EClimbSurfaceTier::Climbable;
	Detection.ClearanceType = EClimbClearanceType::Full;
	return Detection;
}
}

// WHAT: Verifies Input_ClimbMove sets committed shimmy direction for values above deadzone.
// WHY: Direction commitment drives shimmy montage and movement polarity.
// EDGE CASES: Positive and negative input update paths.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeShimmyDirectionUpdatesAboveDeadzoneTest,
	"ClimbingSystem.Actions.Shimmy.Runtime.DirectionUpdatesAboveDeadzone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeShimmyDirectionUpdatesAboveDeadzoneTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Shimmy runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	const FInputActionValue MoveRight(FVector2D(0.6f, 0.0f));
	Character->Input_ClimbMove(MoveRight);
	TestEqual(TEXT("Shimmy runtime: positive input above deadzone should commit right direction"),
		Character->CommittedShimmyDir, 1.0f);

	const FInputActionValue MoveLeft(FVector2D(-0.7f, 0.0f));
	Character->Input_ClimbMove(MoveLeft);
	TestEqual(TEXT("Shimmy runtime: negative input above deadzone should commit left direction"),
		Character->CommittedShimmyDir, -1.0f);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies Input_ClimbMove preserves committed direction for values inside deadzone.
// WHY: Deadzone hysteresis prevents rapid left/right flicker near analog center.
// EDGE CASES: Input magnitude below deadzone threshold.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeShimmyDirectionPersistsInsideDeadzoneTest,
	"ClimbingSystem.Actions.Shimmy.Runtime.DirectionPersistsInsideDeadzone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeShimmyDirectionPersistsInsideDeadzoneTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Shimmy runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	Character->ShimmyDirectionDeadzone = 0.1f;
	Character->CommittedShimmyDir = 1.0f;
	const FInputActionValue SmallInput(FVector2D(0.05f, 0.0f));
	Character->Input_ClimbMove(SmallInput);

	TestEqual(TEXT("Shimmy runtime: below-deadzone input should keep previous committed direction"),
		Character->CommittedShimmyDir, 1.0f);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies TickHangingState transitions to Shimmying when horizontal climb input is active.
// WHY: Hanging-to-shimmy transition is the primary lateral traversal entrypoint.
// EDGE CASES: Requires valid current detection result.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeHangingToShimmyTransitionTest,
	"ClimbingSystem.Actions.Shimmy.Runtime.HangingTransitionsToShimmying",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeHangingToShimmyTransitionTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Shimmy runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Shimmy runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->CurrentDetectionResult = MakeShimmyDetection();
	const FInputActionValue MoveRight(FVector2D(0.6f, 0.0f));
	Character->Input_ClimbMove(MoveRight);
	Character->TickHangingState(0.016f);

	TestEqual(TEXT("Shimmy runtime: active horizontal input from Hanging should transition to Shimmying"),
		Movement->CurrentClimbingState, EClimbingState::Shimmying);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies TickShimmyingState returns to Hanging when climb input is released.
// WHY: Release-to-hang behavior is required for precise shimmy stops.
// EDGE CASES: Completed input event sets CurrentClimbMoveInput to zero.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeShimmyReleaseReturnsToHangingTest,
	"ClimbingSystem.Actions.Shimmy.Runtime.ReleaseReturnsToHanging",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeShimmyReleaseReturnsToHangingTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Shimmy runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Shimmy runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Shimmying);
	Character->CurrentDetectionResult = MakeShimmyDetection();
	Character->Input_ClimbMove(FInputActionValue(FVector2D(0.5f, 0.0f)));
	Character->Input_ClimbMoveCompleted(FInputActionValue(FVector2D::ZeroVector));
	Character->TickShimmyingState(0.016f);

	TestEqual(TEXT("Shimmy runtime: releasing input in Shimmying should return to Hanging"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
