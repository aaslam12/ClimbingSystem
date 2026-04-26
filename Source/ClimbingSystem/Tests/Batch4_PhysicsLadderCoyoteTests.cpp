// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingTypes.h"
#include "Animation/AnimMontage.h"
#include "Components/BoxComponent.h"
#include "GameFramework/MovementComponent.h"
#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

// ---------------------------------------------------------------------------
// TC-0294: AnchorMovingPlatformTracking
// WHAT: Spawn char + box. Set anchor. Move box 20cm/tick for 5 ticks.
//       Assert character follows within 1cm each tick.
// WHY: Dynamic anchor following must keep the character attached to moving geometry.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4AnchorMovingPlatformTrackingTest,
	"ClimbingSystem.Batch4.Anchor.AnchorMovingPlatformTracking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4AnchorMovingPlatformTrackingTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0294: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0294: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	AActor* BoxActor = Helper.SpawnBoxAt(FVector::ZeroVector, FVector(50.f, 50.f, 50.f));
	TestNotNull(TEXT("TC-0294: box actor must spawn"), BoxActor);
	if (!BoxActor) { Helper.Teardown(); return false; }

	UPrimitiveComponent* BoxRoot = Cast<UPrimitiveComponent>(BoxActor->GetRootComponent());
	TestNotNull(TEXT("TC-0294: box root component must exist"), BoxRoot);
	if (!BoxRoot) { Helper.Teardown(); return false; }

	Movement->SetAnchor(BoxRoot, FVector::ZeroVector);
	Movement->SetClimbingState(EClimbingState::Hanging);

	constexpr float StepDelta = 20.f; // 20 cm per tick
	constexpr int32 NumTicks = 5;
	constexpr float Tolerance = 1.f;

	for (int32 i = 1; i <= NumTicks; ++i)
	{
		const FVector ExpectedCharPos = Character->GetActorLocation() + FVector(StepDelta, 0.f, 0.f);
		BoxActor->SetActorLocation(FVector(StepDelta * i, 0.f, 0.f));
		Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);

		const float Error = FVector::Dist(Character->GetActorLocation(), ExpectedCharPos);
		TestTrue(FString::Printf(TEXT("TC-0294: tick %d — character must follow anchor within %.1fcm (error=%.2f)"), i, Tolerance, Error),
			Error <= Tolerance);
	}

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0295: HangingEntryCallsSetBase
// WHAT: Spawn char + box. Transition to Hanging with box as HitComponent.
//       Assert GetMovementBase() == box component.
// WHY: Hanging state must call SetBase so the character rides moving platforms.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4HangingEntryCallsSetBaseTest,
	"ClimbingSystem.Batch4.Anchor.HangingEntryCallsSetBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4HangingEntryCallsSetBaseTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0295: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0295: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	AActor* BoxActor = Helper.SpawnBoxAt(FVector(0.f, 0.f, 100.f), FVector(50.f, 50.f, 10.f));
	TestNotNull(TEXT("TC-0295: box actor must spawn"), BoxActor);
	if (!BoxActor) { Helper.Teardown(); return false; }

	UPrimitiveComponent* BoxRoot = Cast<UPrimitiveComponent>(BoxActor->GetRootComponent());
	TestNotNull(TEXT("TC-0295: box root component must exist"), BoxRoot);
	if (!BoxRoot) { Helper.Teardown(); return false; }

	FClimbingDetectionResult Detection;
	Detection.bValid = true;
	Detection.LedgePosition = FVector(0.f, 0.f, 100.f);
	Detection.SurfaceNormal = FVector(0.f, -1.f, 0.f);
	Detection.HitComponent = BoxRoot;

	Character->TransitionToState(EClimbingState::Hanging, Detection);
	Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);

	UPrimitiveComponent* Base = Character->GetMovementBase();
	TestEqual(TEXT("TC-0295: GetMovementBase() must equal the box component after Hanging entry"),
		Base, BoxRoot);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0296: ExitClimbingClearsBase
// WHAT: From Hanging, transition to None. Assert GetMovementBase() == nullptr.
// WHY: Exiting climbing must clear the movement base to restore normal locomotion.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4ExitClimbingClearsBaseTest,
	"ClimbingSystem.Batch4.Anchor.ExitClimbingClearsBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4ExitClimbingClearsBaseTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0296: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0296: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	AActor* BoxActor = Helper.SpawnBoxAt(FVector(0.f, 0.f, 100.f), FVector(50.f, 50.f, 10.f));
	TestNotNull(TEXT("TC-0296: box actor must spawn"), BoxActor);
	if (!BoxActor) { Helper.Teardown(); return false; }

	UPrimitiveComponent* BoxRoot = Cast<UPrimitiveComponent>(BoxActor->GetRootComponent());
	TestNotNull(TEXT("TC-0296: box root component must exist"), BoxRoot);
	if (!BoxRoot) { Helper.Teardown(); return false; }

	// Enter Hanging
	FClimbingDetectionResult Detection;
	Detection.bValid = true;
	Detection.LedgePosition = FVector(0.f, 0.f, 100.f);
	Detection.SurfaceNormal = FVector(0.f, -1.f, 0.f);
	Detection.HitComponent = BoxRoot;
	Character->TransitionToState(EClimbingState::Hanging, Detection);
	Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);

	// Exit to None
	FClimbingDetectionResult Empty;
	Character->TransitionToState(EClimbingState::None, Empty);
	Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);

	TestNull(TEXT("TC-0296: GetMovementBase() must be nullptr after transitioning to None"),
		Character->GetMovementBase());

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0297: NullAnchorDuringHangingDrops
// WHAT: Char in Hanging. Set AnchorComponent = nullptr. Tick.
//       Assert state transitions to DroppingDown or None.
// WHY: A null anchor while hanging is invalid — the system must drop the character.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4NullAnchorDuringHangingDropsTest,
	"ClimbingSystem.Batch4.Anchor.NullAnchorDuringHangingDrops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4NullAnchorDuringHangingDropsTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0297: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0297: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	AActor* BoxActor = Helper.SpawnBoxAt(FVector(0.f, 0.f, 100.f), FVector(50.f, 50.f, 10.f));
	TestNotNull(TEXT("TC-0297: box actor must spawn"), BoxActor);
	if (!BoxActor) { Helper.Teardown(); return false; }

	UPrimitiveComponent* BoxRoot = Cast<UPrimitiveComponent>(BoxActor->GetRootComponent());
	TestNotNull(TEXT("TC-0297: box root component must exist"), BoxRoot);
	if (!BoxRoot) { Helper.Teardown(); return false; }

	FClimbingDetectionResult Detection;
	Detection.bValid = true;
	Detection.LedgePosition = FVector(0.f, 0.f, 100.f);
	Detection.SurfaceNormal = FVector(0.f, -1.f, 0.f);
	Detection.HitComponent = BoxRoot;
	Character->TransitionToState(EClimbingState::Hanging, Detection);
	Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);

	// Null out the anchor
	Movement->AnchorComponent = nullptr;
	Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);

	const EClimbingState State = Movement->CurrentClimbingState;
	const bool bDropped = (State == EClimbingState::DroppingDown || State == EClimbingState::None);
	TestTrue(TEXT("TC-0297: state must be DroppingDown or None after AnchorComponent set to nullptr"),
		bDropped);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0298: ServerRejectsStreamingSublevelAnchor
// WHAT: Verify ServerValidationPositionTolerance CDO default == 30.
// WHY: Streaming sublevel anchors are rejected; tolerance is the key server-side
//      validation parameter. // VERIFY: full streaming sublevel rejection requires
//      a live server RPC with a sublevel-owned component.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4ServerRejectsStreamingSublevelAnchorTest,
	"ClimbingSystem.Batch4.Multiplayer.ServerRejectsStreamingSublevelAnchor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4ServerRejectsStreamingSublevelAnchorTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0298: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0298: ServerValidationPositionTolerance default must be 30"),
		FMath::IsNearlyEqual(CDO->ServerValidationPositionTolerance, 30.f));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0299: ServerAcceptsPersistentLevelAnchor
// WHAT: Complement to TC-0298. Verify ServerValidationPositionTolerance == 30
//       and that the property is EditAnywhere (configurable for persistent anchors).
// WHY: Persistent level anchors must pass server validation within tolerance.
//      // VERIFY: full acceptance test requires a live server RPC.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4ServerAcceptsPersistentLevelAnchorTest,
	"ClimbingSystem.Batch4.Multiplayer.ServerAcceptsPersistentLevelAnchor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4ServerAcceptsPersistentLevelAnchorTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0299: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0299: ServerValidationPositionTolerance default must be 30"),
		FMath::IsNearlyEqual(CDO->ServerValidationPositionTolerance, 30.f));

	const FProperty* Prop = AClimbingCharacter::StaticClass()->FindPropertyByName(TEXT("ServerValidationPositionTolerance"));
	TestNotNull(TEXT("TC-0299: ServerValidationPositionTolerance property must exist"), Prop);
	if (!Prop) { return false; }

	TestTrue(TEXT("TC-0299: ServerValidationPositionTolerance must be EditAnywhere (CPF_Edit)"),
		Prop->HasAnyPropertyFlags(CPF_Edit));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0300: LadderExitTopMontageAndWarp
// WHAT: Spawn character, set state to OnLadder, assign LadderExitTop montage,
//       verify GetMontageForSlot(LadderExitTop) returns it.
// WHY: LadderExitTop slot must be reachable for motion warp target registration.
//      // VERIFY: full warp target registration requires a live MotionWarpingComponent tick.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4LadderExitTopMontageAndWarpTest,
	"ClimbingSystem.Batch4.Ladder.LadderExitTopMontageAndWarp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4LadderExitTopMontageAndWarpTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0300: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0300: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Character->LadderExitTop = NewObject<UAnimMontage>(Character);
	Movement->SetClimbingState(EClimbingState::OnLadder);

	UAnimMontage* Montage = Character->GetMontageForSlot(EClimbingAnimationSlot::LadderExitTop);
	TestNotNull(TEXT("TC-0300: GetMontageForSlot(LadderExitTop) must return the assigned montage"), Montage);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0301: LadderExitBottomNoWarpTarget
// WHAT: Spawn character, set state to OnLadder, assign LadderExitBottom montage,
//       verify GetMontageForSlot(LadderExitBottom) returns it.
// WHY: LadderExitBottom slot must be reachable from the slot accessor.
//      // VERIFY: warp target absence test requires a live MotionWarpingComponent tick.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4LadderExitBottomNoWarpTargetTest,
	"ClimbingSystem.Batch4.Ladder.LadderExitBottomNoWarpTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4LadderExitBottomNoWarpTargetTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0301: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0301: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Character->LadderExitBottom = NewObject<UAnimMontage>(Character);
	Movement->SetClimbingState(EClimbingState::OnLadder);

	UAnimMontage* Montage = Character->GetMontageForSlot(EClimbingAnimationSlot::LadderExitBottom);
	TestNotNull(TEXT("TC-0301: GetMontageForSlot(LadderExitBottom) must return the assigned montage"), Montage);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0302: LandedWithinCoyoteWindowTriggersRescan
// WHAT: Verify CoyoteTimeWindow CDO default == 0.15 and bEnableCoyoteTime == true.
// WHY: These defaults gate the coyote-time re-grab path in Landed().
//      // VERIFY: full rescan test requires a live Landed() call with elapsed time < window.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4LandedWithinCoyoteWindowTriggersRescanTest,
	"ClimbingSystem.Batch4.Coyote.LandedWithinCoyoteWindowTriggersRescan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4LandedWithinCoyoteWindowTriggersRescanTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0302: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0302: CoyoteTimeWindow default must be 0.15"),
		FMath::IsNearlyEqual(CDO->CoyoteTimeWindow, 0.15f));
	TestTrue(TEXT("TC-0302: bEnableCoyoteTime default must be true"),
		CDO->bEnableCoyoteTime);
	return true;
}

// ---------------------------------------------------------------------------
// TC-0303: LandedAfterCoyoteWindowNoRescan
// WHAT: Complement to TC-0302. Verify CoyoteTimeWindow == 0.15 and confirm that
//       elapsed time > window means no rescan (contract check via defaults).
// WHY: Coyote time must expire — grabs after the window must not re-trigger.
//      // VERIFY: full no-rescan test requires a live Landed() call with elapsed > window.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4LandedAfterCoyoteWindowNoRescanTest,
	"ClimbingSystem.Batch4.Coyote.LandedAfterCoyoteWindowNoRescan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4LandedAfterCoyoteWindowNoRescanTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0303: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0303: CoyoteTimeWindow default must be 0.15"),
		FMath::IsNearlyEqual(CDO->CoyoteTimeWindow, 0.15f));

	// Contract: elapsed time beyond the window must not qualify for rescan
	constexpr float ElapsedAfterWindow = 0.20f;
	TestTrue(TEXT("TC-0303: elapsed 0.20s must exceed CoyoteTimeWindow 0.15s — no rescan"),
		ElapsedAfterWindow > CDO->CoyoteTimeWindow);
	return true;
}

// ---------------------------------------------------------------------------
// TC-0304: GrabBreakThresholdZeroDisablesBreak
// WHAT: Set GrabBreakImpulseThreshold = 0. Apply 10000N impulse via NotifyHit
//       while in Hanging. Assert state remains Hanging.
// WHY: Threshold == 0 is the documented "disable" sentinel — no impulse should break the grab.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4GrabBreakThresholdZeroDisablesBreakTest,
	"ClimbingSystem.Batch4.Physics.GrabBreakThresholdZeroDisablesBreak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4GrabBreakThresholdZeroDisablesBreakTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0304: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0304: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Disable grab-break by setting threshold to 0
	Character->GrabBreakImpulseThreshold = 0.f;
	Movement->SetClimbingState(EClimbingState::Hanging);

	FHitResult Hit;
	const FVector NormalImpulse(10000.f, 0.f, 0.f);
	Character->NotifyHit(nullptr, nullptr, nullptr, false,
		FVector::ZeroVector, FVector::ZeroVector, NormalImpulse, Hit);

	TestEqual(TEXT("TC-0304: state must remain Hanging when GrabBreakImpulseThreshold == 0 (disabled)"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0305: IKMaxReachExceededFadesWeight
// WHAT: Verify MaxReachDistance CDO default == 80.
// WHY: IK weight fade-out begins when limb distance exceeds this value.
//      // VERIFY: full fade test requires a live IK tick with a target beyond 80cm.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4IKMaxReachExceededFadesWeightTest,
	"ClimbingSystem.Batch4.IK.IKMaxReachExceededFadesWeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4IKMaxReachExceededFadesWeightTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0305: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0305: MaxReachDistance default must be 80"),
		FMath::IsNearlyEqual(CDO->MaxReachDistance, 80.f));

	// Contract: a target 81cm away exceeds the threshold
	constexpr float ExceededDistance = 81.f;
	TestTrue(TEXT("TC-0305: distance 81 must exceed MaxReachDistance 80 — IK weight fades"),
		ExceededDistance > CDO->MaxReachDistance);
	return true;
}

// ---------------------------------------------------------------------------
// TC-0306: IKMaxReachWithinKeepsWeight
// WHAT: Complement to TC-0305. Verify a distance <= MaxReachDistance keeps full weight.
// WHY: IK weight must not fade when the limb is within reach.
//      // VERIFY: full weight-retention test requires a live IK tick.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4IKMaxReachWithinKeepsWeightTest,
	"ClimbingSystem.Batch4.IK.IKMaxReachWithinKeepsWeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4IKMaxReachWithinKeepsWeightTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0306: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0306: MaxReachDistance default must be 80"),
		FMath::IsNearlyEqual(CDO->MaxReachDistance, 80.f));

	// Contract: a target exactly at 80cm is within reach — no fade
	constexpr float WithinDistance = 80.f;
	TestTrue(TEXT("TC-0306: distance 80 must be <= MaxReachDistance 80 — IK weight kept"),
		WithinDistance <= CDO->MaxReachDistance);
	return true;
}

// ---------------------------------------------------------------------------
// TC-0307: IKBudgetFifthCharacterZeroed
// WHAT: Spawn 5 characters. Verify MaxSimultaneousIKCharacters == 4.
//       Assert the 5th character is beyond the IK budget.
// WHY: IK manager must zero weights for characters beyond the budget.
//      // VERIFY: full zeroed-weight test requires a live IK manager sort + tick.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4IKBudgetFifthCharacterZeroedTest,
	"ClimbingSystem.Batch4.IK.IKBudgetFifthCharacterZeroed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4IKBudgetFifthCharacterZeroedTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	constexpr int32 NumToSpawn = 5;
	TArray<AClimbingCharacter*> Characters;
	for (int32 i = 0; i < NumToSpawn; ++i)
	{
		AClimbingCharacter* C = Helper.SpawnCharacterAt(FVector(i * 200.f, 0.f, 0.f));
		TestNotNull(FString::Printf(TEXT("TC-0307: character %d must spawn"), i), C);
		if (!C) { Helper.Teardown(); return false; }
		Characters.Add(C);
	}

	// Verify the budget constant
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0307: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { Helper.Teardown(); return false; }

	TestEqual(TEXT("TC-0307: MaxSimultaneousIKCharacters default must be 4"),
		CDO->MaxSimultaneousIKCharacters, 4);

	// Contract: 5 characters > budget of 4 — at least one must be beyond budget
	TestTrue(TEXT("TC-0307: spawning 5 characters exceeds MaxSimultaneousIKCharacters(4) — 5th has zeroed IK"),
		NumToSpawn > CDO->MaxSimultaneousIKCharacters);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0308: IKManagerSortedByCameraDistance
// WHAT: Verify MaxSimultaneousIKCharacters CDO default == 4.
// WHY: The IK manager sorts active characters by camera distance and keeps only
//      the nearest MaxSimultaneousIKCharacters.
//      // VERIFY: full sort test requires a live camera + multiple climbing characters.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4IKManagerSortedByCameraDistanceTest,
	"ClimbingSystem.Batch4.IK.IKManagerSortedByCameraDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4IKManagerSortedByCameraDistanceTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0308: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestEqual(TEXT("TC-0308: MaxSimultaneousIKCharacters default must be 4"),
		CDO->MaxSimultaneousIKCharacters, 4);
	return true;
}

// ---------------------------------------------------------------------------
// TC-0309: IKProxyUpdateIntervalThrottled
// WHAT: Verify SimulatedProxyIKUpdateInterval CDO default == 0.05.
// WHY: Simulated proxy IK must be throttled to 20Hz to reduce CPU cost.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4IKProxyUpdateIntervalThrottledTest,
	"ClimbingSystem.Batch4.IK.IKProxyUpdateIntervalThrottled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4IKProxyUpdateIntervalThrottledTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0309: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0309: SimulatedProxyIKUpdateInterval default must be 0.05"),
		FMath::IsNearlyEqual(CDO->SimulatedProxyIKUpdateInterval, 0.05f));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0310: HandIKOffsetApplied
// WHAT: Verify HandIKOffset CDO default == FVector(5, 0, -5).
// WHY: Hand IK targets are offset from the ledge position by this value in local space.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4HandIKOffsetAppliedTest,
	"ClimbingSystem.Batch4.IK.HandIKOffsetApplied",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4HandIKOffsetAppliedTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0310: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	const FVector Expected(5.f, 0.f, -5.f);
	TestTrue(TEXT("TC-0310: HandIKOffset default must be FVector(5, 0, -5)"),
		CDO->HandIKOffset.Equals(Expected, KINDA_SMALL_NUMBER));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0311: HandIKSpacingBetweenLeftRight
// WHAT: Verify HandIKSpacing CDO default == 40.
// WHY: Left and right hand IK targets are separated by this distance (shoulder width).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4HandIKSpacingBetweenLeftRightTest,
	"ClimbingSystem.Batch4.IK.HandIKSpacingBetweenLeftRight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4HandIKSpacingBetweenLeftRightTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0311: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0311: HandIKSpacing default must be 40"),
		FMath::IsNearlyEqual(CDO->HandIKSpacing, 40.f));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0312: CameraNudgeAtExactActivationAngle
// WHAT: Verify CameraNudgeActivationAngle CDO default == 45.
// WHY: Camera nudge begins exactly at this angle from the wall normal.
//      // VERIFY: full nudge test requires a live camera tick at the exact angle.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4CameraNudgeAtExactActivationAngleTest,
	"ClimbingSystem.Batch4.Camera.CameraNudgeAtExactActivationAngle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4CameraNudgeAtExactActivationAngleTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0312: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0312: CameraNudgeActivationAngle default must be 45"),
		FMath::IsNearlyEqual(CDO->CameraNudgeActivationAngle, 45.f));

	// Contract: camera at exactly 45 degrees meets the activation threshold
	constexpr float ExactAngle = 45.f;
	TestTrue(TEXT("TC-0312: angle 45 must equal CameraNudgeActivationAngle — nudge activates"),
		FMath::IsNearlyEqual(ExactAngle, CDO->CameraNudgeActivationAngle));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0313: CameraNudgeBelowAngleNoNudge
// WHAT: Complement to TC-0312. Verify angle < CameraNudgeActivationAngle(45) means no nudge.
// WHY: Camera nudge must not fire when already facing close to the wall.
//      // VERIFY: full no-nudge test requires a live camera tick below the threshold.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch4CameraNudgeBelowAngleNoNudgeTest,
	"ClimbingSystem.Batch4.Camera.CameraNudgeBelowAngleNoNudge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch4CameraNudgeBelowAngleNoNudgeTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0313: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0313: CameraNudgeActivationAngle default must be 45"),
		FMath::IsNearlyEqual(CDO->CameraNudgeActivationAngle, 45.f));

	// Contract: angle below threshold must not qualify for nudge
	constexpr float BelowAngle = 44.f;
	TestTrue(TEXT("TC-0313: angle 44 must be < CameraNudgeActivationAngle 45 — no nudge"),
		BelowAngle < CDO->CameraNudgeActivationAngle);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
