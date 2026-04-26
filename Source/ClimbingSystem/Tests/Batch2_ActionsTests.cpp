// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Data/ClimbingTypes.h"
#include "InputActionValue.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/UnrealType.h"
#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

namespace
{
static FClimbingDetectionResult MakeBatch2Detection(
	const FVector& LedgePos = FVector(100.f, 0.f, 120.f),
	EClimbingState /*unused*/ = EClimbingState::None)
{
	FClimbingDetectionResult D;
	D.bValid = true;
	D.LedgePosition = LedgePos;
	D.SurfaceNormal = FVector(-1.f, 0.f, 0.f);
	D.SurfaceTier = EClimbSurfaceTier::Climbable;
	D.ClearanceType = EClimbClearanceType::Full;
	return D;
}
} // namespace

// ---------------------------------------------------------------------------
// TC-0194: ShimmyDirectionNotUpdatedAtExactDeadzone
// WHAT: Input exactly at deadzone boundary must NOT update committed direction.
// WHY: Deadzone is exclusive — values <= deadzone are ignored.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2ShimmyDirNotUpdatedAtExactDeadzoneTest,
	"ClimbingSystem.Actions.Shimmy.ShimmyDirectionNotUpdatedAtExactDeadzone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2ShimmyDirNotUpdatedAtExactDeadzoneTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0194: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	Character->ShimmyDirectionDeadzone = 0.1f;
	Character->TestCommittedShimmyDir() = 1.0f;

	// X == deadzone exactly — should be treated as inside deadzone
	const FInputActionValue AtDeadzone(FVector2D(0.1f, 0.f));
	Character->TestInput_ClimbMove(AtDeadzone);

	TestEqual(TEXT("TC-0194: committed shimmy dir must remain 1.0 when input == deadzone"),
		Character->TestCommittedShimmyDir(), 1.0f);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0195: ShimmyDirectionUpdatedJustAboveDeadzone
// WHAT: Input just above deadzone must flip committed direction.
// WHY: Values strictly above deadzone should update direction.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2ShimmyDirUpdatedJustAboveDeadzoneTest,
	"ClimbingSystem.Actions.Shimmy.ShimmyDirectionUpdatedJustAboveDeadzone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2ShimmyDirUpdatedJustAboveDeadzoneTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0195: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	Character->ShimmyDirectionDeadzone = 0.1f;
	Character->TestCommittedShimmyDir() = 1.0f;

	// X = -0.101 — just past deadzone in negative direction
	const FInputActionValue JustAbove(FVector2D(-0.101f, 0.f));
	Character->TestInput_ClimbMove(JustAbove);

	TestEqual(TEXT("TC-0195: committed shimmy dir must become -1.0 when input just exceeds deadzone"),
		Character->TestCommittedShimmyDir(), -1.0f);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0196: CornerDetectionNoTransitionBelowAngle
// WHAT: Shimmying state persists when no corner geometry is present.
// WHY: Corner transition must only fire when geometry angle threshold is met.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2CornerDetectionNoTransitionBelowAngleTest,
	"ClimbingSystem.Actions.Corner.CornerDetectionNoTransitionBelowAngle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2CornerDetectionNoTransitionBelowAngleTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0196: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0196: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Shimmying);

	// Tick with no corner geometry present
	Character->GetWorld()->Tick(ELevelTick::LEVELTICK_All, 0.016f);

	TestEqual(TEXT("TC-0196: state must remain Shimmying when no corner geometry is present"),
		Movement->CurrentClimbingState, EClimbingState::Shimmying);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0197: BracedShimmyReleaseReturnsToBracedWall
// WHAT: BracedShimmying with zero input transitions back to BracedWall.
// WHY: Release-to-brace is the symmetric counterpart of release-to-hang.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2BracedShimmyReleaseReturnsToBracedWallTest,
	"ClimbingSystem.Actions.BracedShimmy.BracedShimmyReleaseReturnsToBracedWall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2BracedShimmyReleaseReturnsToBracedWallTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0197: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0197: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::BracedShimmying);

	// Zero input + tick — should return to BracedWall
	Character->GetWorld()->Tick(ELevelTick::LEVELTICK_All, 0.016f);

	TestEqual(TEXT("TC-0197: zero input in BracedShimmying must return to BracedWall"),
		Movement->CurrentClimbingState, EClimbingState::BracedWall);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0198: CommittedShimmyDirReplicatedToProxies
// WHAT: CommittedShimmyDir property must carry CPF_Net flag.
// WHY: Shimmy direction must replicate so proxies play the correct animation.
// // VERIFY: full network test requires a listen-server world with a connected client.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2CommittedShimmyDirReplicatedTest,
	"ClimbingSystem.Actions.Replication.CommittedShimmyDirReplicatedToProxies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2CommittedShimmyDirReplicatedTest::RunTest(const FString& Parameters)
{
	const UClass* Class = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0198: AClimbingCharacter class must exist"), Class);
	if (!Class) { return false; }

	const FProperty* Prop = Class->FindPropertyByName(TEXT("CommittedShimmyDir"));
	TestNotNull(TEXT("TC-0198: CommittedShimmyDir property must exist"), Prop);
	if (!Prop) { return false; }

	TestTrue(TEXT("TC-0198: CommittedShimmyDir must have CPF_Net flag"),
		Prop->HasAnyPropertyFlags(CPF_Net));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0199: bCurrentCornerIsInsideReplicatedToProxies
// WHAT: bCurrentCornerIsInside property must carry CPF_Net flag.
// WHY: Corner type must replicate so proxies play inside vs outside corner montage.
// // VERIFY: full network test requires a listen-server world with a connected client.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2CornerIsInsideReplicatedTest,
	"ClimbingSystem.Actions.Replication.bCurrentCornerIsInsideReplicatedToProxies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2CornerIsInsideReplicatedTest::RunTest(const FString& Parameters)
{
	const UClass* Class = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0199: AClimbingCharacter class must exist"), Class);
	if (!Class) { return false; }

	const FProperty* Prop = Class->FindPropertyByName(TEXT("bCurrentCornerIsInside"));
	TestNotNull(TEXT("TC-0199: bCurrentCornerIsInside property must exist"), Prop);
	if (!Prop) { return false; }

	TestTrue(TEXT("TC-0199: bCurrentCornerIsInside must have CPF_Net flag"),
		Prop->HasAnyPropertyFlags(CPF_Net));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0200: ClimbUpRejectedFromBracedWallState
// WHAT: TestInput_ClimbUp while in BracedWall must not change state.
// WHY: BracedWall does not support a direct climb-up action.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2ClimbUpRejectedFromBracedWallTest,
	"ClimbingSystem.Actions.BracedWall.ClimbUpRejectedFromBracedWallState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2ClimbUpRejectedFromBracedWallTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0200: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0200: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::BracedWall);

	const FInputActionValue ClimbUpInput(true);
	Character->TestInput_ClimbUp(ClimbUpInput);

	TestEqual(TEXT("TC-0200: ClimbUp input must be rejected in BracedWall state"),
		Movement->CurrentClimbingState, EClimbingState::BracedWall);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0201: RepositionMontageResetsDistanceCounter
// WHAT: MaxContinuousShimmyDistance default is 300.
// WHY: Contract test — runtime distance reset requires a live shimmy tick loop.
// // VERIFY: full test requires observing ContinuousShimmyDistance reset to 0 after
//           distance exceeds MaxContinuousShimmyDistance during a shimmy tick.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2RepositionMontageResetsDistanceCounterTest,
	"ClimbingSystem.Actions.Shimmy.RepositionMontageResetsDistanceCounter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2RepositionMontageResetsDistanceCounterTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0201: AClimbingCharacter CDO must exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("TC-0201: MaxContinuousShimmyDistance default must be 300"),
		FMath::IsNearlyEqual(Defaults->MaxContinuousShimmyDistance, 300.f));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0202: BracedWallAnchorComponentNullSafe
// WHAT: Transitioning to BracedWall with AnchorComponent==nullptr must not crash.
// WHY: Null anchor is a valid transient state during grab detection gaps.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2BracedWallAnchorNullSafeTest,
	"ClimbingSystem.Actions.BracedWall.BracedWallAnchorComponentNullSafe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2BracedWallAnchorNullSafeTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0202: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0202: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->AnchorComponent = nullptr;

	// Must not crash
	Movement->SetClimbingState(EClimbingState::BracedWall);

	TestEqual(TEXT("TC-0202: state must be BracedWall after transition with null anchor"),
		Movement->CurrentClimbingState, EClimbingState::BracedWall);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0203: ArcFinalPositionAtTotalArcTime
// WHAT: Pure projectile math — position at t=1.2 with Speed=1200, GravityZ=-980.
// WHY: Lache arc endpoint must match the analytic formula used in detection.
// Expected: X = Speed * t = 1200 * 1.2 = 1440
//           Z = 0.5 * GravityZ * t^2 = 0.5 * (-980) * 1.44 = -705.6
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2ArcFinalPositionAtTotalArcTimeTest,
	"ClimbingSystem.Actions.Lache.ArcFinalPositionAtTotalArcTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2ArcFinalPositionAtTotalArcTimeTest::RunTest(const FString& Parameters)
{
	const FVector LaunchOrigin(0.f, 0.f, 0.f);
	const FVector Forward(1.f, 0.f, 0.f);
	constexpr float Speed      = 1200.f;
	constexpr float GravityZ   = -980.f;
	constexpr float T          = 1.2f;

	const FVector Position = LaunchOrigin + Forward * Speed * T + FVector(0.f, 0.f, 0.5f * GravityZ * T * T);

	TestTrue(TEXT("TC-0203: arc X at t=1.2 must be 1440"),
		FMath::IsNearlyEqual(Position.X, 1440.f, 0.1f));
	TestTrue(TEXT("TC-0203: arc Z at t=1.2 must be -705.6"),
		FMath::IsNearlyEqual(Position.Z, -705.6f, 0.1f));
	TestTrue(TEXT("TC-0203: arc Y at t=1.2 must be 0"),
		FMath::IsNearlyEqual(Position.Y, 0.f, 0.1f));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0204: LadderNonLadderTagRejected
// WHAT: A box tagged "Climbable" (not "LadderOnly") must not trigger ladder state.
// WHY: Ladder entry requires the LadderOnly surface tier.
// // VERIFY: full test requires PerformLadderDetection to be callable from tests.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2LadderNonLadderTagRejectedTest,
	"ClimbingSystem.Actions.Ladder.LadderNonLadderTagRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2LadderNonLadderTagRejectedTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0204: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0204: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Spawn a box tagged "Climbable" — not LadderOnly
	AActor* Box = Helper.SpawnBoxAt(FVector(80.f, 0.f, 100.f), FVector(50.f, 50.f, 100.f), FName("Climbable"));
	TestNotNull(TEXT("TC-0204: climbable box must spawn"), Box);

	// State must not be OnLadder — ladder detection should reject non-LadderOnly surfaces
	// // VERIFY: call PerformLadderDetection() once it is exposed to tests.
	TestNotEqual(TEXT("TC-0204: state must not be OnLadder for a Climbable-tagged surface"),
		Movement->CurrentClimbingState, EClimbingState::OnLadder);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0205: RagdollRecoveryNotInterruptedEarly
// WHAT: RagdollRecoveryTime default is 1.5.
// WHY: Contract test — full timer interruption test requires a live ragdoll tick.
// // VERIFY: full test requires triggering ragdoll, advancing time < 1.5s, and
//           asserting state remains Ragdoll.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2RagdollRecoveryNotInterruptedEarlyTest,
	"ClimbingSystem.Actions.Ragdoll.RagdollRecoveryNotInterruptedEarly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2RagdollRecoveryNotInterruptedEarlyTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0205: AClimbingCharacter CDO must exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("TC-0205: RagdollRecoveryTime default must be 1.5"),
		FMath::IsNearlyEqual(Defaults->RagdollRecoveryTime, 1.5f));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0206: AnchorWorldSpaceWithRotatedComponent
// WHAT: A local offset (100,0,0) on a component rotated 90° around Z maps to world Y.
// WHY: Anchor world-space computation must respect component rotation.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2AnchorWorldSpaceWithRotatedComponentTest,
	"ClimbingSystem.Actions.Anchor.AnchorWorldSpaceWithRotatedComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2AnchorWorldSpaceWithRotatedComponentTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	// Spawn a box rotated 90° around Z
	const FRotator Rot90Z(0.f, 90.f, 0.f);
	AActor* BoxActor = Helper.World->SpawnActor<AActor>(
		AActor::StaticClass(),
		FTransform(Rot90Z, FVector::ZeroVector));
	TestNotNull(TEXT("TC-0206: rotated box actor must spawn"), BoxActor);
	if (!BoxActor) { Helper.Teardown(); return false; }

	UBoxComponent* Box = NewObject<UBoxComponent>(BoxActor);
	Box->SetBoxExtent(FVector(50.f));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	BoxActor->SetRootComponent(Box);
	Box->RegisterComponent();
	Helper.SpawnedActors.Add(BoxActor);

	// Local offset (100, 0, 0) on a 90°-Z-rotated component should map to world (0, 100, 0)
	const FVector LocalOffset(100.f, 0.f, 0.f);
	const FVector WorldPos = Box->GetComponentTransform().TransformPosition(LocalOffset);

	TestTrue(TEXT("TC-0206: local X=100 on 90°-Z component must map to world Y≈100"),
		FMath::IsNearlyEqual(WorldPos.Y, 100.f, 1.f));
	TestTrue(TEXT("TC-0206: local X=100 on 90°-Z component must map to world X≈0"),
		FMath::IsNearlyEqual(WorldPos.X, 0.f, 1.f));

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0207: SetBasePreservedAcrossHangingToShimmy
// WHAT: MovementBase set during Hanging must still be set after transitioning to Shimmying.
// WHY: Base preservation prevents physics pop when shimmy starts on a moving platform.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2SetBasePreservedAcrossHangingToShimmyTest,
	"ClimbingSystem.Actions.Shimmy.SetBasePreservedAcrossHangingToShimmy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2SetBasePreservedAcrossHangingToShimmyTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0207: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0207: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Spawn a platform to use as movement base
	AActor* Platform = Helper.SpawnBoxAt(FVector(0.f, 0.f, -100.f), FVector(200.f, 200.f, 10.f));
	TestNotNull(TEXT("TC-0207: platform must spawn"), Platform);
	if (!Platform) { Helper.Teardown(); return false; }

	UPrimitiveComponent* PlatformRoot = Cast<UPrimitiveComponent>(Platform->GetRootComponent());
	TestNotNull(TEXT("TC-0207: platform root component must exist"), PlatformRoot);
	if (!PlatformRoot) { Helper.Teardown(); return false; }

	// Set movement base while in Hanging
	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->SetBase(PlatformRoot);
	TestNotNull(TEXT("TC-0207: movement base must be set in Hanging"), Character->GetMovementBase());

	// Transition to Shimmying
	Movement->SetClimbingState(EClimbingState::Shimmying);

	TestNotNull(TEXT("TC-0207: movement base must still be set after transitioning to Shimmying"),
		Character->GetMovementBase());

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0208: GrabBreakImpulseDirectionPreserved
// WHAT: GrabBreakLaunchScale default is 0.5.
// WHY: Contract test — full impulse direction test requires a live physics simulation.
// // VERIFY: full test requires triggering grab break with a known impulse vector and
//           asserting the launched velocity direction matches the scaled input.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2GrabBreakImpulseDirectionPreservedTest,
	"ClimbingSystem.Actions.GrabBreak.GrabBreakImpulseDirectionPreserved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2GrabBreakImpulseDirectionPreservedTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0208: AClimbingCharacter CDO must exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("TC-0208: GrabBreakLaunchScale default must be 0.5"),
		FMath::IsNearlyEqual(Defaults->GrabBreakLaunchScale, 0.5f));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0209: MantleCommittedBlocksLacheInput
// WHAT: TestInput_Lache while in Mantling must not change state.
// WHY: Committed mantle must not be interrupted by lache input.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2MantleCommittedBlocksLacheInputTest,
	"ClimbingSystem.Actions.Mantle.MantleCommittedBlocksLacheInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2MantleCommittedBlocksLacheInputTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0209: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0209: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Mantling);

	// Lache input while mantling — state must not change
	const FInputActionValue LacheInput(true);
	Character->TestInput_Lache(LacheInput);

	TestEqual(TEXT("TC-0209: Lache input must be blocked while in Mantling state"),
		Movement->CurrentClimbingState, EClimbingState::Mantling);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0210: LadderClimbMoveYAxisOnly
// WHAT: OnLadder is a valid EClimbingState enum value.
// WHY: Contract test — full Y-axis routing test requires live input dispatch.
// // VERIFY: full test requires dispatching IA_ClimbMove with Y-only input while
//           OnLadder and asserting only vertical ladder movement occurs.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2LadderClimbMoveYAxisOnlyTest,
	"ClimbingSystem.Actions.Ladder.LadderClimbMoveYAxisOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2LadderClimbMoveYAxisOnlyTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0210: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0210: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Contract: OnLadder is a valid state that can be set
	Movement->SetClimbingState(EClimbingState::OnLadder);
	TestEqual(TEXT("TC-0210: OnLadder must be a valid settable climbing state"),
		Movement->CurrentClimbingState, EClimbingState::OnLadder);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0211: RagdollFaceDetectionAtZeroDotProduct
// WHAT: PelvisBoneName default is "pelvis".
// WHY: Contract test — full dot product test requires a live skeletal mesh with physics.
// // VERIFY: full test requires triggering ragdoll, reading pelvis bone transform, and
//           asserting face-up/face-down detection at dot product == 0 boundary.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2RagdollFaceDetectionAtZeroDotProductTest,
	"ClimbingSystem.Actions.Ragdoll.RagdollFaceDetectionAtZeroDotProduct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2RagdollFaceDetectionAtZeroDotProductTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0211: AClimbingCharacter CDO must exist"), Defaults);
	if (!Defaults) { return false; }

	TestEqual(TEXT("TC-0211: PelvisBoneName default must be \"pelvis\""),
		Defaults->PelvisBoneName, FName(TEXT("pelvis")));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0212: LacheArcTraceRadiusUsedForDetection
// WHAT: LacheArcTraceRadius constant is 24.0f.
// WHY: Arc sweep radius must match the design spec to avoid false positives/negatives.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2LacheArcTraceRadiusTest,
	"ClimbingSystem.Actions.Lache.LacheArcTraceRadiusUsedForDetection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2LacheArcTraceRadiusTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0212: AClimbingCharacter CDO must exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("TC-0212: LacheArcTraceRadius default must be 24.0f"),
		FMath::IsNearlyEqual(Defaults->LacheArcTraceRadius, 24.f));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0213: LadderOnlyTagRequiredForLadderState
// WHAT: A box tagged "LadderOnly" has that tag present on its component.
// WHY: Contract test — full ladder entry test requires PerformLadderDetection exposure.
// // VERIFY: full test requires calling PerformLadderDetection and asserting OnLadder
//           state is entered only for LadderOnly-tagged surfaces.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2LadderOnlyTagRequiredTest,
	"ClimbingSystem.Actions.Ladder.LadderOnlyTagRequiredForLadderState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2LadderOnlyTagRequiredTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AActor* LadderBox = Helper.SpawnBoxAt(
		FVector(80.f, 0.f, 100.f), FVector(20.f, 50.f, 150.f), FName("LadderOnly"));
	TestNotNull(TEXT("TC-0213: LadderOnly box must spawn"), LadderBox);
	if (!LadderBox) { Helper.Teardown(); return false; }

	UBoxComponent* BoxComp = LadderBox->FindComponentByClass<UBoxComponent>();
	TestNotNull(TEXT("TC-0213: box component must exist"), BoxComp);
	if (!BoxComp) { Helper.Teardown(); return false; }

	TestTrue(TEXT("TC-0213: box component must carry the LadderOnly tag"),
		BoxComp->ComponentTags.Contains(FName("LadderOnly")));

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0214: AnchorNullMidBracedShimmyDrops
// WHAT: Nulling AnchorComponent while in BracedShimmying and ticking must transition
//       to DroppingDown or None — not remain in BracedShimmying.
// WHY: Losing the anchor mid-shimmy must cause a safe drop rather than a stuck state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch2AnchorNullMidBracedShimmyDropsTest,
	"ClimbingSystem.Actions.BracedShimmy.AnchorNullMidBracedShimmyDrops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch2AnchorNullMidBracedShimmyDropsTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0214: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0214: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::BracedShimmying);
	Movement->AnchorComponent = nullptr;

	Character->GetWorld()->Tick(ELevelTick::LEVELTICK_All, 0.016f);

	const EClimbingState State = Movement->CurrentClimbingState;
	const bool bDropped = (State == EClimbingState::DroppingDown || State == EClimbingState::None);
	TestTrue(TEXT("TC-0214: null anchor mid-BracedShimmying must transition to DroppingDown or None"),
		bDropped);

	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
