// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingTypes.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingShimmySpeedVerticalWallTest,
	"ClimbingSystem.Movement.ShimmySpeed.VerticalWall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingShimmySpeedVerticalWallTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement)
	{
		return false;
	}

	Movement->BaseShimmySpeed = 160.0f;
	Movement->OverhangPenaltyStartAngle = 15.0f;
	Movement->OverhangPenaltyRangeAngle = 30.0f;
	Movement->OverhangMaxPenaltyScalar = 0.5f;

	const float Speed = Movement->CalculateEffectiveShimmySpeed(FVector::ForwardVector, 1.0f);
	TestEqual(TEXT("Vertical wall should apply no overhang penalty"), Speed, Movement->BaseShimmySpeed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingShimmySpeedOverhangTest,
	"ClimbingSystem.Movement.ShimmySpeed.FullOverhang",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingShimmySpeedOverhangTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement)
	{
		return false;
	}

	Movement->BaseShimmySpeed = 200.0f;
	Movement->OverhangPenaltyStartAngle = 10.0f;
	Movement->OverhangPenaltyRangeAngle = 20.0f;
	Movement->OverhangMaxPenaltyScalar = 0.4f;

	const float Speed = Movement->CalculateEffectiveShimmySpeed(-FVector::UpVector, 1.0f);
	const float Expected = Movement->BaseShimmySpeed * Movement->OverhangMaxPenaltyScalar;
	TestTrue(TEXT("Full overhang should clamp to max penalty scalar"), FMath::IsNearlyEqual(Speed, Expected, KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingLadderSpeedModifiersTest,
	"ClimbingSystem.Movement.LadderSpeed.Modifiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingLadderSpeedModifiersTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement)
	{
		return false;
	}

	Movement->BaseLadderClimbSpeed = 100.0f;
	Movement->LadderSprintMultiplier = 1.8f;
	Movement->LadderFastDescentMultiplier = 1.6f;

	const float BaseSpeed = Movement->CalculateEffectiveLadderSpeed(false, false, 1.0f);
	const float SprintSpeed = Movement->CalculateEffectiveLadderSpeed(true, false, 1.0f);
	const float FastDownSpeed = Movement->CalculateEffectiveLadderSpeed(false, true, 1.0f);
	const float SprintPrioritySpeed = Movement->CalculateEffectiveLadderSpeed(true, true, 1.0f);

	TestTrue(TEXT("Base ladder speed should be unchanged with no modifiers"), FMath::IsNearlyEqual(BaseSpeed, 100.0f));
	TestTrue(TEXT("Sprint speed should apply sprint multiplier"), FMath::IsNearlyEqual(SprintSpeed, 180.0f));
	TestTrue(TEXT("Fast descent speed should apply crouch multiplier"), FMath::IsNearlyEqual(FastDownSpeed, 160.0f));
	TestTrue(TEXT("Sprint should take precedence when both modifiers are active"), FMath::IsNearlyEqual(SprintPrioritySpeed, 180.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingPackedRpcPolicyTest,
	"ClimbingSystem.Movement.RPCPolicy.PackedMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingPackedRpcPolicyTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement)
	{
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	TestFalse(TEXT("Hanging should disable packed movement RPCs"), Movement->ShouldUsePackedMovementRPCs());

	Movement->SetClimbingState(EClimbingState::Shimmying);
	TestFalse(TEXT("Shimmying should disable packed movement RPCs"), Movement->ShouldUsePackedMovementRPCs());

	Movement->SetClimbingState(EClimbingState::OnLadder);
	TestFalse(TEXT("OnLadder should disable packed movement RPCs"), Movement->ShouldUsePackedMovementRPCs());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingStateTransitionRulesTest,
	"ClimbingSystem.Movement.StateTransitions.CoreRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingStateTransitionRulesTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement)
	{
		return false;
	}

	Movement->SetClimbingState(EClimbingState::None);
	TestTrue(TEXT("None -> Hanging should be valid"), Movement->IsValidStateTransition(EClimbingState::Hanging));
	TestTrue(TEXT("None -> OnLadder should be valid"), Movement->IsValidStateTransition(EClimbingState::OnLadder));
	TestFalse(TEXT("None -> CornerTransition should be invalid"), Movement->IsValidStateTransition(EClimbingState::CornerTransition));

	Movement->SetClimbingState(EClimbingState::Hanging);
	TestTrue(TEXT("Hanging -> Shimmying should be valid"), Movement->IsValidStateTransition(EClimbingState::Shimmying));
	TestTrue(TEXT("Hanging -> Lache should be valid"), Movement->IsValidStateTransition(EClimbingState::Lache));
	TestFalse(TEXT("Hanging -> OnLadder should be invalid"), Movement->IsValidStateTransition(EClimbingState::OnLadder));

	Movement->SetClimbingState(EClimbingState::OnLadder);
	TestTrue(TEXT("OnLadder -> LadderTransition should be valid"), Movement->IsValidStateTransition(EClimbingState::LadderTransition));
	TestFalse(TEXT("OnLadder -> ClimbingUp should be invalid"), Movement->IsValidStateTransition(EClimbingState::ClimbingUp));

	Movement->SetClimbingState(EClimbingState::ClimbingUp);
	TestFalse(TEXT("ClimbingUp should not be interruptible at montage completion 0"), Movement->CanInterruptCurrentState());
	TestTrue(TEXT("Transition to None should always be allowed"), Movement->IsValidStateTransition(EClimbingState::None));

	Movement->SetClimbingState(EClimbingState::Ragdoll);
	TestTrue(TEXT("Ragdoll -> None should be valid"), Movement->IsValidStateTransition(EClimbingState::None));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingStateInterruptibilityTest,
	"ClimbingSystem.Movement.StateTransitions.Interruptibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingStateInterruptibilityTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement)
	{
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	TestTrue(TEXT("Hanging should be interruptible"), Movement->CanInterruptCurrentState());

	Movement->SetClimbingState(EClimbingState::OnLadder);
	TestTrue(TEXT("OnLadder should be interruptible"), Movement->CanInterruptCurrentState());

	Movement->SetClimbingState(EClimbingState::CornerTransition);
	TestFalse(TEXT("CornerTransition should be committed at montage completion 0"), Movement->CanInterruptCurrentState());

	Movement->SetClimbingState(EClimbingState::LadderTransition);
	TestFalse(TEXT("LadderTransition should be committed at montage completion 0"), Movement->CanInterruptCurrentState());

	Movement->SetClimbingState(EClimbingState::LacheInAir);
	TestTrue(TEXT("LacheInAir should be interruptible at completion threshold 0"), Movement->CanInterruptCurrentState());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingDetectionResultContractTest,
	"ClimbingSystem.Types.DetectionResult.NetConversion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingDetectionResultContractTest::RunTest(const FString& Parameters)
{
	FClimbingDetectionResult LocalResult;
	LocalResult.LedgePosition = FVector(100.0f, -25.0f, 333.0f);
	LocalResult.SurfaceNormal = FVector(0.0f, -1.0f, 0.0f);
	LocalResult.SurfaceTier = EClimbSurfaceTier::ClimbableOneWay;
	LocalResult.ClearanceType = EClimbClearanceType::CrouchOnly;
	LocalResult.bValid = true;

	const FClimbingDetectionResultNet NetResult = LocalResult.ToNetResult();
	TestTrue(TEXT("Ledge position should be copied to net result"), FVector(NetResult.LedgePosition).Equals(LocalResult.LedgePosition, 1.0f));
	TestTrue(TEXT("Surface normal should be copied to net result"), FVector(NetResult.SurfaceNormal).Equals(LocalResult.SurfaceNormal.GetSafeNormal(), 0.01f));
	TestEqual(TEXT("Surface tier should match"), NetResult.SurfaceTier, LocalResult.SurfaceTier);
	TestEqual(TEXT("Clearance type should match"), NetResult.ClearanceType, LocalResult.ClearanceType);
	TestEqual(TEXT("Validity should match"), NetResult.bValid, LocalResult.bValid);

	LocalResult.Reset();
	TestFalse(TEXT("Reset should invalidate local result"), LocalResult.bValid);
	TestTrue(TEXT("Reset should clear local ledge position"), LocalResult.LedgePosition.IsNearlyZero());

	FClimbingDetectionResultNet ResetNet = NetResult;
	ResetNet.Reset();
	TestFalse(TEXT("Reset should invalidate net result"), ResetNet.bValid);
	TestTrue(TEXT("Reset should clear net ledge position"), FVector(ResetNet.LedgePosition).IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingSetStateNoopTest,
	"ClimbingSystem.Movement.StateTransitions.NoopSameState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingSetStateNoopTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement)
	{
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	const EClimbingState PreviousAfterFirstSet = Movement->PreviousClimbingState;
	Movement->SetClimbingState(EClimbingState::Hanging);

	TestEqual(TEXT("Setting same state should not update previous state"), Movement->PreviousClimbingState, PreviousAfterFirstSet);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMaxSpeedByStateTest,
	"ClimbingSystem.Movement.Speed.MaxSpeedByState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMaxSpeedByStateTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement)
	{
		return false;
	}

	Movement->BaseShimmySpeed = 170.0f;
	Movement->BaseLadderClimbSpeed = 110.0f;

	Movement->SetClimbingState(EClimbingState::Shimmying);
	TestTrue(TEXT("Shimmying max speed should match BaseShimmySpeed"), FMath::IsNearlyEqual(Movement->GetMaxSpeed(), 170.0f));

	Movement->SetClimbingState(EClimbingState::BracedShimmying);
	TestTrue(TEXT("Braced shimmying max speed should match BaseShimmySpeed"), FMath::IsNearlyEqual(Movement->GetMaxSpeed(), 170.0f));

	Movement->SetClimbingState(EClimbingState::OnLadder);
	TestTrue(TEXT("OnLadder max speed should match BaseLadderClimbSpeed"), FMath::IsNearlyEqual(Movement->GetMaxSpeed(), 110.0f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
