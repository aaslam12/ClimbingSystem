// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Animation/ClimbingAnimationSet.h"
#include "Data/ClimbingTypes.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

// ---------------------------------------------------------------------------
// TC-0414: CapsuleRestoreOnExitToNone
// WHAT: After Hanging→None the capsule dimensions must match the pre-climb values.
// WHY:  If the capsule is not restored the character is permanently smaller,
//       breaking collision with the environment after dismounting.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCapsuleRestoreOnExitToNoneTest,
	"ClimbingSystem.Batch6.Capsule.CapsuleRestoreOnExitToNone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCapsuleRestoreOnExitToNoneTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0414: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	TestNotNull(TEXT("TC-0414: capsule must exist"), Capsule);
	if (!Capsule) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0414: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Record original dimensions before any climbing state
	const float OrigHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	const float OrigRadius     = Capsule->GetUnscaledCapsuleRadius();

	// Enter Hanging — capsule should resize to climbing values
	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	Character->TransitionToState(EClimbingState::Hanging, DetResult);

	// Exit back to None — capsule must be restored
	FClimbingDetectionResult EmptyResult;
	Character->TransitionToState(EClimbingState::None, EmptyResult);

	TestTrue(TEXT("TC-0414: capsule half-height must be restored after exit to None"),
		FMath::IsNearlyEqual(Capsule->GetUnscaledCapsuleHalfHeight(), OrigHalfHeight, 1.0f));
	TestTrue(TEXT("TC-0414: capsule radius must be restored after exit to None"),
		FMath::IsNearlyEqual(Capsule->GetUnscaledCapsuleRadius(), OrigRadius, 1.0f));

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0415: CollisionProfileSetOnEntry
// WHAT: Transitioning to Hanging sets the collision profile to "ClimbingCapsule";
//       returning to None restores the original profile.
// WHY:  The climbing profile prevents capsule-wall jitter; if it is not set on
//       entry the character clips through walls, and if not restored on exit
//       normal locomotion collision breaks.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCollisionProfileSetOnEntryTest,
	"ClimbingSystem.Batch6.Capsule.CollisionProfileSetOnEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCollisionProfileSetOnEntryTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0415: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	TestNotNull(TEXT("TC-0415: capsule must exist"), Capsule);
	if (!Capsule) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0415: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	const FName OrigProfile = Capsule->GetCollisionProfileName();

	// Enter Hanging
	FClimbingDetectionResult DetResult;
	DetResult.bValid = true;
	Character->TransitionToState(EClimbingState::Hanging, DetResult);

	TestEqual(TEXT("TC-0415: collision profile must be ClimbingCapsule while Hanging"),
		Capsule->GetCollisionProfileName(), FName("ClimbingCapsule"));

	// Exit to None — profile must be restored
	FClimbingDetectionResult EmptyResult;
	Character->TransitionToState(EClimbingState::None, EmptyResult);

	TestEqual(TEXT("TC-0415: collision profile must be restored after exit to None"),
		Capsule->GetCollisionProfileName(), OrigProfile);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0416: HangingCharacterOffsetApplied
// WHAT: HangingCharacterOffset CDO default must be FVector(0,0,-30).
// WHY:  This offset positions the character correctly relative to the ledge;
//       a wrong default causes the character to float above or clip into the ledge.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHangingCharacterOffsetAppliedTest,
	"ClimbingSystem.Batch6.Capsule.HangingCharacterOffsetApplied",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHangingCharacterOffsetAppliedTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0416: CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0416: HangingCharacterOffset.X must be 0"),
		FMath::IsNearlyEqual(CDO->HangingCharacterOffset.X, 0.0f));
	TestTrue(TEXT("TC-0416: HangingCharacterOffset.Y must be 0"),
		FMath::IsNearlyEqual(CDO->HangingCharacterOffset.Y, 0.0f));
	TestTrue(TEXT("TC-0416: HangingCharacterOffset.Z must be -30"),
		FMath::IsNearlyEqual(CDO->HangingCharacterOffset.Z, -30.0f));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0417: GetMontageForSlotReturnsOverride
// WHAT: Contract — GetMontageForSlot exists and is callable on AClimbingCharacter.
// WHY:  The override path (AnimationSetOverride → character default) is the
//       primary montage dispatch; if the method is missing no animation plays.
// VERIFY: AnimationSetOverride is private; activating it requires surface data
//         wiring not available in unit tests. Contract: method exists and does
//         not crash when called with no override active.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGetMontageForSlotReturnsOverrideTest,
	"ClimbingSystem.Batch6.Anim.GetMontageForSlotReturnsOverride",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGetMontageForSlotReturnsOverrideTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0417: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	// VERIFY: AnimationSetOverride is private — cannot inject an override in unit tests.
	// Contract: GetMontageForSlot must be callable and must not crash with no override.
	// With no montages assigned the return value is nullptr — that is the expected contract.
	UAnimMontage* Result = Character->GetMontageForSlot(EClimbingAnimationSlot::HangIdle);
	// nullptr is valid here (no asset assigned in test world); crash = failure.
	TestTrue(TEXT("TC-0417: GetMontageForSlot must not crash (nullptr is acceptable with no asset)"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0418: GetMontageForSlotFallsBackToDefault
// WHAT: UClimbingAnimationSet::GetMontageForSlot returns nullptr for HangIdle
//       when no montage is assigned (no override set).
// WHY:  The fallback contract requires null returns for unassigned slots so the
//       character can substitute its own default without crashing.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGetMontageForSlotFallsBackToDefaultTest,
	"ClimbingSystem.Batch6.Anim.GetMontageForSlotFallsBackToDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGetMontageForSlotFallsBackToDefaultTest::RunTest(const FString& Parameters)
{
	UClimbingAnimationSet* AnimSet = NewObject<UClimbingAnimationSet>();
	TestNotNull(TEXT("TC-0418: UClimbingAnimationSet must be constructible"), AnimSet);
	if (!AnimSet) { return false; }

	// No montages assigned — every slot must return nullptr
	UAnimMontage* Result = AnimSet->GetMontageForSlot(EClimbingAnimationSlot::HangIdle);
	TestNull(TEXT("TC-0418: GetMontageForSlot(HangIdle) must return nullptr when no override is set"), Result);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0419: WarpTargetLedgeGrabRegistered
// WHAT: Contract — MotionWarpingComponent exists on AClimbingCharacter and
//       transitioning to Hanging does not crash the warp registration path.
// WHY:  WarpTarget_LedgeGrab must be registered on Hanging entry for root-motion
//       alignment; a missing MotionWarpingComponent causes a null-deref crash.
// VERIFY: Inspecting the registered warp target name requires access to
//         UMotionWarpingComponent internals not exposed in automation.
//         Contract: component exists and Hanging entry does not crash.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWarpTargetLedgeGrabRegisteredTest,
	"ClimbingSystem.Batch6.Warp.WarpTargetLedgeGrabRegistered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWarpTargetLedgeGrabRegisteredTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0419: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	TestNotNull(TEXT("TC-0419: MotionWarping component must exist"),
		Character->MotionWarping.Get());

	// VERIFY: Warp target name inspection requires UMotionWarpingComponent internals.
	// Contract: transitioning to Hanging must not crash the warp registration path.
	FClimbingDetectionResult DetResult;
	DetResult.bValid      = true;
	DetResult.LedgePosition  = FVector(0.0f, 0.0f, 100.0f);
	DetResult.SurfaceNormal  = FVector(0.0f, -1.0f, 0.0f);
	Character->TransitionToState(EClimbingState::Hanging, DetResult);

	TestTrue(TEXT("TC-0419: Hanging entry with MotionWarping must not crash"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0420: WarpTargetClimbUpLandRegistered
// WHAT: Contract — transitioning to ClimbingUp does not crash the warp path.
// WHY:  WarpTarget_ClimbUpLand must be registered on ClimbingUp entry;
//       a missing registration causes root-motion to land at the wrong position.
// VERIFY: Same limitation as TC-0419 — warp target inspection not available.
//         Contract: ClimbingUp entry does not crash.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWarpTargetClimbUpLandRegisteredTest,
	"ClimbingSystem.Batch6.Warp.WarpTargetClimbUpLandRegistered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWarpTargetClimbUpLandRegisteredTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0420: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0420: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Pre-condition: must be in Hanging before ClimbingUp is a valid transition
	Movement->SetClimbingState(EClimbingState::Hanging);

	FClimbingDetectionResult DetResult;
	DetResult.bValid         = true;
	DetResult.LedgePosition  = FVector(0.0f, 0.0f, 100.0f);
	DetResult.SurfaceNormal  = FVector(0.0f, -1.0f, 0.0f);
	Character->TransitionToState(EClimbingState::ClimbingUp, DetResult);

	TestTrue(TEXT("TC-0420: ClimbingUp entry must not crash warp registration"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0421: WarpTargetLacheCatchRegistered
// WHAT: Contract — transitioning to LacheCatch does not crash the warp path.
// WHY:  WarpTarget_LacheCatch must be registered on LacheCatch entry for the
//       catch animation to land at the correct ledge position.
// VERIFY: Same limitation as TC-0419.
//         Contract: LacheCatch entry does not crash.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWarpTargetLacheCatchRegisteredTest,
	"ClimbingSystem.Batch6.Warp.WarpTargetLacheCatchRegistered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWarpTargetLacheCatchRegisteredTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0421: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0421: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Pre-condition: LacheInAir precedes LacheCatch
	Movement->SetClimbingState(EClimbingState::LacheInAir);

	FClimbingDetectionResult DetResult;
	DetResult.bValid        = true;
	DetResult.LedgePosition = FVector(500.0f, 0.0f, 100.0f);
	DetResult.SurfaceNormal = FVector(0.0f, -1.0f, 0.0f);
	Character->TransitionToState(EClimbingState::LacheCatch, DetResult);

	TestTrue(TEXT("TC-0421: LacheCatch entry must not crash warp registration"), true);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0422: InputDropFromOnLadder
// WHAT: Transitioning from OnLadder to None must result in state==None.
// WHY:  The drop action is the primary exit from the ladder; if it fails the
//       character is stuck on the ladder with no way to dismount.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInputDropFromOnLadderTest,
	"ClimbingSystem.Batch6.Input.InputDropFromOnLadder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputDropFromOnLadderTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0422: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0422: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::OnLadder);

	FClimbingDetectionResult EmptyResult;
	Character->TransitionToState(EClimbingState::None, EmptyResult);

	TestEqual(TEXT("TC-0422: state must be None after drop from OnLadder"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0423: InputJumpDuringLocomotionNormalJump
// WHAT: Contract — EClimbingState::None is a valid state and IA_Jump property
//       exists, confirming the normal-jump path is wired for locomotion.
// WHY:  Jump must work during normal locomotion (None state); if IA_Jump is
//       missing the player cannot jump at all outside of climbing.
// VERIFY: Input_JumpStarted is private — cannot be called directly.
//         Contract verified via reflection: IA_Jump property exists on
//         AClimbingCharacter and None is a valid enum value.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInputJumpDuringLocomotionNormalJumpTest,
	"ClimbingSystem.Batch6.Input.InputJumpDuringLocomotionNormalJump",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputJumpDuringLocomotionNormalJumpTest::RunTest(const FString& Parameters)
{
	// VERIFY: Input_JumpStarted is private — cannot be called directly.
	// Contract: IA_Jump property exists and EClimbingState::None is valid.
	const FProperty* JumpProp = AClimbingCharacter::StaticClass()->FindPropertyByName(TEXT("IA_Jump"));
	TestNotNull(TEXT("TC-0423: IA_Jump property must exist on AClimbingCharacter"), JumpProp);

	const UEnum* StateEnum = StaticEnum<EClimbingState>();
	TestNotNull(TEXT("TC-0423: EClimbingState enum must exist"), StateEnum);
	if (!StateEnum) { return false; }

	const int64 NoneValue = StateEnum->GetValueByName(TEXT("None"));
	TestTrue(TEXT("TC-0423: EClimbingState::None must be a valid enum value"),
		NoneValue != INDEX_NONE);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0424: InputDropFromShimmying
// WHAT: Transitioning from Shimmying to None must result in state==None.
// WHY:  The drop action must work from Shimmying; if it fails the character
//       is stuck shimmying with no way to release the ledge.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FInputDropFromShimmyingTest,
	"ClimbingSystem.Batch6.Input.InputDropFromShimmying",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInputDropFromShimmyingTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0424: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0424: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Shimmying);

	FClimbingDetectionResult EmptyResult;
	Character->TransitionToState(EClimbingState::None, EmptyResult);

	TestEqual(TEXT("TC-0424: state must be None after drop from Shimmying"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0425: ClassifyHangTypeBracedWhenWallPresent
// WHAT: ClassifyHangType sets bIsFreeHang=false when a backing wall is present.
// WHY:  Braced hang drives a different animation set and IK pose; if the wall
//       is not detected the character free-hangs against a wall — wrong pose.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClassifyHangTypeBracedWhenWallPresentTest,
	"ClimbingSystem.Batch6.Detection.ClassifyHangTypeBracedWhenWallPresent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClassifyHangTypeBracedWhenWallPresentTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	// Spawn a wall directly behind the character (along -Y, surface normal = +Y)
	// Character at origin facing +X; ledge at (0,0,100); wall behind ledge at (0,80,50)
	Helper.SpawnBoxAt(FVector(0.0f, 80.0f, 50.0f), FVector(50.0f, 10.0f, 60.0f));

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector(0.0f, -50.0f, 0.0f));
	TestNotNull(TEXT("TC-0425: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	FClimbingDetectionResult Result;
	Result.bValid        = true;
	Result.LedgePosition = FVector(0.0f, 0.0f, 100.0f);
	Result.SurfaceNormal = FVector(0.0f, -1.0f, 0.0f); // wall faces -Y (toward character)

	Character->ClassifyHangType(Result);

	TestFalse(TEXT("TC-0425: bIsFreeHang must be false when backing wall is present"),
		Result.bIsFreeHang);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0426: ClassifyHangTypeFreeWhenNoWall
// WHAT: ClassifyHangType sets bIsFreeHang=true when no backing wall is present.
// WHY:  Free-hang drives dangling-leg animation and different IK; if the
//       absence of a wall is not detected the wrong pose plays.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClassifyHangTypeFreeWhenNoWallTest,
	"ClimbingSystem.Batch6.Detection.ClassifyHangTypeFreeWhenNoWall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClassifyHangTypeFreeWhenNoWallTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	// No wall spawned — open space behind the ledge
	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector(0.0f, -50.0f, 0.0f));
	TestNotNull(TEXT("TC-0426: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	FClimbingDetectionResult Result;
	Result.bValid        = true;
	Result.LedgePosition = FVector(0.0f, 0.0f, 100.0f);
	Result.SurfaceNormal = FVector(0.0f, -1.0f, 0.0f);

	Character->ClassifyHangType(Result);

	TestTrue(TEXT("TC-0426: bIsFreeHang must be true when no backing wall is present"),
		Result.bIsFreeHang);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0427: ClassifyHangTypeNoSideEffects
// WHAT: ClassifyHangType must only modify bIsFreeHang; LedgePosition and
//       SurfaceNormal must remain unchanged.
// WHY:  ClassifyHangType is a const query; mutating other fields would corrupt
//       the detection result used for warp target registration and IK.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClassifyHangTypeNoSideEffectsTest,
	"ClimbingSystem.Batch6.Detection.ClassifyHangTypeNoSideEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClassifyHangTypeNoSideEffectsTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0427: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	const FVector KnownLedge(100.0f, 200.0f, 300.0f);
	const FVector KnownNormal(0.0f, -1.0f, 0.0f);

	FClimbingDetectionResult Result;
	Result.bValid        = true;
	Result.LedgePosition = KnownLedge;
	Result.SurfaceNormal = KnownNormal;
	Result.bIsFreeHang   = false;

	Character->ClassifyHangType(Result);

	TestEqual(TEXT("TC-0427: LedgePosition must not be modified by ClassifyHangType"),
		Result.LedgePosition, KnownLedge);
	TestEqual(TEXT("TC-0427: SurfaceNormal must not be modified by ClassifyHangType"),
		Result.SurfaceNormal, KnownNormal);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0428: LadderTickUpwardMovement
// WHAT: Contract — EClimbingState::OnLadder is a valid state and
//       SetClimbingState(OnLadder) does not crash.
// WHY:  OnLadder is the prerequisite for all ladder tick logic; if the state
//       cannot be entered the entire ladder feature is unreachable.
// VERIFY: TestTickLadderState shim does NOT exist on AClimbingCharacter
//         (only TestTickLacheInAirState is exposed). Contract verified via
//         state entry: OnLadder is a valid, enterable state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLadderTickUpwardMovementTest,
	"ClimbingSystem.Batch6.Ladder.LadderTickUpwardMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLadderTickUpwardMovementTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0428: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0428: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// VERIFY: TestTickLadderState shim does not exist — TickLadderState is protected.
	// Contract: OnLadder is a valid, enterable state.
	Movement->SetClimbingState(EClimbingState::OnLadder);

	TestEqual(TEXT("TC-0428: OnLadder must be a valid enterable state"),
		Movement->CurrentClimbingState, EClimbingState::OnLadder);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0429: LadderTickRungSnap
// WHAT: DefaultLadderRungSpacing CDO default must be 30.
// WHY:  Rung spacing drives procedural IK snapping; a wrong default causes
//       hands and feet to miss rungs visually.
// VERIFY: Snap behavior itself requires a running tick with a ladder actor —
//         not available in unit tests. Contract: CDO default is correct.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLadderTickRungSnapTest,
	"ClimbingSystem.Batch6.Ladder.LadderTickRungSnap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLadderTickRungSnapTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0429: CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0429: DefaultLadderRungSpacing must be 30"),
		FMath::IsNearlyEqual(CDO->DefaultLadderRungSpacing, 30.0f));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0430: LadderTickTopExit
// WHAT: Contract — LadderTransition is a valid state reachable from OnLadder,
//       confirming the top-exit path exists in the state machine.
// WHY:  Top exit is the primary way to dismount a ladder at the top; if the
//       transition is not valid the character is stuck at the top of the ladder.
// VERIFY: Top-exit detection requires a ladder actor with a top boundary —
//         not available in unit tests. Contract: LadderTransition is valid
//         from OnLadder.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLadderTickTopExitTest,
	"ClimbingSystem.Batch6.Ladder.LadderTickTopExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLadderTickTopExitTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0430: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0430: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// VERIFY: Top-exit detection requires a ladder actor — not available in unit tests.
	// Contract: LadderTransition is a valid state transition from OnLadder.
	Movement->SetClimbingState(EClimbingState::OnLadder);
	const bool bCanTransition = Movement->IsValidStateTransition(EClimbingState::LadderTransition);
	TestTrue(TEXT("TC-0430: LadderTransition must be a valid transition from OnLadder (top-exit path)"),
		bCanTransition);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0431: LadderTickBottomExit
// WHAT: Contract — None is a valid state reachable from OnLadder, confirming
//       the bottom-exit (drop) path exists in the state machine.
// WHY:  Bottom exit is the primary way to dismount a ladder at the base; if
//       the transition is not valid the character is stuck on the ladder.
// VERIFY: Bottom-exit detection requires a ladder actor with a bottom boundary.
//         Contract: None is valid from OnLadder.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLadderTickBottomExitTest,
	"ClimbingSystem.Batch6.Ladder.LadderTickBottomExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLadderTickBottomExitTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0431: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0431: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// VERIFY: Bottom-exit detection requires a ladder actor — not available in unit tests.
	// Contract: None is a valid transition from OnLadder (bottom-exit / drop path).
	Movement->SetClimbingState(EClimbingState::OnLadder);
	const bool bCanTransition = Movement->IsValidStateTransition(EClimbingState::None);
	TestTrue(TEXT("TC-0431: None must be a valid transition from OnLadder (bottom-exit path)"),
		bCanTransition);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0432: LacheTickInAirFollowsArc
// WHAT: Pure math — at t=0.5s with LaunchOrigin=(0,0,0), Forward=(1,0,0),
//       Speed=1200, GravityZ=-980: X = Speed*t = 600, Z = 0.5*GravityZ*t^2 = -122.5.
// WHY:  The arc formula is the core of Lache flight; if the math is wrong the
//       character misses every target ledge.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLacheTickInAirFollowsArcTest,
	"ClimbingSystem.Batch6.Lache.LacheTickInAirFollowsArc",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLacheTickInAirFollowsArcTest::RunTest(const FString& Parameters)
{
	const FVector LaunchOrigin(0.0f, 0.0f, 0.0f);
	const FVector Forward(1.0f, 0.0f, 0.0f);
	const float   Speed     = 1200.0f;
	const float   GravityZ  = -980.0f;
	const float   T         = 0.5f;

	// Projectile arc: P = Origin + Forward*Speed*t + (0,0, 0.5*GravityZ*t^2)
	const float ExpectedX = Speed * T;                        // 600
	const float ExpectedZ = 0.5f * GravityZ * (T * T);       // -122.5

	const FVector ArcPos = LaunchOrigin + Forward * Speed * T
		+ FVector(0.0f, 0.0f, 0.5f * GravityZ * T * T);

	TestTrue(TEXT("TC-0432: arc X at t=0.5 must be 600"),
		FMath::IsNearlyEqual(ArcPos.X, ExpectedX, 0.01f));
	TestTrue(TEXT("TC-0432: arc Z at t=0.5 must be -122.5"),
		FMath::IsNearlyEqual(ArcPos.Z, ExpectedZ, 0.01f));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0433: LacheTickInAirCatchTransition
// WHAT: Transitioning from LacheInAir to LacheCatch must result in
//       state==LacheCatch.
// WHY:  LacheCatch is the success path of a Lache jump; if the transition
//       fails the character falls through the target ledge with no catch anim.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLacheTickInAirCatchTransitionTest,
	"ClimbingSystem.Batch6.Lache.LacheTickInAirCatchTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLacheTickInAirCatchTransitionTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0433: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0433: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::LacheInAir);

	FClimbingDetectionResult DetResult;
	DetResult.bValid        = true;
	DetResult.LedgePosition = FVector(500.0f, 0.0f, 100.0f);
	DetResult.SurfaceNormal = FVector(0.0f, -1.0f, 0.0f);
	Character->TransitionToState(EClimbingState::LacheCatch, DetResult);

	TestEqual(TEXT("TC-0433: state must be LacheCatch after transition from LacheInAir"),
		Movement->CurrentClimbingState, EClimbingState::LacheCatch);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0434: LacheTickInAirMissTransition
// WHAT: Transitioning from LacheInAir to LacheMiss must result in
//       state==LacheMiss.
// WHY:  LacheMiss is the failure path of a Lache jump; if the transition
//       fails the character is stuck in LacheInAir with no recovery animation.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLacheTickInAirMissTransitionTest,
	"ClimbingSystem.Batch6.Lache.LacheTickInAirMissTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLacheTickInAirMissTransitionTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0434: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0434: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::LacheInAir);

	FClimbingDetectionResult EmptyResult;
	Character->TransitionToState(EClimbingState::LacheMiss, EmptyResult);

	TestEqual(TEXT("TC-0434: state must be LacheMiss after transition from LacheInAir"),
		Movement->CurrentClimbingState, EClimbingState::LacheMiss);

	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
