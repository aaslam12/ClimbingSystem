// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimNotify_ClimbSoundCue.h"
#include "Misc/AutomationTest.h"

// WHAT: Validates ClimbSoundCue notify default property values
// WHY: Incorrect defaults cause silent audio or distorted playback
// EDGE CASES: Default construction only

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbSoundNotifyDefaultsTest,
	"ClimbingSystem.Audio.SoundNotify.DefaultValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbSoundNotifyDefaultsTest::RunTest(const FString& Parameters)
{
	UAnimNotify_ClimbSoundCue* Notify = NewObject<UAnimNotify_ClimbSoundCue>();
	TestNotNull(TEXT("ClimbSoundCue notify should be created"), Notify);
	if (!Notify) { return false; }

	TestTrue(TEXT("ClimbSoundCue: default VolumeMultiplier should be 1.0"),
		FMath::IsNearlyEqual(Notify->VolumeMultiplier, 1.0f));
	TestTrue(TEXT("ClimbSoundCue: default PitchMultiplier should be 1.0"),
		FMath::IsNearlyEqual(Notify->PitchMultiplier, 1.0f));
	TestTrue(TEXT("ClimbSoundCue: default BoneName should be None (empty)"),
		Notify->BoneName.IsNone());

	return true;
}

// WHAT: Validates GetNotifyName returns a non-empty display name
// WHY: Empty names make notifies invisible in the animation editor timeline
// EDGE CASES: Default construction

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbSoundNotifyNameTest,
	"ClimbingSystem.Audio.SoundNotify.NameNonEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbSoundNotifyNameTest::RunTest(const FString& Parameters)
{
	UAnimNotify_ClimbSoundCue* Notify = NewObject<UAnimNotify_ClimbSoundCue>();
	TestNotNull(TEXT("ClimbSoundCue notify should be created"), Notify);
	if (!Notify) { return false; }

	TestFalse(TEXT("ClimbSoundCue: GetNotifyName should not be empty"),
		Notify->GetNotifyName().IsEmpty());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
