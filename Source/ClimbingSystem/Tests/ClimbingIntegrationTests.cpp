// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingSurfaceData.h"
#include "Animation/ClimbingAnimationSet.h"
#include "Animation/ClimbingAnimInstance.h"
#include "Helpers/SharedTestHelpers.h"
#include "Misc/AutomationTest.h"

// ============================================================================
// CATEGORY 6: Integration Tests — Full Feature Flows
// ============================================================================

// FLOW: State machine full cycle — None → Hanging → Shimmying → None
// Step 1: Spawn character, verify initial state is None
// Step 2: Transition to Hanging via movement component
// Step 3: Verify Hanging is interruptible
// Step 4: Transition to Shimmying
// Step 5: Verify previous state is Hanging, then return to None

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingStateMachineCycleIntegrationTest,
	"ClimbingSystem.Integration.StateMachine.FullCycleHangShimmyDrop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingStateMachineCycleIntegrationTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Integration: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Integration: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	// Step 1: Initial state
	TestEqual(TEXT("Integration: initial state should be None"),
		Movement->CurrentClimbingState, EClimbingState::None);

	// Step 2: Transition to Hanging
	TestTrue(TEXT("Integration: None -> Hanging should be valid"),
		Movement->IsValidStateTransition(EClimbingState::Hanging));
	Movement->SetClimbingState(EClimbingState::Hanging);
	TestEqual(TEXT("Integration: state should be Hanging"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	// Step 3: Hanging is interruptible
	TestTrue(TEXT("Integration: Hanging should be interruptible"),
		Movement->CanInterruptCurrentState());

	// Step 4: Transition to Shimmying
	TestTrue(TEXT("Integration: Hanging -> Shimmying should be valid"),
		Movement->IsValidStateTransition(EClimbingState::Shimmying));
	Movement->SetClimbingState(EClimbingState::Shimmying);
	TestEqual(TEXT("Integration: state should be Shimmying"),
		Movement->CurrentClimbingState, EClimbingState::Shimmying);

	// Step 5: Verify previous state and return to None
	TestEqual(TEXT("Integration: previous state should be Hanging"),
		Movement->PreviousClimbingState, EClimbingState::Hanging);
	Movement->SetClimbingState(EClimbingState::None);
	TestEqual(TEXT("Integration: final state should be None"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// FLOW: Surface data integration — verify surface data defaults affect speed calculation
// Step 1: Create movement component
// Step 2: Calculate shimmy speed with default multiplier (1.0)
// Step 3: Calculate shimmy speed with reduced multiplier (0.5 — icy surface)
// Step 4: Verify reduced multiplier produces slower speed

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingSurfaceSpeedIntegrationTest,
	"ClimbingSystem.Integration.Surface.SpeedMultiplierAffectsShimmy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingSurfaceSpeedIntegrationTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Integration: movement component should be created"), Movement);
	if (!Movement) { return false; }

	Movement->BaseShimmySpeed = 160.0f;
	Movement->OverhangPenaltyStartAngle = 15.0f;
	Movement->OverhangPenaltyRangeAngle = 30.0f;
	Movement->OverhangMaxPenaltyScalar = 0.5f;

	UClimbingSurfaceData* NormalSurface = NewObject<UClimbingSurfaceData>();
	UClimbingSurfaceData* IcySurface = NewObject<UClimbingSurfaceData>();
	IcySurface->ClimbSpeedMultiplier = 0.5f;

	const FVector VerticalWall = FVector::ForwardVector;

	const float NormalSpeed = Movement->CalculateEffectiveShimmySpeed(VerticalWall, NormalSurface->ClimbSpeedMultiplier);
	const float IcySpeed = Movement->CalculateEffectiveShimmySpeed(VerticalWall, IcySurface->ClimbSpeedMultiplier);

	TestTrue(TEXT("Integration: normal surface speed should be 160"),
		FMath::IsNearlyEqual(NormalSpeed, 160.0f));
	TestTrue(TEXT("Integration: icy surface (0.5x) speed should be 80"),
		FMath::IsNearlyEqual(IcySpeed, 80.0f));
	TestTrue(TEXT("Integration: icy surface should be slower than normal"),
		IcySpeed < NormalSpeed);

	return true;
}

// FLOW: Lache state chain — validates the transition graph for the full Lache arc.
// Lache and LacheCatch are committed states; their outbound transitions are tested
// by entering them via SetClimbingState (which resets completion to 0) and verifying
// that None is always reachable (the escape hatch), and that LacheInAir/LacheMiss
// are reachable from the states that precede them in the graph.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingLacheStateChainIntegrationTest,
	"ClimbingSystem.Integration.Lache.FullStateChain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingLacheStateChainIntegrationTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Integration: movement component should be created"), Movement);
	if (!Movement) { return false; }

	// Hanging -> Lache: freely interruptible, valid immediately
	Movement->SetClimbingState(EClimbingState::Hanging);
	TestTrue(TEXT("Integration: Hanging -> Lache should be valid"),
		Movement->IsValidStateTransition(EClimbingState::Lache));

	// Lache is committed — None (abort) is always valid; LacheInAir blocked until completion
	Movement->SetClimbingState(EClimbingState::Lache);
	TestTrue(TEXT("Integration: Lache -> None (abort) should always be valid"),
		Movement->IsValidStateTransition(EClimbingState::None));
	TestFalse(TEXT("Integration: Lache -> LacheInAir blocked at 0% completion"),
		Movement->IsValidStateTransition(EClimbingState::LacheInAir));

	// LacheInAir (MinCompletion=0.0) — both outcomes valid immediately
	Movement->SetClimbingState(EClimbingState::LacheInAir);
	TestTrue(TEXT("Integration: LacheInAir -> LacheCatch should be valid"),
		Movement->IsValidStateTransition(EClimbingState::LacheCatch));
	TestTrue(TEXT("Integration: LacheInAir -> LacheMiss should be valid"),
		Movement->IsValidStateTransition(EClimbingState::LacheMiss));

	// LacheCatch is committed — None is always valid; Hanging blocked until completion
	Movement->SetClimbingState(EClimbingState::LacheCatch);
	TestTrue(TEXT("Integration: LacheCatch -> None (abort) should always be valid"),
		Movement->IsValidStateTransition(EClimbingState::None));
	TestFalse(TEXT("Integration: LacheCatch -> Hanging blocked at 0% completion"),
		Movement->IsValidStateTransition(EClimbingState::Hanging));

	return true;
}

// FLOW: Ladder state chain — None → OnLadder → LadderTransition → None
// Step 1: Start in None
// Step 2: Transition to OnLadder
// Step 3: Verify ladder speed calculations work
// Step 4: Transition to LadderTransition (exit)
// Step 5: Return to None

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingLadderStateChainIntegrationTest,
	"ClimbingSystem.Integration.Ladder.FullStateChain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingLadderStateChainIntegrationTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Integration: movement component should be created"), Movement);
	if (!Movement) { return false; }

	// Step 1-2: None -> OnLadder
	TestTrue(TEXT("Integration: None -> OnLadder should be valid"),
		Movement->IsValidStateTransition(EClimbingState::OnLadder));
	Movement->SetClimbingState(EClimbingState::OnLadder);

	// Step 3: Ladder speed works in this state
	Movement->BaseLadderClimbSpeed = 100.0f;
	Movement->LadderSprintMultiplier = 1.5f;
	const float Speed = Movement->CalculateEffectiveLadderSpeed(false, false, 1.0f);
	TestTrue(TEXT("Integration: ladder base speed should be 100"),
		FMath::IsNearlyEqual(Speed, 100.0f));

	// Step 4: OnLadder -> LadderTransition
	TestTrue(TEXT("Integration: OnLadder -> LadderTransition should be valid"),
		Movement->IsValidStateTransition(EClimbingState::LadderTransition));
	Movement->SetClimbingState(EClimbingState::LadderTransition);

	// Step 5: LadderTransition -> None
	TestTrue(TEXT("Integration: LadderTransition -> None should be valid"),
		Movement->IsValidStateTransition(EClimbingState::None));
	Movement->SetClimbingState(EClimbingState::None);
	TestEqual(TEXT("Integration: final state should be None"),
		Movement->CurrentClimbingState, EClimbingState::None);

	return true;
}

// FLOW: AnimInstance IK weight lifecycle — enable → blend → disable → reset
// Step 1: Create AnimInstance, verify all weights start at 0
// Step 2: Blend toward target weight over simulated frames
// Step 3: Verify weight increases
// Step 4: Reset all weights
// Step 5: Verify all weights are 0

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingIKWeightLifecycleIntegrationTest,
	"ClimbingSystem.Integration.IK.WeightLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingIKWeightLifecycleIntegrationTest::RunTest(const FString& Parameters)
{
	USkeletalMeshComponent* MeshComp = NewObject<USkeletalMeshComponent>();
	UClimbingAnimInstance* AnimInstance = MeshComp ? NewObject<UClimbingAnimInstance>(MeshComp) : nullptr;
	TestNotNull(TEXT("Integration: AnimInstance should be created"), AnimInstance);
	if (!AnimInstance) { return false; }

	// Step 1: All weights start at 0
	TestTrue(TEXT("Integration: initial left hand weight should be 0"),
		FMath::IsNearlyZero(AnimInstance->IKWeightHandLeft));
	TestTrue(TEXT("Integration: initial right hand weight should be 0"),
		FMath::IsNearlyZero(AnimInstance->IKWeightHandRight));

	// Step 2-3: Blend toward 1.0 over simulated time
	const float BlendedWeight = UClimbingAnimInstance::BlendIKWeight(0.0f, 1.0f, 0.15f, 0.15f);
	TestTrue(TEXT("Integration: blending full duration should reach target"),
		FMath::IsNearlyEqual(BlendedWeight, 1.0f));

	// Step 4-5: Set weights then reset
	AnimInstance->IKWeightHandLeft = 1.0f;
	AnimInstance->IKWeightHandRight = 0.8f;
	AnimInstance->IKWeightFootLeft = 0.6f;
	AnimInstance->IKWeightFootRight = 0.4f;
	AnimInstance->ResetAllIKWeights();

	TestTrue(TEXT("Integration: reset should zero left hand"),
		FMath::IsNearlyZero(AnimInstance->IKWeightHandLeft));
	TestTrue(TEXT("Integration: reset should zero right hand"),
		FMath::IsNearlyZero(AnimInstance->IKWeightHandRight));
	TestTrue(TEXT("Integration: reset should zero left foot"),
		FMath::IsNearlyZero(AnimInstance->IKWeightFootLeft));
	TestTrue(TEXT("Integration: reset should zero right foot"),
		FMath::IsNearlyZero(AnimInstance->IKWeightFootRight));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
