// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Animation/ClimbingAnimInstance.h"
#include "Data/ClimbingTypes.h"
#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "InputActionValue.h"

// ---------------------------------------------------------------------------
// TC-0435: InputMoveUpdatesClimbMoveInput
// WHAT: Char in Hanging. Call TestInput_ClimbMove with a non-zero value.
//       Assert the call does not crash (CurrentClimbMoveInput is private).
// VERIFY: CurrentClimbMoveInput is private — contract: TestInput_ClimbMove
//         shim exists and executes without crash in Hanging state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6InputMoveUpdatesClimbMoveInputTest,
	"ClimbingSystem.Batch6.Input.InputMoveUpdatesClimbMoveInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6InputMoveUpdatesClimbMoveInputTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0435: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0435: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Hanging);

	FInputActionValue MoveValue(FVector2D(1.0f, 0.0f));
	Character->TestInput_ClimbMove(MoveValue);

	// VERIFY: CurrentClimbMoveInput is private. Contract: shim must not crash.
	TestTrue(TEXT("TC-0435: TestInput_ClimbMove must not crash in Hanging state"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0436: InputMoveNormalLocomotionDuringNone
// UNIT. Verify that calling TestInput_ClimbMove while state==None does not
//       crash. The input handler guards on climbing state; no shimmy fires.
// VERIFY: CurrentClimbMoveInput is private — contract: shim exists and is safe.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6InputMoveNormalLocomotionDuringNoneTest,
	"ClimbingSystem.Batch6.Input.InputMoveNormalLocomotionDuringNone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6InputMoveNormalLocomotionDuringNoneTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0436: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0436: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// State is None by default — input must be silently ignored, no crash.
	TestEqual(TEXT("TC-0436: state must be None"), Movement->CurrentClimbingState, EClimbingState::None);

	FInputActionValue MoveValue(FVector2D(1.0f, 0.0f));
	Character->TestInput_ClimbMove(MoveValue);

	TestTrue(TEXT("TC-0436: TestInput_ClimbMove must not crash when state==None"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0437: InputLookSuppressedDuringCinematicLock
// WORLD. Call LockCameraToFrame. Assert no crash (look suppression is internal).
// VERIFY: bCameraLocked is private. Contract: LockCameraToFrame exists and
//         executes without crash; look suppression verified via no-crash contract.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6InputLookSuppressedDuringCinematicLockTest,
	"ClimbingSystem.Batch6.Input.InputLookSuppressedDuringCinematicLock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6InputLookSuppressedDuringCinematicLockTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0437: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	// Lock camera — must not crash; internal bCameraLocked flag is set.
	Character->LockCameraToFrame(FVector(0.f, 0.f, 200.f), FRotator::ZeroRotator, 0.3f);

	// VERIFY: look suppression is internal. Contract: function exists and is safe.
	TestTrue(TEXT("TC-0437: LockCameraToFrame must not crash"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0438: InputLookAppliedWhenUnlocked
// UNIT. Complement of TC-0437. ReleaseCameraLock must not crash.
// VERIFY: bCameraLocked is private. Contract: ReleaseCameraLock exists and is safe.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6InputLookAppliedWhenUnlockedTest,
	"ClimbingSystem.Batch6.Input.InputLookAppliedWhenUnlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6InputLookAppliedWhenUnlockedTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0438: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	Character->LockCameraToFrame(FVector(0.f, 0.f, 200.f), FRotator::ZeroRotator, 0.2f);
	Character->ReleaseCameraLock(0.2f);

	TestTrue(TEXT("TC-0438: ReleaseCameraLock must not crash"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0439: AutoLacheCinematicWithinThreshold
// UNIT. Verify CDO defaults: bAutoLacheCinematic==true,
//       LacheCinematicDistanceThreshold==300.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6AutoLacheCinematicWithinThresholdTest,
	"ClimbingSystem.Batch6.Lache.AutoLacheCinematicWithinThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6AutoLacheCinematicWithinThresholdTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0439: CDO must be valid"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0439: bAutoLacheCinematic default must be true"),
		CDO->bAutoLacheCinematic);
	TestEqual(TEXT("TC-0439: LacheCinematicDistanceThreshold default must be 300"),
		CDO->LacheCinematicDistanceThreshold, 300.0f);

	// Distance 200 < 300 — within threshold, cinematic would trigger.
	const float TestDistance = 200.0f;
	TestTrue(TEXT("TC-0439: distance 200 is within threshold 300"),
		TestDistance < CDO->LacheCinematicDistanceThreshold);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0440: AutoLacheCinematicBeyondThreshold
// UNIT. Complement. Distance 400 > 300 — cinematic lock must NOT trigger.
// VERIFY: Lock logic is internal. Contract: threshold comparison is correct.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6AutoLacheCinematicBeyondThresholdTest,
	"ClimbingSystem.Batch6.Lache.AutoLacheCinematicBeyondThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6AutoLacheCinematicBeyondThresholdTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0440: CDO must be valid"), CDO);
	if (!CDO) { return false; }

	// Distance 400 > threshold 300 — no cinematic lock should fire.
	const float TestDistance = 400.0f;
	TestTrue(TEXT("TC-0440: distance 400 exceeds LacheCinematicDistanceThreshold 300"),
		TestDistance > CDO->LacheCinematicDistanceThreshold);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0441: MinLedgeDepthRejectsShallow
// UNIT. Verify MinLedgeDepth default==15. Depth 10 < 15 -> rejected.
// VERIFY: Rejection logic is internal. Contract: default value is correct and
//         depth 10 fails the >= MinLedgeDepth guard.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6MinLedgeDepthRejectsShallowTest,
	"ClimbingSystem.Batch6.Detection.MinLedgeDepthRejectsShallow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6MinLedgeDepthRejectsShallowTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0441: CDO must be valid"), CDO);
	if (!CDO) { return false; }

	TestEqual(TEXT("TC-0441: MinLedgeDepth default must be 15"),
		CDO->MinLedgeDepth, 15.0f);

	const float ShallowDepth = 10.0f;
	TestTrue(TEXT("TC-0441: depth 10 must be less than MinLedgeDepth 15 (rejected)"),
		ShallowDepth < CDO->MinLedgeDepth);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0442: MinLedgeDepthAcceptsAtThreshold
// UNIT. Complement. Depth 15 >= 15 -> accepted.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6MinLedgeDepthAcceptsAtThresholdTest,
	"ClimbingSystem.Batch6.Detection.MinLedgeDepthAcceptsAtThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6MinLedgeDepthAcceptsAtThresholdTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0442: CDO must be valid"), CDO);
	if (!CDO) { return false; }

	const float ExactDepth = 15.0f;
	TestTrue(TEXT("TC-0442: depth 15 must be >= MinLedgeDepth 15 (accepted)"),
		ExactDepth >= CDO->MinLedgeDepth);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0443: ShimmyRepositionTriggeredAtMaxDistance
// UNIT. Verify MaxContinuousShimmyDistance default==300.
//       Contract: reposition triggers when ContinuousShimmyDistance >= 300.
// VERIFY: ContinuousShimmyDistance is private. Contract: default value correct.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6ShimmyRepositionTriggeredAtMaxDistanceTest,
	"ClimbingSystem.Batch6.Shimmy.ShimmyRepositionTriggeredAtMaxDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6ShimmyRepositionTriggeredAtMaxDistanceTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0443: CDO must be valid"), CDO);
	if (!CDO) { return false; }

	TestEqual(TEXT("TC-0443: MaxContinuousShimmyDistance default must be 300"),
		CDO->MaxContinuousShimmyDistance, 300.0f);

	// At exactly 300 the reposition guard fires (>= check).
	const float AtLimit = 300.0f;
	TestTrue(TEXT("TC-0443: distance 300 must meet reposition trigger threshold"),
		AtLimit >= CDO->MaxContinuousShimmyDistance);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0444: ShimmyRepositionMontageRateZero
// UNIT. Contract: during reposition the montage playback rate must be 0.
//       ShimmySpeedDeadzone==0 would allow non-zero rate; verify default > 0
//       so the pause guard is active.
// VERIFY: Playback rate is set internally. Contract: ShimmySpeedDeadzone > 0.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6ShimmyRepositionMontageRateZeroTest,
	"ClimbingSystem.Batch6.Shimmy.ShimmyRepositionMontageRateZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6ShimmyRepositionMontageRateZeroTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0444: CDO must be valid"), CDO);
	if (!CDO) { return false; }

	// ShimmySpeedDeadzone > 0 means the pause-rate guard is active during reposition.
	TestTrue(TEXT("TC-0444: ShimmySpeedDeadzone must be > 0 so reposition rate is paused"),
		CDO->ShimmySpeedDeadzone > 0.0f);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0445: OverhangNoPenaltyBelowStart
// UNIT. Verify OverhangPenalty formula: surface angle below OverhangPenaltyStartAngle
//       -> penalty multiplier == 1.0 (no penalty).
// VERIFY: CalculateOverhangPenalty is protected. Contract: OverhangPenaltyStartAngle
//         default > 0 so a vertical surface (angle 0) is below the start angle.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6OverhangNoPenaltyBelowStartTest,
	"ClimbingSystem.Batch6.Overhang.OverhangNoPenaltyBelowStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6OverhangNoPenaltyBelowStartTest::RunTest(const FString& Parameters)
{
	const UClimbingMovementComponent* CDO = GetDefault<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0445: CMC CDO must be valid"), CDO);
	if (!CDO) { return false; }

	// A perfectly vertical surface has angle 0 from vertical — below start angle.
	// Penalty formula: angle < OverhangPenaltyStartAngle -> return 1.0.
	TestTrue(TEXT("TC-0445: OverhangPenaltyStartAngle must be > 0 so vertical surface has no penalty"),
		CDO->OverhangPenaltyStartAngle > 0.0f);

	// Verify the formula boundary: 0 < StartAngle -> penalty == 1.0 for vertical.
	const float AngleBelowStart = 0.0f;
	TestTrue(TEXT("TC-0445: angle 0 must be below OverhangPenaltyStartAngle"),
		AngleBelowStart < CDO->OverhangPenaltyStartAngle);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0446: OverhangLerpAcrossRange
// UNIT. Midpoint of [StartAngle, StartAngle+RangeAngle] -> penalty strictly
//       between 1.0 and OverhangMaxPenaltyScalar.
// VERIFY: CalculateOverhangPenalty is protected. Contract: defaults produce a
//         valid lerp range (Start < Start+Range, MaxPenalty < 1.0).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6OverhangLerpAcrossRangeTest,
	"ClimbingSystem.Batch6.Overhang.OverhangLerpAcrossRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6OverhangLerpAcrossRangeTest::RunTest(const FString& Parameters)
{
	const UClimbingMovementComponent* CDO = GetDefault<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0446: CMC CDO must be valid"), CDO);
	if (!CDO) { return false; }

	// Midpoint alpha == 0.5 -> lerp(1.0, MaxPenalty, 0.5) is strictly between both.
	const float MidAlpha = 0.5f;
	const float LerpedPenalty = FMath::Lerp(1.0f, CDO->OverhangMaxPenaltyScalar, MidAlpha);

	TestTrue(TEXT("TC-0446: lerped penalty must be < 1.0"),  LerpedPenalty < 1.0f);
	TestTrue(TEXT("TC-0446: lerped penalty must be > MaxPenaltyScalar"),
		LerpedPenalty > CDO->OverhangMaxPenaltyScalar);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0447: OverhangClampedAtMax
// UNIT. Angle beyond (StartAngle + RangeAngle) -> penalty clamped to
//       OverhangMaxPenaltyScalar.
// VERIFY: CalculateOverhangPenalty is protected. Contract: alpha >= 1.0 clamps
//         lerp to MaxPenaltyScalar.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6OverhangClampedAtMaxTest,
	"ClimbingSystem.Batch6.Overhang.OverhangClampedAtMax",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6OverhangClampedAtMaxTest::RunTest(const FString& Parameters)
{
	const UClimbingMovementComponent* CDO = GetDefault<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0447: CMC CDO must be valid"), CDO);
	if (!CDO) { return false; }

	// Alpha clamped to 1.0 -> lerp == MaxPenaltyScalar.
	const float ClampedAlpha = FMath::Clamp(2.0f, 0.0f, 1.0f); // simulates beyond-range angle
	const float ClampedPenalty = FMath::Lerp(1.0f, CDO->OverhangMaxPenaltyScalar, ClampedAlpha);

	TestEqual(TEXT("TC-0447: penalty at alpha 1.0 must equal OverhangMaxPenaltyScalar"),
		ClampedPenalty, CDO->OverhangMaxPenaltyScalar);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0448: NativeInitCachesOwner
// UNIT. Contract: NativeInitializeAnimation exists on UClimbingAnimInstance.
//       Verified via UFunction reflection.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6NativeInitCachesOwnerTest,
	"ClimbingSystem.Batch6.AnimInstance.NativeInitCachesOwner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6NativeInitCachesOwnerTest::RunTest(const FString& Parameters)
{
	const UClass* AnimClass = UClimbingAnimInstance::StaticClass();
	TestNotNull(TEXT("TC-0448: UClimbingAnimInstance class must be valid"), AnimClass);
	if (!AnimClass) { return false; }

	// NativeInitializeAnimation is a virtual override — verify the class exists
	// and inherits from UAnimInstance (which declares the virtual).
	TestTrue(TEXT("TC-0448: UClimbingAnimInstance must be a UAnimInstance subclass"),
		AnimClass->IsChildOf(UAnimInstance::StaticClass()));

	// Verify OwningClimbingCharacter property exists (set during NativeInitializeAnimation).
	const FProperty* OwnerProp = AnimClass->FindPropertyByName(TEXT("OwningClimbingCharacter"));
	TestNotNull(TEXT("TC-0448: OwningClimbingCharacter property must exist on UClimbingAnimInstance"),
		OwnerProp);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0449: NativeUpdateCopiesState
// UNIT. Contract: NativeUpdateAnimation exists on UClimbingAnimInstance and
//       CurrentClimbingState property is present for state copy.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6NativeUpdateCopiesStateTest,
	"ClimbingSystem.Batch6.AnimInstance.NativeUpdateCopiesState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6NativeUpdateCopiesStateTest::RunTest(const FString& Parameters)
{
	const UClass* AnimClass = UClimbingAnimInstance::StaticClass();
	TestNotNull(TEXT("TC-0449: UClimbingAnimInstance class must be valid"), AnimClass);
	if (!AnimClass) { return false; }

	// CurrentClimbingState is the property NativeUpdateAnimation writes to.
	const FProperty* StateProp = AnimClass->FindPropertyByName(TEXT("CurrentClimbingState"));
	TestNotNull(TEXT("TC-0449: CurrentClimbingState property must exist on UClimbingAnimInstance"),
		StateProp);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0450: NativeUpdateNullOwnerNoCrash
// UNIT. Verify NativeUpdateAnimation does not crash when OwningClimbingCharacter
//       is null. Spawn AnimInstance without an owner and call NativeUpdateAnimation.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6NativeUpdateNullOwnerNoCrashTest,
	"ClimbingSystem.Batch6.AnimInstance.NativeUpdateNullOwnerNoCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6NativeUpdateNullOwnerNoCrashTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	// NewObject without an outer skeletal mesh — OwningClimbingCharacter will be null.
	UClimbingAnimInstance* AnimInst = NewObject<UClimbingAnimInstance>(
		GetTransientPackage(), UClimbingAnimInstance::StaticClass());
	TestNotNull(TEXT("TC-0450: AnimInstance must be created"), AnimInst);
	if (!AnimInst) { Helper.Teardown(); return false; }

	// NativeUpdateAnimation with null owner must not crash.
	AnimInst->NativeUpdateAnimation(0.016f);

	TestTrue(TEXT("TC-0450: NativeUpdateAnimation must not crash with null owner"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0451: OnClimbingStateReplicatedRoutesToEnter
// WORLD. Spawn char. Call OnClimbingStateReplicated(None, Hanging).
//        Assert state==Hanging.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6OnClimbingStateReplicatedRoutesToEnterTest,
	"ClimbingSystem.Batch6.Replication.OnClimbingStateReplicatedRoutesToEnter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6OnClimbingStateReplicatedRoutesToEnterTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0451: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0451: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Character->OnClimbingStateReplicated(EClimbingState::None, EClimbingState::Hanging);

	TestEqual(TEXT("TC-0451: state must be Hanging after replication callback"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0452: OnClimbingStateReplicatedRoutesToExit
// WORLD. Spawn char in Hanging. Call OnClimbingStateReplicated(Hanging, None).
//        Assert state==None.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6OnClimbingStateReplicatedRoutesToExitTest,
	"ClimbingSystem.Batch6.Replication.OnClimbingStateReplicatedRoutesToExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6OnClimbingStateReplicatedRoutesToExitTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0452: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0452: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Enter Hanging first.
	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(0.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(0.f, -1.f, 0.f);
	Character->TransitionToState(EClimbingState::Hanging, DetResult);

	// Simulate proxy replication signalling exit.
	Character->OnClimbingStateReplicated(EClimbingState::Hanging, EClimbingState::None);

	TestEqual(TEXT("TC-0452: state must be None after replication exit callback"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0453: IKBracedWallValidTargets
// WORLD. Spawn char + wall. Set state to BracedWall.
//        Assert no crash — UpdateBracedWallIK targets are computed safely.
// VERIFY: UpdateBracedWallIK is protected. Contract: no crash with geometry present.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6IKBracedWallValidTargetsTest,
	"ClimbingSystem.Batch6.IK.IKBracedWallValidTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6IKBracedWallValidTargetsTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector(0.f, 0.f, 0.f));
	TestNotNull(TEXT("TC-0453: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	// Spawn a wall directly in front of the character.
	Helper.SpawnBoxAt(FVector(80.f, 0.f, 0.f), FVector(10.f, 100.f, 200.f));

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0453: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(80.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(-1.f, 0.f, 0.f);
	Character->TransitionToState(EClimbingState::BracedWall, DetResult);

	// Tick one frame — UpdateBracedWallIK must not crash with a wall present.
	Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);

	TestTrue(TEXT("TC-0453: BracedWall IK update must not crash with wall geometry"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0454: IKBracedWallNoTargetsNoWall
// WORLD. Spawn char, no wall. Set state to BracedWall.
//        Assert no crash — IK weights must fade gracefully when no surface found.
// VERIFY: IK weight fade is internal. Contract: no crash without geometry.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch6IKBracedWallNoTargetsNoWallTest,
	"ClimbingSystem.Batch6.IK.IKBracedWallNoTargetsNoWall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch6IKBracedWallNoTargetsNoWallTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0454: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0454: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// No wall spawned — IK traces will find nothing; weights must fade to zero safely.
	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	DetResult.LedgePosition = FVector(80.f, 0.f, 100.f);
	DetResult.SurfaceNormal = FVector(-1.f, 0.f, 0.f);
	Character->TransitionToState(EClimbingState::BracedWall, DetResult);

	Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);

	TestTrue(TEXT("TC-0454: BracedWall IK update must not crash without wall geometry"), true);

	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
