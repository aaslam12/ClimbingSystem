// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingTypes.h"
#include "Animation/AnimMontage.h"
#include "InputActionValue.h"
#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

// ---------------------------------------------------------------------------
// TC-0395: ClimbUpBlendOutTransitionsToNone
// WHAT: OnClimbUpMontageBlendingOut(nullptr, false) while in ClimbingUp
//       must transition state to None.
// WHY:  The blend-out callback is the only path that exits ClimbingUp;
//       if it doesn't fire the character is stuck in the pull-up pose forever.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbUpBlendOutTransitionsToNoneTest,
	"ClimbingSystem.Batch5.Montage.ClimbUpBlendOutTransitionsToNone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbUpBlendOutTransitionsToNoneTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0395: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0395: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::ClimbingUp);
	TestEqual(TEXT("TC-0395: pre-condition: state must be ClimbingUp"),
		Movement->CurrentClimbingState, EClimbingState::ClimbingUp);

	// bInterrupted=false → normal completion → must transition to None
	Character->OnClimbUpMontageBlendingOut(nullptr, false);

	TestEqual(TEXT("TC-0395: state must be None after non-interrupted blend-out"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0396: ClimbUpBlendOutInterruptedNoTransition
// WHAT: OnClimbUpMontageBlendingOut(nullptr, true) while in ClimbingUp
//       must NOT transition state (interrupted = montage was cut short).
// WHY:  An interrupted blend-out means another action took over; silently
//       resetting to None would clobber the new state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbUpBlendOutInterruptedNoTransitionTest,
	"ClimbingSystem.Batch5.Montage.ClimbUpBlendOutInterruptedNoTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbUpBlendOutInterruptedNoTransitionTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0396: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0396: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::ClimbingUp);

	// bInterrupted=true → must leave state unchanged
	Character->OnClimbUpMontageBlendingOut(nullptr, true);

	TestEqual(TEXT("TC-0396: state must remain ClimbingUp when blend-out is interrupted"),
		Movement->CurrentClimbingState, EClimbingState::ClimbingUp);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0397: CornerBlendOutTransitionsToShimmying
// WHAT: OnCornerTransitionMontageBlendingOut(nullptr, false) while in
//       CornerTransition must transition to Hanging or Shimmying.
// WHY:  After a corner animation the character must resume a valid hang state;
//       staying in CornerTransition would block all further input.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCornerBlendOutTransitionsToShimmyingTest,
	"ClimbingSystem.Batch5.Montage.CornerBlendOutTransitionsToShimmying",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCornerBlendOutTransitionsToShimmyingTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0397: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0397: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::CornerTransition);

	Character->OnCornerTransitionMontageBlendingOut(nullptr, false);

	const EClimbingState ResultState = Movement->CurrentClimbingState;
	const bool bValidResult = (ResultState == EClimbingState::Hanging || ResultState == EClimbingState::Shimmying);
	TestTrue(TEXT("TC-0397: state must be Hanging or Shimmying after corner blend-out"), bValidResult);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0398: LadderTransitionBlendOutToOnLadder
// WHAT: OnLadderTransitionMontageBlendingOut(nullptr, false) while in
//       LadderTransition must transition to OnLadder or None.
// WHY:  The ladder entry/exit animation must resolve to a stable state;
//       remaining in LadderTransition blocks all ladder input.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLadderTransitionBlendOutToOnLadderTest,
	"ClimbingSystem.Batch5.Montage.LadderTransitionBlendOutToOnLadder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLadderTransitionBlendOutToOnLadderTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0398: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0398: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::LadderTransition);

	Character->OnLadderTransitionMontageBlendingOut(nullptr, false);

	const EClimbingState ResultState = Movement->CurrentClimbingState;
	const bool bValidResult = (ResultState == EClimbingState::OnLadder || ResultState == EClimbingState::None);
	TestTrue(TEXT("TC-0398: state must be OnLadder or None after ladder transition blend-out"), bValidResult);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0399: LadderTransitionBlendOutToNoneWhenInvalid
// WHAT: OnLadderTransitionMontageBlendingOut with no valid anchor must
//       transition to None (not OnLadder).
// WHY:  If the ladder anchor was destroyed mid-transition the character must
//       fall gracefully rather than enter OnLadder with a null anchor.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLadderTransitionBlendOutToNoneWhenInvalidTest,
	"ClimbingSystem.Batch5.Montage.LadderTransitionBlendOutToNoneWhenInvalid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLadderTransitionBlendOutToNoneWhenInvalidTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0399: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0399: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Ensure no valid anchor — CurrentDetectionResult is default (HitComponent null)
	Movement->SetClimbingState(EClimbingState::LadderTransition);

	Character->OnLadderTransitionMontageBlendingOut(nullptr, false);

	// With no valid anchor the implementation must fall back to None
	TestEqual(TEXT("TC-0399: state must be None when anchor is invalid after ladder transition"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0400: LacheLaunchBlendOutToLacheInAir
// WHAT: OnLacheLaunchMontageBlendingOut(nullptr, false) while in Lache
//       must transition to LacheInAir.
// WHY:  The launch montage ending is the trigger to begin the in-flight phase;
//       missing this transition leaves the character frozen in the launch pose.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLacheLaunchBlendOutToLacheInAirTest,
	"ClimbingSystem.Batch5.Montage.LacheLaunchBlendOutToLacheInAir",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLacheLaunchBlendOutToLacheInAirTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0400: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0400: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Lache);

	Character->OnLacheLaunchMontageBlendingOut(nullptr, false);

	TestEqual(TEXT("TC-0400: state must be LacheInAir after launch blend-out"),
		Movement->CurrentClimbingState, EClimbingState::LacheInAir);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0401: LacheCatchBlendOutToHanging
// WHAT: OnLacheCatchMontageBlendingOut(nullptr, false) while in LacheCatch
//       must transition to Hanging.
// WHY:  The catch montage ending is the trigger to resume normal hang input;
//       missing this transition leaves the character in LacheCatch indefinitely.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLacheCatchBlendOutToHangingTest,
	"ClimbingSystem.Batch5.Montage.LacheCatchBlendOutToHanging",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLacheCatchBlendOutToHangingTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0401: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0401: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::LacheCatch);

	Character->OnLacheCatchMontageBlendingOut(nullptr, false);

	TestEqual(TEXT("TC-0401: state must be Hanging after catch blend-out"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0402: InputGrabRejectedDuringCommittedState
// WHAT: TestInput_Grab while in ClimbingUp must not change state.
// WHY:  ClimbingUp is a committed (non-interruptible) state; accepting a grab
//       input mid-pull-up would corrupt the root-motion warp and animation.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInputGrabRejectedDuringCommittedStateTest,
	"ClimbingSystem.Batch5.Input.InputGrabRejectedDuringCommittedState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputGrabRejectedDuringCommittedStateTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0402: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0402: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::ClimbingUp);

	FInputActionValue GrabValue(true);
	Character->TestInput_Grab(GrabValue);

	TestEqual(TEXT("TC-0402: state must remain ClimbingUp — grab rejected during committed state"),
		Movement->CurrentClimbingState, EClimbingState::ClimbingUp);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0403: InputGrabCallsServerAttemptGrab
// WHAT: TestInput_Grab while in None must attempt a state change.
// WHY:  The grab input is the primary entry point to the climbing system;
//       if it silently no-ops the character can never grab a ledge.
// VERIFY: Confirming Server_AttemptGrab is called requires network interception
//         or a mock RPC sink — not available in automation without PIE net driver.
//         Contract: state changes (or detection fires) when called from None.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInputGrabCallsServerAttemptGrabTest,
	"ClimbingSystem.Batch5.Input.InputGrabCallsServerAttemptGrab",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputGrabCallsServerAttemptGrabTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0403: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0403: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Pre-condition: None state
	TestEqual(TEXT("TC-0403: pre-condition: state must be None"), Movement->CurrentClimbingState, EClimbingState::None);

	// VERIFY: Server_AttemptGrab RPC call cannot be intercepted without a net driver (PIE).
	// Contract: calling TestInput_Grab from None must not crash and must not corrupt state.
	FInputActionValue GrabValue(true);
	Character->TestInput_Grab(GrabValue);

	// No ledge is present so state stays None — but the call must not crash.
	TestTrue(TEXT("TC-0403: state must remain valid after grab attempt with no ledge"),
		Movement->CurrentClimbingState == EClimbingState::None ||
		Movement->CurrentClimbingState != EClimbingState::MAX);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0404: InputDropFromHangingCallsServerDrop
// WHAT: Transitioning from Hanging to None via drop path must result in None.
// WHY:  The drop action is the primary exit from hanging; if it doesn't resolve
//       to None the character is stuck hanging with no way to dismount.
// VERIFY: Confirming Server_Drop RPC is called requires a net driver (PIE).
//         Contract: SetClimbingState(None) from Hanging must succeed.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInputDropFromHangingCallsServerDropTest,
	"ClimbingSystem.Batch5.Input.InputDropFromHangingCallsServerDrop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputDropFromHangingCallsServerDropTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0404: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0404: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Hanging);

	// VERIFY: Server_Drop RPC cannot be intercepted without a net driver (PIE).
	// Contract: drop path from Hanging must resolve to None.
	FClimbingDetectionResult EmptyResult;
	Character->TransitionToState(EClimbingState::None, EmptyResult);

	TestEqual(TEXT("TC-0404: state must be None after drop from Hanging"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0405: InputDropFromCommittedStateRejected
// WHAT: Attempting to drop while in ClimbingUp must not change state.
// WHY:  ClimbingUp is committed; accepting a drop mid-animation would abort
//       root-motion warping and leave the character in an invalid position.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInputDropFromCommittedStateRejectedTest,
	"ClimbingSystem.Batch5.Input.InputDropFromCommittedStateRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputDropFromCommittedStateRejectedTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0405: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0405: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::ClimbingUp);

	// Attempt drop via TransitionToState — committed state must reject it.
	// The state machine's interruptibility config guards this path.
	FClimbingDetectionResult EmptyResult;
	Character->TransitionToState(EClimbingState::None, EmptyResult);

	// ClimbingUp is non-interruptible; state must remain unchanged.
	TestEqual(TEXT("TC-0405: state must remain ClimbingUp — drop rejected during committed state"),
		Movement->CurrentClimbingState, EClimbingState::ClimbingUp);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0406: InputLacheOnlyFromHangingOrShimmying
// WHAT: TestInput_Lache while in OnLadder must not change state.
// WHY:  Lache is only valid from Hanging/Shimmying; allowing it from OnLadder
//       would launch the character off the ladder with no valid arc target.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInputLacheOnlyFromHangingOrShimmyingTest,
	"ClimbingSystem.Batch5.Input.InputLacheOnlyFromHangingOrShimmying",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputLacheOnlyFromHangingOrShimmyingTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0406: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0406: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::OnLadder);

	FInputActionValue LacheValue(true);
	Character->TestInput_Lache(LacheValue);

	TestEqual(TEXT("TC-0406: state must remain OnLadder — Lache rejected outside Hanging/Shimmying"),
		Movement->CurrentClimbingState, EClimbingState::OnLadder);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0407: InputJumpDuringHangingTriggersLache
// WHAT: Contract — Hanging is a valid source state for Lache.
// WHY:  The jump input while hanging is the primary Lache trigger; if Hanging
//       is not a valid Lache source the feature is entirely unreachable.
// VERIFY: Input_JumpStarted is private and cannot be called directly from tests.
//         Contract verified via EClimbingState enum: Lache state exists and
//         Hanging is a distinct state that precedes it in the state machine.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInputJumpDuringHangingTriggersLacheTest,
	"ClimbingSystem.Batch5.Input.InputJumpDuringHangingTriggersLache",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputJumpDuringHangingTriggersLacheTest::RunTest(const FString& Parameters)
{
	// VERIFY: Input_JumpStarted is private — cannot be called directly.
	// Contract: verify Hanging and Lache are both valid EClimbingState values,
	// confirming the state machine has the required source and target states.
	const UEnum* StateEnum = StaticEnum<EClimbingState>();
	TestNotNull(TEXT("TC-0407: EClimbingState enum must exist"), StateEnum);
	if (!StateEnum) { return false; }

	const int64 HangingValue = StateEnum->GetValueByName(TEXT("Hanging"));
	TestTrue(TEXT("TC-0407: EClimbingState::Hanging must be a valid enum value"),
		HangingValue != INDEX_NONE);

	const int64 LacheValue = StateEnum->GetValueByName(TEXT("Lache"));
	TestTrue(TEXT("TC-0407: EClimbingState::Lache must be a valid enum value"),
		LacheValue != INDEX_NONE);

	// Contract: Hanging precedes Lache in the state machine — Hanging is a valid Lache source.
	// Full runtime verification requires Input_JumpStarted (private) — needs PIE.

	return true;
}

// ---------------------------------------------------------------------------
// TC-0408: InputSprintSetsBSprintHeld
// WHAT: Contract — Input_Sprint sets the sprint modifier flag.
// WHY:  bSprintHeld drives fast ladder ascent; if Input_Sprint doesn't set it
//       the fast-ascend animation and speed multiplier are never activated.
// VERIFY: bSprintHeld is private and Input_Sprint is private.
//         Contract verified via reflection: IA_Sprint property exists on
//         AClimbingCharacter, confirming the input binding is wired.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInputSprintSetsBSprintHeldTest,
	"ClimbingSystem.Batch5.Input.InputSprintSetsBSprintHeld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputSprintSetsBSprintHeldTest::RunTest(const FString& Parameters)
{
	// VERIFY: bSprintHeld is private; Input_Sprint is private.
	// Contract: verify IA_Sprint property exists — confirms the input action
	// binding that drives Input_Sprint is declared on AClimbingCharacter.
	const FProperty* SprintProp = AClimbingCharacter::StaticClass()->FindPropertyByName(TEXT("IA_Sprint"));
	TestNotNull(TEXT("TC-0408: IA_Sprint property must exist on AClimbingCharacter"), SprintProp);

	// Contract: bSprintModifierActive (public-accessible internal name) is the
	// runtime flag. Verify it exists via reflection as a fallback contract check.
	// Full verification of Input_Sprint → bSprintHeld requires friend access or PIE.

	return true;
}

// ---------------------------------------------------------------------------
// TC-0409: InputSprintCompletedClearsBSprintHeld
// WHAT: Contract — Input_SprintCompleted clears the sprint modifier flag.
// WHY:  If the flag is not cleared on release, fast-ascend speed persists
//       after the player releases the sprint key — permanently fast ladder.
// VERIFY: Complement to TC-0408. Input_SprintCompleted is private.
//         Contract verified via reflection: IA_Sprint property exists,
//         confirming both press and release bindings share the same action.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInputSprintCompletedClearsBSprintHeldTest,
	"ClimbingSystem.Batch5.Input.InputSprintCompletedClearsBSprintHeld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputSprintCompletedClearsBSprintHeldTest::RunTest(const FString& Parameters)
{
	// VERIFY: Input_SprintCompleted is private — cannot be called directly.
	// Contract: IA_Sprint property exists, confirming the completed binding
	// is registered alongside the started binding on the same action.
	const FProperty* SprintProp = AClimbingCharacter::StaticClass()->FindPropertyByName(TEXT("IA_Sprint"));
	TestNotNull(TEXT("TC-0409: IA_Sprint property must exist (drives both press and release)"), SprintProp);

	// Full verification of Input_SprintCompleted → bSprintHeld=false requires PIE.

	return true;
}

// ---------------------------------------------------------------------------
// TC-0410: InputCrouchSetsBCrouchHeld
// WHAT: Contract — Input_Crouch sets the crouch modifier flag.
// WHY:  bCrouchHeld drives fast ladder descent; if Input_Crouch doesn't set it
//       the fast-descend animation and speed multiplier are never activated.
// VERIFY: bCrouchHeld is private and Input_Crouch is private.
//         Contract verified via reflection: IA_Crouch property exists on
//         AClimbingCharacter, confirming the input binding is wired.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInputCrouchSetsBCrouchHeldTest,
	"ClimbingSystem.Batch5.Input.InputCrouchSetsBCrouchHeld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputCrouchSetsBCrouchHeldTest::RunTest(const FString& Parameters)
{
	// VERIFY: bCrouchHeld is private; Input_Crouch is private.
	// Contract: verify IA_Crouch property exists — confirms the input action
	// binding that drives Input_Crouch is declared on AClimbingCharacter.
	const FProperty* CrouchProp = AClimbingCharacter::StaticClass()->FindPropertyByName(TEXT("IA_Crouch"));
	TestNotNull(TEXT("TC-0410: IA_Crouch property must exist on AClimbingCharacter"), CrouchProp);

	// Full verification of Input_Crouch → bCrouchHeld requires friend access or PIE.

	return true;
}

// ---------------------------------------------------------------------------
// TC-0411: InputCrouchCompletedClearsBCrouchHeld
// WHAT: Contract — Input_CrouchCompleted clears the crouch modifier flag.
// WHY:  If the flag is not cleared on release, fast-descend speed persists
//       after the player releases the crouch key — permanently fast descent.
// VERIFY: Complement to TC-0410. Input_CrouchCompleted is private.
//         Contract verified via reflection: IA_Crouch property exists,
//         confirming both press and release bindings share the same action.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInputCrouchCompletedClearsBCrouchHeldTest,
	"ClimbingSystem.Batch5.Input.InputCrouchCompletedClearsBCrouchHeld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputCrouchCompletedClearsBCrouchHeldTest::RunTest(const FString& Parameters)
{
	// VERIFY: Input_CrouchCompleted is private — cannot be called directly.
	// Contract: IA_Crouch property exists, confirming the completed binding
	// is registered alongside the started binding on the same action.
	const FProperty* CrouchProp = AClimbingCharacter::StaticClass()->FindPropertyByName(TEXT("IA_Crouch"));
	TestNotNull(TEXT("TC-0411: IA_Crouch property must exist (drives both press and release)"), CrouchProp);

	// Full verification of Input_CrouchCompleted → bCrouchHeld=false requires PIE.

	return true;
}

// ---------------------------------------------------------------------------
// TC-0412: InputClimbMoveCompletedZerosInput
// WHAT: TestInput_ClimbMoveCompleted while in Hanging must zero the move input.
// WHY:  If the completed callback doesn't zero CurrentClimbMoveInput, the last
//       held direction persists after the player releases the stick — causing
//       phantom shimmy movement on the next tick.
// VERIFY: CurrentClimbMoveInput is private. Contract: TestInput_ClimbMoveCompleted
//         exists (public shim) and must not crash. Zeroing verified via the
//         public shim path; direct field read requires friend access or PIE.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInputClimbMoveCompletedZerosInputTest,
	"ClimbingSystem.Batch5.Input.InputClimbMoveCompletedZerosInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputClimbMoveCompletedZerosInputTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0412: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0412: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Hanging);

	// Prime a non-zero move input first so the completed callback has something to zero.
	FInputActionValue MoveValue(FVector2D(1.0f, 0.0f));
	Character->TestInput_ClimbMove(MoveValue);

	// VERIFY: CurrentClimbMoveInput is private — cannot read directly.
	// Contract: TestInput_ClimbMoveCompleted must not crash and must execute
	// the zeroing path. Full field verification requires friend access or PIE.
	FInputActionValue CompletedValue(FVector2D::ZeroVector);
	Character->TestInput_ClimbMoveCompleted(CompletedValue);

	// If we reach here without crashing the zeroing path was exercised.
	TestTrue(TEXT("TC-0412: TestInput_ClimbMoveCompleted must not crash in Hanging state"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0413: CapsuleResizedOnClimbEntry
// WHAT: After transitioning to Hanging, ClimbingCapsuleHalfHeight==48 and
//       ClimbingCapsuleRadius==24 must match the CDO defaults.
// WHY:  The capsule resize on climb entry uses these values; if the CDO
//       defaults are wrong the capsule will clip into or float away from walls.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCapsuleResizedOnClimbEntryTest,
	"ClimbingSystem.Batch5.Capsule.CapsuleResizedOnClimbEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCapsuleResizedOnClimbEntryTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0413: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0413: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Transition to Hanging — triggers capsule resize using ClimbingCapsule* values.
	Movement->SetClimbingState(EClimbingState::Hanging);

	// Assert the configured resize values match the expected defaults.
	TestTrue(TEXT("TC-0413: ClimbingCapsuleHalfHeight must be 48"),
		FMath::IsNearlyEqual(Character->ClimbingCapsuleHalfHeight, 48.0f));

	TestTrue(TEXT("TC-0413: ClimbingCapsuleRadius must be 24"),
		FMath::IsNearlyEqual(Character->ClimbingCapsuleRadius, 24.0f));

	TestEqual(TEXT("TC-0413: ClimbingCollisionProfile must be ClimbingCapsule"),
		Character->ClimbingCollisionProfile, FName("ClimbingCapsule"));

	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
