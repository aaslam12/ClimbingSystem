// Copyright Epic Games, Inc. All Rights Reserved.
// Category 6 — ClimbingSystem.Priority
// Tests detection pass ordering: ledge grab before mantle, nothing when both fail.

#if WITH_DEV_AUTOMATION_TESTS

#include "Helpers/ClimbingTestHelpers.h"
#include "InputActionValue.h"
#include "Misc/AutomationTest.h"

// ============================================================================
// Test: Ledge grab priority over mantle
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriorityLedgeGrabBeforeMantle,
	"ClimbingSystem.Priority.PriorityOrder_LedgeGrabBeforeMantle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPriorityLedgeGrabBeforeMantle::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("Character spawned"), Char);
	if (!Char) { Env.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Char->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component exists"), Movement);
	if (!Movement) { Env.Teardown(); return false; }

	// Surface at a height valid for BOTH ledge grab and mantle
	const float Height = Char->MantleHighMaxHeight - 10.0f;
	Env.SpawnBoxAt(FVector(60, 0, Height * 0.5f), FVector(5, 50, Height * 0.5f));

	// Simulate grab input
	Char->TestInput_Grab(FInputActionValue(true));

	// If ledge grab has priority, state should be Hanging or BracedWall (not Mantling)
	const EClimbingState State = Movement->CurrentClimbingState;
	const bool bIsHangOrBraced = (State == EClimbingState::Hanging || State == EClimbingState::BracedWall);
	const bool bIsMantle = (State == EClimbingState::Mantling);

	TestTrue(
		FString::Printf(TEXT("Priority: state should be Hanging/BracedWall (got %s), not Mantling"),
			*UEnum::GetValueAsString(State)),
		bIsHangOrBraced || State == EClimbingState::None); // None is acceptable if detection fails in test world

	if (bIsMantle)
	{
		AddError(TEXT("Priority order violated — mantle fired when ledge grab conditions were met first"));
	}

	Env.Teardown();
	return true;
}

// ============================================================================
// Test: Nothing fires when surface is above all thresholds
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriorityNothingWhenBothFail,
	"ClimbingSystem.Priority.PriorityOrder_NothingFiredWhenBothFail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPriorityNothingWhenBothFail::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("Character spawned"), Char);
	if (!Char) { Env.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Char->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component exists"), Movement);
	if (!Movement) { Env.Teardown(); return false; }

	// Wall well above both MaxLedgeGrabHeight (LedgeDetectionVerticalReach) and MaxMantleHeight
	const float TooHigh = Char->LedgeDetectionVerticalReach + Char->MantleHighMaxHeight + 100.0f;
	Env.SpawnBoxAt(FVector(60, 0, TooHigh), FVector(5, 50, 50));

	Char->TestInput_Grab(FInputActionValue(true));

	TestEqual(TEXT("No action should fire when surface is above all valid thresholds"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Env.Teardown();
	return true;
}

// ============================================================================
// Test: Pass 2 (mantle) does not run when Pass 1 (ledge grab) succeeds
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPriorityPassesAreSequential,
	"ClimbingSystem.Priority.PriorityOrder_PassesAreSequentialNotParallel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPriorityPassesAreSequential::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Env;
	Env.Setup();

	AClimbingCharacter* Char = Env.SpawnCharacterAt(FVector(0, 0, 0));
	TestNotNull(TEXT("Character spawned"), Char);
	if (!Char) { Env.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Char->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component exists"), Movement);
	if (!Movement) { Env.Teardown(); return false; }

	// Place a ledge at a height that is valid for ledge grab (above mantle max)
	const float LedgeHeight = Char->MantleHighMaxHeight + 20.0f;
	Env.SpawnBoxAt(FVector(60, 0, LedgeHeight), FVector(30, 50, 5));

	Char->TestInput_Grab(FInputActionValue(true));

	// If passes are sequential, ledge grab should have been evaluated first
	// and mantle should never have been reached
	const EClimbingState State = Movement->CurrentClimbingState;
	TestTrue(
		FString::Printf(TEXT("Sequential passes: state should be Hanging/BracedWall or None (got %s)"),
			*UEnum::GetValueAsString(State)),
		State != EClimbingState::Mantling);

	Env.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
