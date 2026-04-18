// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingTypes.h"
#include "Misc/AutomationTest.h"

// WHAT: Validates shimmy speed returns 0 when surface multiplier is 0
// WHY: Zero multiplier (e.g. frozen surface) must produce zero movement, not NaN
// EDGE CASES: Multiplier = 0.0

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingShimmySpeedZeroMultiplierTest,
	"ClimbingSystem.Movement.ShimmySpeed.ZeroMultiplierReturnsZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingShimmySpeedZeroMultiplierTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	Movement->BaseShimmySpeed = 160.0f;
	Movement->OverhangPenaltyStartAngle = 15.0f;
	Movement->OverhangPenaltyRangeAngle = 30.0f;
	Movement->OverhangMaxPenaltyScalar = 0.5f;

	const float Speed = Movement->CalculateEffectiveShimmySpeed(FVector::ForwardVector, 0.0f);
	TestTrue(TEXT("Movement: zero surface multiplier should produce zero shimmy speed"),
		FMath::IsNearlyZero(Speed));

	return true;
}

// WHAT: Validates overhang penalty is exactly 1.0 at the boundary start angle
// WHY: At exactly OverhangPenaltyStartAngle, penalty lerp alpha should be 0 (no penalty)
// EDGE CASES: Exact boundary value

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingShimmySpeedPartialOverhangTest,
	"ClimbingSystem.Movement.ShimmySpeed.BoundaryAngleNoPenalty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingShimmySpeedPartialOverhangTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	Movement->BaseShimmySpeed = 200.0f;
	Movement->OverhangPenaltyStartAngle = 15.0f;
	Movement->OverhangPenaltyRangeAngle = 30.0f;
	Movement->OverhangMaxPenaltyScalar = 0.5f;

	// Normal at exactly OverhangPenaltyStartAngle (15 degrees past vertical)
	// OverhangAngleDeg = angle(Normal, WorldUp) - 90 = 105 - 90 = 15
	// At exactly start angle, penalty alpha = 0 → penalty = 1.0
	const FVector BoundaryNormal = FVector(
		FMath::Cos(FMath::DegreesToRadians(15.0f)), 0.0f,
		-FMath::Sin(FMath::DegreesToRadians(15.0f))).GetSafeNormal();

	const float Speed = Movement->CalculateEffectiveShimmySpeed(BoundaryNormal, 1.0f);
	TestTrue(TEXT("Movement: at exactly OverhangPenaltyStartAngle, speed should equal BaseShimmySpeed (no penalty)"),
		FMath::IsNearlyEqual(Speed, 200.0f, 1.0f));

	return true;
}

// WHAT: Validates ladder speed returns 0 when surface multiplier is 0
// WHY: Zero multiplier must produce zero movement
// EDGE CASES: Multiplier = 0.0

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingLadderSpeedZeroMultiplierTest,
	"ClimbingSystem.Movement.LadderSpeed.ZeroMultiplierReturnsZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingLadderSpeedZeroMultiplierTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	Movement->BaseLadderClimbSpeed = 100.0f;
	Movement->LadderSprintMultiplier = 1.8f;
	Movement->LadderFastDescentMultiplier = 1.6f;

	const float Speed = Movement->CalculateEffectiveLadderSpeed(false, false, 0.0f);
	TestTrue(TEXT("Movement: zero surface multiplier should produce zero ladder speed"),
		FMath::IsNearlyZero(Speed));

	return true;
}

// WHAT: Validates BracedWall transition rules
// WHY: BracedWall -> Hanging is valid (lip detected), BracedWall -> Shimmying is invalid (must go through BracedShimmying)
// EDGE CASES: Transition direction matters

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingBracedToHangTransitionTest,
	"ClimbingSystem.Movement.StateTransition.BracedWallToHangValidShimmyInvalid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingBracedToHangTransitionTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	Movement->SetClimbingState(EClimbingState::BracedWall);
	TestTrue(TEXT("Movement: BracedWall -> Hanging should be valid (lip detected above)"),
		Movement->IsValidStateTransition(EClimbingState::Hanging));
	TestFalse(TEXT("Movement: BracedWall -> Shimmying should be invalid (must use BracedShimmying)"),
		Movement->IsValidStateTransition(EClimbingState::Shimmying));

	return true;
}

// WHAT: Validates Ragdoll only allows transition to None
// WHY: Ragdoll is a committed state — recovery always goes through None first
// EDGE CASES: All non-None states should be rejected

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRagdollTransitionRestrictionsTest,
	"ClimbingSystem.Movement.StateTransition.RagdollOnlyExitsToNone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRagdollTransitionRestrictionsTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	Movement->SetClimbingState(EClimbingState::Ragdoll);
	TestTrue(TEXT("Movement: Ragdoll -> None should be valid"),
		Movement->IsValidStateTransition(EClimbingState::None));
	TestFalse(TEXT("Movement: Ragdoll -> Hanging should be invalid"),
		Movement->IsValidStateTransition(EClimbingState::Hanging));
	TestFalse(TEXT("Movement: Ragdoll -> Shimmying should be invalid"),
		Movement->IsValidStateTransition(EClimbingState::Shimmying));
	TestFalse(TEXT("Movement: Ragdoll -> OnLadder should be invalid"),
		Movement->IsValidStateTransition(EClimbingState::OnLadder));

	return true;
}

// WHAT: Validates Mantling is committed and blocks early exit to non-None states
// WHY: Mantling animation must complete — only None (abort) is allowed at 0% completion
// EDGE CASES: Committed state at 0% completion

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMantlingTransitionRestrictionsTest,
	"ClimbingSystem.Movement.StateTransition.MantlingCommittedBlocksEarlyExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMantlingTransitionRestrictionsTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	Movement->SetClimbingState(EClimbingState::Mantling);
	TestFalse(TEXT("Movement: Mantling should not be interruptible at 0% completion"),
		Movement->CanInterruptCurrentState());
	TestTrue(TEXT("Movement: Mantling -> None (abort) should always be valid"),
		Movement->IsValidStateTransition(EClimbingState::None));
	TestFalse(TEXT("Movement: Mantling -> Hanging should be blocked at 0% completion"),
		Movement->IsValidStateTransition(EClimbingState::Hanging));

	return true;
}

// WHAT: Validates DroppingDown -> None is valid
// WHY: Drop animation completes and returns to locomotion
// EDGE CASES: DroppingDown exit path

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingDroppingDownTransitionTest,
	"ClimbingSystem.Movement.StateTransition.DroppingDownExitsToNone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingDroppingDownTransitionTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	Movement->SetClimbingState(EClimbingState::DroppingDown);
	TestTrue(TEXT("Movement: DroppingDown -> None should be valid"),
		Movement->IsValidStateTransition(EClimbingState::None));

	return true;
}

// WHAT: Validates GetMaxSpeed in None state returns default walking speed
// WHY: After exiting climbing, movement speed must revert to standard locomotion
// EDGE CASES: None state should not return climbing-specific speeds

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMovementModeNoneStateTest,
	"ClimbingSystem.Movement.MaxSpeed.NoneStateReturnsWalkingSpeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMovementModeNoneStateTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	Movement->BaseShimmySpeed = 170.0f;
	Movement->BaseLadderClimbSpeed = 110.0f;

	Movement->SetClimbingState(EClimbingState::None);
	const float NoneSpeed = Movement->GetMaxSpeed();

	// None state should NOT return shimmy or ladder speed
	TestTrue(TEXT("Movement: None state max speed should not equal BaseShimmySpeed"),
		!FMath::IsNearlyEqual(NoneSpeed, 170.0f));
	TestTrue(TEXT("Movement: None state max speed should not equal BaseLadderClimbSpeed"),
		!FMath::IsNearlyEqual(NoneSpeed, 110.0f));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
