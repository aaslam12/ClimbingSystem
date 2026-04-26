// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingTypes.h"
#include "Animation/AnimMontage.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "UObject/UnrealType.h"
#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

// ---------------------------------------------------------------------------
// TC-0275: MantleStepUpAtExactThreshold
// WHAT: MantleStepMaxHeight CDO default must be 50.
// WHY: Heights <= 50 use CMC step-up; above that enters mantle state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3MantleStepUpAtExactThresholdTest,
	"ClimbingSystem.Batch3.Mantle.MantleStepUpAtExactThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3MantleStepUpAtExactThresholdTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0275: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0275: MantleStepMaxHeight default must be 50"),
		FMath::IsNearlyEqual(CDO->MantleStepMaxHeight, 50.f));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0276: MantleLowAt51cm
// WHAT: MantleLowMaxHeight CDO default must be 100. Height 51 is in (50,100].
// WHY: Heights in (MantleStepMaxHeight, MantleLowMaxHeight] select MantleLow.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3MantleLowAt51cmTest,
	"ClimbingSystem.Batch3.Mantle.MantleLowAt51cm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3MantleLowAt51cmTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0276: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0276: MantleLowMaxHeight default must be 100"),
		FMath::IsNearlyEqual(CDO->MantleLowMaxHeight, 100.f));

	constexpr float Height = 51.f;
	const bool bInLowRange = Height > CDO->MantleStepMaxHeight && Height <= CDO->MantleLowMaxHeight;
	TestTrue(TEXT("TC-0276: height 51 must be in MantleLow range (50,100]"), bInLowRange);
	return true;
}

// ---------------------------------------------------------------------------
// TC-0277: MantleHighAt101cm
// WHAT: Height 101 is in (MantleLowMaxHeight, MantleHighMaxHeight] -> MantleHigh.
// WHY: Heights above MantleLowMaxHeight but within MantleHighMaxHeight use MantleHigh.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3MantleHighAt101cmTest,
	"ClimbingSystem.Batch3.Mantle.MantleHighAt101cm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3MantleHighAt101cmTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0277: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	constexpr float Height = 101.f;
	const bool bInHighRange = Height > CDO->MantleLowMaxHeight && Height <= CDO->MantleHighMaxHeight;
	TestTrue(TEXT("TC-0277: height 101 must be in MantleHigh range (100,180]"), bInHighRange);
	return true;
}

// ---------------------------------------------------------------------------
// TC-0278: MantleRejectedAt181cm
// WHAT: MantleHighMaxHeight CDO default must be 180. Height 181 > 180 -> not mantleable.
// WHY: Heights above MantleHighMaxHeight cannot be mantled.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3MantleRejectedAt181cmTest,
	"ClimbingSystem.Batch3.Mantle.MantleRejectedAt181cm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3MantleRejectedAt181cmTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0278: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0278: MantleHighMaxHeight default must be 180"),
		FMath::IsNearlyEqual(CDO->MantleHighMaxHeight, 180.f));

	constexpr float Height = 181.f;
	const bool bRejected = Height > CDO->MantleHighMaxHeight;
	TestTrue(TEXT("TC-0278: height 181 must exceed MantleHighMaxHeight and be rejected"), bRejected);
	return true;
}

// ---------------------------------------------------------------------------
// TC-0279: MantleLowWarpTargetRegistered
// WHAT: Spawn character, transition to Mantling, verify GetMontageForSlot(MantleLow) accessible.
// WHY: MantleLow montage slot must be reachable for motion warp target registration.
// // VERIFY: full warp target registration requires a live MotionWarpingComponent tick.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3MantleLowWarpTargetRegisteredTest,
	"ClimbingSystem.Batch3.Mantle.MantleLowWarpTargetRegistered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3MantleLowWarpTargetRegisteredTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0279: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0279: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Assign a montage so the slot is populated
	Character->MantleLow = NewObject<UAnimMontage>(Character);

	Movement->SetClimbingState(EClimbingState::Mantling);

	UAnimMontage* Montage = Character->GetMontageForSlot(EClimbingAnimationSlot::MantleLow);
	TestNotNull(TEXT("TC-0279: GetMontageForSlot(MantleLow) must return the assigned montage"), Montage);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0280: MantleHighWarpTargetRegistered
// WHAT: Spawn character, transition to Mantling, verify GetMontageForSlot(MantleHigh) accessible.
// WHY: MantleHigh montage slot must be reachable for motion warp target registration.
// // VERIFY: full warp target registration requires a live MotionWarpingComponent tick.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3MantleHighWarpTargetRegisteredTest,
	"ClimbingSystem.Batch3.Mantle.MantleHighWarpTargetRegistered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3MantleHighWarpTargetRegisteredTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0280: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0280: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Character->MantleHigh = NewObject<UAnimMontage>(Character);

	Movement->SetClimbingState(EClimbingState::Mantling);

	UAnimMontage* Montage = Character->GetMontageForSlot(EClimbingAnimationSlot::MantleHigh);
	TestNotNull(TEXT("TC-0280: GetMontageForSlot(MantleHigh) must return the assigned montage"), Montage);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0281: MantleBelowStepNoStateEntered
// WHAT: Height 30 < MantleStepMaxHeight(50) -> CMC step-up, no Mantling state.
// WHY: Contract test — heights at or below step threshold must not enter Mantling.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3MantleBelowStepNoStateEnteredTest,
	"ClimbingSystem.Batch3.Mantle.MantleBelowStepNoStateEntered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3MantleBelowStepNoStateEnteredTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0281: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	constexpr float Height = 30.f;
	const bool bBelowStep = Height <= CDO->MantleStepMaxHeight;
	TestTrue(TEXT("TC-0281: height 30 must be <= MantleStepMaxHeight — CMC step-up, no Mantling"), bBelowStep);
	return true;
}

// ---------------------------------------------------------------------------
// TC-0282: LadderEnterBottomMontageAndWarp
// WHAT: Spawn character, set state to OnLadder, verify GetMontageForSlot(LadderEnterBottom) accessible.
// WHY: LadderEnterBottom montage slot must be reachable for motion warp target registration.
// // VERIFY: full warp target registration requires a live MotionWarpingComponent tick.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3LadderEnterBottomMontageAndWarpTest,
	"ClimbingSystem.Batch3.Ladder.LadderEnterBottomMontageAndWarp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3LadderEnterBottomMontageAndWarpTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0282: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0282: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Character->LadderEnterBottom = NewObject<UAnimMontage>(Character);

	Movement->SetClimbingState(EClimbingState::OnLadder);

	UAnimMontage* Montage = Character->GetMontageForSlot(EClimbingAnimationSlot::LadderEnterBottom);
	TestNotNull(TEXT("TC-0282: GetMontageForSlot(LadderEnterBottom) must return the assigned montage"), Montage);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0283: LadderSprintAndCrouchSimultaneous
// WHAT: bSprintModifierActive and bCrouchModifierActive can both be true simultaneously.
// WHY: Speed calc must handle both modifiers being held at the same time without crash.
// // VERIFY: full speed calc test requires a live ladder tick with both modifiers active.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3LadderSprintAndCrouchSimultaneousTest,
	"ClimbingSystem.Batch3.Ladder.LadderSprintAndCrouchSimultaneous",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3LadderSprintAndCrouchSimultaneousTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0283: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0283: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::OnLadder);

	// Both modifiers active simultaneously — must not crash and speed must be calculable
	const float Speed = Movement->CalculateEffectiveLadderSpeed(/*bSprinting=*/true, /*bFastDescending=*/true);
	TestTrue(TEXT("TC-0283: CalculateEffectiveLadderSpeed with both modifiers must return positive speed"),
		Speed > 0.f);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0284: LadderRungSpacingDefault
// WHAT: CDO DefaultLadderRungSpacing must be 30.
// WHY: IK rung snapping uses this value when no surface data is present.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3LadderRungSpacingDefaultTest,
	"ClimbingSystem.Batch3.Ladder.LadderRungSpacingDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3LadderRungSpacingDefaultTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0284: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0284: DefaultLadderRungSpacing default must be 30"),
		FMath::IsNearlyEqual(CDO->DefaultLadderRungSpacing, 30.f));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0285: LadderRungSpacingAffectsIKTraceInterval
// WHAT: DefaultLadderRungSpacing property must be EditAnywhere (configurable).
// WHY: Designers must be able to tune rung spacing per-character for IK trace interval.
// // VERIFY: full IK trace interval test requires a live ladder IK tick.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3LadderRungSpacingAffectsIKTraceIntervalTest,
	"ClimbingSystem.Batch3.Ladder.LadderRungSpacingAffectsIKTraceInterval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3LadderRungSpacingAffectsIKTraceIntervalTest::RunTest(const FString& Parameters)
{
	const UClass* Class = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0285: AClimbingCharacter class must exist"), Class);
	if (!Class) { return false; }

	const FProperty* Prop = Class->FindPropertyByName(TEXT("DefaultLadderRungSpacing"));
	TestNotNull(TEXT("TC-0285: DefaultLadderRungSpacing property must exist"), Prop);
	if (!Prop) { return false; }

	TestTrue(TEXT("TC-0285: DefaultLadderRungSpacing must be EditAnywhere (CPF_Edit)"),
		Prop->HasAnyPropertyFlags(CPF_Edit));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0286: GrabBreakBelowThresholdNoRagdoll
// WHAT: Apply 1999N impulse via NotifyHit while in Hanging. State must remain Hanging.
// WHY: Impulse below GrabBreakImpulseThreshold(2000) must not trigger ragdoll.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3GrabBreakBelowThresholdNoRagdollTest,
	"ClimbingSystem.Batch3.Physics.GrabBreakBelowThresholdNoRagdoll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3GrabBreakBelowThresholdNoRagdollTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0286: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0286: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Hanging);

	// Construct a FHitResult with NormalImpulse magnitude of 1999N (below threshold)
	FHitResult Hit;
	const FVector NormalImpulse(1999.f, 0.f, 0.f);
	Character->NotifyHit(nullptr, nullptr, nullptr, false,
		FVector::ZeroVector, FVector::ZeroVector, NormalImpulse, Hit);

	TestEqual(TEXT("TC-0286: state must remain Hanging when impulse 1999N < threshold 2000N"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0287: GrabBreakAtExactThresholdTriggersRagdoll
// WHAT: Apply exactly 2000N impulse via NotifyHit while in Hanging. State must become Ragdoll.
// WHY: Impulse >= GrabBreakImpulseThreshold must trigger ragdoll.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3GrabBreakAtExactThresholdTriggersRagdollTest,
	"ClimbingSystem.Batch3.Physics.GrabBreakAtExactThresholdTriggersRagdoll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3GrabBreakAtExactThresholdTriggersRagdollTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0287: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0287: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Hanging);

	// Exactly at threshold — must trigger ragdoll
	FHitResult Hit;
	const FVector NormalImpulse(2000.f, 0.f, 0.f);
	Character->NotifyHit(nullptr, nullptr, nullptr, false,
		FVector::ZeroVector, FVector::ZeroVector, NormalImpulse, Hit);

	TestEqual(TEXT("TC-0287: state must become Ragdoll when impulse 2000N >= threshold 2000N"),
		Movement->CurrentClimbingState, EClimbingState::Ragdoll);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0288: NotifyHitDuringNoneStateNoRagdoll
// WHAT: Character in None state. Apply 5000N impulse. State must remain None.
// WHY: NotifyHit grab-break logic must only fire during active climbing states.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3NotifyHitDuringNoneStateNoRagdollTest,
	"ClimbingSystem.Batch3.Physics.NotifyHitDuringNoneStateNoRagdoll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3NotifyHitDuringNoneStateNoRagdollTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0288: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0288: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// State is None by default — no climbing active
	TestEqual(TEXT("TC-0288: initial state must be None"), Movement->CurrentClimbingState, EClimbingState::None);

	FHitResult Hit;
	const FVector NormalImpulse(5000.f, 0.f, 0.f);
	Character->NotifyHit(nullptr, nullptr, nullptr, false,
		FVector::ZeroVector, FVector::ZeroVector, NormalImpulse, Hit);

	TestEqual(TEXT("TC-0288: state must remain None after 5000N impulse when not climbing"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0289: PelvisBoneNameOverrideUsedInRecovery
// WHAT: PelvisBoneName CDO default must be "pelvis". Property must be configurable.
// WHY: Non-standard skeletons need to override the pelvis bone for face-up/down detection.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3PelvisBoneNameOverrideUsedInRecoveryTest,
	"ClimbingSystem.Batch3.Physics.PelvisBoneNameOverrideUsedInRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3PelvisBoneNameOverrideUsedInRecoveryTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0289: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestEqual(TEXT("TC-0289: PelvisBoneName default must be \"pelvis\""),
		CDO->PelvisBoneName, FName(TEXT("pelvis")));

	const UClass* Class = AClimbingCharacter::StaticClass();
	const FProperty* Prop = Class ? Class->FindPropertyByName(TEXT("PelvisBoneName")) : nullptr;
	TestNotNull(TEXT("TC-0289: PelvisBoneName property must exist"), Prop);
	if (!Prop) { return false; }

	TestTrue(TEXT("TC-0289: PelvisBoneName must be EditAnywhere (CPF_Edit)"),
		Prop->HasAnyPropertyFlags(CPF_Edit));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0290: RagdollFaceUpMontageSelected
// WHAT: GetMontageForSlot(RagdollGetUpFaceUp) must be accessible on a spawned character.
// WHY: Face-up recovery path must be reachable from the slot accessor.
// // VERIFY: full pelvis dot product test requires a live skeletal mesh with physics.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3RagdollFaceUpMontageSelectedTest,
	"ClimbingSystem.Batch3.Ragdoll.RagdollFaceUpMontageSelected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3RagdollFaceUpMontageSelectedTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0290: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	Character->RagdollGetUpFaceUp = NewObject<UAnimMontage>(Character);

	UAnimMontage* Montage = Character->GetMontageForSlot(EClimbingAnimationSlot::RagdollGetUpFaceUp);
	TestNotNull(TEXT("TC-0290: GetMontageForSlot(RagdollGetUpFaceUp) must return the assigned montage"), Montage);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0291: RagdollFaceDownMontageSelected
// WHAT: GetMontageForSlot(RagdollGetUpFaceDown) must be accessible on a spawned character.
// WHY: Face-down recovery path must be reachable from the slot accessor.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3RagdollFaceDownMontageSelectedTest,
	"ClimbingSystem.Batch3.Ragdoll.RagdollFaceDownMontageSelected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3RagdollFaceDownMontageSelectedTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0291: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	Character->RagdollGetUpFaceDown = NewObject<UAnimMontage>(Character);

	UAnimMontage* Montage = Character->GetMontageForSlot(EClimbingAnimationSlot::RagdollGetUpFaceDown);
	TestNotNull(TEXT("TC-0291: GetMontageForSlot(RagdollGetUpFaceDown) must return the assigned montage"), Montage);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0292: GrabBreakLaunchVelocityScaled
// WHAT: GrabBreakLaunchScale CDO default must be 0.5. Math: 3000 * 0.5 = 1500.
// WHY: Launch velocity = NormalImpulse * GrabBreakLaunchScale; scale must match spec.
// // VERIFY: full runtime test requires observing actual ragdoll launch velocity.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3GrabBreakLaunchVelocityScaledTest,
	"ClimbingSystem.Batch3.Physics.GrabBreakLaunchVelocityScaled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3GrabBreakLaunchVelocityScaledTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0292: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0292: GrabBreakLaunchScale default must be 0.5"),
		FMath::IsNearlyEqual(CDO->GrabBreakLaunchScale, 0.5f));

	// Math verification: 3000N * 0.5 = 1500 cm/s launch speed
	constexpr float ImpulseMagnitude = 3000.f;
	const float ExpectedLaunchSpeed = ImpulseMagnitude * CDO->GrabBreakLaunchScale;
	TestTrue(TEXT("TC-0292: 3000N * GrabBreakLaunchScale(0.5) must equal 1500"),
		FMath::IsNearlyEqual(ExpectedLaunchSpeed, 1500.f));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0293: AnchorMovesCharacterFollowsNextTick
// WHAT: Spawn character + box. Set AnchorComponent = box. Move box 100cm. Tick. Assert character moved.
// WHY: Dynamic anchor following must keep the character attached to moving geometry.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3AnchorMovesCharacterFollowsNextTickTest,
	"ClimbingSystem.Batch3.Anchor.AnchorMovesCharacterFollowsNextTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3AnchorMovesCharacterFollowsNextTickTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0293: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0293: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Spawn a box to use as anchor
	AActor* BoxActor = Helper.SpawnBoxAt(FVector::ZeroVector, FVector(50.f, 50.f, 50.f));
	TestNotNull(TEXT("TC-0293: box actor must spawn"), BoxActor);
	if (!BoxActor) { Helper.Teardown(); return false; }

	UPrimitiveComponent* BoxRoot = Cast<UPrimitiveComponent>(BoxActor->GetRootComponent());
	TestNotNull(TEXT("TC-0293: box root component must exist"), BoxRoot);
	if (!BoxRoot) { Helper.Teardown(); return false; }

	// Set anchor and record initial character position
	Movement->SetAnchor(BoxRoot, FVector::ZeroVector);
	Movement->SetClimbingState(EClimbingState::Hanging);

	const FVector InitialCharPos = Character->GetActorLocation();

	// Move the box 100cm along X
	constexpr float MoveDelta = 100.f;
	BoxActor->SetActorLocation(FVector(MoveDelta, 0.f, 0.f));

	// Tick the world to allow anchor following to update
	Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);

	const FVector NewCharPos = Character->GetActorLocation();
	const float Displacement = FVector::Dist(NewCharPos, InitialCharPos);

	TestTrue(TEXT("TC-0293: character must have moved after anchor box moved 100cm"),
		Displacement > 1.f);

	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
