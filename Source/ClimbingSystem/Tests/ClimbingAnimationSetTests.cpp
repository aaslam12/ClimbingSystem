// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/ClimbingAnimationSet.h"
#include "Misc/AutomationTest.h"

// WHAT: Validates GetMontageForSlot returns nullptr for all slots on a default-constructed set
// WHY: Per-slot fallback to character defaults requires null returns from unassigned override sets
// EDGE CASES: Every slot in the enum is checked — ensures switch is exhaustive

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingAnimSetDefaultNullTest,
	"ClimbingSystem.AnimationSet.GetMontageForSlot.DefaultReturnsNull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingAnimSetDefaultNullTest::RunTest(const FString& Parameters)
{
	UClimbingAnimationSet* AnimSet = NewObject<UClimbingAnimationSet>();
	TestNotNull(TEXT("AnimationSet should be created"), AnimSet);
	if (!AnimSet) { return false; }

	for (uint8 i = 0; i < static_cast<uint8>(EClimbingAnimationSlot::MAX); ++i)
	{
		EClimbingAnimationSlot Slot = static_cast<EClimbingAnimationSlot>(i);
		UAnimMontage* Montage = AnimSet->GetMontageForSlot(Slot);
		TestNull(
			*FString::Printf(TEXT("AnimationSet: slot %s should be null on default construction"),
				*UEnum::GetValueAsString(Slot)),
			Montage);
	}

	return true;
}

// WHAT: Validates GetPrimaryAssetId returns a valid identifier
// WHY: Asset manager uses this for async loading of animation set overrides
// EDGE CASES: Type string correctness

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingAnimSetAssetIdTest,
	"ClimbingSystem.AnimationSet.AssetId.ReturnsValidId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingAnimSetAssetIdTest::RunTest(const FString& Parameters)
{
	UClimbingAnimationSet* AnimSet = NewObject<UClimbingAnimationSet>();
	TestNotNull(TEXT("AnimationSet should be created"), AnimSet);
	if (!AnimSet) { return false; }

	FPrimaryAssetId AssetId = AnimSet->GetPrimaryAssetId();
	TestTrue(TEXT("AnimationSet: PrimaryAssetId should be valid"),
		AssetId.IsValid());
	TestEqual(TEXT("AnimationSet: PrimaryAssetType should be ClimbingAnimationSet"),
		AssetId.PrimaryAssetType.GetName(), FName(TEXT("ClimbingAnimationSet")));

	return true;
}

// WHAT: Validates GetMontageForSlot returns nullptr for MAX/invalid slot
// WHY: Out-of-range slot values must not crash — switch default handles this
// EDGE CASES: MAX enum value, cast from out-of-range uint8

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingAnimSetInvalidSlotTest,
	"ClimbingSystem.AnimationSet.GetMontageForSlot.InvalidSlotReturnsNull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingAnimSetInvalidSlotTest::RunTest(const FString& Parameters)
{
	UClimbingAnimationSet* AnimSet = NewObject<UClimbingAnimationSet>();
	TestNotNull(TEXT("AnimationSet should be created"), AnimSet);
	if (!AnimSet) { return false; }

	TestNull(TEXT("AnimationSet: MAX slot should return null"),
		AnimSet->GetMontageForSlot(EClimbingAnimationSlot::MAX));
	TestNull(TEXT("AnimationSet: out-of-range slot should return null"),
		AnimSet->GetMontageForSlot(static_cast<EClimbingAnimationSlot>(255)));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
