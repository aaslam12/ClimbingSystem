// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingTypes.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

// WHAT: Validates AnchorLocalTransform on movement component is replicated
// WHY: Simulated proxies need anchor transform to compute world-space grab point
// EDGE CASES: Property must have CPF_Net flag

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingAnchorLocalTransformReplicatedTest,
	"ClimbingSystem.Multiplayer.Anchor.LocalTransformIsReplicated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingAnchorLocalTransformReplicatedTest::RunTest(const FString& Parameters)
{
	const UClass* MovementClass = UClimbingMovementComponent::StaticClass();
	TestNotNull(TEXT("Movement class should exist"), MovementClass);
	if (!MovementClass) { return false; }

	const FProperty* TransformProp = MovementClass->FindPropertyByName(TEXT("AnchorLocalTransform"));
	TestNotNull(TEXT("Multiplayer: AnchorLocalTransform property should exist"), TransformProp);
	if (!TransformProp) { return false; }

	TestTrue(TEXT("Multiplayer: AnchorLocalTransform should be replicated"),
		TransformProp->HasAnyPropertyFlags(CPF_Net));

	return true;
}

// WHAT: Validates ServerValidationPositionTolerance default via CDO
// WHY: Wrong tolerance causes excessive rejections in high-latency games
// EDGE CASES: CDO default check

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingServerValidationToleranceDefaultTest,
	"ClimbingSystem.Multiplayer.Validation.ToleranceDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingServerValidationToleranceDefaultTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("Character defaults should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("Multiplayer: ServerValidationPositionTolerance default should be 30"),
		FMath::IsNearlyEqual(Defaults->ServerValidationPositionTolerance, 30.0f));

	return true;
}

// WHAT: Validates FClimbingDetectionResultNet has no FWeakObjectProperty fields
// WHY: TWeakObjectPtr is not replicable — net struct must be pointer-free
// EDGE CASES: Reflection scan for all property types

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingReplicationContractNoWeakPtrTest,
	"ClimbingSystem.Multiplayer.NetStruct.NoWeakObjectProperties",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingReplicationContractNoWeakPtrTest::RunTest(const FString& Parameters)
{
	const UScriptStruct* NetStruct = FClimbingDetectionResultNet::StaticStruct();
	TestNotNull(TEXT("Net detection struct should exist"), NetStruct);
	if (!NetStruct) { return false; }

	int32 WeakPtrCount = 0;
	for (TFieldIterator<FWeakObjectProperty> It(NetStruct); It; ++It)
	{
		++WeakPtrCount;
	}
	TestEqual(TEXT("Multiplayer: FClimbingDetectionResultNet should have zero FWeakObjectProperty fields"),
		WeakPtrCount, 0);

	return true;
}

// WHAT: Validates ClimbingInputMappingContext and LocomotionInputMappingContext properties exist
// WHY: Missing IMC properties prevent input context push/pop on climbing entry/exit
// EDGE CASES: Property existence via reflection

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingInputMappingContextPropertyTest,
	"ClimbingSystem.Multiplayer.Input.MappingContextPropertiesExist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingInputMappingContextPropertyTest::RunTest(const FString& Parameters)
{
	const UClass* CharClass = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("Character class should exist"), CharClass);
	if (!CharClass) { return false; }

	const FProperty* ClimbingIMC = CharClass->FindPropertyByName(TEXT("ClimbingInputMappingContext"));
	const FProperty* LocomotionIMC = CharClass->FindPropertyByName(TEXT("LocomotionInputMappingContext"));

	TestNotNull(TEXT("Multiplayer: ClimbingInputMappingContext property should exist"), ClimbingIMC);
	TestNotNull(TEXT("Multiplayer: LocomotionInputMappingContext property should exist"), LocomotionIMC);

	return true;
}

// WHAT: Validates OnRep_ClimbingState function exists on UClimbingMovementComponent
// WHY: RepNotify drives proxy animation and confirmation trace — missing function breaks proxies
// EDGE CASES: Function existence via FindFunctionByName

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingOnRepClimbingStateFunctionExistsTest,
	"ClimbingSystem.Multiplayer.RepNotify.OnRepFunctionExists",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingOnRepClimbingStateFunctionExistsTest::RunTest(const FString& Parameters)
{
	const UClass* MovementClass = UClimbingMovementComponent::StaticClass();
	TestNotNull(TEXT("Movement class should exist"), MovementClass);
	if (!MovementClass) { return false; }

	const UFunction* OnRepFunc = MovementClass->FindFunctionByName(TEXT("OnRep_ClimbingState"));
	TestNotNull(TEXT("Multiplayer: OnRep_ClimbingState function should exist on UClimbingMovementComponent"),
		OnRepFunc);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
