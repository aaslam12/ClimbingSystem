// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

// Test-only access to protected IMC lifecycle methods/flags.
#define protected public
#include "Character/ClimbingCharacter.h"
#undef protected

#include "InputMappingContext.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Helpers/SharedTestHelpers.h"
#include "Misc/AutomationTest.h"

// WHAT: Verifies AddClimbingInputMappingContext is guarded by local control and does not flip the active flag on remote/non-local pawns.
// WHY: IMC lifecycle must not mutate state for non-local actors in multiplayer.
// EDGE CASES: Mapping context is valid but IsLocallyControlled() is false.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingInputRuntimeAddClimbingRequiresLocalControlTest,
	"ClimbingSystem.Input.Runtime.AddClimbing.RequiresLocalControl",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingInputRuntimeAddClimbingRequiresLocalControlTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Input runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	Character->ClimbingInputMappingContext = NewObject<UInputMappingContext>(Character);
	Character->bClimbingIMCActive = false;
	TestFalse(TEXT("Input runtime: spawned test character should start non-local in automation world"),
		Character->IsLocallyControlled());

	Character->AddClimbingInputMappingContext();
	TestFalse(TEXT("Input runtime: AddClimbingInputMappingContext should not activate IMC when pawn is not locally controlled"),
		Character->bClimbingIMCActive);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies AddLocomotionInputMappingContext is guarded by local control for non-local pawns.
// WHY: Locomotion IMC should only be managed on the owning client.
// EDGE CASES: Valid mapping context but local-control guard should block activation.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingInputRuntimeAddLocomotionRequiresLocalControlTest,
	"ClimbingSystem.Input.Runtime.AddLocomotion.RequiresLocalControl",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingInputRuntimeAddLocomotionRequiresLocalControlTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Input runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	Character->LocomotionInputMappingContext = NewObject<UInputMappingContext>(Character);
	Character->bLocomotionIMCActive = false;

	Character->AddLocomotionInputMappingContext();
	TestFalse(TEXT("Input runtime: AddLocomotionInputMappingContext should not activate IMC when pawn is not locally controlled"),
		Character->bLocomotionIMCActive);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies RemoveClimbingInputMappingContext early-returns for non-local pawns and preserves active flag state.
// WHY: Prevents remote pawns from mutating local input-subsystem bookkeeping.
// EDGE CASES: Active flag preset to true before call.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingInputRuntimeRemoveClimbingRequiresLocalControlTest,
	"ClimbingSystem.Input.Runtime.RemoveClimbing.RequiresLocalControl",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingInputRuntimeRemoveClimbingRequiresLocalControlTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Input runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	Character->ClimbingInputMappingContext = NewObject<UInputMappingContext>(Character);
	Character->bClimbingIMCActive = true;

	Character->RemoveClimbingInputMappingContext();
	TestTrue(TEXT("Input runtime: RemoveClimbingInputMappingContext should preserve active flag when local-control guard blocks execution"),
		Character->bClimbingIMCActive);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies RemoveLocomotionInputMappingContext early-returns for non-local pawns and preserves active flag state.
// WHY: Ensures remote instances do not alter local input mapping lifecycle state.
// EDGE CASES: Active flag preset to true before call.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingInputRuntimeRemoveLocomotionRequiresLocalControlTest,
	"ClimbingSystem.Input.Runtime.RemoveLocomotion.RequiresLocalControl",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingInputRuntimeRemoveLocomotionRequiresLocalControlTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Input runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	Character->LocomotionInputMappingContext = NewObject<UInputMappingContext>(Character);
	Character->bLocomotionIMCActive = true;

	Character->RemoveLocomotionInputMappingContext();
	TestTrue(TEXT("Input runtime: RemoveLocomotionInputMappingContext should preserve active flag when local-control guard blocks execution"),
		Character->bLocomotionIMCActive);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies PawnClientRestart clears both IMC active flags before attempting re-application.
// WHY: Possession/controller restart path must reset bookkeeping to avoid stale active-state drift.
// EDGE CASES: Both flags pre-set true before restart.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingInputRuntimePawnClientRestartClearsFlagsTest,
	"ClimbingSystem.Input.Runtime.PawnClientRestart.ClearsIMCFlags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingInputRuntimePawnClientRestartClearsFlagsTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Input runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	Character->bClimbingIMCActive = true;
	Character->bLocomotionIMCActive = true;
	Character->PawnClientRestart();

	TestFalse(TEXT("Input runtime: PawnClientRestart should clear bClimbingIMCActive before re-apply logic"),
		Character->bClimbingIMCActive);
	TestFalse(TEXT("Input runtime: PawnClientRestart should clear bLocomotionIMCActive before re-apply logic"),
		Character->bLocomotionIMCActive);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies PawnClientRestart does not alter climbing state while performing IMC lifecycle recovery.
// WHY: Restart should rebuild input contexts without corrupting authoritative climbing state.
// EDGE CASES: Character starts in Hanging state with both IMC flags set.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingInputRuntimePawnClientRestartPreservesStateTest,
	"ClimbingSystem.Input.Runtime.PawnClientRestart.PreservesClimbingState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingInputRuntimePawnClientRestartPreservesStateTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Input runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Input runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->bClimbingIMCActive = true;
	Character->bLocomotionIMCActive = true;

	Character->PawnClientRestart();

	TestEqual(TEXT("Input runtime: PawnClientRestart should preserve current climbing state"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);
	TestFalse(TEXT("Input runtime: climbing IMC flag should be cleared on restart in non-local automation context"),
		Character->bClimbingIMCActive);
	TestFalse(TEXT("Input runtime: locomotion IMC flag should be cleared on restart in non-local automation context"),
		Character->bLocomotionIMCActive);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
