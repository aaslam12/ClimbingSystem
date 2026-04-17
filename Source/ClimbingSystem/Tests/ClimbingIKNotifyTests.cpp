// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimNotify_EnableClimbIK.h"
#include "Animation/AnimNotify_DisableClimbIK.h"
#include "Misc/AutomationTest.h"

// WHAT: Validates EnableClimbIK notify default LimbMask is BothHands
// WHY: PROMPT.md specifies GrabLedge notify enables both hands at hand-contact frame
// EDGE CASES: Default construction only

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnableClimbIKDefaultsTest,
	"ClimbingSystem.IKNotify.Enable.DefaultIsBothHands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnableClimbIKDefaultsTest::RunTest(const FString& Parameters)
{
	UAnimNotify_EnableClimbIK* Notify = NewObject<UAnimNotify_EnableClimbIK>();
	TestNotNull(TEXT("EnableClimbIK notify should be created"), Notify);
	if (!Notify) { return false; }

	TestEqual(TEXT("EnableClimbIK: default LimbMask should be BothHands"),
		Notify->LimbMask, static_cast<int32>(EClimbIKLimbMask::BothHands));
	TestTrue(TEXT("EnableClimbIK: default TargetWeight should be 1.0"),
		FMath::IsNearlyEqual(Notify->TargetWeight, 1.0f));

	return true;
}

// WHAT: Validates DisableClimbIK notify default LimbMask is BothHands
// WHY: PROMPT.md specifies DropDown notify disables both hands at hand-release frame
// EDGE CASES: Default construction only

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDisableClimbIKDefaultsTest,
	"ClimbingSystem.IKNotify.Disable.DefaultIsBothHands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDisableClimbIKDefaultsTest::RunTest(const FString& Parameters)
{
	UAnimNotify_DisableClimbIK* Notify = NewObject<UAnimNotify_DisableClimbIK>();
	TestNotNull(TEXT("DisableClimbIK notify should be created"), Notify);
	if (!Notify) { return false; }

	TestEqual(TEXT("DisableClimbIK: default LimbMask should be BothHands"),
		Notify->LimbMask, static_cast<int32>(EClimbIKLimbMask::BothHands));

	return true;
}

// WHAT: Validates EClimbIKLimbMask bitmask combinations are correct
// WHY: Incorrect bitmask values cause wrong limbs to be enabled/disabled
// EDGE CASES: Convenience masks (BothHands, BothFeet, AllLimbs)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbIKLimbMaskBitmaskTest,
	"ClimbingSystem.IKNotify.LimbMask.BitmaskCombinations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbIKLimbMaskBitmaskTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("IKLimbMask: BothHands should be HandLeft | HandRight"),
		static_cast<uint8>(EClimbIKLimbMask::BothHands),
		static_cast<uint8>(EClimbIKLimbMask::HandLeft) | static_cast<uint8>(EClimbIKLimbMask::HandRight));

	TestEqual(TEXT("IKLimbMask: BothFeet should be FootLeft | FootRight"),
		static_cast<uint8>(EClimbIKLimbMask::BothFeet),
		static_cast<uint8>(EClimbIKLimbMask::FootLeft) | static_cast<uint8>(EClimbIKLimbMask::FootRight));

	TestEqual(TEXT("IKLimbMask: AllLimbs should be BothHands | BothFeet"),
		static_cast<uint8>(EClimbIKLimbMask::AllLimbs),
		static_cast<uint8>(EClimbIKLimbMask::BothHands) | static_cast<uint8>(EClimbIKLimbMask::BothFeet));

	return true;
}

// WHAT: Validates GetNotifyName returns a non-empty display name
// WHY: Empty names make notifies invisible in the animation editor timeline
// EDGE CASES: Both enable and disable notifies

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbIKNotifyNamesTest,
	"ClimbingSystem.IKNotify.Names.NonEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbIKNotifyNamesTest::RunTest(const FString& Parameters)
{
	UAnimNotify_EnableClimbIK* EnableNotify = NewObject<UAnimNotify_EnableClimbIK>();
	UAnimNotify_DisableClimbIK* DisableNotify = NewObject<UAnimNotify_DisableClimbIK>();

	TestFalse(TEXT("EnableClimbIK: GetNotifyName should not be empty"),
		EnableNotify->GetNotifyName().IsEmpty());
	TestFalse(TEXT("DisableClimbIK: GetNotifyName should not be empty"),
		DisableNotify->GetNotifyName().IsEmpty());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
