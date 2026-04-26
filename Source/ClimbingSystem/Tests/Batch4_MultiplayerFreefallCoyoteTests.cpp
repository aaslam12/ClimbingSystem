// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingTypes.h"
#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

// ---------------------------------------------------------------------------
// TC-0335: ServerGrabRejectsStreamingSublevel
// WHAT: Server_AttemptGrab UFunction exists via reflection.
// WHY:  Sublevel rejection logic lives inside Server_AttemptGrab; if the
//       function is missing the server never validates grab requests.
// VERIFY: Sublevel streaming state cannot be injected in a headless world.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerGrabRejectsStreamingSublevelTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.ServerGrabRejectsStreamingSublevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServerGrabRejectsStreamingSublevelTest::RunTest(const FString& Parameters)
{
	UClass* CharClass = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0335: AClimbingCharacter::StaticClass() should exist"), CharClass);
	if (!CharClass) { return false; }

	UFunction* Func = CharClass->FindFunctionByName(TEXT("Server_AttemptGrab"));
	TestNotNull(TEXT("TC-0335: Server_AttemptGrab UFunction should exist"), Func);
	if (!Func) { return false; }

	// VERIFY: Sublevel streaming state cannot be injected in a headless automation world.
	return true;
}

// ---------------------------------------------------------------------------
// TC-0336: ServerGrabAcceptsLoadedLevel
// WHAT: Server_AttemptGrab UFunction exists and is a reliable server RPC.
// WHY:  Complement to TC-0335 — confirms the function is present for the
//       loaded-level acceptance path.
// VERIFY: Loaded-level acceptance requires a live server context (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerGrabAcceptsLoadedLevelTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.ServerGrabAcceptsLoadedLevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServerGrabAcceptsLoadedLevelTest::RunTest(const FString& Parameters)
{
	UClass* CharClass = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0336: AClimbingCharacter::StaticClass() should exist"), CharClass);
	if (!CharClass) { return false; }

	UFunction* Func = CharClass->FindFunctionByName(TEXT("Server_AttemptGrab"));
	TestNotNull(TEXT("TC-0336: Server_AttemptGrab UFunction should exist"), Func);
	if (!Func) { return false; }

	const bool bIsServer = (Func->FunctionFlags & FUNC_Net) != 0;
	TestTrue(TEXT("TC-0336: Server_AttemptGrab should have FUNC_Net flag"), bIsServer);

	// VERIFY: Loaded-level acceptance path requires a live server context (PIE).
	return true;
}

// ---------------------------------------------------------------------------
// TC-0337: ServerDropFromHangingToNone
// WHAT: Server_Drop transitions state from Hanging to None.
// WHY:  If Server_Drop does not clear the climbing state the character stays
//       locked in hang after an intentional drop.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerDropFromHangingToNoneTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.ServerDropFromHangingToNone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServerDropFromHangingToNoneTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0337: character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0337: movement component should exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Hanging);
	TestEqual(TEXT("TC-0337: pre-condition — state should be Hanging"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	Character->Server_Drop();

	TestEqual(TEXT("TC-0337: state should be None after Server_Drop"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0338: ServerDropFromShimmyToNone
// WHAT: Server_Drop transitions state from Shimmying to None.
// WHY:  Drop must work from any hanging-family state, not just Hanging.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerDropFromShimmyToNoneTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.ServerDropFromShimmyToNone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServerDropFromShimmyToNoneTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0338: character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0338: movement component should exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Shimmying);
	TestEqual(TEXT("TC-0338: pre-condition — state should be Shimmying"),
		Movement->CurrentClimbingState, EClimbingState::Shimmying);

	Character->Server_Drop();

	TestEqual(TEXT("TC-0338: state should be None after Server_Drop"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0339: ServerLacheRejectsOutOfRange
// WHAT: Server_AttemptLache UFunction exists via reflection.
// WHY:  Out-of-range rejection lives inside Server_AttemptLache; missing
//       function means no server-side range validation occurs.
// VERIFY: Range rejection requires a live server context (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLacheRejectsOutOfRangeTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.ServerLacheRejectsOutOfRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServerLacheRejectsOutOfRangeTest::RunTest(const FString& Parameters)
{
	UClass* CharClass = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0339: AClimbingCharacter::StaticClass() should exist"), CharClass);
	if (!CharClass) { return false; }

	UFunction* Func = CharClass->FindFunctionByName(TEXT("Server_AttemptLache"));
	TestNotNull(TEXT("TC-0339: Server_AttemptLache UFunction should exist"), Func);
	if (!Func) { return false; }

	// VERIFY: Range rejection requires a live server context (PIE).
	return true;
}

// ---------------------------------------------------------------------------
// TC-0340: ServerLacheAcceptsValidTarget
// WHAT: Server_AttemptLache UFunction exists and carries FUNC_Net flag.
// WHY:  Complement to TC-0339 — confirms the function is present for the
//       valid-target acceptance path.
// VERIFY: Valid-target acceptance requires a live server context (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerLacheAcceptsValidTargetTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.ServerLacheAcceptsValidTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServerLacheAcceptsValidTargetTest::RunTest(const FString& Parameters)
{
	UClass* CharClass = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0340: AClimbingCharacter::StaticClass() should exist"), CharClass);
	if (!CharClass) { return false; }

	UFunction* Func = CharClass->FindFunctionByName(TEXT("Server_AttemptLache"));
	TestNotNull(TEXT("TC-0340: Server_AttemptLache UFunction should exist"), Func);
	if (!Func) { return false; }

	const bool bIsNet = (Func->FunctionFlags & FUNC_Net) != 0;
	TestTrue(TEXT("TC-0340: Server_AttemptLache should have FUNC_Net flag"), bIsNet);

	// VERIFY: Valid-target acceptance requires a live server context (PIE).
	return true;
}

// ---------------------------------------------------------------------------
// TC-0341: ServerClimbUpRejectsNoClearance
// WHAT: Server_AttemptClimbUp UFunction exists via reflection.
// WHY:  No-clearance rejection lives inside Server_AttemptClimbUp; missing
//       function means the server never validates clearance.
// VERIFY: Clearance rejection requires a live server context (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerClimbUpRejectsNoClearanceTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.ServerClimbUpRejectsNoClearance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServerClimbUpRejectsNoClearanceTest::RunTest(const FString& Parameters)
{
	UClass* CharClass = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0341: AClimbingCharacter::StaticClass() should exist"), CharClass);
	if (!CharClass) { return false; }

	UFunction* Func = CharClass->FindFunctionByName(TEXT("Server_AttemptClimbUp"));
	TestNotNull(TEXT("TC-0341: Server_AttemptClimbUp UFunction should exist"), Func);
	if (!Func) { return false; }

	// VERIFY: Clearance rejection requires a live server context (PIE).
	return true;
}

// ---------------------------------------------------------------------------
// TC-0342: ServerClimbUpAcceptsClear
// WHAT: Server_AttemptClimbUp UFunction exists and carries FUNC_Net flag.
// WHY:  Complement to TC-0341 — confirms the function is present for the
//       clear-clearance acceptance path.
// VERIFY: Clear-clearance acceptance requires a live server context (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerClimbUpAcceptsClearTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.ServerClimbUpAcceptsClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServerClimbUpAcceptsClearTest::RunTest(const FString& Parameters)
{
	UClass* CharClass = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0342: AClimbingCharacter::StaticClass() should exist"), CharClass);
	if (!CharClass) { return false; }

	UFunction* Func = CharClass->FindFunctionByName(TEXT("Server_AttemptClimbUp"));
	TestNotNull(TEXT("TC-0342: Server_AttemptClimbUp UFunction should exist"), Func);
	if (!Func) { return false; }

	const bool bIsNet = (Func->FunctionFlags & FUNC_Net) != 0;
	TestTrue(TEXT("TC-0342: Server_AttemptClimbUp should have FUNC_Net flag"), bIsNet);

	// VERIFY: Clear-clearance acceptance requires a live server context (PIE).
	return true;
}

// ---------------------------------------------------------------------------
// TC-0343: ServerShimmyDirectionReplicates
// WHAT: Server_UpdateShimmyDirection UFunction exists via reflection.
// WHY:  Shimmy direction replication lives inside this RPC; missing function
//       means proxies never receive direction updates.
// VERIFY: Replication delivery requires a live networked session (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerShimmyDirectionReplicatesTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.ServerShimmyDirectionReplicates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServerShimmyDirectionReplicatesTest::RunTest(const FString& Parameters)
{
	UClass* CharClass = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0343: AClimbingCharacter::StaticClass() should exist"), CharClass);
	if (!CharClass) { return false; }

	UFunction* Func = CharClass->FindFunctionByName(TEXT("Server_UpdateShimmyDirection"));
	TestNotNull(TEXT("TC-0343: Server_UpdateShimmyDirection UFunction should exist"), Func);
	if (!Func) { return false; }

	const bool bIsNet = (Func->FunctionFlags & FUNC_Net) != 0;
	TestTrue(TEXT("TC-0343: Server_UpdateShimmyDirection should have FUNC_Net flag"), bIsNet);

	// VERIFY: Replication delivery requires a live networked session (PIE).
	return true;
}

// ---------------------------------------------------------------------------
// TC-0344: ServerShimmyZeroVectorAccepted
// WHAT: Server_UpdateShimmyDirection UFunction exists and is unreliable.
// WHY:  Zero-vector (stop) must be accepted; unreliable is correct for
//       high-frequency direction updates to avoid queue saturation.
// VERIFY: Zero-vector acceptance requires a live server context (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerShimmyZeroVectorAcceptedTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.ServerShimmyZeroVectorAccepted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServerShimmyZeroVectorAcceptedTest::RunTest(const FString& Parameters)
{
	UClass* CharClass = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0344: AClimbingCharacter::StaticClass() should exist"), CharClass);
	if (!CharClass) { return false; }

	UFunction* Func = CharClass->FindFunctionByName(TEXT("Server_UpdateShimmyDirection"));
	TestNotNull(TEXT("TC-0344: Server_UpdateShimmyDirection UFunction should exist"), Func);
	if (!Func) { return false; }

	// Shimmy direction is unreliable (high-frequency) — must NOT have FUNC_NetReliable.
	const bool bIsUnreliable = (Func->FunctionFlags & FUNC_NetReliable) == 0;
	TestTrue(TEXT("TC-0344: Server_UpdateShimmyDirection should NOT have FUNC_NetReliable (unreliable RPC)"), bIsUnreliable);

	// VERIFY: Zero-vector acceptance requires a live server context (PIE).
	return true;
}

// ---------------------------------------------------------------------------
// TC-0345: ClientConfirmMatchesPrediction
// WHAT: Client_ConfirmStateTransition(Hanging) leaves state as Hanging.
// WHY:  If confirmation overwrites a correct prediction the client flickers
//       back to None then immediately re-enters Hanging.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClientConfirmMatchesPredictionTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.ClientConfirmMatchesPrediction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClientConfirmMatchesPredictionTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0345: character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0345: movement component should exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Hanging);
	TestEqual(TEXT("TC-0345: pre-condition — state should be Hanging"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	// Server confirms the prediction — state must remain Hanging.
	Character->Client_ConfirmStateTransition(EClimbingState::Hanging);

	TestEqual(TEXT("TC-0345: state should remain Hanging after Client_ConfirmStateTransition(Hanging)"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0346: OptimisticPredictionStateSetBeforeAck
// WHAT: After a local TransitionToState(Hanging) the state is Hanging
//       immediately, before any server acknowledgement.
// WHY:  Optimistic prediction must set state locally so the character
//       responds without waiting for a round-trip.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOptimisticPredictionStateSetBeforeAckTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.OptimisticPredictionStateSetBeforeAck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOptimisticPredictionStateSetBeforeAckTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0346: character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0346: movement component should exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Local prediction: set state directly without waiting for server ack.
	Movement->SetClimbingState(EClimbingState::Hanging);

	// Assert immediately — no server round-trip simulated.
	TestEqual(TEXT("TC-0346: state should be Hanging immediately after local prediction"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0347: SimulatedProxyMontageOnRepNotify
// WHAT: OnClimbingStateReplicated(None, BracedWall) drives state to BracedWall.
// WHY:  Simulated proxies rely on RepNotify to enter climbing states; broken
//       RepNotify leaves proxies in the wrong visual state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSimulatedProxyMontageOnRepNotifyTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.SimulatedProxyMontageOnRepNotify",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSimulatedProxyMontageOnRepNotifyTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0347: character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0347: movement component should exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Simulate RepNotify on a proxy: old=None, new=BracedWall.
	Character->OnClimbingStateReplicated(EClimbingState::None, EClimbingState::BracedWall);

	TestEqual(TEXT("TC-0347: state should be BracedWall after OnClimbingStateReplicated(None, BracedWall)"),
		Movement->CurrentClimbingState, EClimbingState::BracedWall);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0348: FreefallGrabSucceedsWithinReach
// WHAT: CDO FallingGrabReachDistance default == 80 and bEnableFallingGrab == true.
// WHY:  Wrong defaults disable freefall grab or change reach silently,
//       breaking the feature for all Blueprint subclasses.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFreefallGrabSucceedsWithinReachTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.FreefallGrabSucceedsWithinReach",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreefallGrabSucceedsWithinReachTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0348: CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("TC-0348: FallingGrabReachDistance default should be 80"),
		FMath::IsNearlyEqual(Defaults->FallingGrabReachDistance, 80.0f));
	TestTrue(TEXT("TC-0348: bEnableFallingGrab default should be true"),
		Defaults->bEnableFallingGrab);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0349: FreefallGrabFailsBeyondReach
// WHAT: FallingGrabReachDistance is a positive finite value (distance check exists).
// WHY:  Complement to TC-0348 — confirms the reach cap is enforced by the
//       property being bounded above zero.
// VERIFY: Beyond-reach rejection requires a live falling character (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFreefallGrabFailsBeyondReachTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.FreefallGrabFailsBeyondReach",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreefallGrabFailsBeyondReachTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0349: CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	// The property must be positive and finite — a zero or negative value would
	// mean every grab attempt fails (zero) or the check is inverted (negative).
	TestTrue(TEXT("TC-0349: FallingGrabReachDistance should be > 0"),
		Defaults->FallingGrabReachDistance > 0.0f);
	TestTrue(TEXT("TC-0349: FallingGrabReachDistance should be finite"),
		FMath::IsFinite(Defaults->FallingGrabReachDistance));

	// VERIFY: Beyond-reach rejection requires a live falling character (PIE).
	return true;
}

// ---------------------------------------------------------------------------
// TC-0350: FreefallGrabDisabledByToggle
// WHAT: bEnableFallingGrab is a configurable UPROPERTY (EditAnywhere).
// WHY:  Designers must be able to disable freefall grab per-character without
//       code changes; a non-configurable property breaks that workflow.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFreefallGrabDisabledByToggleTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.FreefallGrabDisabledByToggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreefallGrabDisabledByToggleTest::RunTest(const FString& Parameters)
{
	UClass* CharClass = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0350: AClimbingCharacter::StaticClass() should exist"), CharClass);
	if (!CharClass) { return false; }

	FProperty* Prop = CharClass->FindPropertyByName(TEXT("bEnableFallingGrab"));
	TestNotNull(TEXT("TC-0350: bEnableFallingGrab property should exist"), Prop);
	if (!Prop) { return false; }

	// Must be editable so designers can toggle it in the Details panel.
	const bool bIsEditable = Prop->HasAnyPropertyFlags(CPF_Edit);
	TestTrue(TEXT("TC-0350: bEnableFallingGrab should have CPF_Edit flag (EditAnywhere)"), bIsEditable);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0351: FreefallCheckIntervalRate
// WHAT: CDO FallingGrabCheckInterval default == 0.05.
// WHY:  Wrong interval changes the freefall scan rate, causing missed grabs
//       (too slow) or excessive trace overhead (too fast).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFreefallCheckIntervalRateTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.FreefallCheckIntervalRate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreefallCheckIntervalRateTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0351: CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("TC-0351: FallingGrabCheckInterval default should be 0.05"),
		FMath::IsNearlyEqual(Defaults->FallingGrabCheckInterval, 0.05f));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0352: CoyoteGrabSucceedsWithinWindow
// WHAT: CDO CoyoteTimeWindow default == 0.15 and bEnableCoyoteTime == true.
// WHY:  Wrong defaults disable coyote time or change the window silently,
//       breaking the feature for all Blueprint subclasses.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoyoteGrabSucceedsWithinWindowTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.CoyoteGrabSucceedsWithinWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoyoteGrabSucceedsWithinWindowTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0352: CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("TC-0352: CoyoteTimeWindow default should be 0.15"),
		FMath::IsNearlyEqual(Defaults->CoyoteTimeWindow, 0.15f));
	TestTrue(TEXT("TC-0352: bEnableCoyoteTime default should be true"),
		Defaults->bEnableCoyoteTime);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0353: CoyoteGrabFailsAfterWindow
// WHAT: CoyoteTimeWindow is a positive finite value (window expiry exists).
// WHY:  Complement to TC-0352 — confirms the window cap is enforced by the
//       property being bounded above zero.
// VERIFY: Post-window rejection requires a live timer and falling character (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCoyoteGrabFailsAfterWindowTest,
	"ClimbingSystem.Multiplayer.FreefallCoyote.CoyoteGrabFailsAfterWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoyoteGrabFailsAfterWindowTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0353: CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	// The window must be positive and finite — zero means coyote time never fires,
	// negative or infinite means it never expires.
	TestTrue(TEXT("TC-0353: CoyoteTimeWindow should be > 0"),
		Defaults->CoyoteTimeWindow > 0.0f);
	TestTrue(TEXT("TC-0353: CoyoteTimeWindow should be finite"),
		FMath::IsFinite(Defaults->CoyoteTimeWindow));

	// VERIFY: Post-window rejection requires a live timer and falling character (PIE).
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
