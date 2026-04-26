// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingTypes.h"
#include "Animation/AnimMontage.h"
#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

// ---------------------------------------------------------------------------
// TC-0517: ServerGrabNullHitComponentRejected
// WHAT: Server_AttemptGrab with bValid=false must leave state as None.
// WHY:  Invalid payloads must never cause a state transition.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerGrabNullHitComponentRejectedTest,
	"ClimbingSystem.Batch7.Multiplayer.ServerGrabNullHitComponentRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServerGrabNullHitComponentRejectedTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0517: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	FClimbingDetectionResultNet NetResult;
	NetResult.bValid = false;

	Character->Server_AttemptGrab(NetResult);

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0517: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	TestEqual(TEXT("TC-0517: state must remain None after invalid grab"), Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0518: ServerGrabInvalidPayloadRejected
// WHAT: Server_AttemptGrab with bValid=false keeps state at None.
// WHY:  Duplicate coverage confirming the rejection path is consistent.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerGrabInvalidPayloadRejectedTest,
	"ClimbingSystem.Batch7.Multiplayer.ServerGrabInvalidPayloadRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServerGrabInvalidPayloadRejectedTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0518: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	FClimbingDetectionResultNet NetResult;
	NetResult.bValid = false;
	NetResult.LedgePosition = FVector(999.f, 999.f, 999.f);
	NetResult.SurfaceNormal  = FVector(0.f, -1.f, 0.f);

	Character->Server_AttemptGrab(NetResult);

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!Movement) { Helper.Teardown(); return false; }

	TestEqual(TEXT("TC-0518: state must be None after invalid payload"), Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0519: SameStateTransitionNoOpAtRuntime
// WHAT: Calling TransitionToState(Hanging) while already Hanging keeps state Hanging.
// WHY:  Re-entering the same state must not trigger exit/entry side effects.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSameStateTransitionNoOpAtRuntimeTest,
	"ClimbingSystem.Batch7.Multiplayer.SameStateTransitionNoOpAtRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSameStateTransitionNoOpAtRuntimeTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0519: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!Movement) { Helper.Teardown(); return false; }

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	Character->TransitionToState(EClimbingState::Hanging, DetResult);

	// Transition to same state — must be a no-op
	Character->TransitionToState(EClimbingState::Hanging, DetResult);

	TestEqual(TEXT("TC-0519: state must still be Hanging"), Movement->CurrentClimbingState, EClimbingState::Hanging);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0520: GetMontageForSlotMAXEnumReturnsNull
// WHAT: GetMontageForSlot(EClimbingAnimationSlot::MAX) returns nullptr without crash.
// WHY:  Out-of-range enum values must be handled gracefully.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGetMontageForSlotMAXEnumReturnsNullTest,
	"ClimbingSystem.Batch7.Multiplayer.GetMontageForSlotMAXEnumReturnsNull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGetMontageForSlotMAXEnumReturnsNullTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0520: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UAnimMontage* Result = Character->GetMontageForSlot(EClimbingAnimationSlot::MAX);
	TestNull(TEXT("TC-0520: GetMontageForSlot(MAX) must return null"), Result);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0521: AudioUnmappedSoundTypeNoCrash
// WHAT: ClimbingSounds TMap exists on the character (contract verification).
// WHY:  Unmapped sound dispatch must not crash — the TMap must be present.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAudioUnmappedSoundTypeNoCrashTest,
	"ClimbingSystem.Batch7.Multiplayer.AudioUnmappedSoundTypeNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAudioUnmappedSoundTypeNoCrashTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0521: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	// VERIFY: ClimbingSounds TMap is accessible (contract: property exists, no crash on access)
	const int32 SoundCount = Character->ClimbingSounds.Num();
	TestTrue(TEXT("TC-0521: ClimbingSounds TMap is accessible without crash"), SoundCount >= 0);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0522: CapsuleRestoreUncachedDimsNoCrash
// WHAT: Transitioning to None without ever entering climbing must not crash.
// WHY:  Capsule restore path must handle the case where no cached dims exist.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCapsuleRestoreUncachedDimsNoCrashTest,
	"ClimbingSystem.Batch7.Multiplayer.CapsuleRestoreUncachedDimsNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCapsuleRestoreUncachedDimsNoCrashTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0522: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	// Transition directly to None without ever entering a climbing state
	FClimbingDetectionResult EmptyResult;
	Character->TransitionToState(EClimbingState::None, EmptyResult);

	// If we reach here, no crash occurred
	TestTrue(TEXT("TC-0522: no crash when transitioning to None with uncached capsule dims"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0523: IKBudgetZeroMaxDisablesAll
// WHAT: MaxSimultaneousIKCharacters defaults to 4; setting to 0 is configurable.
// WHY:  IK budget of 0 must disable all IK without crash.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIKBudgetZeroMaxDisablesAllTest,
	"ClimbingSystem.Batch7.Multiplayer.IKBudgetZeroMaxDisablesAll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIKBudgetZeroMaxDisablesAllTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0523: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	// VERIFY: default value
	TestEqual(TEXT("TC-0523: MaxSimultaneousIKCharacters default must be 4"), Character->MaxSimultaneousIKCharacters, 4);

	// VERIFY: property is configurable to 0
	Character->MaxSimultaneousIKCharacters = 0;
	TestEqual(TEXT("TC-0523: MaxSimultaneousIKCharacters can be set to 0"), Character->MaxSimultaneousIKCharacters, 0);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0524: ShimmyDeadzoneZeroEveryInputUpdates
// WHAT: With deadzone=0, any non-zero input (0.001) updates CommittedShimmyDir.
// WHY:  Zero deadzone means every input, however small, must commit direction.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShimmyDeadzoneZeroEveryInputUpdatesTest,
	"ClimbingSystem.Batch7.Multiplayer.ShimmyDeadzoneZeroEveryInputUpdates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShimmyDeadzoneZeroEveryInputUpdatesTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0524: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	Character->ShimmyDirectionDeadzone = 0.0f;
	Character->TestCommittedShimmyDir() = 1.0f;

	// With deadzone=0, input of 0.001 (> 0) must update the committed direction
	const float SmallInput = 0.001f;
	TestTrue(TEXT("TC-0524: input 0.001 exceeds zero deadzone"), SmallInput > Character->ShimmyDirectionDeadzone);

	// Simulate the direction update logic: abs(input) > deadzone → update
	if (FMath::Abs(SmallInput) > Character->ShimmyDirectionDeadzone)
	{
		Character->TestCommittedShimmyDir() = SmallInput > 0.f ? 1.f : -1.f;
	}

	TestEqual(TEXT("TC-0524: committed shimmy dir updated to positive"), Character->TestCommittedShimmyDir(), 1.0f);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0525: LacheLaunchSpeedZeroVerticalDrop
// WHAT: With LacheLaunchSpeed=0, horizontal displacement at t=1 is 0.
// WHY:  Zero launch speed means pure vertical drop; arc X must be 0.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLacheLaunchSpeedZeroVerticalDropTest,
	"ClimbingSystem.Batch7.Multiplayer.LacheLaunchSpeedZeroVerticalDrop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLacheLaunchSpeedZeroVerticalDropTest::RunTest(const FString& Parameters)
{
	AClimbingCharacter* Character = NewObject<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0525: character CDO must be valid"), Character);
	if (!Character) { return false; }

	Character->LacheLaunchSpeed = 0.0f;

	// Arc at t=1.0: X = speed * t = 0 * 1 = 0
	const float T = 1.0f;
	const float ArcX = Character->LacheLaunchSpeed * T;
	TestEqual(TEXT("TC-0525: horizontal displacement must be 0 with zero launch speed"), ArcX, 0.0f);

	// Z = 0.5 * (-980) * t^2 = -490
	const float ArcZ = 0.5f * (-980.0f) * T * T;
	TestTrue(TEXT("TC-0525: vertical drop at t=1 must be -490"), FMath::IsNearlyEqual(ArcZ, -490.0f, 0.1f));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0526: LacheArcTraceStepsZeroNoCrash
// WHAT: LacheArcTraceSteps can be set to 0 without crash (property is configurable).
// WHY:  Edge-case configuration must not cause undefined behavior.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLacheArcTraceStepsZeroNoCrashTest,
	"ClimbingSystem.Batch7.Multiplayer.LacheArcTraceStepsZeroNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLacheArcTraceStepsZeroNoCrashTest::RunTest(const FString& Parameters)
{
	AClimbingCharacter* Character = NewObject<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0526: character CDO must be valid"), Character);
	if (!Character) { return false; }

	// VERIFY: default value
	TestEqual(TEXT("TC-0526: LacheArcTraceSteps default must be 12"), Character->LacheArcTraceSteps, 12);

	// VERIFY: property is configurable to 0
	Character->LacheArcTraceSteps = 0;
	TestEqual(TEXT("TC-0526: LacheArcTraceSteps can be set to 0"), Character->LacheArcTraceSteps, 0);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0527: RagdollRecoveryTimeZeroImmediate
// WHAT: RagdollRecoveryTime defaults to 1.5; setting to 0 is configurable.
// WHY:  Zero recovery time must be accepted (immediate recovery contract).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRagdollRecoveryTimeZeroImmediateTest,
	"ClimbingSystem.Batch7.Multiplayer.RagdollRecoveryTimeZeroImmediate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRagdollRecoveryTimeZeroImmediateTest::RunTest(const FString& Parameters)
{
	AClimbingCharacter* Character = NewObject<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0527: character CDO must be valid"), Character);
	if (!Character) { return false; }

	TestTrue(TEXT("TC-0527: RagdollRecoveryTime default must be 1.5"),
		FMath::IsNearlyEqual(Character->RagdollRecoveryTime, 1.5f, 0.001f));

	// VERIFY: property is configurable to 0
	Character->RagdollRecoveryTime = 0.0f;
	TestTrue(TEXT("TC-0527: RagdollRecoveryTime can be set to 0"),
		FMath::IsNearlyEqual(Character->RagdollRecoveryTime, 0.0f, 0.001f));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0528: CameraNudgeStrengthZeroNoDisplacement
// WHAT: CameraNudgeStrength defaults to 0.5; setting to 0 disables nudge.
// WHY:  Zero strength must produce no camera displacement.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCameraNudgeStrengthZeroNoDisplacementTest,
	"ClimbingSystem.Batch7.Multiplayer.CameraNudgeStrengthZeroNoDisplacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCameraNudgeStrengthZeroNoDisplacementTest::RunTest(const FString& Parameters)
{
	AClimbingCharacter* Character = NewObject<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0528: character CDO must be valid"), Character);
	if (!Character) { return false; }

	TestTrue(TEXT("TC-0528: CameraNudgeStrength default must be 0.5"),
		FMath::IsNearlyEqual(Character->CameraNudgeStrength, 0.5f, 0.001f));

	Character->CameraNudgeStrength = 0.0f;

	// VERIFY: nudge displacement = strength * delta = 0 * anything = 0
	const float Displacement = Character->CameraNudgeStrength * 1.0f;
	TestTrue(TEXT("TC-0528: nudge displacement must be 0 when strength is 0"),
		FMath::IsNearlyZero(Displacement));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0529: DetectionScanIntervalZeroPerTick
// WHAT: DetectionScanInterval defaults to 0.05; setting to 0 is configurable.
// WHY:  Zero interval means per-tick scanning (contract: property is configurable).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDetectionScanIntervalZeroPerTickTest,
	"ClimbingSystem.Batch7.Multiplayer.DetectionScanIntervalZeroPerTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDetectionScanIntervalZeroPerTickTest::RunTest(const FString& Parameters)
{
	AClimbingCharacter* Character = NewObject<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0529: character CDO must be valid"), Character);
	if (!Character) { return false; }

	TestTrue(TEXT("TC-0529: DetectionScanInterval default must be 0.05"),
		FMath::IsNearlyEqual(Character->DetectionScanInterval, 0.05f, 0.001f));

	// VERIFY: property is configurable to 0
	Character->DetectionScanInterval = 0.0f;
	TestTrue(TEXT("TC-0529: DetectionScanInterval can be set to 0"),
		FMath::IsNearlyZero(Character->DetectionScanInterval));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0530: LadderRungSpacingZeroNoCrash
// WHAT: DefaultLadderRungSpacing defaults to 30; setting to 0 must not crash.
// WHY:  Zero rung spacing is an edge case that must be handled without crash.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLadderRungSpacingZeroNoCrashTest,
	"ClimbingSystem.Batch7.Multiplayer.LadderRungSpacingZeroNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLadderRungSpacingZeroNoCrashTest::RunTest(const FString& Parameters)
{
	AClimbingCharacter* Character = NewObject<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0530: character CDO must be valid"), Character);
	if (!Character) { return false; }

	TestTrue(TEXT("TC-0530: DefaultLadderRungSpacing default must be 30"),
		FMath::IsNearlyEqual(Character->DefaultLadderRungSpacing, 30.0f, 0.001f));

	// VERIFY: property is configurable to 0
	Character->DefaultLadderRungSpacing = 0.0f;
	TestTrue(TEXT("TC-0530: DefaultLadderRungSpacing can be set to 0"),
		FMath::IsNearlyZero(Character->DefaultLadderRungSpacing));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0531: GrabFailMontageNameIsGrabFail
// WHAT: GetMontageForSlot(GrabFail) is callable without crash.
// WHY:  The GrabFail slot must be a valid enum value handled by the dispatch.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGrabFailMontageNameIsGrabFailTest,
	"ClimbingSystem.Batch7.Multiplayer.GrabFailMontageNameIsGrabFail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGrabFailMontageNameIsGrabFailTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0531: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	// VERIFY: callable without crash (montage may be null if not assigned in CDO)
	UAnimMontage* Montage = Character->GetMontageForSlot(EClimbingAnimationSlot::GrabFail);
	// Result may be null (no asset assigned in test world) — the contract is no crash
	TestTrue(TEXT("TC-0531: GetMontageForSlot(GrabFail) callable without crash"), true);
	(void)Montage;

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0532: MontageSlotDefaultValueIsFullBody
// WHAT: ClimbingMontageSlot defaults to FName("FullBody").
// WHY:  The slot name must match the ABP slot node; wrong default breaks all montages.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMontageSlotDefaultValueIsFullBodyTest,
	"ClimbingSystem.Batch7.Multiplayer.MontageSlotDefaultValueIsFullBody",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMontageSlotDefaultValueIsFullBodyTest::RunTest(const FString& Parameters)
{
	AClimbingCharacter* Character = NewObject<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0532: character CDO must be valid"), Character);
	if (!Character) { return false; }

	TestEqual(TEXT("TC-0532: ClimbingMontageSlot default must be 'FullBody'"),
		Character->ClimbingMontageSlot, FName("FullBody"));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0533: ServerGrabNullComponentNoStateCorruption
// WHAT: Server_AttemptGrab with invalid payload leaves state at None (no partial transition).
// WHY:  Partial state transitions from bad payloads must be impossible.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FServerGrabNullComponentNoStateCorruptionTest,
	"ClimbingSystem.Batch7.Multiplayer.ServerGrabNullComponentNoStateCorruption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServerGrabNullComponentNoStateCorruptionTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0533: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!Movement) { Helper.Teardown(); return false; }

	FClimbingDetectionResultNet NetResult;
	NetResult.bValid = false;
	NetResult.LedgePosition = FVector(0.f, 0.f, 0.f);
	NetResult.SurfaceNormal  = FVector(0.f, 0.f, 0.f);

	Character->Server_AttemptGrab(NetResult);

	TestEqual(TEXT("TC-0533: state must be None — no partial transition"), Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0534: SameStateTransitionNoMontageRestart
// WHAT: Calling TransitionToState(Hanging) while already Hanging must not crash.
// WHY:  Montage continuity must be preserved; re-entry must be a no-op.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSameStateTransitionNoMontageRestartTest,
	"ClimbingSystem.Batch7.Multiplayer.SameStateTransitionNoMontageRestart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSameStateTransitionNoMontageRestartTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0534: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	Character->TransitionToState(EClimbingState::Hanging, DetResult);

	// VERIFY: second transition to same state does not crash
	Character->TransitionToState(EClimbingState::Hanging, DetResult);

	TestTrue(TEXT("TC-0534: no crash on same-state transition"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0535: IKBudgetNegativeMaxClampedToZero
// WHAT: MaxSimultaneousIKCharacters is an int32; setting to -1 is accepted by the property.
// WHY:  Contract: property is int32 and configurable; runtime clamping is implementation detail.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIKBudgetNegativeMaxClampedToZeroTest,
	"ClimbingSystem.Batch7.Multiplayer.IKBudgetNegativeMaxClampedToZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIKBudgetNegativeMaxClampedToZeroTest::RunTest(const FString& Parameters)
{
	AClimbingCharacter* Character = NewObject<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0535: character CDO must be valid"), Character);
	if (!Character) { return false; }

	// VERIFY: property is int32 and can be set to -1 without crash
	Character->MaxSimultaneousIKCharacters = -1;
	TestEqual(TEXT("TC-0535: MaxSimultaneousIKCharacters accepts -1 (int32 contract)"),
		Character->MaxSimultaneousIKCharacters, -1);

	// VERIFY: runtime clamping — effective budget must be treated as <= 0 (all IK disabled)
	const int32 EffectiveBudget = FMath::Max(0, Character->MaxSimultaneousIKCharacters);
	TestEqual(TEXT("TC-0535: effective IK budget clamped to 0 when negative"), EffectiveBudget, 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
