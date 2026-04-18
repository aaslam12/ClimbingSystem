// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Misc/AutomationTest.h"

// WHAT: Validates physics-related default property values match PROMPT.md spec
// WHY: Incorrect physics defaults cause grab-break failures or ragdoll jitter
// EDGE CASES: PelvisBoneName and RagdollCameraTargetSocket already tested — skipped

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingPhysicsSettingsDefaultsTest,
	"ClimbingSystem.Character.Physics.DefaultValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingPhysicsSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("Character defaults should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("Physics: GrabBreakImpulseThreshold should be 2000"),
		FMath::IsNearlyEqual(Defaults->GrabBreakImpulseThreshold, 2000.0f));
	TestTrue(TEXT("Physics: GrabBreakLaunchScale should be 0.5"),
		FMath::IsNearlyEqual(Defaults->GrabBreakLaunchScale, 0.5f));
	TestTrue(TEXT("Physics: RagdollRecoveryTime should be 1.5"),
		FMath::IsNearlyEqual(Defaults->RagdollRecoveryTime, 1.5f));

	return true;
}

// WHAT: Validates state machine capsule/collision defaults match PROMPT.md spec
// WHY: Wrong capsule size causes wall clipping; wrong profile causes jitter
// EDGE CASES: FName comparison for collision profile

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingStateMachineSettingsDefaultsTest,
	"ClimbingSystem.Character.StateMachine.DefaultValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingStateMachineSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("Character defaults should exist"), Defaults);
	if (!Defaults) { return false; }

	TestEqual(TEXT("StateMachine: ClimbingCollisionProfile should be ClimbingCapsule"),
		Defaults->ClimbingCollisionProfile, FName(TEXT("ClimbingCapsule")));
	TestTrue(TEXT("StateMachine: ClimbingCapsuleHalfHeight should be 48"),
		FMath::IsNearlyEqual(Defaults->ClimbingCapsuleHalfHeight, 48.0f));
	TestTrue(TEXT("StateMachine: ClimbingCapsuleRadius should be 24"),
		FMath::IsNearlyEqual(Defaults->ClimbingCapsuleRadius, 24.0f));

	return true;
}

// WHAT: Validates multiplayer settings not covered by existing tests
// WHY: Wrong tolerance causes excessive server rejections in high-latency games
// EDGE CASES: PredictionRollbackBlendOut already tested — skipped

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMultiplayerSettingsDefaultsTest,
	"ClimbingSystem.Character.Multiplayer.DefaultValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMultiplayerSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("Character defaults should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("Multiplayer: ServerValidationPositionTolerance should be 30"),
		FMath::IsNearlyEqual(Defaults->ServerValidationPositionTolerance, 30.0f));

	return true;
}

// WHAT: Validates ClimbingSounds map is empty by default
// WHY: Pre-populated sound map would override designer intent; empty = explicit assignment required
// EDGE CASES: Default construction only

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingAudioMapDefaultsTest,
	"ClimbingSystem.Character.Audio.DefaultMapEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingAudioMapDefaultsTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("Character defaults should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("Audio: ClimbingSounds map should be empty by default"),
		Defaults->ClimbingSounds.IsEmpty());

	return true;
}

// WHAT: Validates IK-related default property values match PROMPT.md spec
// WHY: Incorrect IK defaults cause hyperextension, culling artifacts, or perf issues
// EDGE CASES: All 6 IK properties checked

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingIKSettingsDefaultsTest,
	"ClimbingSystem.Character.IK.DefaultValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingIKSettingsDefaultsTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("Character defaults should exist"), Defaults);
	if (!Defaults) { return false; }

	TestEqual(TEXT("IK: MaxSimultaneousIKCharacters should be 4"),
		Defaults->MaxSimultaneousIKCharacters, 4);
	TestTrue(TEXT("IK: SimulatedProxyIKCullDistance should be 1500"),
		FMath::IsNearlyEqual(Defaults->SimulatedProxyIKCullDistance, 1500.0f));
	TestTrue(TEXT("IK: SimulatedProxyIKUpdateInterval should be 0.05"),
		FMath::IsNearlyEqual(Defaults->SimulatedProxyIKUpdateInterval, 0.05f));
	TestTrue(TEXT("IK: MaxReachDistance should be 80"),
		FMath::IsNearlyEqual(Defaults->MaxReachDistance, 80.0f));
	TestTrue(TEXT("IK: IKFadeOutBlendTime should be 0.15"),
		FMath::IsNearlyEqual(Defaults->IKFadeOutBlendTime, 0.15f));
	TestTrue(TEXT("IK: HandIKSpacing should be 40"),
		FMath::IsNearlyEqual(Defaults->HandIKSpacing, 40.0f));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
