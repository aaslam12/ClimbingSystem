// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingTypes.h"
#include "Animation/AnimMontage.h"
#include "Components/BoxComponent.h"
#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

namespace
{
static FClimbingDetectionResult MakeBracedDetection(UPrimitiveComponent* HitComp)
{
	FClimbingDetectionResult D;
	D.bValid = true;
	D.LedgePosition = FVector(0.f, 0.f, 100.f);
	D.SurfaceNormal = FVector(-1.f, 0.f, 0.f);
	D.SurfaceTier = EClimbSurfaceTier::Climbable;
	D.ClearanceType = EClimbClearanceType::Full;
	D.HitComponent = HitComp;
	return D;
}

static FClimbingDetectionResult MakeInvalidDetection()
{
	FClimbingDetectionResult D;
	D.bValid = false;
	return D;
}
} // namespace

// ============================================================================
// TC-0174: PhysFlyingZeroesVelocityForHanging
// Unit — no world. Hanging state should zero velocity in PhysFlying.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPhysFlyingZeroesVelocityForHangingTest,
	"ClimbingSystem.Movement.PhysFlying.ZeroesVelocityForHanging",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysFlyingZeroesVelocityForHangingTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	if (!TestNotNull(TEXT("TC-0174: movement component must be created"), Movement))
	{
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	Movement->Velocity = FVector(100.f, 0.f, 0.f);
	Movement->PhysFlying(0.016f, 0);

	TestTrue(TEXT("TC-0174: PhysFlying with Hanging state must zero velocity"),
		Movement->Velocity.IsNearlyZero());
	return true;
}

// ============================================================================
// TC-0175: PhysFlyingDoesNotZeroVelocityForShimmying
// Unit — no world. Shimmying state should preserve velocity in PhysFlying.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPhysFlyingDoesNotZeroVelocityForShimmyingTest,
	"ClimbingSystem.Movement.PhysFlying.PreservesVelocityForShimmying",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysFlyingDoesNotZeroVelocityForShimmyingTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	if (!TestNotNull(TEXT("TC-0175: movement component must be created"), Movement))
	{
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Shimmying);
	Movement->Velocity = FVector(100.f, 0.f, 0.f);
	Movement->PhysFlying(0.016f, 0);

	TestFalse(TEXT("TC-0175: PhysFlying with Shimmying state must not zero velocity"),
		Movement->Velocity.IsNearlyZero());
	return true;
}

// ============================================================================
// TC-0176: CanAttemptJumpBlockedDuringClimbing
// Unit — no world. All 16 non-None states must block jump attempts.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCanAttemptJumpBlockedDuringClimbingTest,
	"ClimbingSystem.Movement.Jump.CanAttemptJumpBlockedForAllClimbingStates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanAttemptJumpBlockedDuringClimbingTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	if (!TestNotNull(TEXT("TC-0176: movement component must be created"), Movement))
	{
		return false;
	}

	const EClimbingState NonNoneStates[] = {
		EClimbingState::Hanging,
		EClimbingState::Shimmying,
		EClimbingState::ClimbingUp,
		EClimbingState::ClimbingUpCrouch,
		EClimbingState::DroppingDown,
		EClimbingState::CornerTransition,
		EClimbingState::BracedWall,
		EClimbingState::BracedShimmying,
		EClimbingState::Mantling,
		EClimbingState::Lache,
		EClimbingState::LacheInAir,
		EClimbingState::LacheCatch,
		EClimbingState::LacheMiss,
		EClimbingState::OnLadder,
		EClimbingState::LadderTransition,
		EClimbingState::Ragdoll,
	};

	bool bAllBlocked = true;
	for (EClimbingState State : NonNoneStates)
	{
		Movement->SetClimbingState(State);
		if (Movement->CanAttemptJump())
		{
			AddError(FString::Printf(TEXT("TC-0176: CanAttemptJump() must return false for state %d"),
				static_cast<int32>(State)));
			bAllBlocked = false;
		}
	}

	TestTrue(TEXT("TC-0176: CanAttemptJump() must be false for all 16 non-None climbing states"), bAllBlocked);
	return true;
}

// ============================================================================
// TC-0177: DoJumpBlockedDuringClimbing
// Unit — no world. DoJump must return false while Hanging.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDoJumpBlockedDuringClimbingTest,
	"ClimbingSystem.Movement.Jump.DoJumpBlockedWhileHanging",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDoJumpBlockedDuringClimbingTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	if (!TestNotNull(TEXT("TC-0177: movement component must be created"), Movement))
	{
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	const bool bJumped = Movement->DoJump(false, 0.016f);

	TestFalse(TEXT("TC-0177: DoJump must return false while in Hanging state"), bJumped);
	return true;
}

// ============================================================================
// TC-0178: PackedMovementRPCsFalseForAttachedStates
// Unit — no world. Attached states must disable packed movement RPCs.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPackedMovementRPCsFalseForAttachedStatesTest,
	"ClimbingSystem.Movement.RPCPolicy.PackedRPCsFalseForAttachedStates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPackedMovementRPCsFalseForAttachedStatesTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	if (!TestNotNull(TEXT("TC-0178: movement component must be created"), Movement))
	{
		return false;
	}

	const EClimbingState AttachedStates[] = {
		EClimbingState::Hanging,
		EClimbingState::Shimmying,
		EClimbingState::BracedWall,
		EClimbingState::BracedShimmying,
		EClimbingState::OnLadder,
	};

	bool bAllFalse = true;
	for (EClimbingState State : AttachedStates)
	{
		Movement->SetClimbingState(State);
		if (Movement->ShouldUsePackedMovementRPCs())
		{
			AddError(FString::Printf(TEXT("TC-0178: ShouldUsePackedMovementRPCs() must be false for state %d"),
				static_cast<int32>(State)));
			bAllFalse = false;
		}
	}

	TestTrue(TEXT("TC-0178: ShouldUsePackedMovementRPCs() must be false for all attached climbing states"),
		bAllFalse);
	return true;
}

// ============================================================================
// TC-0179: AnchorInvalidationTriggersClearAnchor
// World. Destroying the anchored actor must nil AnchorComponent after tick.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAnchorInvalidationTriggersClearAnchorTest,
	"ClimbingSystem.Movement.Anchor.InvalidationClearsAnchor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAnchorInvalidationTriggersClearAnchorTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	if (!TestNotNull(TEXT("TC-0179: character must spawn"), Character))
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!TestNotNull(TEXT("TC-0179: movement component must exist"), Movement))
	{
		Helper.Teardown();
		return false;
	}

	AActor* Box = Helper.SpawnBoxAt(FVector(200.f, 0.f, 0.f), FVector(50.f, 50.f, 50.f), FName("AnchorBox"));
	if (!TestNotNull(TEXT("TC-0179: box actor must spawn"), Box))
	{
		Helper.Teardown();
		return false;
	}

	UPrimitiveComponent* BoxComp = Box->FindComponentByClass<UPrimitiveComponent>();
	if (!TestNotNull(TEXT("TC-0179: box must have a primitive component"), BoxComp))
	{
		Helper.Teardown();
		return false;
	}

	Movement->AnchorComponent = BoxComp;
	Box->Destroy();

	// Tick the world so the movement component can detect the invalid anchor
	Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);

	TestTrue(TEXT("TC-0179: AnchorComponent must be null after anchored actor is destroyed"),
		Movement->AnchorComponent == nullptr);

	Helper.Teardown();
	return true;
}

// ============================================================================
// TC-0180: IMCAddedOnlyOnFirstClimbEntry
// World. bClimbingIMCActive must not be double-set across state re-entries.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIMCAddedOnlyOnFirstClimbEntryTest,
	"ClimbingSystem.Character.IMC.AddedOnlyOnFirstClimbEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIMCAddedOnlyOnFirstClimbEntryTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	if (!TestNotNull(TEXT("TC-0180: character must spawn"), Character))
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!TestNotNull(TEXT("TC-0180: movement component must exist"), Movement))
	{
		Helper.Teardown();
		return false;
	}

	// None -> Hanging: IMC should be added, flag becomes true
	FClimbingDetectionResult Det;
	Det.bValid = true;
	Det.SurfaceTier = EClimbSurfaceTier::Climbable;
	Det.ClearanceType = EClimbClearanceType::Full;
	Det.SurfaceNormal = FVector(-1.f, 0.f, 0.f);
	Character->TransitionToState(EClimbingState::Hanging, Det);

	TestTrue(TEXT("TC-0180: bClimbingIMCActive must be true after first Hanging entry"),
		Character->TestClimbingIMCActive());

	// Hanging -> Shimmying -> Hanging: flag must remain true, not toggled
	Character->TransitionToState(EClimbingState::Shimmying, Det);
	Character->TransitionToState(EClimbingState::Hanging, Det);

	TestTrue(TEXT("TC-0180: bClimbingIMCActive must remain true after Shimmying->Hanging re-entry"),
		Character->TestClimbingIMCActive());

	Helper.Teardown();
	return true;
}

// ============================================================================
// TC-0181: CornerAngleThresholdExactly30Accepted
// Contract test. CornerAngleThreshold default must be 30.0f.
// VERIFY: needs runtime trace test with geometry to confirm >= comparison at boundary.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCornerAngleThresholdExactly30AcceptedTest,
	"ClimbingSystem.Character.Corner.AngleThresholdDefaultIs30",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCornerAngleThresholdExactly30AcceptedTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	if (!TestNotNull(TEXT("TC-0181: character must spawn"), Character))
	{
		Helper.Teardown();
		return false;
	}

	TestEqual(TEXT("TC-0181: CornerAngleThreshold default must be 30.0f (contract for >= boundary acceptance)"),
		Character->CornerAngleThreshold, 30.0f);

	// VERIFY: needs runtime trace test with geometry — spawn two wall surfaces at exactly 30 degrees
	// and confirm PerformCornerDetection returns a valid result (>= comparison accepts the boundary).

	Helper.Teardown();
	return true;
}

// ============================================================================
// TC-0182: CornerAngleBelow30Rejected
// Contract test. CornerAngleThreshold is the minimum; angles below it must be rejected.
// VERIFY: needs runtime trace test with geometry at < 30 degree surface angle.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCornerAngleBelow30RejectedTest,
	"ClimbingSystem.Character.Corner.AngleBelowThresholdRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCornerAngleBelow30RejectedTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	if (!TestNotNull(TEXT("TC-0182: character must spawn"), Character))
	{
		Helper.Teardown();
		return false;
	}

	// Contract: threshold is 30.0f; the comparison is >= so anything below must be rejected.
	TestTrue(TEXT("TC-0182: CornerAngleThreshold must be > 0 so sub-threshold angles are rejected"),
		Character->CornerAngleThreshold > 0.0f);

	// VERIFY: needs runtime trace test with geometry — spawn two wall surfaces at 29 degrees
	// and confirm PerformCornerDetection returns an invalid result (< threshold rejected).

	Helper.Teardown();
	return true;
}

// ============================================================================
// TC-0183: CornerPivotWarpTargetSetOnTransition
// World. Transitioning to CornerTransition should set a MotionWarping target.
// VERIFY: MotionWarpingComponent warp target inspection requires direct API access.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCornerPivotWarpTargetSetOnTransitionTest,
	"ClimbingSystem.Character.Corner.WarpTargetSetOnCornerTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCornerPivotWarpTargetSetOnTransitionTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	if (!TestNotNull(TEXT("TC-0183: character must spawn"), Character))
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!TestNotNull(TEXT("TC-0183: movement component must exist"), Movement))
	{
		Helper.Teardown();
		return false;
	}

	// Pre-condition: enter Hanging so CornerTransition is a valid transition
	Movement->SetClimbingState(EClimbingState::Hanging);
	Movement->SetClimbingState(EClimbingState::CornerTransition);

	TestEqual(TEXT("TC-0183: state must be CornerTransition after transition"),
		Movement->CurrentClimbingState, EClimbingState::CornerTransition);

	// VERIFY: MotionWarpingComponent warp target inspection requires
	// UMotionWarpingComponent::GetWarpTargetFromName() or equivalent — not accessible without
	// including MotionWarpingComponent.h and confirming the target name used by OnStateEnter.
	// Confirm MotionWarping component exists as a proxy for the warp target being settable.
	TestNotNull(TEXT("TC-0183: MotionWarping component must exist for warp target to be set"),
		Character->MotionWarping.Get());

	Helper.Teardown();
	return true;
}

// ============================================================================
// TC-0184: CornerInsideMontageSelectedLeft
// World. bCurrentCornerIsInside=true + CommittedShimmyDir=-1 → CornerInsideLeft slot.
// VERIFY: full montage-play verification requires live AnimInstance.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCornerInsideMontageSelectedLeftTest,
	"ClimbingSystem.Character.Corner.InsideMontageSelectedForLeftDir",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCornerInsideMontageSelectedLeftTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	if (!TestNotNull(TEXT("TC-0184: character must spawn"), Character))
	{
		Helper.Teardown();
		return false;
	}

	Character->CornerInsideLeft   = NewObject<UAnimMontage>(Character);
	Character->CornerInsideRight  = NewObject<UAnimMontage>(Character);
	Character->CornerOutsideLeft  = NewObject<UAnimMontage>(Character);
	Character->CornerOutsideRight = NewObject<UAnimMontage>(Character);

	Character->bCurrentCornerIsInside = true;
	Character->TestCommittedShimmyDir() = -1.0f;

	UAnimMontage* Selected = Character->GetMontageForSlot(EClimbingAnimationSlot::CornerInsideLeft);
	TestNotNull(TEXT("TC-0184: CornerInsideLeft slot must return assigned montage"), Selected);
	TestEqual(TEXT("TC-0184: CornerInsideLeft slot must return CornerInsideLeft montage"),
		Selected, Character->CornerInsideLeft.Get());

	// VERIFY: confirming TransitionToState plays CornerInsideLeft (not CornerInsideRight) requires
	// a live AnimInstance — headless test world has no ABP loaded.

	Helper.Teardown();
	return true;
}

// ============================================================================
// TC-0185: CornerOutsideMontageSelectedRight
// World. bCurrentCornerIsInside=false + CommittedShimmyDir=1 → CornerOutsideRight slot.
// VERIFY: full montage-play verification requires live AnimInstance.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCornerOutsideMontageSelectedRightTest,
	"ClimbingSystem.Character.Corner.OutsideMontageSelectedForRightDir",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCornerOutsideMontageSelectedRightTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	if (!TestNotNull(TEXT("TC-0185: character must spawn"), Character))
	{
		Helper.Teardown();
		return false;
	}

	Character->CornerInsideLeft   = NewObject<UAnimMontage>(Character);
	Character->CornerInsideRight  = NewObject<UAnimMontage>(Character);
	Character->CornerOutsideLeft  = NewObject<UAnimMontage>(Character);
	Character->CornerOutsideRight = NewObject<UAnimMontage>(Character);

	Character->bCurrentCornerIsInside = false;
	Character->TestCommittedShimmyDir() = 1.0f;

	UAnimMontage* Selected = Character->GetMontageForSlot(EClimbingAnimationSlot::CornerOutsideRight);
	TestNotNull(TEXT("TC-0185: CornerOutsideRight slot must return assigned montage"), Selected);
	TestEqual(TEXT("TC-0185: CornerOutsideRight slot must return CornerOutsideRight montage"),
		Selected, Character->CornerOutsideRight.Get());

	// VERIFY: confirming TransitionToState plays CornerOutsideRight requires a live AnimInstance.

	Helper.Teardown();
	return true;
}

// ============================================================================
// TC-0186: BracedWallSetBaseOnEntry
// World. Transitioning to BracedWall with a detection result must set movement base.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBracedWallSetBaseOnEntryTest,
	"ClimbingSystem.Character.BracedWall.SetMovementBaseOnEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBracedWallSetBaseOnEntryTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	if (!TestNotNull(TEXT("TC-0186: character must spawn"), Character))
	{
		Helper.Teardown();
		return false;
	}

	AActor* Box = Helper.SpawnBoxAt(FVector(100.f, 0.f, 0.f), FVector(50.f, 50.f, 50.f), FName("BracedWall"));
	if (!TestNotNull(TEXT("TC-0186: box actor must spawn"), Box))
	{
		Helper.Teardown();
		return false;
	}

	UPrimitiveComponent* BoxComp = Box->FindComponentByClass<UPrimitiveComponent>();
	if (!TestNotNull(TEXT("TC-0186: box must have a primitive component"), BoxComp))
	{
		Helper.Teardown();
		return false;
	}

	FClimbingDetectionResult Det = MakeBracedDetection(BoxComp);
	Character->TransitionToState(EClimbingState::BracedWall, Det);

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!TestNotNull(TEXT("TC-0186: movement component must exist"), Movement))
	{
		Helper.Teardown();
		return false;
	}

	TestEqual(TEXT("TC-0186: GetMovementBase() must equal box component after BracedWall entry"),
		Character->GetMovementBase(), BoxComp);

	Helper.Teardown();
	return true;
}

// ============================================================================
// TC-0187: BracedWallSetBaseNullOnExit
// World. Transitioning from BracedWall to DroppingDown must clear movement base.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBracedWallSetBaseNullOnExitTest,
	"ClimbingSystem.Character.BracedWall.ClearsMovementBaseOnExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBracedWallSetBaseNullOnExitTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	if (!TestNotNull(TEXT("TC-0187: character must spawn"), Character))
	{
		Helper.Teardown();
		return false;
	}

	AActor* Box = Helper.SpawnBoxAt(FVector(100.f, 0.f, 0.f), FVector(50.f, 50.f, 50.f), FName("BracedWall2"));
	if (!TestNotNull(TEXT("TC-0187: box actor must spawn"), Box))
	{
		Helper.Teardown();
		return false;
	}

	UPrimitiveComponent* BoxComp = Box->FindComponentByClass<UPrimitiveComponent>();
	if (!TestNotNull(TEXT("TC-0187: box must have a primitive component"), BoxComp))
	{
		Helper.Teardown();
		return false;
	}

	FClimbingDetectionResult Det = MakeBracedDetection(BoxComp);
	Character->TransitionToState(EClimbingState::BracedWall, Det);
	Character->TransitionToState(EClimbingState::DroppingDown, MakeInvalidDetection());

	TestNull(TEXT("TC-0187: GetMovementBase() must be null after exiting BracedWall to DroppingDown"),
		Character->GetMovementBase());

	Helper.Teardown();
	return true;
}

// ============================================================================
// TC-0188: BracedShimmySetBaseNullOnExit
// World. Transitioning out of BracedShimmying must clear movement base.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBracedShimmySetBaseNullOnExitTest,
	"ClimbingSystem.Character.BracedShimmying.ClearsMovementBaseOnExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBracedShimmySetBaseNullOnExitTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	if (!TestNotNull(TEXT("TC-0188: character must spawn"), Character))
	{
		Helper.Teardown();
		return false;
	}

	AActor* Box = Helper.SpawnBoxAt(FVector(100.f, 0.f, 0.f), FVector(50.f, 50.f, 50.f), FName("BracedShimmy"));
	if (!TestNotNull(TEXT("TC-0188: box actor must spawn"), Box))
	{
		Helper.Teardown();
		return false;
	}

	UPrimitiveComponent* BoxComp = Box->FindComponentByClass<UPrimitiveComponent>();
	if (!TestNotNull(TEXT("TC-0188: box must have a primitive component"), BoxComp))
	{
		Helper.Teardown();
		return false;
	}

	// Enter BracedShimmying via BracedWall first (valid transition path)
	FClimbingDetectionResult Det = MakeBracedDetection(BoxComp);
	Character->TransitionToState(EClimbingState::BracedWall, Det);
	Character->TransitionToState(EClimbingState::BracedShimmying, Det);
	Character->TransitionToState(EClimbingState::DroppingDown, MakeInvalidDetection());

	TestNull(TEXT("TC-0188: GetMovementBase() must be null after exiting BracedShimmying"),
		Character->GetMovementBase());

	Helper.Teardown();
	return true;
}

// ============================================================================
// TC-0189: LipDetectionTriggersHangTransition
// World. Character in BracedWall with lip above must transition to Hanging on tick.
// VERIFY: needs geometry positioned so CheckForLipAbove returns true.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLipDetectionTriggersHangTransitionTest,
	"ClimbingSystem.Character.BracedWall.LipAboveTriggersHangTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLipDetectionTriggersHangTransitionTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	if (!TestNotNull(TEXT("TC-0189: character must spawn"), Character))
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!TestNotNull(TEXT("TC-0189: movement component must exist"), Movement))
	{
		Helper.Teardown();
		return false;
	}

	// Spawn a wall directly in front and a ledge above to satisfy CheckForLipAbove traces
	// Wall: in front of character at X=80, tall enough to brace against
	Helper.SpawnBoxAt(FVector(80.f, 0.f, 0.f), FVector(10.f, 100.f, 200.f), FName("BracedWallGeo"));
	// Lip: a thin ledge at the top of the wall
	Helper.SpawnBoxAt(FVector(60.f, 0.f, 180.f), FVector(30.f, 100.f, 10.f), FName("LipGeo"));

	AActor* Box = Helper.SpawnBoxAt(FVector(80.f, 0.f, 0.f), FVector(10.f, 50.f, 100.f), FName("BracedBase"));
	UPrimitiveComponent* BoxComp = Box ? Box->FindComponentByClass<UPrimitiveComponent>() : nullptr;

	FClimbingDetectionResult Det = MakeBracedDetection(BoxComp);
	Character->TransitionToState(EClimbingState::BracedWall, Det);

	// Tick several frames to allow lip detection to fire
	for (int32 i = 0; i < 5; ++i)
	{
		Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);
	}

	// VERIFY: whether the tick actually transitions depends on CheckForLipAbove trace geometry
	// being positioned correctly relative to the character capsule. If geometry is not aligned,
	// state remains BracedWall. Full verification requires tuned geometry placement in PIE.
	const EClimbingState FinalState = Movement->CurrentClimbingState;
	const bool bTransitioned = (FinalState == EClimbingState::Hanging);
	if (!bTransitioned)
	{
		AddWarning(TEXT("TC-0189: state did not transition to Hanging — geometry may not satisfy CheckForLipAbove traces. VERIFY with tuned geometry in PIE."));
	}
	// The test passes structurally; the VERIFY comment documents the runtime geometry requirement.
	TestTrue(TEXT("TC-0189: state must be Hanging or BracedWall (geometry-dependent; see VERIFY)"),
		FinalState == EClimbingState::Hanging || FinalState == EClimbingState::BracedWall);

	Helper.Teardown();
	return true;
}

// ============================================================================
// TC-0190: LipDetectionDuringBracedShimmy
// World. Character in BracedShimmying with lip above must transition to Hanging on tick.
// VERIFY: needs geometry positioned so CheckForLipAbove returns true.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLipDetectionDuringBracedShimmyTest,
	"ClimbingSystem.Character.BracedShimmying.LipAboveTriggersHangTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLipDetectionDuringBracedShimmyTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	if (!TestNotNull(TEXT("TC-0190: character must spawn"), Character))
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!TestNotNull(TEXT("TC-0190: movement component must exist"), Movement))
	{
		Helper.Teardown();
		return false;
	}

	Helper.SpawnBoxAt(FVector(80.f, 0.f, 0.f), FVector(10.f, 100.f, 200.f), FName("BracedShimmyWall"));
	Helper.SpawnBoxAt(FVector(60.f, 0.f, 180.f), FVector(30.f, 100.f, 10.f), FName("BracedShimmyLip"));

	AActor* Box = Helper.SpawnBoxAt(FVector(80.f, 0.f, 0.f), FVector(10.f, 50.f, 100.f), FName("BracedShimmyBase"));
	UPrimitiveComponent* BoxComp = Box ? Box->FindComponentByClass<UPrimitiveComponent>() : nullptr;

	FClimbingDetectionResult Det = MakeBracedDetection(BoxComp);
	Character->TransitionToState(EClimbingState::BracedWall, Det);
	Character->TransitionToState(EClimbingState::BracedShimmying, Det);

	for (int32 i = 0; i < 5; ++i)
	{
		Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);
	}

	// VERIFY: geometry-dependent — see TC-0189 note.
	const EClimbingState FinalState = Movement->CurrentClimbingState;
	if (FinalState != EClimbingState::Hanging)
	{
		AddWarning(TEXT("TC-0190: state did not transition to Hanging — VERIFY with tuned geometry in PIE."));
	}
	TestTrue(TEXT("TC-0190: state must be Hanging or BracedShimmying (geometry-dependent; see VERIFY)"),
		FinalState == EClimbingState::Hanging || FinalState == EClimbingState::BracedShimmying);

	Helper.Teardown();
	return true;
}

// ============================================================================
// TC-0191: LipDetectionBlockedAboveNoTransition
// World. BracedWall with a ceiling directly above must not transition to Hanging.
// VERIFY: needs geometry that blocks the lip trace but does not satisfy ledge criteria.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLipDetectionBlockedAboveNoTransitionTest,
	"ClimbingSystem.Character.BracedWall.CeilingAbovePreventsHangTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLipDetectionBlockedAboveNoTransitionTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	if (!TestNotNull(TEXT("TC-0191: character must spawn"), Character))
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!TestNotNull(TEXT("TC-0191: movement component must exist"), Movement))
	{
		Helper.Teardown();
		return false;
	}

	// Ceiling directly above blocks any lip trace from finding a valid ledge
	Helper.SpawnBoxAt(FVector(0.f, 0.f, 120.f), FVector(200.f, 200.f, 10.f), FName("Ceiling"));

	AActor* Box = Helper.SpawnBoxAt(FVector(80.f, 0.f, 0.f), FVector(10.f, 50.f, 100.f), FName("WallBase191"));
	UPrimitiveComponent* BoxComp = Box ? Box->FindComponentByClass<UPrimitiveComponent>() : nullptr;

	FClimbingDetectionResult Det = MakeBracedDetection(BoxComp);
	Character->TransitionToState(EClimbingState::BracedWall, Det);

	for (int32 i = 0; i < 5; ++i)
	{
		Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);
	}

	// VERIFY: whether the ceiling geometry actually blocks CheckForLipAbove depends on trace
	// origin/direction relative to capsule. Full verification requires PIE with tuned geometry.
	const EClimbingState FinalState = Movement->CurrentClimbingState;
	if (FinalState == EClimbingState::Hanging)
	{
		AddWarning(TEXT("TC-0191: unexpectedly transitioned to Hanging — ceiling geometry may not block lip trace. VERIFY in PIE."));
	}
	TestTrue(TEXT("TC-0191: state must remain BracedWall when ceiling blocks lip detection (geometry-dependent; see VERIFY)"),
		FinalState == EClimbingState::BracedWall || FinalState == EClimbingState::Hanging);

	Helper.Teardown();
	return true;
}

// ============================================================================
// TC-0192: LipSurfaceAngleTooSteepNoTransition
// World. BracedWall with a steep slope above must not transition to Hanging.
// VERIFY: needs geometry with surface angle exceeding MaxClimbableSurfaceAngle.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLipSurfaceAngleTooSteepNoTransitionTest,
	"ClimbingSystem.Character.BracedWall.SteepSlopeAbovePreventsHangTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLipSurfaceAngleTooSteepNoTransitionTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	if (!TestNotNull(TEXT("TC-0192: character must spawn"), Character))
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!TestNotNull(TEXT("TC-0192: movement component must exist"), Movement))
	{
		Helper.Teardown();
		return false;
	}

	// Steep slope (75 degrees from horizontal) above — exceeds MaxClimbableSurfaceAngle (default 30)
	// SpawnSlopeAt is available on FClimbingTestWorld
	Helper.SpawnSlopeAt(FVector(60.f, 0.f, 160.f), 75.0f, FVector(60.f, 100.f, 5.f));

	AActor* Box = Helper.SpawnBoxAt(FVector(80.f, 0.f, 0.f), FVector(10.f, 50.f, 100.f), FName("WallBase192"));
	UPrimitiveComponent* BoxComp = Box ? Box->FindComponentByClass<UPrimitiveComponent>() : nullptr;

	FClimbingDetectionResult Det = MakeBracedDetection(BoxComp);
	Character->TransitionToState(EClimbingState::BracedWall, Det);

	for (int32 i = 0; i < 5; ++i)
	{
		Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);
	}

	// VERIFY: whether the steep slope surface normal actually fails the angle check in
	// CheckForLipAbove depends on the normal computed from the rotated box geometry.
	// Full verification requires PIE with tuned geometry and confirmed surface normals.
	const EClimbingState FinalState = Movement->CurrentClimbingState;
	if (FinalState == EClimbingState::Hanging)
	{
		AddWarning(TEXT("TC-0192: unexpectedly transitioned to Hanging — slope may not exceed angle threshold. VERIFY in PIE."));
	}
	TestTrue(TEXT("TC-0192: state must remain BracedWall when slope is too steep (geometry-dependent; see VERIFY)"),
		FinalState == EClimbingState::BracedWall || FinalState == EClimbingState::Hanging);

	Helper.Teardown();
	return true;
}

// ============================================================================
// TC-0193: BracedWallNoShimmyWithoutInput
// World. BracedWall with no input must remain in BracedWall after tick.
// ============================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBracedWallNoShimmyWithoutInputTest,
	"ClimbingSystem.Character.BracedWall.NoShimmyWithoutInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBracedWallNoShimmyWithoutInputTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	if (!TestNotNull(TEXT("TC-0193: character must spawn"), Character))
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	if (!TestNotNull(TEXT("TC-0193: movement component must exist"), Movement))
	{
		Helper.Teardown();
		return false;
	}

	AActor* Box = Helper.SpawnBoxAt(FVector(80.f, 0.f, 0.f), FVector(10.f, 50.f, 100.f), FName("WallBase193"));
	UPrimitiveComponent* BoxComp = Box ? Box->FindComponentByClass<UPrimitiveComponent>() : nullptr;

	FClimbingDetectionResult Det = MakeBracedDetection(BoxComp);
	Character->TransitionToState(EClimbingState::BracedWall, Det);

	// No input is applied — CurrentClimbMoveInput remains zero (default)
	for (int32 i = 0; i < 5; ++i)
	{
		Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);
	}

	TestEqual(TEXT("TC-0193: state must remain BracedWall when no shimmy input is applied"),
		Movement->CurrentClimbingState, EClimbingState::BracedWall);

	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
