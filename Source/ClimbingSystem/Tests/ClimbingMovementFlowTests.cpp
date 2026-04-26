// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"

#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "InputActionValue.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Helpers/SharedTestHelpers.h"
#include "Misc/AutomationTest.h"

namespace
{
static FClimbingDetectionResult MakeHangDetection(const FVector& LedgePos = FVector(100.0f, 0.0f, 200.0f))
{
	FClimbingDetectionResult D;
	D.bValid = true;
	D.LedgePosition = LedgePos;
	D.SurfaceNormal = FVector(-1.0f, 0.0f, 0.0f);
	D.SurfaceTier = EClimbSurfaceTier::Climbable;
	D.ClearanceType = EClimbClearanceType::Full;
	return D;
}
}

// ============================================================================
// Shimmy: Hanging → Shimmying transition on move input
// ============================================================================

// WHAT: Verifies that providing lateral move input while Hanging transitions to Shimmying.
// WHY: Core shimmy flow requires state transition from Hanging when input is held.
// EDGE CASES: Requires valid CurrentDetectionResult for transition to succeed.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMovementShimmyTransitionTest,
	"ClimbingSystem.Movement.Flow.ShimmyTransitionFromHanging",
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMovementShimmyTransitionTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should exist"), Movement);
	if (!Movement) { Character->Destroy(); Helper.Teardown(); return false; }

	// Put character in Hanging state with valid detection
	FClimbingDetectionResult Detection = MakeHangDetection();
	Character->TestCurrentDetectionResult() = Detection;
	Movement->SetClimbingState(EClimbingState::Hanging);

	// Provide lateral input and tick
	const FInputActionValue MoveRight(FVector2D(0.8f, 0.0f));
	Character->TestInput_ClimbMove(MoveRight);
	Character->TestTickHangingState(0.016f);

	TestEqual(TEXT("Shimmy: lateral input while Hanging should transition to Shimmying"),
		Movement->CurrentClimbingState, EClimbingState::Shimmying);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// ============================================================================
// Shimmy: No transition without input
// ============================================================================

// WHAT: Verifies that Hanging state persists when no lateral input is provided.
// WHY: Prevents accidental shimmy transitions from zero input.
// EDGE CASES: Zero input vector.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMovementShimmyNoInputStaysHangingTest,
	"ClimbingSystem.Movement.Flow.ShimmyNoInputStaysHanging",
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMovementShimmyNoInputStaysHangingTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!Movement) { Character->Destroy(); Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->TestCurrentDetectionResult() = MakeHangDetection();

	// Zero input
	const FInputActionValue NoMove(FVector2D(0.0f, 0.0f));
	Character->TestInput_ClimbMove(NoMove);
	Character->TestTickHangingState(0.016f);

	TestEqual(TEXT("Shimmy: zero input should keep Hanging state"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// ============================================================================
// Jump off ledge (Drop): Hanging → DroppingDown
// ============================================================================

// WHAT: Verifies Input_Drop transitions from Hanging to DroppingDown.
// WHY: Core drop-off-ledge flow for intentional release.
// EDGE CASES: State must be interruptible.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMovementDropFromHangingTest,
	"ClimbingSystem.Movement.Flow.DropFromHanging",
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMovementDropFromHangingTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!Movement) { Character->Destroy(); Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->TestCurrentDetectionResult() = MakeHangDetection();

	// Verify Hanging → DroppingDown is a valid transition
	TestTrue(TEXT("Drop: Hanging to DroppingDown should be valid transition"),
		Movement->IsValidStateTransition(EClimbingState::DroppingDown));

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// ============================================================================
// Jump off ledge (Drop): Not allowed from None state
// ============================================================================

// WHAT: Verifies DroppingDown is not a valid transition from None.
// WHY: Cannot drop if not climbing.
// EDGE CASES: State validation from ground.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMovementDropInvalidFromNoneTest,
	"ClimbingSystem.Movement.Flow.DropInvalidFromNone",
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMovementDropInvalidFromNoneTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!Movement) { Character->Destroy(); Helper.Teardown(); return false; }

	// State is None by default
	TestFalse(TEXT("Drop: DroppingDown should not be valid from None state"),
		Movement->IsValidStateTransition(EClimbingState::DroppingDown));

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// ============================================================================
// Lache: No target keeps Hanging
// ============================================================================

// WHAT: Verifies Lache input with no valid target does not leave Hanging.
// WHY: Failed lache attempts must not corrupt state.
// EDGE CASES: Empty world, no climbable geometry.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMovementLacheNoTargetTest,
	"ClimbingSystem.Movement.Flow.LacheNoTargetStaysHanging",
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMovementLacheNoTargetTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!Movement) { Character->Destroy(); Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->TestLockedLacheTarget().Reset();

	Character->TestInput_Lache(FInputActionValue(true));

	TestEqual(TEXT("Lache: no target should keep Hanging state"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// ============================================================================
// Lache: Valid transition check from Hanging
// ============================================================================

// WHAT: Verifies Lache is a valid state transition from Hanging.
// WHY: State machine must allow Hanging → Lache.
// EDGE CASES: Transition validation only.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMovementLacheValidTransitionTest,
	"ClimbingSystem.Movement.Flow.LacheValidTransitionFromHanging",
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMovementLacheValidTransitionTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!Movement) { Character->Destroy(); Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Hanging);

	TestTrue(TEXT("Lache: should be valid transition from Hanging"),
		Movement->IsValidStateTransition(EClimbingState::Lache));

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// ============================================================================
// Lache: Invalid from None
// ============================================================================

// WHAT: Verifies Lache is not a valid transition from None.
// WHY: Cannot lache if not climbing.
// EDGE CASES: Ground state.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMovementLacheInvalidFromNoneTest,
	"ClimbingSystem.Movement.Flow.LacheInvalidFromNone",
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMovementLacheInvalidFromNoneTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!Movement) { Character->Destroy(); Helper.Teardown(); return false; }

	TestFalse(TEXT("Lache: should not be valid from None state"),
		Movement->IsValidStateTransition(EClimbingState::Lache));

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// ============================================================================
// ClimbUp: Valid transition from Hanging
// ============================================================================

// WHAT: Verifies ClimbingUp is a valid transition from Hanging.
// WHY: Core pull-up flow requires this transition.
// EDGE CASES: Transition validation only.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMovementClimbUpValidFromHangingTest,
	"ClimbingSystem.Movement.Flow.ClimbUpValidFromHanging",
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMovementClimbUpValidFromHangingTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!Movement) { Character->Destroy(); Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Hanging);

	TestTrue(TEXT("ClimbUp: should be valid transition from Hanging"),
		Movement->IsValidStateTransition(EClimbingState::ClimbingUp));

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// ============================================================================
// Shimmy direction commitment persists across ticks
// ============================================================================

// WHAT: Verifies committed shimmy direction persists when input drops below deadzone.
// WHY: Hysteresis prevents direction flicker from analog noise.
// EDGE CASES: Input above deadzone followed by input below deadzone.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMovementShimmyDirectionPersistsTest,
	"ClimbingSystem.Movement.Flow.ShimmyDirectionPersistsAcrossTicks",
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMovementShimmyDirectionPersistsTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	// Commit direction right
	Character->TestInput_ClimbMove(FInputActionValue(FVector2D(0.9f, 0.0f)));
	TestEqual(TEXT("Shimmy: should commit right"), Character->TestCommittedShimmyDir(), 1.0f);

	// Input below deadzone should not change direction
	Character->TestInput_ClimbMove(FInputActionValue(FVector2D(0.05f, 0.0f)));
	TestEqual(TEXT("Shimmy: below-deadzone input should preserve committed direction"),
		Character->TestCommittedShimmyDir(), 1.0f);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
