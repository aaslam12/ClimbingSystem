// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

// Test-only access for Input_ClimbUp and cached detection state.
#define protected public
#include "Character/ClimbingCharacter.h"
#undef protected

#include "InputActionValue.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Helpers/SharedTestHelpers.h"
#include "Misc/AutomationTest.h"

namespace
{
static FClimbingDetectionResult MakeDetection(EClimbClearanceType Clearance)
{
	FClimbingDetectionResult Detection;
	Detection.bValid = true;
	Detection.LedgePosition = FVector(120.0f, 0.0f, 120.0f);
	Detection.SurfaceNormal = FVector(-1.0f, 0.0f, 0.0f);
	Detection.SurfaceTier = EClimbSurfaceTier::Climbable;
	Detection.ClearanceType = Clearance;
	return Detection;
}
}

// WHAT: Verifies Input_ClimbUp selects ClimbingUp when cached detection has Full clearance.
// WHY: Full-clearance climb-up path is a core traversal action.
// EDGE CASES: Fresh detection miss falls back to cached current detection.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeClimbUpSelectsFullClearanceStateTest,
	"ClimbingSystem.Character.Runtime.ClimbUpSelectsFullClearanceState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeClimbUpSelectsFullClearanceStateTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("ClimbUp runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("ClimbUp runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->CurrentDetectionResult = MakeDetection(EClimbClearanceType::Full);

	const FInputActionValue Pressed(true);
	Character->Input_ClimbUp(Pressed);

	TestEqual(TEXT("ClimbUp runtime: full clearance should transition to ClimbingUp"),
		Movement->CurrentClimbingState, EClimbingState::ClimbingUp);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies Input_ClimbUp selects ClimbingUpCrouch when cached detection has CrouchOnly clearance.
// WHY: Crouch-clearance path is needed for low-overhang ledges.
// EDGE CASES: State selection between two climb-up variants.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeClimbUpSelectsCrouchClearanceStateTest,
	"ClimbingSystem.Character.Runtime.ClimbUpSelectsCrouchClearanceState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeClimbUpSelectsCrouchClearanceStateTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("ClimbUp runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("ClimbUp runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->CurrentDetectionResult = MakeDetection(EClimbClearanceType::CrouchOnly);

	const FInputActionValue Pressed(true);
	Character->Input_ClimbUp(Pressed);

	TestEqual(TEXT("ClimbUp runtime: crouch-only clearance should transition to ClimbingUpCrouch"),
		Movement->CurrentClimbingState, EClimbingState::ClimbingUpCrouch);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies Input_ClimbUp is blocked when cached detection has no clearance.
// WHY: Prevents invalid climb-up attempts into blocked overhead geometry.
// EDGE CASES: None-clearance early return path.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeClimbUpBlockedNoClearanceTest,
	"ClimbingSystem.Character.Runtime.ClimbUpBlockedNoClearance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeClimbUpBlockedNoClearanceTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("ClimbUp runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("ClimbUp runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->CurrentDetectionResult = MakeDetection(EClimbClearanceType::None);

	const FInputActionValue Pressed(true);
	Character->Input_ClimbUp(Pressed);

	TestEqual(TEXT("ClimbUp runtime: no-clearance path should keep state in Hanging"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies Input_ClimbUp consumes LastValidatedDetectionResult when current/fresh detection is invalid.
// WHY: Multiplayer fallback protects climb-up responsiveness during transient local detection misses.
// EDGE CASES: Net-fallback path through LastValidatedDetectionResult.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeClimbUpUsesLastValidatedFallbackTest,
	"ClimbingSystem.Character.Runtime.ClimbUpUsesLastValidatedFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeClimbUpUsesLastValidatedFallbackTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("ClimbUp runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("ClimbUp runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->CurrentDetectionResult.Reset();

	Movement->LastValidatedDetectionResult.bValid = true;
	Movement->LastValidatedDetectionResult.LedgePosition = FVector(150.0f, 0.0f, 130.0f);
	Movement->LastValidatedDetectionResult.SurfaceNormal = FVector(-1.0f, 0.0f, 0.0f);
	Movement->LastValidatedDetectionResult.SurfaceTier = EClimbSurfaceTier::Climbable;
	Movement->LastValidatedDetectionResult.ClearanceType = EClimbClearanceType::CrouchOnly;

	const FInputActionValue Pressed(true);
	Character->Input_ClimbUp(Pressed);

	TestEqual(TEXT("ClimbUp runtime: valid replicated fallback with crouch clearance should transition to ClimbingUpCrouch"),
		Movement->CurrentClimbingState, EClimbingState::ClimbingUpCrouch);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies Input_ClimbUp does not transition when called outside Hanging/Shimmying states.
// WHY: Guards accidental climb-up requests from invalid traversal states.
// EDGE CASES: Invalid-state rejection path.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeClimbUpRejectedFromInvalidStateTest,
	"ClimbingSystem.Character.Runtime.ClimbUpRejectedFromInvalidState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeClimbUpRejectedFromInvalidStateTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("ClimbUp runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("ClimbUp runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::None);
	Character->CurrentDetectionResult = MakeDetection(EClimbClearanceType::Full);

	const FInputActionValue Pressed(true);
	Character->Input_ClimbUp(Pressed);

	TestEqual(TEXT("ClimbUp runtime: invalid-state invocation should remain in None"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
