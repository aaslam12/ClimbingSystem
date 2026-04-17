// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

// ============================================================================
// CATEGORY 4: Networking Tests — Single-Process Approximation
// NETWORK: single-process approximation — full replication requires PIE multi-player session
// ============================================================================

// WHAT: Validates all Server_ RPCs exist as UFUNCTION properties on the class
// WHY: Missing RPCs cause silent input failures in multiplayer
// EDGE CASES: Checks function existence, not invocation

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingServerRPCExistenceTest,
	"ClimbingSystem.Multiplayer.RPCs.ServerFunctionsExist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingServerRPCExistenceTest::RunTest(const FString& Parameters)
{
	const UClass* CharClass = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("Character class should exist"), CharClass);
	if (!CharClass) { return false; }

	const FName ServerRPCs[] = {
		TEXT("Server_AttemptGrab"),
		TEXT("Server_Drop"),
		TEXT("Server_AttemptLache"),
		TEXT("Server_AttemptClimbUp"),
		TEXT("Server_UpdateShimmyDirection")
	};

	for (const FName& RPCName : ServerRPCs)
	{
		const UFunction* Func = CharClass->FindFunctionByName(RPCName);
		TestNotNull(
			*FString::Printf(TEXT("Multiplayer: Server RPC '%s' should exist as UFUNCTION"), *RPCName.ToString()),
			Func);
	}

	return true;
}

// WHAT: Validates all Client_ RPCs exist as UFUNCTION properties on the class
// WHY: Missing client RPCs prevent server rejection/confirmation from reaching clients
// EDGE CASES: Checks function existence, not invocation

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingClientRPCExistenceTest,
	"ClimbingSystem.Multiplayer.RPCs.ClientFunctionsExist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingClientRPCExistenceTest::RunTest(const FString& Parameters)
{
	const UClass* CharClass = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("Character class should exist"), CharClass);
	if (!CharClass) { return false; }

	const FName ClientRPCs[] = {
		TEXT("Client_RejectStateTransition"),
		TEXT("Client_ConfirmStateTransition")
	};

	for (const FName& RPCName : ClientRPCs)
	{
		const UFunction* Func = CharClass->FindFunctionByName(RPCName);
		TestNotNull(
			*FString::Printf(TEXT("Multiplayer: Client RPC '%s' should exist as UFUNCTION"), *RPCName.ToString()),
			Func);
	}

	return true;
}

// WHAT: Validates AnchorComponent on movement component is TObjectPtr (replicable), not TWeakObjectPtr
// WHY: PROMPT.md explicitly states TWeakObjectPtr is not replicable — AnchorComponent must be TObjectPtr
// EDGE CASES: Property type reflection check

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingAnchorReplicableTypeTest,
	"ClimbingSystem.Multiplayer.Anchor.IsReplicableType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingAnchorReplicableTypeTest::RunTest(const FString& Parameters)
{
	const UClass* MovementClass = UClimbingMovementComponent::StaticClass();
	TestNotNull(TEXT("Movement class should exist"), MovementClass);
	if (!MovementClass) { return false; }

	const FProperty* AnchorProp = MovementClass->FindPropertyByName(TEXT("AnchorComponent"));
	TestNotNull(TEXT("Multiplayer: AnchorComponent property should exist"), AnchorProp);
	if (!AnchorProp) { return false; }

	// AnchorComponent must be replicated
	TestTrue(TEXT("Multiplayer: AnchorComponent should be replicated"),
		AnchorProp->HasAnyPropertyFlags(CPF_Net));

	return true;
}

// WHAT: Validates LastValidatedDetectionResult is replicated
// WHY: Simulated proxies need this to resolve HitComponent via confirmation trace
// EDGE CASES: Property replication flag check

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingLastValidatedResultReplicatedTest,
	"ClimbingSystem.Multiplayer.DetectionResult.IsReplicated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingLastValidatedResultReplicatedTest::RunTest(const FString& Parameters)
{
	const UClass* MovementClass = UClimbingMovementComponent::StaticClass();
	TestNotNull(TEXT("Movement class should exist"), MovementClass);
	if (!MovementClass) { return false; }

	const FProperty* ResultProp = MovementClass->FindPropertyByName(TEXT("LastValidatedDetectionResult"));
	TestNotNull(TEXT("Multiplayer: LastValidatedDetectionResult property should exist"), ResultProp);
	if (!ResultProp) { return false; }

	TestTrue(TEXT("Multiplayer: LastValidatedDetectionResult should be replicated"),
		ResultProp->HasAnyPropertyFlags(CPF_Net));

	return true;
}

// WHAT: Validates CurrentClimbingState on movement component has RepNotify
// WHY: OnRep_ClimbingState drives proxy animation and confirmation trace
// EDGE CASES: RepNotify flag check

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingStateRepNotifyTest,
	"ClimbingSystem.Multiplayer.State.HasRepNotify",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingStateRepNotifyTest::RunTest(const FString& Parameters)
{
	const UClass* MovementClass = UClimbingMovementComponent::StaticClass();
	TestNotNull(TEXT("Movement class should exist"), MovementClass);
	if (!MovementClass) { return false; }

	const FProperty* StateProp = MovementClass->FindPropertyByName(TEXT("CurrentClimbingState"));
	TestNotNull(TEXT("Multiplayer: CurrentClimbingState property should exist"), StateProp);
	if (!StateProp) { return false; }

	TestTrue(TEXT("Multiplayer: CurrentClimbingState should be replicated"),
		StateProp->HasAnyPropertyFlags(CPF_Net));
	TestTrue(TEXT("Multiplayer: CurrentClimbingState should have RepNotify"),
		StateProp->HasAnyPropertyFlags(CPF_RepNotify));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
