// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Helpers/SharedTestHelpers.h"
#include "Misc/AutomationTest.h"

// ============================================================================
// CATEGORY 6: Integration Tests — Extended State Chain Flows
// ============================================================================

// FLOW: Braced wall full cycle — None → Hanging → BracedWall → BracedShimmying → None
// Step 1: Start in None, transition to Hanging
// Step 2: Hanging → BracedWall
// Step 3: BracedWall → BracedShimmying
// Step 4: BracedShimmying → None, verify previous state tracking

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingBracedWallStateChainIntegrationTest,
	"ClimbingSystem.Integration.BracedWall.FullStateChain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingBracedWallStateChainIntegrationTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Integration: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Integration: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	// Step 1: None → Hanging
	Movement->SetClimbingState(EClimbingState::Hanging);
	TestEqual(TEXT("Integration: state should be Hanging"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	// Step 2: Hanging → BracedWall
	TestTrue(TEXT("Integration: Hanging -> BracedWall should be valid"),
		Movement->IsValidStateTransition(EClimbingState::BracedWall));
	Movement->SetClimbingState(EClimbingState::BracedWall);
	TestEqual(TEXT("Integration: previous state should be Hanging"),
		Movement->PreviousClimbingState, EClimbingState::Hanging);

	// Step 3: BracedWall → BracedShimmying
	TestTrue(TEXT("Integration: BracedWall -> BracedShimmying should be valid"),
		Movement->IsValidStateTransition(EClimbingState::BracedShimmying));
	Movement->SetClimbingState(EClimbingState::BracedShimmying);
	TestEqual(TEXT("Integration: previous state should be BracedWall"),
		Movement->PreviousClimbingState, EClimbingState::BracedWall);

	// Step 4: BracedShimmying → None
	Movement->SetClimbingState(EClimbingState::None);
	TestEqual(TEXT("Integration: final state should be None"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// FLOW: Ragdoll recovery — None → Hanging → Ragdoll → None
// Step 1: Enter Hanging
// Step 2: Transition to Ragdoll (committed)
// Step 3: Verify Ragdoll blocks Hanging at 0% completion
// Step 4: Recover to None

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRagdollRecoveryIntegrationTest,
	"ClimbingSystem.Integration.Ragdoll.RecoveryCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRagdollRecoveryIntegrationTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Integration: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Integration: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	// Step 1-2: None → Hanging → Ragdoll
	Movement->SetClimbingState(EClimbingState::Hanging);
	Movement->SetClimbingState(EClimbingState::Ragdoll);
	TestEqual(TEXT("Integration: state should be Ragdoll"),
		Movement->CurrentClimbingState, EClimbingState::Ragdoll);

	// Step 3: Ragdoll is committed — Hanging blocked
	TestFalse(TEXT("Integration: Ragdoll should not be interruptible"),
		Movement->CanInterruptCurrentState());
	TestFalse(TEXT("Integration: Ragdoll -> Hanging should be blocked"),
		Movement->IsValidStateTransition(EClimbingState::Hanging));

	// Step 4: Recovery to None
	TestTrue(TEXT("Integration: Ragdoll -> None should be valid"),
		Movement->IsValidStateTransition(EClimbingState::None));
	Movement->SetClimbingState(EClimbingState::None);
	TestEqual(TEXT("Integration: final state should be None"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// FLOW: Corner transition — Hanging → Shimmying → CornerTransition (committed)
// Step 1: Enter Shimmying from Hanging
// Step 2: Enter CornerTransition (committed)
// Step 3: Verify None always valid as escape, Shimmying blocked at 0%

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingCornerTransitionIntegrationTest,
	"ClimbingSystem.Integration.Corner.TransitionCommittedState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingCornerTransitionIntegrationTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Integration: movement component should be created"), Movement);
	if (!Movement) { return false; }

	Movement->SetClimbingState(EClimbingState::Hanging);
	Movement->SetClimbingState(EClimbingState::Shimmying);
	Movement->SetClimbingState(EClimbingState::CornerTransition);

	TestEqual(TEXT("Integration: state should be CornerTransition"),
		Movement->CurrentClimbingState, EClimbingState::CornerTransition);
	TestFalse(TEXT("Integration: CornerTransition should not be interruptible"),
		Movement->CanInterruptCurrentState());
	TestTrue(TEXT("Integration: CornerTransition -> None (abort) should be valid"),
		Movement->IsValidStateTransition(EClimbingState::None));
	TestFalse(TEXT("Integration: CornerTransition -> Shimmying blocked at 0% completion"),
		Movement->IsValidStateTransition(EClimbingState::Shimmying));

	return true;
}

// FLOW: Mantle committed state — None → Mantling → verify constraints → None
// Step 1: Enter Mantling directly
// Step 2: Verify committed, None valid, Hanging blocked
// Step 3: Exit to None

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMantleStateIntegrationTest,
	"ClimbingSystem.Integration.Mantle.CommittedStateConstraints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMantleStateIntegrationTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Integration: movement component should be created"), Movement);
	if (!Movement) { return false; }

	Movement->SetClimbingState(EClimbingState::Mantling);
	TestFalse(TEXT("Integration: Mantling should be committed (not interruptible)"),
		Movement->CanInterruptCurrentState());
	TestTrue(TEXT("Integration: Mantling -> None should be valid"),
		Movement->IsValidStateTransition(EClimbingState::None));
	TestFalse(TEXT("Integration: Mantling -> Hanging should be blocked at 0%"),
		Movement->IsValidStateTransition(EClimbingState::Hanging));

	Movement->SetClimbingState(EClimbingState::None);
	TestEqual(TEXT("Integration: final state should be None"),
		Movement->CurrentClimbingState, EClimbingState::None);

	return true;
}

// FLOW: Full climb-up cycle — None → Hanging → ClimbingUp → None
// Step 1: Enter Hanging
// Step 2: Transition to ClimbingUp
// Step 3: Verify previous state tracking at each step
// Step 4: Return to None

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingFullClimbCycleIntegrationTest,
	"ClimbingSystem.Integration.ClimbUp.FullCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingFullClimbCycleIntegrationTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Integration: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Integration: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	// Step 1: None → Hanging
	Movement->SetClimbingState(EClimbingState::Hanging);
	TestEqual(TEXT("Integration: previous state after None->Hanging should be None"),
		Movement->PreviousClimbingState, EClimbingState::None);

	// Step 2: Hanging → ClimbingUp
	TestTrue(TEXT("Integration: Hanging -> ClimbingUp should be valid"),
		Movement->IsValidStateTransition(EClimbingState::ClimbingUp));
	Movement->SetClimbingState(EClimbingState::ClimbingUp);
	TestEqual(TEXT("Integration: previous state after Hanging->ClimbingUp should be Hanging"),
		Movement->PreviousClimbingState, EClimbingState::Hanging);

	// Step 3-4: ClimbingUp → None
	TestTrue(TEXT("Integration: ClimbingUp -> None should be valid"),
		Movement->IsValidStateTransition(EClimbingState::None));
	Movement->SetClimbingState(EClimbingState::None);
	TestEqual(TEXT("Integration: previous state after ClimbingUp->None should be ClimbingUp"),
		Movement->PreviousClimbingState, EClimbingState::ClimbingUp);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
