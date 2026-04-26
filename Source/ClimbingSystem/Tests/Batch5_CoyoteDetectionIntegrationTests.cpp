// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingTypes.h"
#include "Components/CapsuleComponent.h"
#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "HAL/PlatformTime.h"

// ---------------------------------------------------------------------------
// TC-0354: CoyoteGrabDisabledByToggle
// WHAT: Verify bEnableCoyoteTime is configurable. Set false on a live instance.
// WHY:  Coyote grab must be suppressible via the toggle; the property must be
//       writable at runtime so designers can disable it per-character.
// VERIFY: Full grab-rejection test requires a live Landed() + IA_Grab sequence.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5CoyoteGrabDisabledByToggleTest,
	"ClimbingSystem.Batch5.Coyote.CoyoteGrabDisabledByToggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5CoyoteGrabDisabledByToggleTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0354: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	TestTrue(TEXT("TC-0354: bEnableCoyoteTime default must be true"), Character->bEnableCoyoteTime);

	Character->bEnableCoyoteTime = false;
	TestFalse(TEXT("TC-0354: bEnableCoyoteTime must be false after assignment"), Character->bEnableCoyoteTime);
	// VERIFY: grab rejection requires a live Landed() + IA_Grab sequence.

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0355: CoyoteLandedClearsWindow
// WHAT: Verify CoyoteTimeWindow > 0 on CDO.
// WHY:  A positive window is required for Landed() to clear the coyote timer;
//       zero would make the feature permanently inactive.
// VERIFY: Full window-clear test requires a live Landed() call with elapsed tracking.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5CoyoteLandedClearsWindowTest,
	"ClimbingSystem.Batch5.Coyote.CoyoteLandedClearsWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5CoyoteLandedClearsWindowTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0355: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0355: CoyoteTimeWindow must be > 0 for Landed() to clear it"),
		CDO->CoyoteTimeWindow > 0.f);
	// VERIFY: window-clear behaviour requires a live Landed() call.
	return true;
}

// ---------------------------------------------------------------------------
// TC-0356: ClimbableOneWayRejectsWrongApproach
// WHAT: Call ValidateOneWayApproach(Normal=(0,1,0), Approach=(0,1,0), Tol=0.5).
//       Same direction means the character is approaching from the back face.
// WHY:  One-way surfaces must reject back-face approaches.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5ClimbableOneWayRejectsWrongApproachTest,
	"ClimbingSystem.Batch5.Detection.ClimbableOneWayRejectsWrongApproach",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5ClimbableOneWayRejectsWrongApproachTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0356: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	const FVector Normal(0.f, 1.f, 0.f);
	const FVector Approach(0.f, 1.f, 0.f); // same direction = back face
	const bool bResult = Character->ValidateOneWayApproach(Normal, Approach, 0.5f);
	TestFalse(TEXT("TC-0356: same-direction approach must be rejected (back face)"), bResult);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0357: ClimbableOneWayAcceptsCorrectApproach
// WHAT: Call ValidateOneWayApproach(Normal=(0,1,0), Approach=(0,-1,0), Tol=0.5).
//       Opposing direction means the character faces the front face.
// WHY:  One-way surfaces must accept front-face approaches.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5ClimbableOneWayAcceptsCorrectApproachTest,
	"ClimbingSystem.Batch5.Detection.ClimbableOneWayAcceptsCorrectApproach",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5ClimbableOneWayAcceptsCorrectApproachTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0357: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	const FVector Normal(0.f, 1.f, 0.f);
	const FVector Approach(0.f, -1.f, 0.f); // opposing = front face
	const bool bResult = Character->ValidateOneWayApproach(Normal, Approach, 0.5f);
	TestTrue(TEXT("TC-0357: opposing approach must be accepted (front face)"), bResult);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0358: GetSurfaceDataParsesValidTag
// WHAT: Spawn box with tag "SurfaceData:/Game/Test". Call GetSurfaceDataFromComponent.
// WHY:  The tag parser must attempt an asset load for well-formed paths.
// VERIFY: Asset load result depends on whether /Game/Test exists in the project.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5GetSurfaceDataParsesValidTagTest,
	"ClimbingSystem.Batch5.Detection.GetSurfaceDataParsesValidTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5GetSurfaceDataParsesValidTagTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0358: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	AActor* BoxActor = Helper.SpawnBoxAt(FVector(200.f, 0.f, 0.f), FVector(50.f, 50.f, 50.f));
	TestNotNull(TEXT("TC-0358: box actor must spawn"), BoxActor);
	if (!BoxActor) { Helper.Teardown(); return false; }

	UPrimitiveComponent* BoxRoot = Cast<UPrimitiveComponent>(BoxActor->GetRootComponent());
	TestNotNull(TEXT("TC-0358: box root component must exist"), BoxRoot);
	if (!BoxRoot) { Helper.Teardown(); return false; }

	BoxRoot->ComponentTags.Add(FName("SurfaceData:/Game/Test"));

	// VERIFY: result depends on whether /Game/Test asset exists; no crash is the contract here.
	const UClimbingSurfaceData* Data = Character->GetSurfaceDataFromComponent(BoxRoot);
	// Data may be null if asset doesn't exist — that is acceptable; we only verify no crash.
	(void)Data;

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0359: GetSurfaceDataMalformedTagReturnsNull
// WHAT: Spawn box with tag "SurfaceData:" (empty path). Assert nullptr returned.
// WHY:  Malformed tags with no asset path must not crash and must return null.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5GetSurfaceDataMalformedTagReturnsNullTest,
	"ClimbingSystem.Batch5.Detection.GetSurfaceDataMalformedTagReturnsNull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5GetSurfaceDataMalformedTagReturnsNullTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0359: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	AActor* BoxActor = Helper.SpawnBoxAt(FVector(200.f, 0.f, 0.f), FVector(50.f, 50.f, 50.f));
	TestNotNull(TEXT("TC-0359: box actor must spawn"), BoxActor);
	if (!BoxActor) { Helper.Teardown(); return false; }

	UPrimitiveComponent* BoxRoot = Cast<UPrimitiveComponent>(BoxActor->GetRootComponent());
	TestNotNull(TEXT("TC-0359: box root component must exist"), BoxRoot);
	if (!BoxRoot) { Helper.Teardown(); return false; }

	BoxRoot->ComponentTags.Add(FName("SurfaceData:"));

	const UClimbingSurfaceData* Data = Character->GetSurfaceDataFromComponent(BoxRoot);
	TestNull(TEXT("TC-0359: malformed tag (empty path) must return nullptr"), Data);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0360: GetSurfaceDataMissingAssetReturnsNull
// WHAT: Spawn box with tag "SurfaceData:/Game/NonExistent.NonExistent". Assert nullptr.
// WHY:  A well-formed but non-existent asset path must return null gracefully.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5GetSurfaceDataMissingAssetReturnsNullTest,
	"ClimbingSystem.Batch5.Detection.GetSurfaceDataMissingAssetReturnsNull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5GetSurfaceDataMissingAssetReturnsNullTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0360: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	AActor* BoxActor = Helper.SpawnBoxAt(FVector(200.f, 0.f, 0.f), FVector(50.f, 50.f, 50.f));
	TestNotNull(TEXT("TC-0360: box actor must spawn"), BoxActor);
	if (!BoxActor) { Helper.Teardown(); return false; }

	UPrimitiveComponent* BoxRoot = Cast<UPrimitiveComponent>(BoxActor->GetRootComponent());
	TestNotNull(TEXT("TC-0360: box root component must exist"), BoxRoot);
	if (!BoxRoot) { Helper.Teardown(); return false; }

	BoxRoot->ComponentTags.Add(FName("SurfaceData:/Game/NonExistent.NonExistent"));

	const UClimbingSurfaceData* Data = Character->GetSurfaceDataFromComponent(BoxRoot);
	TestNull(TEXT("TC-0360: non-existent asset path must return nullptr"), Data);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0361: IsSurfaceClimbableClimbableTiersTrue
// WHAT: Call IsSurfaceClimbable for Climbable, ClimbableOneWay, Untagged. Assert all true.
// WHY:  These tiers must allow climbing in the default context.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IsSurfaceClimbableClimbableTiersTrueTest,
	"ClimbingSystem.Batch5.Detection.IsSurfaceClimbableClimbableTiersTrue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IsSurfaceClimbableClimbableTiersTrueTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0361: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	TestTrue(TEXT("TC-0361: Climbable tier must be climbable"),
		Character->IsSurfaceClimbable(EClimbSurfaceTier::Climbable));
	TestTrue(TEXT("TC-0361: ClimbableOneWay tier must be climbable"),
		Character->IsSurfaceClimbable(EClimbSurfaceTier::ClimbableOneWay));
	TestTrue(TEXT("TC-0361: Untagged tier must be climbable"),
		Character->IsSurfaceClimbable(EClimbSurfaceTier::Untagged));

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0362: IsSurfaceClimbableUnclimbableTiersFalse
// WHAT: Call IsSurfaceClimbable for Unclimbable, LadderOnly. Assert both false.
// WHY:  These tiers must block normal climbing.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IsSurfaceClimbableUnclimbableTiersFalseTest,
	"ClimbingSystem.Batch5.Detection.IsSurfaceClimbableUnclimbableTiersFalse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IsSurfaceClimbableUnclimbableTiersFalseTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0362: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	TestFalse(TEXT("TC-0362: Unclimbable tier must not be climbable"),
		Character->IsSurfaceClimbable(EClimbSurfaceTier::Unclimbable));
	TestFalse(TEXT("TC-0362: LadderOnly tier must not be climbable in normal context"),
		Character->IsSurfaceClimbable(EClimbSurfaceTier::LadderOnly));

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0363: SimProxyIKUpdateIntervalThrottles
// WHAT: Verify SimulatedProxyIKUpdateInterval CDO default == 0.05.
// WHY:  Simulated proxy IK must be throttled to 20Hz to reduce CPU cost.
// VERIFY: Full throttle test requires a live simulated proxy IK tick.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5SimProxyIKUpdateIntervalThrottlesTest,
	"ClimbingSystem.Batch5.IK.SimProxyIKUpdateIntervalThrottles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5SimProxyIKUpdateIntervalThrottlesTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0363: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0363: SimulatedProxyIKUpdateInterval default must be 0.05"),
		FMath::IsNearlyEqual(CDO->SimulatedProxyIKUpdateInterval, 0.05f));
	// VERIFY: throttle behaviour requires a live simulated proxy IK tick.
	return true;
}

// ---------------------------------------------------------------------------
// TC-0364: DetectionScanIntervalGroundRate
// WHAT: Verify DetectionScanInterval CDO default == 0.05.
// WHY:  Ground-locomotion detection must run at 20Hz (0.05s interval).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5DetectionScanIntervalGroundRateTest,
	"ClimbingSystem.Batch5.Detection.DetectionScanIntervalGroundRate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5DetectionScanIntervalGroundRateTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0364: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0364: DetectionScanInterval default must be 0.05"),
		FMath::IsNearlyEqual(CDO->DetectionScanInterval, 0.05f));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0365: IntegrationLacheLaunchFlightCatchHang
// WHAT: Spawn char. Drive state chain Hanging->Lache->LacheInAir->LacheCatch->Hanging.
// WHY:  Full lache success path must traverse all intermediate states without crash.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationLacheLaunchFlightCatchHangTest,
	"ClimbingSystem.Batch5.Integration.LacheLaunchFlightCatchHang",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationLacheLaunchFlightCatchHangTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0365: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0365: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	FClimbingDetectionResult Det;
	Det.bValid = true;
	Det.LedgePosition = FVector(0.f, 0.f, 100.f);
	Det.SurfaceNormal = FVector(0.f, -1.f, 0.f);

	Character->TransitionToState(EClimbingState::Hanging, Det);
	TestEqual(TEXT("TC-0365: state must be Hanging"), Movement->CurrentClimbingState, EClimbingState::Hanging);

	Character->TransitionToState(EClimbingState::Lache, Det);
	TestEqual(TEXT("TC-0365: state must be Lache"), Movement->CurrentClimbingState, EClimbingState::Lache);

	Character->TransitionToState(EClimbingState::LacheInAir, Det);
	TestEqual(TEXT("TC-0365: state must be LacheInAir"), Movement->CurrentClimbingState, EClimbingState::LacheInAir);

	Character->TransitionToState(EClimbingState::LacheCatch, Det);
	TestEqual(TEXT("TC-0365: state must be LacheCatch"), Movement->CurrentClimbingState, EClimbingState::LacheCatch);

	Character->TransitionToState(EClimbingState::Hanging, Det);
	TestEqual(TEXT("TC-0365: final state must be Hanging"), Movement->CurrentClimbingState, EClimbingState::Hanging);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0366: IntegrationLacheMissResultsInFall
// WHAT: Spawn char. Drive state chain Hanging->Lache->LacheInAir->LacheMiss->None.
// WHY:  A missed lache must terminate in None (freefall), not leave a dangling state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationLacheMissResultsInFallTest,
	"ClimbingSystem.Batch5.Integration.LacheMissResultsInFall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationLacheMissResultsInFallTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0366: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0366: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	FClimbingDetectionResult Det;
	Det.bValid = true;
	Det.LedgePosition = FVector(0.f, 0.f, 100.f);
	Det.SurfaceNormal = FVector(0.f, -1.f, 0.f);

	Character->TransitionToState(EClimbingState::Hanging, Det);
	Character->TransitionToState(EClimbingState::Lache, Det);
	Character->TransitionToState(EClimbingState::LacheInAir, Det);
	Character->TransitionToState(EClimbingState::LacheMiss, Det);
	TestEqual(TEXT("TC-0366: state must be LacheMiss"), Movement->CurrentClimbingState, EClimbingState::LacheMiss);

	FClimbingDetectionResult Empty;
	Character->TransitionToState(EClimbingState::None, Empty);
	TestEqual(TEXT("TC-0366: final state must be None after miss"), Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0367: IntegrationGrabShimmyCornerShimmyDrop
// WHAT: Spawn char. Drive None->Hanging->Shimmying->CornerTransition->Hanging->Shimmying->None.
// WHY:  Full shimmy-corner chain must complete without crash or stuck state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationGrabShimmyCornerShimmyDropTest,
	"ClimbingSystem.Batch5.Integration.GrabShimmyCornerShimmyDrop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationGrabShimmyCornerShimmyDropTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0367: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0367: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	FClimbingDetectionResult Det;
	Det.bValid = true;
	Det.LedgePosition = FVector(0.f, 0.f, 100.f);
	Det.SurfaceNormal = FVector(0.f, -1.f, 0.f);

	Character->TransitionToState(EClimbingState::Hanging, Det);
	TestEqual(TEXT("TC-0367: Hanging"), Movement->CurrentClimbingState, EClimbingState::Hanging);

	Character->TransitionToState(EClimbingState::Shimmying, Det);
	TestEqual(TEXT("TC-0367: Shimmying"), Movement->CurrentClimbingState, EClimbingState::Shimmying);

	Character->TransitionToState(EClimbingState::CornerTransition, Det);
	TestEqual(TEXT("TC-0367: CornerTransition"), Movement->CurrentClimbingState, EClimbingState::CornerTransition);

	Character->TransitionToState(EClimbingState::Hanging, Det);
	TestEqual(TEXT("TC-0367: Hanging again"), Movement->CurrentClimbingState, EClimbingState::Hanging);

	Character->TransitionToState(EClimbingState::Shimmying, Det);
	TestEqual(TEXT("TC-0367: Shimmying again"), Movement->CurrentClimbingState, EClimbingState::Shimmying);

	FClimbingDetectionResult Empty;
	Character->TransitionToState(EClimbingState::None, Empty);
	TestEqual(TEXT("TC-0367: final state must be None"), Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0368: IntegrationClimbUpCapsuleRestore
// WHAT: Record original capsule. Transition to Hanging (capsule resized).
//       Transition ClimbingUp->None. Assert capsule restored.
// WHY:  Capsule must return to original dimensions after exiting climbing.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationClimbUpCapsuleRestoreTest,
	"ClimbingSystem.Batch5.Integration.ClimbUpCapsuleRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationClimbUpCapsuleRestoreTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0368: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0368: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	TestNotNull(TEXT("TC-0368: capsule component must exist"), Capsule);
	if (!Capsule) { Helper.Teardown(); return false; }

	const float OrigHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
	const float OrigRadius = Capsule->GetUnscaledCapsuleRadius();

	FClimbingDetectionResult Det;
	Det.bValid = true;
	Det.LedgePosition = FVector(0.f, 0.f, 100.f);
	Det.SurfaceNormal = FVector(0.f, -1.f, 0.f);

	Character->TransitionToState(EClimbingState::Hanging, Det);
	Character->TransitionToState(EClimbingState::ClimbingUp, Det);

	FClimbingDetectionResult Empty;
	Character->TransitionToState(EClimbingState::None, Empty);

	TestTrue(TEXT("TC-0368: capsule half-height must be restored after ClimbingUp->None"),
		FMath::IsNearlyEqual(Capsule->GetUnscaledCapsuleHalfHeight(), OrigHalfHeight, 1.f));
	TestTrue(TEXT("TC-0368: capsule radius must be restored after ClimbingUp->None"),
		FMath::IsNearlyEqual(Capsule->GetUnscaledCapsuleRadius(), OrigRadius, 1.f));

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0369: StressRapidGrabSpam
// WHAT: Spawn char + ledge. Call TransitionToState(Hanging) 10 times rapidly.
//       Assert state == Hanging and no crash.
// WHY:  Rapid repeated grab calls must be idempotent and not corrupt state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5StressRapidGrabSpamTest,
	"ClimbingSystem.Batch5.Stress.RapidGrabSpam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5StressRapidGrabSpamTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0369: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0369: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Helper.SpawnBoxAt(FVector(0.f, 60.f, 100.f), FVector(50.f, 10.f, 10.f));

	FClimbingDetectionResult Det;
	Det.bValid = true;
	Det.LedgePosition = FVector(0.f, 0.f, 100.f);
	Det.SurfaceNormal = FVector(0.f, -1.f, 0.f);

	for (int32 i = 0; i < 10; ++i)
	{
		Character->TransitionToState(EClimbingState::Hanging, Det);
	}

	TestEqual(TEXT("TC-0369: state must be Hanging after 10 rapid grab calls"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0370: StressRapidNoneHangingCycle
// WHAT: Spawn char. Cycle None->Hanging->None 100 times. Assert final state == None.
// WHY:  Repeated grab/drop cycles must not leak resources or corrupt state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5StressRapidNoneHangingCycleTest,
	"ClimbingSystem.Batch5.Stress.RapidNoneHangingCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5StressRapidNoneHangingCycleTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0370: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0370: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	FClimbingDetectionResult Det;
	Det.bValid = true;
	Det.LedgePosition = FVector(0.f, 0.f, 100.f);
	Det.SurfaceNormal = FVector(0.f, -1.f, 0.f);

	FClimbingDetectionResult Empty;

	for (int32 i = 0; i < 100; ++i)
	{
		Character->TransitionToState(EClimbingState::Hanging, Det);
		Character->TransitionToState(EClimbingState::None, Empty);
	}

	TestEqual(TEXT("TC-0370: final state must be None after 100 cycles"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0371: StressFiveClimbersIKBudget
// WHAT: Spawn 5 characters all in Hanging. Verify MaxSimultaneousIKCharacters == 4.
//       Assert no crash with 5 active climbers.
// WHY:  IK budget must cap at 4; the 5th character must not crash the system.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5StressFiveClimbersIKBudgetTest,
	"ClimbingSystem.Batch5.Stress.FiveClimbersIKBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5StressFiveClimbersIKBudgetTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0371: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { Helper.Teardown(); return false; }

	TestEqual(TEXT("TC-0371: MaxSimultaneousIKCharacters default must be 4"),
		CDO->MaxSimultaneousIKCharacters, 4);

	FClimbingDetectionResult Det;
	Det.bValid = true;
	Det.LedgePosition = FVector(0.f, 0.f, 100.f);
	Det.SurfaceNormal = FVector(0.f, -1.f, 0.f);

	for (int32 i = 0; i < 5; ++i)
	{
		AClimbingCharacter* C = Helper.SpawnCharacterAt(FVector(i * 200.f, 0.f, 0.f));
		TestNotNull(FString::Printf(TEXT("TC-0371: character %d must spawn"), i), C);
		if (!C) { Helper.Teardown(); return false; }
		C->TransitionToState(EClimbingState::Hanging, Det);
	}

	Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);
	// No crash reaching here is the primary assertion.

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0372: PerfDetectionScanUnder1ms
// WHAT: Spawn char + geometry. Time 100 PerformLedgeDetection calls.
//       Assert median < 1ms.
// WHY:  Detection must be cheap enough to run at 20Hz without frame budget impact.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5PerfDetectionScanUnder1msTest,
	"ClimbingSystem.Batch5.Perf.DetectionScanUnder1ms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5PerfDetectionScanUnder1msTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0372: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	Helper.SpawnBoxAt(FVector(0.f, 80.f, 80.f), FVector(100.f, 10.f, 10.f));

	constexpr int32 NumSamples = 100;
	TArray<double> Samples;
	Samples.Reserve(NumSamples);

	for (int32 i = 0; i < NumSamples; ++i)
	{
		const double Start = FPlatformTime::Seconds();
		Character->PerformLedgeDetection();
		Samples.Add((FPlatformTime::Seconds() - Start) * 1000.0); // ms
	}

	Samples.Sort();
	const double Median = Samples[NumSamples / 2];

	TestTrue(FString::Printf(TEXT("TC-0372: median detection time must be < 1ms (got %.3fms)"), Median),
		Median < 1.0);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0373: IntegrationTransitionDuringMontageBlendOut
// WHAT: Spawn char in ClimbingUp. Transition to None during blend-out.
//       Assert no crash and final state is deterministic (None).
// WHY:  Interrupting a montage mid-blend-out must not leave the state machine
//       in an undefined state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch5IntegrationTransitionDuringMontageBlendOutTest,
	"ClimbingSystem.Batch5.Integration.TransitionDuringMontageBlendOut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch5IntegrationTransitionDuringMontageBlendOutTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0373: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0373: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	FClimbingDetectionResult Det;
	Det.bValid = true;
	Det.LedgePosition = FVector(0.f, 0.f, 100.f);
	Det.SurfaceNormal = FVector(0.f, -1.f, 0.f);

	Character->TransitionToState(EClimbingState::Hanging, Det);
	Character->TransitionToState(EClimbingState::ClimbingUp, Det);

	// Simulate a partial tick to enter blend-out territory, then interrupt.
	Helper.World->Tick(ELevelTick::LEVELTICK_All, 0.016f);

	FClimbingDetectionResult Empty;
	Character->TransitionToState(EClimbingState::None, Empty);

	// No crash reaching here is the primary assertion; state must be deterministic.
	TestEqual(TEXT("TC-0373: final state must be None after mid-blend-out transition"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
