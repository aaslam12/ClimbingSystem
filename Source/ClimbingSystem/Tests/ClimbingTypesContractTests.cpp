// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Data/ClimbingTypes.h"
#include "Misc/AutomationTest.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingEnumLayoutTest,
	"ClimbingSystem.Types.Enums.Layout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingEnumLayoutTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Animation slot enum count should match expected layout"), static_cast<uint8>(EClimbingAnimationSlot::MAX), static_cast<uint8>(37));
	TestEqual(TEXT("Climbing state enum count should match expected layout"), static_cast<uint8>(EClimbingState::MAX), static_cast<uint8>(17));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingStructBlueprintExposureTest,
	"ClimbingSystem.Types.Structs.BlueprintExposure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingStructBlueprintExposureTest::RunTest(const FString& Parameters)
{
	const UScriptStruct* LocalStruct = FClimbingDetectionResult::StaticStruct();
	const UScriptStruct* NetStruct = FClimbingDetectionResultNet::StaticStruct();
	TestNotNull(TEXT("Local detection struct should exist"), LocalStruct);
	TestNotNull(TEXT("Net detection struct should exist"), NetStruct);
	if (!LocalStruct || !NetStruct)
	{
		return false;
	}

	const bool bLocalBlueprintType = LocalStruct->HasMetaData(TEXT("BlueprintType"));
	const bool bNetBlueprintType = NetStruct->HasMetaData(TEXT("BlueprintType"));

	TestTrue(TEXT("FClimbingDetectionResult should be BlueprintType"), bLocalBlueprintType);
	TestFalse(TEXT("FClimbingDetectionResultNet should not be BlueprintType"), bNetBlueprintType);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingNetStructSafetyTest,
	"ClimbingSystem.Types.Structs.NetSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingNetStructSafetyTest::RunTest(const FString& Parameters)
{
	const UScriptStruct* NetStruct = FClimbingDetectionResultNet::StaticStruct();
	TestNotNull(TEXT("Net detection struct should exist"), NetStruct);
	if (!NetStruct)
	{
		return false;
	}

	int32 ObjectPropertyCount = 0;
	for (TFieldIterator<FObjectPropertyBase> It(NetStruct); It; ++It)
	{
		++ObjectPropertyCount;
	}
	TestEqual(TEXT("Net detection struct should not contain UObject pointer properties"), ObjectPropertyCount, 0);

	constexpr bool bHasCustomNetSerializer = TStructOpsTypeTraits<FClimbingDetectionResultNet>::WithNetSerializer;
	TestFalse(TEXT("Net detection struct should not define custom NetSerialize"), bHasCustomNetSerializer);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingStateConfigDefaultsTest,
	"ClimbingSystem.Types.Structs.StateConfigDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingStateConfigDefaultsTest::RunTest(const FString& Parameters)
{
	const FClimbingStateConfig DefaultConfig;
	TestTrue(TEXT("Default state config should be interruptible"), DefaultConfig.bInterruptible);
	TestTrue(TEXT("Default min completion should be 0"), FMath::IsNearlyZero(DefaultConfig.MinCompletionBeforeCancel));

	const FClimbingStateConfig CustomConfig(false, 0.7f);
	TestFalse(TEXT("Custom constructor should set interruptibility"), CustomConfig.bInterruptible);
	TestTrue(TEXT("Custom constructor should set min completion"), FMath::IsNearlyEqual(CustomConfig.MinCompletionBeforeCancel, 0.7f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
