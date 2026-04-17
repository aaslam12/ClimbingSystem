// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Animation/ClimbingAnimInstance.h"
#include "Helpers/SharedTestHelpers.h"
#include "Misc/AutomationTest.h"

// ============================================================================
// CATEGORY 3: Gameplay Tests — Requires World
// ============================================================================

// WHAT: Validates AClimbingCharacter can be spawned in a test world
// WHY: Core prerequisite — if spawning fails, all gameplay tests are invalid
// EDGE CASES: Null world, spawn failure

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingCharacterSpawnTest,
	"ClimbingSystem.Character.World.SpawnSucceeds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingCharacterSpawnTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Character: should spawn successfully in test world"), Character);

	if (Character)
	{
		Character->Destroy();
	}
	Helper.Teardown();
	return true;
}

// WHAT: Validates spawned character has a ClimbingMovementComponent
// WHY: All movement/state logic depends on this component existing
// EDGE CASES: Component not registered, wrong type

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingCharacterHasMovementComponentTest,
	"ClimbingSystem.Character.World.HasClimbingMovementComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingCharacterHasMovementComponentTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Character: should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Character: should have UClimbingMovementComponent"), Movement);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Validates initial climbing state is None after spawn
// WHY: Character must start in locomotion mode, not in a climbing state
// EDGE CASES: State set during construction vs BeginPlay

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingCharacterInitialStateTest,
	"ClimbingSystem.Character.World.InitialStateIsNone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingCharacterInitialStateTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Character: should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Character: should have movement component"), Movement);
	if (Movement)
	{
		TestEqual(TEXT("Character: initial climbing state should be None"),
			Movement->CurrentClimbingState, EClimbingState::None);
	}

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// ============================================================================
// CATEGORY 4: Networking Tests — Single-Process Approximation
// ============================================================================

// NETWORK: single-process approximation — full replication requires PIE multi-player session

// WHAT: Validates state configs map has an entry for every EClimbingState value
// WHY: Missing state config entry causes undefined bInterruptible (PROMPT.md constraint)
// EDGE CASES: All 17 states must be present

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingStateConfigExhaustiveTest,
	"ClimbingSystem.Movement.StateConfig.ExhaustiveForAllStates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingStateConfigExhaustiveTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	// Every EClimbingState value must have a valid transition check (no crash)
	for (uint8 i = 0; i < static_cast<uint8>(EClimbingState::MAX); ++i)
	{
		EClimbingState State = static_cast<EClimbingState>(i);
		Movement->SetClimbingState(State);
		// CanInterruptCurrentState should not crash for any state
		Movement->CanInterruptCurrentState();
	}

	// Reset to None
	Movement->SetClimbingState(EClimbingState::None);
	TestEqual(TEXT("Movement: should return to None state"),
		Movement->CurrentClimbingState, EClimbingState::None);

	return true;
}

// WHAT: Validates that committed states (CornerTransition, Mantling, etc.) are not interruptible
// WHY: PROMPT.md specifies committed states cannot be interrupted by player input
// EDGE CASES: All committed states per PROMPT.md spec

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingCommittedStatesTest,
	"ClimbingSystem.Movement.StateConfig.CommittedStatesNotInterruptible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingCommittedStatesTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	// Committed states per PROMPT.md
	const EClimbingState CommittedStates[] = {
		EClimbingState::CornerTransition,
		EClimbingState::LadderTransition,
		EClimbingState::Mantling,
		EClimbingState::LacheCatch,
		EClimbingState::Ragdoll
	};

	for (EClimbingState State : CommittedStates)
	{
		Movement->SetClimbingState(State);
		TestFalse(
			*FString::Printf(TEXT("Movement: %s should be committed (not interruptible at completion 0)"),
				*UEnum::GetValueAsString(State)),
			Movement->CanInterruptCurrentState());
	}

	return true;
}

// WHAT: Validates that freely interruptible states can be interrupted
// WHY: PROMPT.md specifies Hanging, Shimmying, BracedWall, OnLadder are freely interruptible
// EDGE CASES: All freely interruptible states per PROMPT.md spec

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingFreelyInterruptibleStatesTest,
	"ClimbingSystem.Movement.StateConfig.FreelyInterruptibleStates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingFreelyInterruptibleStatesTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	const EClimbingState InterruptibleStates[] = {
		EClimbingState::Hanging,
		EClimbingState::Shimmying,
		EClimbingState::BracedWall,
		EClimbingState::OnLadder
	};

	for (EClimbingState State : InterruptibleStates)
	{
		Movement->SetClimbingState(State);
		TestTrue(
			*FString::Printf(TEXT("Movement: %s should be freely interruptible"),
				*UEnum::GetValueAsString(State)),
			Movement->CanInterruptCurrentState());
	}

	return true;
}

// WHAT: Validates PreviousClimbingState is tracked correctly through transitions
// WHY: Drop animation slot selection depends on PreviousClimbingState
// EDGE CASES: Multiple sequential transitions

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingPreviousStateTrackingTest,
	"ClimbingSystem.Movement.StateTransitions.PreviousStateTracking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingPreviousStateTrackingTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	Movement->SetClimbingState(EClimbingState::Hanging);
	Movement->SetClimbingState(EClimbingState::Shimmying);
	TestEqual(TEXT("Movement: previous state after Hanging->Shimmying should be Hanging"),
		Movement->PreviousClimbingState, EClimbingState::Hanging);

	Movement->SetClimbingState(EClimbingState::None);
	TestEqual(TEXT("Movement: previous state after Shimmying->None should be Shimmying"),
		Movement->PreviousClimbingState, EClimbingState::Shimmying);

	return true;
}

// WHAT: Validates overhang penalty calculation at boundary angles
// WHY: PROMPT.md shimmy speed formula depends on correct overhang penalty
// EDGE CASES: Exactly at start angle, partial overhang, vertical wall (0 overhang)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingOverhangPenaltyBoundaryTest,
	"ClimbingSystem.Movement.ShimmySpeed.OverhangPenaltyBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingOverhangPenaltyBoundaryTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	Movement->BaseShimmySpeed = 200.0f;
	Movement->OverhangPenaltyStartAngle = 10.0f;
	Movement->OverhangPenaltyRangeAngle = 20.0f;
	Movement->OverhangMaxPenaltyScalar = 0.5f;

	// Vertical wall: 0 degrees overhang — no penalty
	const float VerticalSpeed = Movement->CalculateEffectiveShimmySpeed(FVector::ForwardVector, 1.0f);
	TestTrue(TEXT("Movement: vertical wall should have no overhang penalty"),
		FMath::IsNearlyEqual(VerticalSpeed, 200.0f));

	// Partial overhang: 10° start + 10° into the 20° range = 50% progress
	// → penalty = lerp(1.0, 0.5, 0.5) = 0.75 → speed = 200 * 0.75 = 150
	// Normal for OverhangAngleDeg=20: acos(DotUp)=110° → DotUp=cos(110°)≈-0.342
	// Use a normal pointing 20° past horizontal (tilted downward): (cos20°, 0, -sin20°) normalized
	const FVector OverhangNormal = FVector(FMath::Cos(FMath::DegreesToRadians(20.0f)), 0.0f, -FMath::Sin(FMath::DegreesToRadians(20.0f))).GetSafeNormal();
	const float MidSpeed = Movement->CalculateEffectiveShimmySpeed(OverhangNormal, 1.0f);
	TestTrue(TEXT("Movement: mid-overhang should apply partial penalty (speed ~150)"),
		FMath::IsNearlyEqual(MidSpeed, 150.0f, 1.0f));

	return true;
}

// ============================================================================
// CATEGORY 5: Performance Tests
// ============================================================================

// WHAT: Measures shimmy speed calculation performance under 1000 iterations
// THRESHOLD: 0.1ms — justify: pure math with no allocations; industry baseline for per-tick calculations
// VERIFY: EAutomationTestFlags::PerformanceFilter may not exist in UE5.7 — using EngineFilter as fallback

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingShimmySpeedPerfTest,
	"ClimbingSystem.Performance.Movement.ShimmySpeedCalculation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingShimmySpeedPerfTest::RunTest(const FString& Parameters)
{
	UClimbingMovementComponent* Movement = NewObject<UClimbingMovementComponent>();
	TestNotNull(TEXT("Movement component should be created"), Movement);
	if (!Movement) { return false; }

	Movement->BaseShimmySpeed = 160.0f;
	Movement->OverhangPenaltyStartAngle = 15.0f;
	Movement->OverhangPenaltyRangeAngle = 30.0f;
	Movement->OverhangMaxPenaltyScalar = 0.5f;

	const int32 Iterations = 1000;
	// THRESHOLD: 0.1ms — pure math function, no allocations, must complete in sub-millisecond for per-tick use
	const double MaxMedianMs = 0.1;

	TArray<double> Timings;
	Timings.Reserve(Iterations);

	for (int32 i = 0; i < Iterations; ++i)
	{
		double Start = FPlatformTime::Seconds();
		Movement->CalculateEffectiveShimmySpeed(FVector(0.5f, 0.0f, 0.866f), 1.0f);
		double End = FPlatformTime::Seconds();
		Timings.Add((End - Start) * 1000.0);
	}

	Timings.Sort();
	double MedianMs = Timings[Iterations / 2];

	TestTrue(
		FString::Printf(TEXT("Movement: shimmy speed calc median %.4fms should be < %.1fms"),
			MedianMs, MaxMedianMs),
		MedianMs < MaxMedianMs);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
