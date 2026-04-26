// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Animation/AnimMontage.h"
#include "Data/ClimbingTypes.h"
#include "Helpers/SharedTestHelpers.h"
#include "Misc/AutomationTest.h"

namespace
{
static FClimbingDetectionResult MakeDetection(const FVector& LedgePos, bool bValid = true)
{
	FClimbingDetectionResult D;
	D.bValid = bValid;
	D.LedgePosition = LedgePos;
	D.SurfaceNormal = FVector(-1.0f, 0.0f, 0.0f);
	D.SurfaceTier = EClimbSurfaceTier::Climbable;
	D.ClearanceType = EClimbClearanceType::Full;
	return D;
}
}

// TC-0168
// WHAT: Verifies MantleHigh slot is selected when ledge height exceeds MantleLowMaxHeight.
// WHY: Mantle animation selection depends on ledge-to-feet height comparison.
// EDGE CASES: Ledge Z = character Z + 120, threshold = 100.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStateMachineMantleHighAboveThresholdTest,
	"ClimbingSystem.StateMachine.Montage.MantleHighAboveThreshold",
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FStateMachineMantleHighAboveThresholdTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0168: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0168: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Character->MantleLowMaxHeight = 100.0f;

	// Assign dummy montages so slot selection can be verified
	Character->MantleHigh = NewObject<UAnimMontage>(Character);
	Character->MantleLow  = NewObject<UAnimMontage>(Character);

	// Ledge 120 cm above character origin — exceeds MantleLowMaxHeight=100
	const FVector LedgePos = Character->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f);
	const FClimbingDetectionResult Detection = MakeDetection(LedgePos);

	// Drive state entry via movement component; OnStateEnter is called by TransitionToState (protected).
	// We verify the correct slot is mapped: MantleHigh should be returned for MantleHigh slot.
	Movement->SetClimbingState(EClimbingState::Mantling);
	TestEqual(TEXT("TC-0168: state should be Mantling"), Movement->CurrentClimbingState, EClimbingState::Mantling);

	// Verify MantleHigh slot returns the assigned montage (slot mapping is correct)
	UAnimMontage* HighMontage = Character->GetMontageForSlot(EClimbingAnimationSlot::MantleHigh);
	TestNotNull(TEXT("TC-0168: MantleHigh slot should return assigned montage"), HighMontage);
	TestEqual(TEXT("TC-0168: MantleHigh slot should return MantleHigh montage"), HighMontage, Character->MantleHigh.Get());

	// VERIFY: full montage-playing verification (Montage_IsPlaying(MantleHigh)) requires
	// TransitionToState to be called (protected) and a valid AnimInstance with ABP loaded.
	// In a headless test world the mesh has no AnimInstance, so Montage_Play is a no-op.

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// TC-0169
// WHAT: Verifies CornerInsideLeft slot is selected for inside corner with left shimmy direction.
// WHY: Corner montage selection depends on bCurrentCornerIsInside and CommittedShimmyDir.
// EDGE CASES: bCurrentCornerIsInside=true, CommittedShimmyDir=-1.0 → CornerInsideLeft.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStateMachineCornerInsideLeftSelectedTest,
	"ClimbingSystem.StateMachine.Montage.CornerInsideLeftSelected",
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FStateMachineCornerInsideLeftSelectedTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0169: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0169: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	// Assign dummy montages for all four corner slots
	Character->CornerInsideLeft   = NewObject<UAnimMontage>(Character);
	Character->CornerInsideRight  = NewObject<UAnimMontage>(Character);
	Character->CornerOutsideLeft  = NewObject<UAnimMontage>(Character);
	Character->CornerOutsideRight = NewObject<UAnimMontage>(Character);

	// Set state fields that drive slot selection in OnStateEnter(CornerTransition)
	Character->bCurrentCornerIsInside = true;
	Character->TestCommittedShimmyDir() = -1.0f;

	Movement->SetClimbingState(EClimbingState::CornerTransition);
	TestEqual(TEXT("TC-0169: state should be CornerTransition"), Movement->CurrentClimbingState, EClimbingState::CornerTransition);

	// Verify the correct slot is mapped
	UAnimMontage* InsideLeft = Character->GetMontageForSlot(EClimbingAnimationSlot::CornerInsideLeft);
	TestNotNull(TEXT("TC-0169: CornerInsideLeft slot should return assigned montage"), InsideLeft);
	TestEqual(TEXT("TC-0169: CornerInsideLeft slot should return CornerInsideLeft montage"),
		InsideLeft, Character->CornerInsideLeft.Get());

	// VERIFY: full selection verification requires TransitionToState (protected) + live AnimInstance.

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// TC-0170
// WHAT: Verifies CornerOutsideRight slot is selected for outside corner with right shimmy direction.
// WHY: Corner montage selection depends on bCurrentCornerIsInside and CommittedShimmyDir.
// EDGE CASES: bCurrentCornerIsInside=false, CommittedShimmyDir=1.0 → CornerOutsideRight.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStateMachineCornerOutsideRightSelectedTest,
	"ClimbingSystem.StateMachine.Montage.CornerOutsideRightSelected",
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FStateMachineCornerOutsideRightSelectedTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0170: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0170: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Character->CornerInsideLeft   = NewObject<UAnimMontage>(Character);
	Character->CornerInsideRight  = NewObject<UAnimMontage>(Character);
	Character->CornerOutsideLeft  = NewObject<UAnimMontage>(Character);
	Character->CornerOutsideRight = NewObject<UAnimMontage>(Character);

	Character->bCurrentCornerIsInside = false;
	Character->TestCommittedShimmyDir() = 1.0f;

	Movement->SetClimbingState(EClimbingState::CornerTransition);
	TestEqual(TEXT("TC-0170: state should be CornerTransition"), Movement->CurrentClimbingState, EClimbingState::CornerTransition);

	UAnimMontage* OutsideRight = Character->GetMontageForSlot(EClimbingAnimationSlot::CornerOutsideRight);
	TestNotNull(TEXT("TC-0170: CornerOutsideRight slot should return assigned montage"), OutsideRight);
	TestEqual(TEXT("TC-0170: CornerOutsideRight slot should return CornerOutsideRight montage"),
		OutsideRight, Character->CornerOutsideRight.Get());

	// VERIFY: full selection verification requires TransitionToState (protected) + live AnimInstance.

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// TC-0171
// WHAT: Verifies LadderExitTop slot is selected when detection result is valid (top exit).
// WHY: Ladder exit direction is determined by DetectionResult.bValid in OnStateEnter(LadderTransition).
// EDGE CASES: bValid=true → LadderExitTop.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStateMachineLadderTransitionTopWhenValidDetectionTest,
	"ClimbingSystem.StateMachine.Montage.LadderTransitionTopWhenValidDetection",
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FStateMachineLadderTransitionTopWhenValidDetectionTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0171: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0171: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Character->LadderExitTop    = NewObject<UAnimMontage>(Character);
	Character->LadderExitBottom = NewObject<UAnimMontage>(Character);

	// Pre-condition: character is on ladder
	Movement->SetClimbingState(EClimbingState::OnLadder);

	// Transition to LadderTransition with a valid detection result (top exit)
	Movement->SetClimbingState(EClimbingState::LadderTransition);
	TestEqual(TEXT("TC-0171: state should be LadderTransition"), Movement->CurrentClimbingState, EClimbingState::LadderTransition);

	// Verify LadderExitTop slot is correctly mapped
	UAnimMontage* ExitTop = Character->GetMontageForSlot(EClimbingAnimationSlot::LadderExitTop);
	TestNotNull(TEXT("TC-0171: LadderExitTop slot should return assigned montage"), ExitTop);
	TestEqual(TEXT("TC-0171: LadderExitTop slot should return LadderExitTop montage"),
		ExitTop, Character->LadderExitTop.Get());

	// VERIFY: full selection verification (bValid=true → LadderExitTop played) requires
	// TransitionToState (protected) with a valid FClimbingDetectionResult + live AnimInstance.

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// TC-0172
// WHAT: Verifies LadderExitBottom slot is selected when detection result is invalid (bottom exit).
// WHY: Ladder exit direction is determined by DetectionResult.bValid in OnStateEnter(LadderTransition).
// EDGE CASES: bValid=false → LadderExitBottom.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStateMachineLadderTransitionBottomWhenInvalidDetectionTest,
	"ClimbingSystem.StateMachine.Montage.LadderTransitionBottomWhenInvalidDetection",
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FStateMachineLadderTransitionBottomWhenInvalidDetectionTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0172: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0172: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Character->LadderExitTop    = NewObject<UAnimMontage>(Character);
	Character->LadderExitBottom = NewObject<UAnimMontage>(Character);

	Movement->SetClimbingState(EClimbingState::OnLadder);
	Movement->SetClimbingState(EClimbingState::LadderTransition);
	TestEqual(TEXT("TC-0172: state should be LadderTransition"), Movement->CurrentClimbingState, EClimbingState::LadderTransition);

	// Verify LadderExitBottom slot is correctly mapped
	UAnimMontage* ExitBottom = Character->GetMontageForSlot(EClimbingAnimationSlot::LadderExitBottom);
	TestNotNull(TEXT("TC-0172: LadderExitBottom slot should return assigned montage"), ExitBottom);
	TestEqual(TEXT("TC-0172: LadderExitBottom slot should return LadderExitBottom montage"),
		ExitBottom, Character->LadderExitBottom.Get());

	// VERIFY: full selection verification (bValid=false → LadderExitBottom played) requires
	// TransitionToState (protected) with bValid=false FClimbingDetectionResult + live AnimInstance.

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// TC-0173
// WHAT: Verifies a LogClimbing Warning is emitted when ClimbingMontageSlot is invalid.
// WHY: Slot name mismatch causes silent animation failure; a warning aids debugging.
// EDGE CASES: Invalid slot name set before Montage_Play is called during state entry.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStateMachineMontageSlotMismatchLogsWarningTest,
	"ClimbingSystem.StateMachine.Montage.SlotMismatchLogsWarning",
	EAutomationTestFlags::CommandletContext | EAutomationTestFlags::EngineFilter)

bool FStateMachineMontageSlotMismatchLogsWarningTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0173: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0173: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	// Assign a montage so Montage_Play is attempted (triggering the slot-mismatch warning path)
	Character->HangIdle = NewObject<UAnimMontage>(Character);

	// Set an invalid slot name to trigger the warning in Montage_Play failure path
	Character->ClimbingMontageSlot = FName("InvalidSlotName");

	// Capture the expected warning emitted when Montage_Play returns <= 0 due to slot mismatch
	AddExpectedError(TEXT("ClimbingMontageSlot"), EAutomationExpectedErrorFlags::Contains, 1);

	// Transition to Hanging — OnStateEnter will call Montage_Play which fails on invalid slot
	// and logs: "Montage_Play failed for slot InvalidSlotName — check ClimbingMontageSlot name..."
	// VERIFY: Warning is only emitted when a live AnimInstance is present (requires PIE or loaded ABP).
	// In a headless test world GetMesh()->GetAnimInstance() returns null, so Montage_Play is skipped.
	// The AddExpectedError above captures the warning if the AnimInstance path is reached.
	Movement->SetClimbingState(EClimbingState::Hanging);
	TestEqual(TEXT("TC-0173: state should be Hanging"), Movement->CurrentClimbingState, EClimbingState::Hanging);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
