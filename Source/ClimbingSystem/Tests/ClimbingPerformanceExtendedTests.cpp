// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingTypes.h"
#include "Misc/AutomationTest.h"
#include "HAL/PlatformTime.h"

// ============================================================================
// CATEGORY 5: Performance Tests — Extended
// ============================================================================

// WHAT: Measures ladder speed calculation performance under 1000 iterations
// THRESHOLD: 0.1ms — pure math with no allocations; must complete sub-millisecond for per-tick use
// EDGE CASES: Cycles through sprint/crouch/normal combos

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingLadderSpeedCalcPerfTest,
	"ClimbingSystem.Performance.Movement.LadderSpeedCalculation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingLadderSpeedCalcPerfTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	Movement->BaseLadderClimbSpeed = 100.0f;
	Movement->LadderSprintMultiplier = 1.8f;
	Movement->LadderFastDescentMultiplier = 1.6f;

	const int32 Iterations = 1000;
	// THRESHOLD: 0.1ms — pure math function, no allocations, per-tick budget
	const double MaxMedianMs = 0.1;

	TArray<double> Timings;
	Timings.Reserve(Iterations);

	for (int32 i = 0; i < Iterations; ++i)
	{
		const bool bSprint = (i % 3 == 0);
		const bool bCrouch = (i % 5 == 0);
		const float Mult = (i % 2 == 0) ? 1.0f : 0.7f;

		double Start = FPlatformTime::Seconds();
		Movement->CalculateEffectiveLadderSpeed(bSprint, bCrouch, Mult);
		double End = FPlatformTime::Seconds();
		Timings.Add((End - Start) * 1000.0);
	}

	Timings.Sort();
	double MedianMs = Timings[Iterations / 2];

	TestTrue(
		FString::Printf(TEXT("Movement: ladder speed calc median %.4fms should be < %.1fms"),
			MedianMs, MaxMedianMs),
		MedianMs < MaxMedianMs);

	return true;
}

// WHAT: Measures state transition validation performance under 1000 iterations
// THRESHOLD: 0.1ms — map lookup, must be fast for per-input-event validation
// EDGE CASES: Cycles through valid and invalid transition pairs

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingStateTransitionValidationPerfTest,
	"ClimbingSystem.Performance.Movement.StateTransitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingStateTransitionValidationPerfTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	const EClimbingState FromStates[] = {
		EClimbingState::None, EClimbingState::Hanging, EClimbingState::Shimmying,
		EClimbingState::BracedWall, EClimbingState::OnLadder
	};
	const EClimbingState ToStates[] = {
		EClimbingState::Hanging, EClimbingState::None, EClimbingState::CornerTransition,
		EClimbingState::BracedShimmying, EClimbingState::LadderTransition
	};

	const int32 Iterations = 1000;
	// THRESHOLD: 0.1ms — map/switch lookup, called on every input event
	const double MaxMedianMs = 0.1;

	TArray<double> Timings;
	Timings.Reserve(Iterations);

	for (int32 i = 0; i < Iterations; ++i)
	{
		const int32 Idx = i % 5;
		Movement->SetClimbingState(FromStates[Idx]);

		double Start = FPlatformTime::Seconds();
		Movement->IsValidStateTransition(ToStates[Idx]);
		double End = FPlatformTime::Seconds();
		Timings.Add((End - Start) * 1000.0);
	}

	Timings.Sort();
	double MedianMs = Timings[Iterations / 2];

	TestTrue(
		FString::Printf(TEXT("Movement: state transition validation median %.4fms should be < %.1fms"),
			MedianMs, MaxMedianMs),
		MedianMs < MaxMedianMs);

	return true;
}

// WHAT: Measures detection result ToNetResult + Reset conversion performance
// THRESHOLD: 0.1ms — struct copy operations, called on every grab attempt
// EDGE CASES: Full field population before each conversion

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingDetectionResultConversionPerfTest,
	"ClimbingSystem.Performance.Types.DetectionResultConversion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingDetectionResultConversionPerfTest::RunTest(const FString& Parameters)
{
	const int32 Iterations = 1000;
	// THRESHOLD: 0.1ms — struct copy + field assignment, no allocations
	const double MaxMedianMs = 0.1;

	TArray<double> Timings;
	Timings.Reserve(Iterations);

	for (int32 i = 0; i < Iterations; ++i)
	{
		FClimbingDetectionResult LocalResult;
		LocalResult.LedgePosition = FVector(100.0f, 200.0f, 300.0f);
		LocalResult.SurfaceNormal = FVector(0.0f, -1.0f, 0.0f);
		LocalResult.SurfaceTier = EClimbSurfaceTier::Climbable;
		LocalResult.ClearanceType = EClimbClearanceType::Full;
		LocalResult.bValid = true;

		double Start = FPlatformTime::Seconds();
		FClimbingDetectionResultNet NetResult = LocalResult.ToNetResult();
		NetResult.Reset();
		double End = FPlatformTime::Seconds();
		Timings.Add((End - Start) * 1000.0);
	}

	Timings.Sort();
	double MedianMs = Timings[Iterations / 2];

	TestTrue(
		FString::Printf(TEXT("Types: detection result conversion median %.4fms should be < %.1fms"),
			MedianMs, MaxMedianMs),
		MedianMs < MaxMedianMs);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
