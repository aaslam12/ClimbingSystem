// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingAnimInstance.h"
#include "Character/ClimbingCharacter.h"

UClimbingAnimInstance::UClimbingAnimInstance()
{
	// Default values set in header
}

void UClimbingAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// Cache reference to owning climbing character
	if (AActor* OwningActor = GetOwningActor())
	{
		OwningClimbingCharacter = Cast<AClimbingCharacter>(OwningActor);
	}
}

void UClimbingAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Update climbing state flag
	bIsClimbing = (CurrentClimbingState != EClimbingState::None);

	// Update IK weight blending
	UpdateIKBlending(DeltaSeconds);
}

void UClimbingAnimInstance::ResetAllIKWeights()
{
	IKWeightHandLeft = 0.0f;
	IKWeightHandRight = 0.0f;
	IKWeightFootLeft = 0.0f;
	IKWeightFootRight = 0.0f;

	TargetIKWeightHandLeft = 0.0f;
	TargetIKWeightHandRight = 0.0f;
	TargetIKWeightFootLeft = 0.0f;
	TargetIKWeightFootRight = 0.0f;
}

void UClimbingAnimInstance::ResetIKNotifyMask()
{
	bHasIKNotifyMask = false;
	IKNotifyEnabledMask = 0;
}

void UClimbingAnimInstance::SetIKNotifyLimbState(int32 LimbMask, bool bEnable)
{
	bHasIKNotifyMask = true;

	const uint8 Mask = static_cast<uint8>(FMath::Clamp(LimbMask, 0, 255));
	if (bEnable)
	{
		IKNotifyEnabledMask |= Mask;
	}
	else
	{
		IKNotifyEnabledMask &= ~Mask;
	}
}

float UClimbingAnimInstance::ApplyNotifyMaskToWeight(uint8 LimbBit, float ProposedWeight) const
{
	if (!bHasIKNotifyMask)
	{
		return ProposedWeight;
	}

	return (IKNotifyEnabledMask & LimbBit) != 0 ? ProposedWeight : 0.0f;
}

float UClimbingAnimInstance::BlendIKWeight(float CurrentWeight, float TargetWeight, float DeltaTime, float BlendTime)
{
	if (BlendTime <= 0.0f)
	{
		return TargetWeight;
	}

	const float BlendSpeed = 1.0f / BlendTime;
	const float Delta = TargetWeight - CurrentWeight;
	const float MaxStep = BlendSpeed * DeltaTime;

	if (FMath::Abs(Delta) <= MaxStep)
	{
		return TargetWeight;
	}

	return CurrentWeight + FMath::Sign(Delta) * MaxStep;
}

void UClimbingAnimInstance::UpdateIKBlending(float DeltaSeconds)
{
	// Determine blend time based on whether we're blending in or out
	auto GetBlendTime = [this](float CurrentWeight, float TargetWeight) -> float
	{
		return (TargetWeight > CurrentWeight) ? IKBlendTimeIn : IKBlendTimeOut;
	};

	// Blend each limb weight toward its target
	IKWeightHandLeft = BlendIKWeight(
		IKWeightHandLeft, 
		TargetIKWeightHandLeft, 
		DeltaSeconds, 
		GetBlendTime(IKWeightHandLeft, TargetIKWeightHandLeft)
	);

	IKWeightHandRight = BlendIKWeight(
		IKWeightHandRight, 
		TargetIKWeightHandRight, 
		DeltaSeconds, 
		GetBlendTime(IKWeightHandRight, TargetIKWeightHandRight)
	);

	IKWeightFootLeft = BlendIKWeight(
		IKWeightFootLeft, 
		TargetIKWeightFootLeft, 
		DeltaSeconds, 
		GetBlendTime(IKWeightFootLeft, TargetIKWeightFootLeft)
	);

	IKWeightFootRight = BlendIKWeight(
		IKWeightFootRight, 
		TargetIKWeightFootRight, 
		DeltaSeconds, 
		GetBlendTime(IKWeightFootRight, TargetIKWeightFootRight)
	);
}
