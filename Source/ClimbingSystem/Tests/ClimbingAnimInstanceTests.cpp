// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimNotify_EnableClimbIK.h"
#include "Animation/ClimbingAnimInstance.h"
#include "Misc/AutomationTest.h"
#include "Components/SkeletalMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingAnimBlendImmediateTest,
	"ClimbingSystem.Anim.IKBlend.Immediate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingAnimBlendImmediateTest::RunTest(const FString& Parameters)
{
	const float Result = UClimbingAnimInstance::BlendIKWeight(0.25f, 1.0f, 0.016f, 0.0f);
	TestTrue(TEXT("Zero blend time should snap to target"), FMath::IsNearlyEqual(Result, 1.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingAnimBlendStepTest,
	"ClimbingSystem.Anim.IKBlend.StepRate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingAnimBlendStepTest::RunTest(const FString& Parameters)
{
	const float ResultUp = UClimbingAnimInstance::BlendIKWeight(0.0f, 1.0f, 0.1f, 0.5f);
	TestTrue(TEXT("Blend up should advance by DeltaTime/BlendTime"), FMath::IsNearlyEqual(ResultUp, 0.2f, KINDA_SMALL_NUMBER));

	const float ResultDown = UClimbingAnimInstance::BlendIKWeight(1.0f, 0.0f, 0.1f, 0.5f);
	TestTrue(TEXT("Blend down should decrease by DeltaTime/BlendTime"), FMath::IsNearlyEqual(ResultDown, 0.8f, KINDA_SMALL_NUMBER));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingAnimNotifyMaskTest,
	"ClimbingSystem.Anim.IKMask.NotifyGating",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingAnimNotifyMaskTest::RunTest(const FString& Parameters)
{
	USkeletalMeshComponent* MeshComp = NewObject<USkeletalMeshComponent>();
	UClimbingAnimInstance* AnimInstance = MeshComp ? NewObject<UClimbingAnimInstance>(MeshComp) : nullptr;
	TestNotNull(TEXT("AnimInstance should be created"), AnimInstance);
	if (!AnimInstance)
	{
		return false;
	}

	// No notify mask yet -> passthrough
	TestTrue(TEXT("Without notify mask, limb weight should pass through"), FMath::IsNearlyEqual(AnimInstance->ApplyNotifyMaskToWeight(1 << 0, 0.75f), 0.75f));

	AnimInstance->SetIKNotifyLimbState(static_cast<int32>(EClimbIKLimbMask::HandLeft), true);
	TestTrue(TEXT("Enabled limb should keep weight"), FMath::IsNearlyEqual(AnimInstance->ApplyNotifyMaskToWeight(1 << 0, 1.0f), 1.0f));
	TestTrue(TEXT("Non-enabled limb should be forced to zero"), FMath::IsNearlyEqual(AnimInstance->ApplyNotifyMaskToWeight(1 << 1, 1.0f), 0.0f));

	AnimInstance->SetIKNotifyLimbState(static_cast<int32>(EClimbIKLimbMask::HandLeft), false);
	TestTrue(TEXT("Disabled limb should be forced to zero"), FMath::IsNearlyEqual(AnimInstance->ApplyNotifyMaskToWeight(1 << 0, 1.0f), 0.0f));

	AnimInstance->ResetIKNotifyMask();
	TestTrue(TEXT("Resetting notify mask should restore passthrough"), FMath::IsNearlyEqual(AnimInstance->ApplyNotifyMaskToWeight(1 << 1, 0.6f), 0.6f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingAnimResetWeightsTest,
	"ClimbingSystem.Anim.IK.ResetWeights",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingAnimResetWeightsTest::RunTest(const FString& Parameters)
{
	USkeletalMeshComponent* MeshComp = NewObject<USkeletalMeshComponent>();
	UClimbingAnimInstance* AnimInstance = MeshComp ? NewObject<UClimbingAnimInstance>(MeshComp) : nullptr;
	TestNotNull(TEXT("AnimInstance should be created"), AnimInstance);
	if (!AnimInstance)
	{
		return false;
	}

	AnimInstance->IKWeightHandLeft = 1.0f;
	AnimInstance->IKWeightHandRight = 0.5f;
	AnimInstance->IKWeightFootLeft = 0.3f;
	AnimInstance->IKWeightFootRight = 0.9f;

	AnimInstance->ResetAllIKWeights();
	TestTrue(TEXT("Left hand weight should be reset"), FMath::IsNearlyZero(AnimInstance->IKWeightHandLeft));
	TestTrue(TEXT("Right hand weight should be reset"), FMath::IsNearlyZero(AnimInstance->IKWeightHandRight));
	TestTrue(TEXT("Left foot weight should be reset"), FMath::IsNearlyZero(AnimInstance->IKWeightFootLeft));
	TestTrue(TEXT("Right foot weight should be reset"), FMath::IsNearlyZero(AnimInstance->IKWeightFootRight));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
