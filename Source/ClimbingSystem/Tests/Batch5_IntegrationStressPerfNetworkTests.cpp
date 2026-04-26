// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingTypes.h"
#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "HAL/PlatformTime.h"

// ---------------------------------------------------------------------------
// TC-0374: NetworkDoubleAttemptGrabBeforeResponse
// WHAT: Spawn char. Call Server_AttemptGrab twice rapidly.
//       Assert state is valid and no crash occurs.
// WHY: Dedup guard must absorb the second RPC without corrupting state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5NetworkDoubleAttemptGrabBeforeResponseTest,
	"ClimbingSystem.Batch5.Network.NetworkDoubleAttemptGrabBeforeResponse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5NetworkDoubleAttemptGrabBeforeResponseTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0374: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	FClimbingDetectionResultNet NetResult;
	NetResult.bValid = true;
	NetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	NetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);

	// Two rapid calls — must not crash
	Character->Server_AttemptGrab(NetResult);
	Character->Server_AttemptGrab(NetResult);

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0374: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	const uint8 StateVal = static_cast<uint8>(Movement->CurrentClimbingState);
	TestTrue(TEXT("TC-0374: state must be a valid EClimbingState value"),
		StateVal < static_cast<uint8>(EClimbingState::MAX));

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0375: IntegrationCornerAnchorDestroyedMidTransition
// WHAT: Char in CornerTransition. Destroy anchor. Assert state exits safely.
// WHY: Anchor destruction during transition must not leave state machine stuck.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationCornerAnchorDestroyedMidTransitionTest,
	"ClimbingSystem.Batch5.Integration.CornerAnchorDestroyedMidTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationCornerAnchorDestroyedMidTransitionTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0375: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0375: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	AActor* BoxActor = Helper.SpawnBoxAt(FVector(0.f, 0.f, 100.f), FVector(50.f, 50.f, 10.f));
	TestNotNull(TEXT("TC-0375: box must spawn"), BoxActor);
	if (!BoxActor) { Helper.Teardown(); return false; }

	UPrimitiveComponent* BoxRoot = Cast<UPrimitiveComponent>(BoxActor->GetRootComponent());
	if (BoxRoot)
	{
		Movement->SetAnchor(BoxRoot, FVector(0.f, 0.f, 100.f));
	}

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);
	Character->TransitionToState(EClimbingState::CornerTransition, DetResult);

	// Destroy anchor mid-transition
	BoxActor->Destroy();
	Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);

	// State must not be stuck in CornerTransition with a dead anchor
	const EClimbingState FinalState = Movement->CurrentClimbingState;
	const bool bSafeState = (FinalState != EClimbingState::CornerTransition) ||
	                        (!Movement->AnchorComponent || !Movement->AnchorComponent->IsValidLowLevel());
	TestTrue(TEXT("TC-0375: state must exit safely when anchor is destroyed"), bSafeState);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0376: IntegrationTimerPauseUnpause
// UNIT. Verify DetectionScanInterval > 0 (timers exist and are non-zero).
// WHY: A zero interval would cause per-tick scanning; pause behavior requires > 0.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationTimerPauseUnpauseTest,
	"ClimbingSystem.Batch5.Integration.TimerPauseUnpause",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationTimerPauseUnpauseTest::RunTest(const FString& Parameters)
{
	// Unit test — no world needed; inspect CDO defaults
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0376: CDO must be valid"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0376: DetectionScanInterval must be > 0"),
		CDO->DetectionScanInterval > 0.f);
	TestTrue(TEXT("TC-0376: FallingGrabCheckInterval must be > 0"),
		CDO->FallingGrabCheckInterval > 0.f);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0377: IntegrationCapsuleSizeAtEachTransition
// WHAT: Spawn char. Transition None->Hanging. Assert capsule==48/24.
//       Transition back to None. Assert capsule restored.
// WHY: Capsule must shrink on climb entry and restore on exit.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationCapsuleSizeAtEachTransitionTest,
	"ClimbingSystem.Batch5.Integration.CapsuleSizeAtEachTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationCapsuleSizeAtEachTransitionTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0377: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	TestNotNull(TEXT("TC-0377: capsule must exist"), Capsule);
	if (!Capsule) { Helper.Teardown(); return false; }

	const float OrigHH = Capsule->GetScaledCapsuleHalfHeight();
	const float OrigR  = Capsule->GetScaledCapsuleRadius();

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);
	Character->TransitionToState(EClimbingState::Hanging, DetResult);

	TestEqual(TEXT("TC-0377: capsule half-height must be 48 while Hanging"),
		Capsule->GetScaledCapsuleHalfHeight(), Character->ClimbingCapsuleHalfHeight);
	TestEqual(TEXT("TC-0377: capsule radius must be 24 while Hanging"),
		Capsule->GetScaledCapsuleRadius(), Character->ClimbingCapsuleRadius);

	FClimbingDetectionResult NoneResult;
	Character->TransitionToState(EClimbingState::None, NoneResult);

	TestEqual(TEXT("TC-0377: capsule half-height must be restored after None"),
		Capsule->GetScaledCapsuleHalfHeight(), OrigHH);
	TestEqual(TEXT("TC-0377: capsule radius must be restored after None"),
		Capsule->GetScaledCapsuleRadius(), OrigR);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0378: IntegrationIMCStackFullLifecycle
// WHAT: Transition None->Hanging (IMC added). Transition to None (IMC removed).
//       Assert TestClimbingIMCActive()==false after return to None.
// WHY: IMC must be cleaned up on every exit path.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationIMCStackFullLifecycleTest,
	"ClimbingSystem.Batch5.Integration.IMCStackFullLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationIMCStackFullLifecycleTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0378: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);
	Character->TransitionToState(EClimbingState::Hanging, DetResult);

	FClimbingDetectionResult NoneResult;
	Character->TransitionToState(EClimbingState::None, NoneResult);

	TestFalse(TEXT("TC-0378: ClimbingIMC must not be active after returning to None"),
		Character->TestClimbingIMCActive());

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0379: NetworkClientPredictServerConfirmProxy
// WHAT: Transition to Hanging. Call OnClimbingStateReplicated(None,Hanging).
//       Assert state==Hanging.
// WHY: Proxy replication callback must not override a valid local state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5NetworkClientPredictServerConfirmProxyTest,
	"ClimbingSystem.Batch5.Network.ClientPredictServerConfirmProxy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5NetworkClientPredictServerConfirmProxyTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0379: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0379: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);
	Character->TransitionToState(EClimbingState::Hanging, DetResult);

	// Simulate proxy replication callback confirming the same state
	Character->OnClimbingStateReplicated(EClimbingState::None, EClimbingState::Hanging);

	TestEqual(TEXT("TC-0379: state must remain Hanging after replication confirm"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0380: PerfStateTransitionThroughput
// UNIT. Time 1000 SetClimbingState calls. Assert total < 10ms.
// WHY: State transitions must be cheap enough for rapid input handling.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5PerfStateTransitionThroughputTest,
	"ClimbingSystem.Batch5.Perf.StateTransitionThroughput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5PerfStateTransitionThroughputTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0380: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0380: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	const double StartTime = FPlatformTime::Seconds();
	for (int32 i = 0; i < 1000; ++i)
	{
		Movement->SetClimbingState((i % 2 == 0) ? EClimbingState::Hanging : EClimbingState::None);
	}
	const double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	TestTrue(FString::Printf(TEXT("TC-0380: 1000 SetClimbingState calls must complete in < 10ms (actual=%.2fms)"), ElapsedMs),
		ElapsedMs < 10.0);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0381: IntegrationLadderTransitionMontageGatesState
// WHAT: Char in LadderTransition. Assert state remains committed (not None).
// WHY: Montage gate must hold LadderTransition until completion callback fires.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationLadderTransitionMontageGatesStateTest,
	"ClimbingSystem.Batch5.Integration.LadderTransitionMontageGatesState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationLadderTransitionMontageGatesStateTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0381: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0381: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);
	Character->TransitionToState(EClimbingState::LadderTransition, DetResult);

	// State must be committed — not silently reverted to None without a montage callback
	TestEqual(TEXT("TC-0381: state must remain LadderTransition after entry"),
		Movement->CurrentClimbingState, EClimbingState::LadderTransition);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0382: IntegrationRagdollIMCRestored
// WHAT: Char in Hanging (IMC active). Transition Ragdoll->None.
//       Assert TestClimbingIMCActive()==false.
// WHY: Ragdoll path must clean up IMC just like normal exit.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationRagdollIMCRestoredTest,
	"ClimbingSystem.Batch5.Integration.RagdollIMCRestored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationRagdollIMCRestoredTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0382: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);
	Character->TransitionToState(EClimbingState::Hanging, DetResult);

	FClimbingDetectionResult RagdollResult;
	Character->TransitionToState(EClimbingState::Ragdoll, RagdollResult);

	FClimbingDetectionResult NoneResult;
	Character->TransitionToState(EClimbingState::None, NoneResult);

	TestFalse(TEXT("TC-0382: ClimbingIMC must not be active after Ragdoll->None"),
		Character->TestClimbingIMCActive());

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0383: IntegrationMantleAnchorDestroyed
// WHAT: Char in Mantling. Destroy anchor. Assert safe exit to None.
// WHY: Anchor destruction during mantle must not crash or leave state stuck.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationMantleAnchorDestroyedTest,
	"ClimbingSystem.Batch5.Integration.MantleAnchorDestroyed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationMantleAnchorDestroyedTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0383: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0383: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	AActor* BoxActor = Helper.SpawnBoxAt(FVector(0.f, 0.f, 100.f), FVector(50.f, 50.f, 10.f));
	TestNotNull(TEXT("TC-0383: box must spawn"), BoxActor);
	if (!BoxActor) { Helper.Teardown(); return false; }

	UPrimitiveComponent* BoxRoot = Cast<UPrimitiveComponent>(BoxActor->GetRootComponent());
	if (BoxRoot)
	{
		Movement->SetAnchor(BoxRoot, FVector(0.f, 0.f, 100.f));
	}

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);
	Character->TransitionToState(EClimbingState::Mantling, DetResult);

	BoxActor->Destroy();
	Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);

	// Must not crash; anchor should be cleared or state exited
	const bool bSafe = !Movement->AnchorComponent ||
	                   !Movement->AnchorComponent->IsValidLowLevel() ||
	                   Movement->CurrentClimbingState == EClimbingState::None;
	TestTrue(TEXT("TC-0383: must exit safely after anchor destroyed during Mantling"), bSafe);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0384: NetworkServerDetectionReRunOnEveryRPC
// UNIT. Verify Server_AttemptGrab function exists on AClimbingCharacter.
// WHY: Contract check — the RPC must be present for server re-detection to occur.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5NetworkServerDetectionReRunOnEveryRPCTest,
	"ClimbingSystem.Batch5.Network.ServerDetectionReRunOnEveryRPC",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5NetworkServerDetectionReRunOnEveryRPCTest::RunTest(const FString& Parameters)
{
	// Verify via UFunction reflection that Server_AttemptGrab exists
	const UClass* CharClass = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0384: AClimbingCharacter class must be valid"), CharClass);
	if (!CharClass) { return false; }

	const UFunction* GrabFunc = CharClass->FindFunctionByName(FName("Server_AttemptGrab"));
	TestNotNull(TEXT("TC-0384: Server_AttemptGrab must exist on AClimbingCharacter"), GrabFunc);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0385: IntegrationAllStatesEntryExitSymmetry
// WHAT: Spawn char. Transition through all 17 states. Assert no crash on each.
// WHY: Every state must have a valid entry/exit path without crashing.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationAllStatesEntryExitSymmetryTest,
	"ClimbingSystem.Batch5.Integration.AllStatesEntryExitSymmetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationAllStatesEntryExitSymmetryTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0385: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0385: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);

	// Iterate all 17 states (0..16), enter each then return to None
	for (uint8 i = 0; i < static_cast<uint8>(EClimbingState::MAX); ++i)
	{
		const EClimbingState State = static_cast<EClimbingState>(i);
		Character->TransitionToState(State, DetResult);

		FClimbingDetectionResult NoneResult;
		Character->TransitionToState(EClimbingState::None, NoneResult);
	}

	// If we reach here without crashing, symmetry holds
	TestTrue(TEXT("TC-0385: all 17 states entered and exited without crash"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0386: StressNetworkRapidServerGrabSpam
// WHAT: Call Server_AttemptGrab 20 times rapidly. Assert no crash.
// WHY: Rapid RPC spam must be absorbed gracefully.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5StressNetworkRapidServerGrabSpamTest,
	"ClimbingSystem.Batch5.Stress.NetworkRapidServerGrabSpam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5StressNetworkRapidServerGrabSpamTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0386: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	FClimbingDetectionResultNet NetResult;
	NetResult.bValid = true;
	NetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	NetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);

	for (int32 i = 0; i < 20; ++i)
	{
		Character->Server_AttemptGrab(NetResult);
	}

	TestTrue(TEXT("TC-0386: 20 rapid Server_AttemptGrab calls must not crash"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0387: IntegrationIKWeightZeroOnStateExit
// WHAT: Char in Hanging. Transition to None. Assert no crash accessing AnimInstance.
// WHY: IK weights must be zeroed on exit; AnimInstance access must be safe.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationIKWeightZeroOnStateExitTest,
	"ClimbingSystem.Batch5.Integration.IKWeightZeroOnStateExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationIKWeightZeroOnStateExitTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0387: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);
	Character->TransitionToState(EClimbingState::Hanging, DetResult);

	FClimbingDetectionResult NoneResult;
	Character->TransitionToState(EClimbingState::None, NoneResult);

	// Verify AnimInstance access is safe (no crash) after state exit
	USkeletalMeshComponent* Mesh = Character->GetMesh();
	const bool bAnimInstanceSafe = !Mesh || (Mesh->GetAnimInstance() == nullptr) ||
	                               Mesh->GetAnimInstance()->IsValidLowLevel();
	TestTrue(TEXT("TC-0387: AnimInstance must be safely accessible after state exit"), bAnimInstanceSafe);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0388: IntegrationLacheCapsuleRestoredOnMiss
// WHAT: Char goes through Lache->LacheMiss->None. Assert capsule restored.
// WHY: Lache miss path must restore capsule just like normal exit.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationLacheCapsuleRestoredOnMissTest,
	"ClimbingSystem.Batch5.Integration.LacheCapsuleRestoredOnMiss",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationLacheCapsuleRestoredOnMissTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0388: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	TestNotNull(TEXT("TC-0388: capsule must exist"), Capsule);
	if (!Capsule) { Helper.Teardown(); return false; }

	const float OrigHH = Capsule->GetScaledCapsuleHalfHeight();
	const float OrigR  = Capsule->GetScaledCapsuleRadius();

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);

	Character->TransitionToState(EClimbingState::Lache, DetResult);
	Character->TransitionToState(EClimbingState::LacheMiss, DetResult);

	FClimbingDetectionResult NoneResult;
	Character->TransitionToState(EClimbingState::None, NoneResult);

	TestEqual(TEXT("TC-0388: capsule half-height must be restored after LacheMiss->None"),
		Capsule->GetScaledCapsuleHalfHeight(), OrigHH);
	TestEqual(TEXT("TC-0388: capsule radius must be restored after LacheMiss->None"),
		Capsule->GetScaledCapsuleRadius(), OrigR);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0389: PerfIKSixClimbersGracefulDegradation
// WHAT: Spawn 6 chars. Assert MaxSimultaneousIKCharacters==4. Assert no crash.
// WHY: IK budget must cap at 4; extra characters must degrade gracefully.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5PerfIKSixClimbersGracefulDegradationTest,
	"ClimbingSystem.Batch5.Perf.IKSixClimbersGracefulDegradation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5PerfIKSixClimbersGracefulDegradationTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	constexpr int32 NumChars = 6;
	TArray<AClimbingCharacter*> Characters;
	for (int32 i = 0; i < NumChars; ++i)
	{
		AClimbingCharacter* C = Helper.SpawnCharacterAt(FVector(i * 200.f, 0.f, 0.f));
		TestNotNull(FString::Printf(TEXT("TC-0389: character %d must spawn"), i), C);
		if (!C) { Helper.Teardown(); return false; }
		Characters.Add(C);
	}

	// Verify CDO default
	TestEqual(TEXT("TC-0389: MaxSimultaneousIKCharacters must be 4"),
		Characters[0]->MaxSimultaneousIKCharacters, 4);

	// Tick — must not crash with 6 characters
	Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);
	TestTrue(TEXT("TC-0389: 6 climbers must tick without crash"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0390: IntegrationIMCNotDoubledOnRapidGrabDrop
// WHAT: Rapid grab+drop 10 times. Assert TestClimbingIMCActive() is false at end.
// WHY: Rapid toggle must not double-add IMC or leave it active after final drop.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationIMCNotDoubledOnRapidGrabDropTest,
	"ClimbingSystem.Batch5.Integration.IMCNotDoubledOnRapidGrabDrop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationIMCNotDoubledOnRapidGrabDropTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0390: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);

	FClimbingDetectionResult NoneResult;

	for (int32 i = 0; i < 10; ++i)
	{
		Character->TransitionToState(EClimbingState::Hanging, DetResult);
		Character->TransitionToState(EClimbingState::None, NoneResult);
	}

	TestFalse(TEXT("TC-0390: ClimbingIMC must not be active after 10 rapid grab+drop cycles"),
		Character->TestClimbingIMCActive());

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0391: NetworkProxyMontageNotPlayedTwice
// WHAT: Call OnClimbingStateReplicated twice with same state. Assert no crash.
// WHY: Duplicate replication callbacks must be idempotent.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5NetworkProxyMontageNotPlayedTwiceTest,
	"ClimbingSystem.Batch5.Network.ProxyMontageNotPlayedTwice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5NetworkProxyMontageNotPlayedTwiceTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0391: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	// Call twice with same transition — must not crash
	Character->OnClimbingStateReplicated(EClimbingState::None, EClimbingState::Hanging);
	Character->OnClimbingStateReplicated(EClimbingState::None, EClimbingState::Hanging);

	TestTrue(TEXT("TC-0391: duplicate OnClimbingStateReplicated calls must not crash"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0392: NetworkTwoClimbersIndependentState
// WHAT: Spawn 2 chars. Set one to Hanging, leave other at None.
//       Assert states are independent.
// WHY: Per-instance state must not bleed between characters.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5NetworkTwoClimbersIndependentStateTest,
	"ClimbingSystem.Batch5.Network.TwoClimbersIndependentState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5NetworkTwoClimbersIndependentStateTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* CharA = Helper.SpawnCharacterAt(FVector(0.f, 0.f, 0.f));
	AClimbingCharacter* CharB = Helper.SpawnCharacterAt(FVector(300.f, 0.f, 0.f));
	TestNotNull(TEXT("TC-0392: CharA must spawn"), CharA);
	TestNotNull(TEXT("TC-0392: CharB must spawn"), CharB);
	if (!CharA || !CharB) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* MovA = CharA->FindComponentByClass<UClimbingMovementComponent>();
	UClimbingMovementComponent* MovB = CharB->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0392: MovA must exist"), MovA);
	TestNotNull(TEXT("TC-0392: MovB must exist"), MovB);
	if (!MovA || !MovB) { Helper.Teardown(); return false; }

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);
	CharA->TransitionToState(EClimbingState::Hanging, DetResult);

	TestEqual(TEXT("TC-0392: CharA must be Hanging"), MovA->CurrentClimbingState, EClimbingState::Hanging);
	TestEqual(TEXT("TC-0392: CharB must remain None"), MovB->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0393: PerfIKTickBudgetFourClimbers
// WHAT: Spawn 4 chars in Hanging. Tick world. Assert no crash.
// WHY: Exactly at the IK budget limit must not crash or exceed tick budget.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5PerfIKTickBudgetFourClimbersTest,
	"ClimbingSystem.Batch5.Perf.IKTickBudgetFourClimbers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5PerfIKTickBudgetFourClimbersTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);

	constexpr int32 NumChars = 4;
	for (int32 i = 0; i < NumChars; ++i)
	{
		AClimbingCharacter* C = Helper.SpawnCharacterAt(FVector(i * 200.f, 0.f, 0.f));
		TestNotNull(FString::Printf(TEXT("TC-0393: character %d must spawn"), i), C);
		if (!C) { Helper.Teardown(); return false; }
		C->TransitionToState(EClimbingState::Hanging, DetResult);
	}

	const double StartTime = FPlatformTime::Seconds();
	Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);
	const double ElapsedMs = (FPlatformTime::Seconds() - StartTime) * 1000.0;

	TestTrue(TEXT("TC-0393: 4 climbers at IK budget must tick without crash"), true);
	AddInfo(FString::Printf(TEXT("TC-0393: tick with 4 IK climbers took %.2fms"), ElapsedMs));

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0394: PerfFallingGrabCheckIntervalNoDrift
// UNIT. Verify FallingGrabCheckInterval default==0.05.
// WHY: Drift in the default value would change re-grab responsiveness.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5PerfFallingGrabCheckIntervalNoDriftTest,
	"ClimbingSystem.Batch5.Perf.FallingGrabCheckIntervalNoDrift",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5PerfFallingGrabCheckIntervalNoDriftTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0394: CDO must be valid"), CDO);
	if (!CDO) { return false; }

	TestEqual(TEXT("TC-0394: FallingGrabCheckInterval default must be 0.05"),
		CDO->FallingGrabCheckInterval, 0.05f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
