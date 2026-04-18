// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/ClimbingAnimInstance.h"
#include "Animation/ClimbingAnimationSet.h"
#include "Animation/AnimNotify_EnableClimbIK.h"
#include "Data/ClimbingTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Misc/AutomationTest.h"

// WHAT: Validates BlendIKWeight clamps to target when DeltaTime exceeds BlendTime
// WHY: Overshooting target weight causes IK jitter (weight > 1.0 or < 0.0)
// EDGE CASES: DeltaTime = 10x BlendTime

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingAnimBlendOvershootTest,
	"ClimbingSystem.Anim.Blend.OvershootClampedToTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingAnimBlendOvershootTest::RunTest(const FString& Parameters)
{
	const float Result = UClimbingAnimInstance::BlendIKWeight(0.0f, 1.0f, 5.0f, 0.5f);
	TestTrue(TEXT("Anim: blending with DeltaTime >> BlendTime should clamp to target 1.0"),
		FMath::IsNearlyEqual(Result, 1.0f));

	return true;
}

// WHAT: Validates BlendIKWeight clamps to 0.0 when blending down with large DeltaTime
// WHY: Negative weight values cause undefined IK behavior
// EDGE CASES: DeltaTime = 20x BlendTime, target = 0.0

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingAnimBlendNegativeTargetTest,
	"ClimbingSystem.Anim.Blend.NegativeClampedToZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingAnimBlendNegativeTargetTest::RunTest(const FString& Parameters)
{
	const float Result = UClimbingAnimInstance::BlendIKWeight(1.0f, 0.0f, 10.0f, 0.5f);
	TestTrue(TEXT("Anim: blending down with DeltaTime >> BlendTime should clamp to 0.0"),
		FMath::IsNearlyEqual(Result, 0.0f));

	return true;
}

// WHAT: Validates multiple limbs can be independently controlled via notify mask
// WHY: Enabling/disabling one limb must not affect others
// EDGE CASES: Enable all 4, disable one, verify other 3 unaffected

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingAnimNotifyMaskMultipleLimbsTest,
	"ClimbingSystem.Anim.IKNotify.MultipleLimbsAreIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingAnimNotifyMaskMultipleLimbsTest::RunTest(const FString& Parameters)
{
	USkeletalMeshComponent* MeshComp = NewObject<USkeletalMeshComponent>();
	UClimbingAnimInstance* AnimInstance = MeshComp ? NewObject<UClimbingAnimInstance>(MeshComp) : nullptr;
	TestNotNull(TEXT("AnimInstance should be created"), AnimInstance);
	if (!AnimInstance) { return false; }

	// Enable all 4 limbs
	AnimInstance->SetIKNotifyLimbState(static_cast<int32>(EClimbIKLimbMask::HandLeft), true);
	AnimInstance->SetIKNotifyLimbState(static_cast<int32>(EClimbIKLimbMask::HandRight), true);
	AnimInstance->SetIKNotifyLimbState(static_cast<int32>(EClimbIKLimbMask::FootLeft), true);
	AnimInstance->SetIKNotifyLimbState(static_cast<int32>(EClimbIKLimbMask::FootRight), true);

	// Disable only FootRight
	AnimInstance->SetIKNotifyLimbState(static_cast<int32>(EClimbIKLimbMask::FootRight), false);

	// HandLeft, HandRight, FootLeft should still pass through
	TestTrue(TEXT("Anim: HandLeft should still be enabled after disabling FootRight"),
		FMath::IsNearlyEqual(AnimInstance->ApplyNotifyMaskToWeight(
			static_cast<int32>(EClimbIKLimbMask::HandLeft), 0.8f), 0.8f));
	TestTrue(TEXT("Anim: HandRight should still be enabled"),
		FMath::IsNearlyEqual(AnimInstance->ApplyNotifyMaskToWeight(
			static_cast<int32>(EClimbIKLimbMask::HandRight), 0.9f), 0.9f));
	TestTrue(TEXT("Anim: FootLeft should still be enabled"),
		FMath::IsNearlyEqual(AnimInstance->ApplyNotifyMaskToWeight(
			static_cast<int32>(EClimbIKLimbMask::FootLeft), 0.7f), 0.7f));

	// FootRight should be forced to zero
	TestTrue(TEXT("Anim: FootRight should be forced to zero after disable"),
		FMath::IsNearlyEqual(AnimInstance->ApplyNotifyMaskToWeight(
			static_cast<int32>(EClimbIKLimbMask::FootRight), 1.0f), 0.0f));

	return true;
}

// WHAT: Validates GetMontageForSlot handles every enum value without crash
// WHY: Switch statement must be exhaustive — missing case causes undefined behavior
// EDGE CASES: Every value from 0 to MAX-1

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingAnimSetSlotCountMatchesEnumTest,
	"ClimbingSystem.Anim.AnimationSet.AllSlotsHandledWithoutCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingAnimSetSlotCountMatchesEnumTest::RunTest(const FString& Parameters)
{
	UClimbingAnimationSet* AnimSet = NewObject<UClimbingAnimationSet>();
	TestNotNull(TEXT("AnimationSet should be created"), AnimSet);
	if (!AnimSet) { return false; }

	for (uint8 i = 0; i < static_cast<uint8>(EClimbingAnimationSlot::MAX); ++i)
	{
		EClimbingAnimationSlot Slot = static_cast<EClimbingAnimationSlot>(i);
		// Should not crash — null return is acceptable for unassigned slots
		AnimSet->GetMontageForSlot(Slot);
	}

	// If we reach here, no crash occurred
	TestTrue(TEXT("Anim: all EClimbingAnimationSlot values handled without crash"), true);

	return true;
}

// WHAT: Validates FClimbingDetectionResult::Reset() clears all fields including enums and weak ptr
// WHY: Stale detection data causes phantom grabs or wrong surface behavior
// EDGE CASES: All fields including SurfaceTier, ClearanceType, HitComponent

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingDetectionResultResetClearsAllFieldsTest,
	"ClimbingSystem.Types.DetectionResult.ResetClearsAllFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingDetectionResultResetClearsAllFieldsTest::RunTest(const FString& Parameters)
{
	FClimbingDetectionResult Result;
	Result.LedgePosition = FVector(100.0f, 200.0f, 300.0f);
	Result.SurfaceNormal = FVector(0.0f, -1.0f, 0.0f);
	Result.SurfaceTier = EClimbSurfaceTier::Climbable;
	Result.ClearanceType = EClimbClearanceType::Full;
	Result.bValid = true;

	Result.Reset();

	TestFalse(TEXT("Types: Reset should invalidate bValid"), Result.bValid);
	TestTrue(TEXT("Types: Reset should zero LedgePosition"),
		Result.LedgePosition.IsNearlyZero());
	TestTrue(TEXT("Types: Reset should zero SurfaceNormal"),
		Result.SurfaceNormal.IsNearlyZero());
	TestEqual(TEXT("Types: Reset should reset SurfaceTier to Untagged"),
		Result.SurfaceTier, EClimbSurfaceTier::Untagged);
	TestEqual(TEXT("Types: Reset should reset ClearanceType to None"),
		Result.ClearanceType, EClimbClearanceType::None);
	TestFalse(TEXT("Types: Reset should clear HitComponent"),
		Result.HitComponent.IsValid());

	return true;
}

// WHAT: Validates FClimbingDetectionResultNet::Reset() clears all fields
// WHY: Stale net data causes incorrect proxy state
// EDGE CASES: All fields on the net-safe struct

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingNetResultResetClearsAllFieldsTest,
	"ClimbingSystem.Types.DetectionResultNet.ResetClearsAllFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingNetResultResetClearsAllFieldsTest::RunTest(const FString& Parameters)
{
	FClimbingDetectionResultNet NetResult;
	NetResult.LedgePosition = FVector(50.0f, 75.0f, 100.0f);
	NetResult.SurfaceNormal = FVector(1.0f, 0.0f, 0.0f);
	NetResult.SurfaceTier = EClimbSurfaceTier::ClimbableOneWay;
	NetResult.ClearanceType = EClimbClearanceType::CrouchOnly;
	NetResult.bValid = true;

	NetResult.Reset();

	TestFalse(TEXT("Types: Net Reset should invalidate bValid"), NetResult.bValid);
	TestTrue(TEXT("Types: Net Reset should zero LedgePosition"),
		FVector(NetResult.LedgePosition).IsNearlyZero());
	TestEqual(TEXT("Types: Net Reset should reset SurfaceTier to Untagged"),
		NetResult.SurfaceTier, EClimbSurfaceTier::Untagged);
	TestEqual(TEXT("Types: Net Reset should reset ClearanceType to None"),
		NetResult.ClearanceType, EClimbClearanceType::None);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
