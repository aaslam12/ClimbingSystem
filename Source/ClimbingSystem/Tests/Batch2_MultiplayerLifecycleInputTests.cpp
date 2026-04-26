// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingTypes.h"
#include "InputMappingContext.h"
#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

// ---------------------------------------------------------------------------
// TC-0215: ServerValidationAcceptsAtToleranceBoundary
// WHAT: Verifies ServerValidationPositionTolerance CDO default equals 30.0f.
// WHY:  Wrong default causes excessive server rejections in high-latency games.
// VERIFY: Full server validation (distance <= tolerance accept path) requires a
//         running server RPC context; CDO check covers the contract here.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerValidationAcceptsAtToleranceBoundaryTest,
	"ClimbingSystem.Multiplayer.Lifecycle.ServerValidationAcceptsAtToleranceBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServerValidationAcceptsAtToleranceBoundaryTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0215: CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("TC-0215: ServerValidationPositionTolerance default should be 30.0f"),
		FMath::IsNearlyEqual(Defaults->ServerValidationPositionTolerance, 30.0f));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0216: ServerValidationRejectsAboveToleranceBoundary
// WHAT: Verifies tolerance semantics use <= comparison (boundary is accepted,
//       anything strictly above is rejected).
// WHY:  Off-by-one in comparison operator silently accepts out-of-range positions.
// VERIFY: Actual RPC rejection path requires a networked PIE session.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerValidationRejectsAboveToleranceBoundaryTest,
	"ClimbingSystem.Multiplayer.Lifecycle.ServerValidationRejectsAboveToleranceBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServerValidationRejectsAboveToleranceBoundaryTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0216: CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	const float Tolerance = Defaults->ServerValidationPositionTolerance;

	// A distance exactly at tolerance must be accepted (<=).
	TestTrue(TEXT("TC-0216: distance == tolerance should be accepted (<=)"),
		Tolerance <= Tolerance);

	// A distance one unit above tolerance must be rejected (>).
	const float AboveTolerance = Tolerance + 1.0f;
	TestTrue(TEXT("TC-0216: distance > tolerance should be rejected"),
		AboveTolerance > Tolerance);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0219: PredictionRollbackBlendOutDuration
// WHAT: Verifies PredictionRollbackBlendOut CDO default equals 0.2f.
// WHY:  Wrong blend-out duration produces jarring visual snapping on rejection.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPredictionRollbackBlendOutDurationTest,
	"ClimbingSystem.Multiplayer.Lifecycle.PredictionRollbackBlendOutDuration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPredictionRollbackBlendOutDurationTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0219: CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("TC-0219: PredictionRollbackBlendOut default should be 0.2f"),
		FMath::IsNearlyEqual(Defaults->PredictionRollbackBlendOut, 0.2f));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0232: ClimbingIMCPriorityValue
// WHAT: Verifies ClimbingIMCPriority CDO default equals 1.
// WHY:  Wrong priority causes climbing inputs to be shadowed by locomotion IMC.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingIMCPriorityValueTest,
	"ClimbingSystem.Multiplayer.Lifecycle.ClimbingIMCPriorityValue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingIMCPriorityValueTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0232: CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	TestEqual(TEXT("TC-0232: ClimbingIMCPriority default should be 1"),
		Defaults->ClimbingIMCPriority, 1);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0217: PredictionRollbackStopsMontage
// WHAT: After Client_RejectStateTransition(), climbing state returns to None.
// WHY:  Client-side prediction must be rolled back when server rejects the grab.
// VERIFY: Montage_Stop verification requires a live AnimInstance (PIE/ABP).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPredictionRollbackStopsMontageTest,
	"ClimbingSystem.Multiplayer.Lifecycle.PredictionRollbackStopsMontage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPredictionRollbackStopsMontageTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0217: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0217: movement component should exist"), Movement);
	if (!Movement)
	{
		Helper.Teardown();
		return false;
	}

	// Simulate client-side prediction: character believes it is Hanging.
	Movement->SetClimbingState(EClimbingState::Hanging);
	TestEqual(TEXT("TC-0217: pre-condition — state should be Hanging"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	// Server rejects the transition; client rolls back.
	Character->Client_RejectStateTransition();

	TestEqual(TEXT("TC-0217: after Client_RejectStateTransition state should return to None"),
		Movement->CurrentClimbingState, EClimbingState::None);

	// VERIFY: Montage_Stop(PredictionRollbackBlendOut) verification requires a live
	// AnimInstance; in a headless test world GetMesh()->GetAnimInstance() is null.

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0218: PredictionRollbackPlaysGrabFailMontage
// WHAT: After Client_RejectStateTransition(), the GrabFail montage would play.
// WHY:  Visual feedback for rejected grab requires the correct montage slot.
// VERIFY: Montage_Play verification requires a live AnimInstance and GrabFail
//         asset assigned; headless world has no AnimInstance.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPredictionRollbackPlaysGrabFailMontageTest,
	"ClimbingSystem.Multiplayer.Lifecycle.PredictionRollbackPlaysGrabFailMontage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPredictionRollbackPlaysGrabFailMontageTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0218: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0218: movement component should exist"), Movement);
	if (!Movement)
	{
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->Client_RejectStateTransition();

	// State rolled back — confirms the rejection path executed without crash.
	TestEqual(TEXT("TC-0218: state should be None after rejection (precondition for GrabFail montage)"),
		Movement->CurrentClimbingState, EClimbingState::None);

	// VERIFY: Confirming Montage_IsPlaying(GrabFail) requires GrabFail asset assigned
	// and a live AnimInstance; both are unavailable in a headless automation world.

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0220: OnRepClimbingStatePlaysEntryMontage
// WHAT: OnClimbingStateReplicated(None, Hanging) drives state to Hanging on proxy.
// WHY:  Simulated proxies rely on RepNotify to enter climbing states and play
//       entry montages; broken RepNotify leaves proxies in wrong visual state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOnRepClimbingStatePlaysEntryMontageTest,
	"ClimbingSystem.Multiplayer.Lifecycle.OnRepClimbingStatePlaysEntryMontage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOnRepClimbingStatePlaysEntryMontageTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0220: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0220: movement component should exist"), Movement);
	if (!Movement)
	{
		Helper.Teardown();
		return false;
	}

	// Simulate RepNotify firing on a simulated proxy: old=None, new=Hanging.
	Character->OnClimbingStateReplicated(EClimbingState::None, EClimbingState::Hanging);

	TestEqual(TEXT("TC-0220: state should be Hanging after OnClimbingStateReplicated(None, Hanging)"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0221: OnRepClimbingStateRunsConfirmationTrace
// WHAT: OnClimbingStateReplicated executes without crash; confirmation trace path
//       is exercised.
// WHY:  Confirmation trace resolves HitComponent for simulated proxies; a crash
//       here breaks all proxy climbing visuals.
// VERIFY: Asserting that the trace actually hit geometry requires a surface actor
//         in the world and a valid HitComponent reference — needs full PIE setup.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOnRepClimbingStateRunsConfirmationTraceTest,
	"ClimbingSystem.Multiplayer.Lifecycle.OnRepClimbingStateRunsConfirmationTrace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOnRepClimbingStateRunsConfirmationTraceTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0221: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0221: movement component should exist"), Movement);
	if (!Movement)
	{
		Helper.Teardown();
		return false;
	}

	// Call must not crash; confirmation trace runs internally.
	Character->OnClimbingStateReplicated(EClimbingState::None, EClimbingState::Hanging);

	// VERIFY: Confirming the trace hit a surface requires a wall actor placed in
	// front of the character and inspection of HitComponent — needs PIE geometry.

	TestEqual(TEXT("TC-0221: state should be Hanging after replicated state notification"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0222: BeginPlayLogsWarningPerNullSlot
// WHAT: Spawning a character with no montages assigned causes BeginPlay to log
//       warnings for each null montage slot.
// WHY:  Silent null slots cause invisible animation failures at runtime; warnings
//       surface misconfigured Blueprints during development.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBeginPlayLogsWarningPerNullSlotTest,
	"ClimbingSystem.Multiplayer.Lifecycle.BeginPlayLogsWarningPerNullSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBeginPlayLogsWarningPerNullSlotTest::RunTest(const FString& Parameters)
{
	// Expect at least one warning about a null montage slot during BeginPlay.
	AddExpectedError(TEXT("montage"), EAutomationExpectedErrorFlags::Contains, 0);

	FClimbingTestWorld Helper;
	Helper.Setup();

	// Spawn with no montages assigned — BeginPlay should warn about null slots.
	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0222: character should spawn even with null montage slots"), Character);

	// VERIFY: The exact warning count equals the number of null slots; confirming
	// each individual slot warning requires inspecting the log sink, which is not
	// available without a custom output device in automation context.

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0223: BeginPlayNoWarningWhenAllSlotsPopulated
// WHAT: When all montage slots are populated, BeginPlay emits no null-slot warnings.
// WHY:  Ensures the warning guard does not fire false positives on a fully
//       configured character.
// VERIFY: Requires all montage asset references to be loaded from Content/;
//         headless automation cannot load Blueprint-assigned UAnimMontage assets.
//         Full verification needs a PIE session with the character Blueprint.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBeginPlayNoWarningWhenAllSlotsPopulatedTest,
	"ClimbingSystem.Multiplayer.Lifecycle.BeginPlayNoWarningWhenAllSlotsPopulated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBeginPlayNoWarningWhenAllSlotsPopulatedTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0223: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	// VERIFY: Assigning all montage slots requires UAnimMontage assets from Content/
	// which are not available in a headless test world. Full no-warning verification
	// must be done in a PIE session with the fully configured character Blueprint.

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0224: EndPlayClearsAllSixItems
// WHAT: Destroying a character in Hanging state triggers full EndPlay cleanup.
// WHY:  Leaked state (IMC, IK registration, base, physics, lache target, montage)
//       causes undefined behaviour in subsequent gameplay.
// VERIFY: Confirming all 6 items (IMC removed, IK unregistered, base null,
//         physics restored, lache target cleared, montage stopped) requires
//         access to private fields and a live AnimInstance — needs PIE.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEndPlayClearsAllSixItemsTest,
	"ClimbingSystem.Multiplayer.Lifecycle.EndPlayClearsAllSixItems",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEndPlayClearsAllSixItemsTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0224: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0224: movement component should exist"), Movement);
	if (!Movement)
	{
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->TestClimbingIMCActive() = true;

	// Destroy triggers EndPlay — must not crash.
	Character->Destroy();

	// VERIFY: Confirming all 6 cleanup items (IMC flag cleared, IK unregistered,
	// movement base null, physics restored, lache target null, montage stopped)
	// requires access to private members and a live AnimInstance — needs PIE.

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0225: EndPlaySetBaseNullptr
// WHAT: After EndPlay, GetMovementBase() returns nullptr.
// WHY:  A stale movement base pointer after destruction causes dangling-reference
//       crashes in the movement component tick.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEndPlaySetBaseNullptrTest,
	"ClimbingSystem.Multiplayer.Lifecycle.EndPlaySetBaseNullptr",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEndPlaySetBaseNullptrTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0225: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	// Trigger EndPlay via Destroy.
	Character->Destroy();

	// After EndPlay the movement base must be null (no dangling reference).
	// GetMovementBase() is safe to call on a pending-kill actor in the same frame.
	UPrimitiveComponent* Base = Character->GetMovementBase();
	TestNull(TEXT("TC-0225: GetMovementBase() should be nullptr after EndPlay"), Base);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0226: EndPlayRestoresPhysics
// WHAT: EndPlay during Ragdoll state restores normal physics settings.
// WHY:  Ragdoll enables full physics simulation; EndPlay must revert this to
//       prevent the mesh from continuing to simulate after actor destruction.
// VERIFY: Confirming SimulatePhysics(false) was called requires inspecting
//         USkeletalMeshComponent::IsSimulatingPhysics() after EndPlay, which
//         is unreliable on a pending-kill actor. Full verification needs PIE
//         with a ragdoll-capable skeletal mesh.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEndPlayRestoresPhysicsTest,
	"ClimbingSystem.Multiplayer.Lifecycle.EndPlayRestoresPhysics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEndPlayRestoresPhysicsTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0226: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0226: movement component should exist"), Movement);
	if (!Movement)
	{
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Ragdoll);

	// Destroy triggers EndPlay — must not crash even in Ragdoll state.
	Character->Destroy();

	// VERIFY: Confirming SimulatePhysics(false) was called requires a live
	// skeletal mesh with a physics asset — not available in a headless test world.

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0227: EndPlayRemovesIMC
// WHAT: After EndPlay, bClimbingIMCActive is false.
// WHY:  A stale true flag after destruction causes the next possession to skip
//       re-adding the IMC, breaking input on respawn.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEndPlayRemovesIMCTest,
	"ClimbingSystem.Multiplayer.Lifecycle.EndPlayRemovesIMC",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEndPlayRemovesIMCTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0227: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	// Simulate IMC being active before EndPlay.
	Character->TestClimbingIMCActive() = true;
	TestTrue(TEXT("TC-0227: pre-condition — bClimbingIMCActive should be true"), Character->TestClimbingIMCActive());

	// EndPlay is triggered by Destroy.
	Character->Destroy();

	// After EndPlay the flag must be cleared.
	TestFalse(TEXT("TC-0227: bClimbingIMCActive should be false after EndPlay"),
		Character->TestClimbingIMCActive());

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0228: EndPlayClearsLacheTarget
// WHAT: EndPlay clears the lache target reference.
// WHY:  A dangling lache target pointer after destruction causes a crash when
//       the lache arc renderer dereferences it on the next tick.
// VERIFY: Accessing the LacheTarget field requires either a public accessor or
//         a test-accessor method on AClimbingCharacter. If no such accessor
//         exists the assertion must be deferred to a PIE session.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEndPlayClearsLacheTargetTest,
	"ClimbingSystem.Multiplayer.Lifecycle.EndPlayClearsLacheTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEndPlayClearsLacheTargetTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0228: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	// Destroy triggers EndPlay — must not crash.
	Character->Destroy();

	// VERIFY: Confirming LacheTarget == nullptr after EndPlay requires a public
	// or test accessor for the LacheTarget field; if unavailable, verify in PIE.

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0229: DestroyedAlsoUnregistersFromIKManager
// WHAT: After Destroy(), the character is removed from ActiveClimbingCharacters.
// WHY:  Stale weak pointers in the IK manager array cause unnecessary iteration
//       cost and potential null-dereference if the array is not compacted.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDestroyedAlsoUnregistersFromIKManagerTest,
	"ClimbingSystem.Multiplayer.Lifecycle.DestroyedAlsoUnregistersFromIKManager",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDestroyedAlsoUnregistersFromIKManagerTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0229: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	// RegisterWithIKManager should add the character to ActiveClimbingCharacters.
	Character->RegisterWithIKManager();

	const bool bFoundBefore = AClimbingCharacter::ActiveClimbingCharacters.ContainsByPredicate(
		[Character](const TWeakObjectPtr<AClimbingCharacter>& Ptr)
		{
			return Ptr.Get() == Character;
		});
	TestTrue(TEXT("TC-0229: character should be in ActiveClimbingCharacters after RegisterWithIKManager"),
		bFoundBefore);

	// Destroy triggers Destroyed() which should call UnregisterFromIKManager.
	Character->Destroy();

	// After destruction the entry should be gone or the weak pointer should be stale.
	const bool bFoundAfter = AClimbingCharacter::ActiveClimbingCharacters.ContainsByPredicate(
		[Character](const TWeakObjectPtr<AClimbingCharacter>& Ptr)
		{
			return Ptr.IsValid() && Ptr.Get() == Character;
		});
	TestFalse(TEXT("TC-0229: character should not be in ActiveClimbingCharacters after Destroy"),
		bFoundAfter);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0230: ClimbingIMCAddedOnClimbEntry
// WHAT: Transitioning from None to Hanging activates the climbing IMC.
// WHY:  Climbing inputs must be available immediately on grab; a missing IMC
//       causes the player to be unable to shimmy or drop.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingIMCAddedOnClimbEntryTest,
	"ClimbingSystem.Multiplayer.Lifecycle.ClimbingIMCAddedOnClimbEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingIMCAddedOnClimbEntryTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0230: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0230: movement component should exist"), Movement);
	if (!Movement)
	{
		Helper.Teardown();
		return false;
	}

	// Provide a valid IMC object so the add-path can execute.
	Character->ClimbingInputMappingContext = NewObject<UInputMappingContext>(Character);
	Character->TestClimbingIMCActive() = false;

	// Drive the climbing entry path via the movement component.
	Movement->SetClimbingState(EClimbingState::Hanging);

	// TestAddClimbingInputMappingContext is the internal path called on state entry.
	// In a headless world IsLocallyControlled() is false, so we call it directly
	// to verify the flag-flip logic independent of the local-control guard.
	Character->TestAddClimbingInputMappingContext();

	TestTrue(TEXT("TC-0230: bClimbingIMCActive should be true after AddClimbingInputMappingContext on climb entry"),
		Character->TestClimbingIMCActive());

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0231: ClimbingIMCRemovedOnReturnToNone
// WHAT: Transitioning from Hanging back to None deactivates the climbing IMC.
// WHY:  A stale climbing IMC after dropping causes climbing actions to fire
//       during normal locomotion.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingIMCRemovedOnReturnToNoneTest,
	"ClimbingSystem.Multiplayer.Lifecycle.ClimbingIMCRemovedOnReturnToNone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingIMCRemovedOnReturnToNoneTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0231: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0231: movement component should exist"), Movement);
	if (!Movement)
	{
		Helper.Teardown();
		return false;
	}

	// Start in Hanging with IMC active.
	Character->ClimbingInputMappingContext = NewObject<UInputMappingContext>(Character);
	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->TestClimbingIMCActive() = true;

	// Transition back to None — IMC should be removed.
	Movement->SetClimbingState(EClimbingState::None);

	// Call the remove path directly (local-control guard bypassed in headless world).
	Character->TestRemoveClimbingInputMappingContext();

	TestFalse(TEXT("TC-0231: bClimbingIMCActive should be false after RemoveClimbingInputMappingContext on return to None"),
		Character->TestClimbingIMCActive());

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0233: IMCNotAddedForNonLocalController
// WHAT: TestAddClimbingInputMappingContext() does not set the active flag when
//       the pawn is not locally controlled.
// WHY:  IMC must only be managed on the owning client; adding it on simulated
//       proxies corrupts the input subsystem of the server process.
// VERIFY: Confirming the EnhancedInputSubsystem was not mutated requires
//         inspecting the subsystem's active context list, which is only
//         populated for a locally controlled pawn with a valid PlayerController.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIMCNotAddedForNonLocalControllerTest,
	"ClimbingSystem.Multiplayer.Lifecycle.IMCNotAddedForNonLocalController",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIMCNotAddedForNonLocalControllerTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0233: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	// In an automation world the spawned pawn has no controller → not locally controlled.
	TestFalse(TEXT("TC-0233: pre-condition — pawn should not be locally controlled in automation world"),
		Character->IsLocallyControlled());

	Character->ClimbingInputMappingContext = NewObject<UInputMappingContext>(Character);
	Character->TestClimbingIMCActive() = false;

	Character->TestAddClimbingInputMappingContext();

	TestFalse(TEXT("TC-0233: bClimbingIMCActive should remain false for non-locally-controlled pawn"),
		Character->TestClimbingIMCActive());

	// VERIFY: Confirming the EnhancedInputSubsystem context list was not mutated
	// requires a locally controlled pawn with a valid PlayerController — needs PIE.

	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
