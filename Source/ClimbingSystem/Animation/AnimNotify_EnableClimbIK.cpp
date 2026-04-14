// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNotify_EnableClimbIK.h"
#include "ClimbingAnimInstance.h"
#include "Character/ClimbingCharacter.h"
#include "Data/ClimbingTypes.h"
#include "Components/SkeletalMeshComponent.h"

UAnimNotify_EnableClimbIK::UAnimNotify_EnableClimbIK()
	: LimbMask(static_cast<int32>(EClimbIKLimbMask::BothHands))
	, TargetWeight(1.0f)
{
}

void UAnimNotify_EnableClimbIK::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	// Get the climbing animation instance
	UClimbingAnimInstance* AnimInstance = Cast<UClimbingAnimInstance>(MeshComp->GetAnimInstance());
	if (!AnimInstance)
	{
		return;
	}

	// Set target weights based on limb mask
	// The AnimInstance will blend to these targets over IKBlendTimeIn
	const EClimbIKLimbMask Mask = static_cast<EClimbIKLimbMask>(LimbMask);
	AnimInstance->SetIKNotifyLimbState(LimbMask, true);

	if (EnumHasAnyFlags(Mask, EClimbIKLimbMask::HandLeft))
	{
		AnimInstance->TargetIKWeightHandLeft = TargetWeight;
	}

	if (EnumHasAnyFlags(Mask, EClimbIKLimbMask::HandRight))
	{
		AnimInstance->TargetIKWeightHandRight = TargetWeight;
	}

	if (EnumHasAnyFlags(Mask, EClimbIKLimbMask::FootLeft))
	{
		AnimInstance->TargetIKWeightFootLeft = TargetWeight;
	}

	if (EnumHasAnyFlags(Mask, EClimbIKLimbMask::FootRight))
	{
		AnimInstance->TargetIKWeightFootRight = TargetWeight;
	}

#if !UE_BUILD_SHIPPING
	// Debug logging
	if (AClimbingCharacter* Character = Cast<AClimbingCharacter>(MeshComp->GetOwner()))
	{
		if (Character->bDrawDebug)
		{
			UE_LOG(LogClimbing, Verbose, TEXT("EnableClimbIK: Mask=0x%02X, TargetWeight=%.2f on %s"),
				LimbMask, TargetWeight, *Character->GetName());
		}
	}
#endif
}

FString UAnimNotify_EnableClimbIK::GetNotifyName_Implementation() const
{
	FString LimbNames;

	const EClimbIKLimbMask Mask = static_cast<EClimbIKLimbMask>(LimbMask);

	if (Mask == EClimbIKLimbMask::AllLimbs)
	{
		LimbNames = TEXT("All");
	}
	else if (Mask == EClimbIKLimbMask::BothHands)
	{
		LimbNames = TEXT("Hands");
	}
	else if (Mask == EClimbIKLimbMask::BothFeet)
	{
		LimbNames = TEXT("Feet");
	}
	else
	{
		TArray<FString> Parts;
		if (EnumHasAnyFlags(Mask, EClimbIKLimbMask::HandLeft)) Parts.Add(TEXT("LH"));
		if (EnumHasAnyFlags(Mask, EClimbIKLimbMask::HandRight)) Parts.Add(TEXT("RH"));
		if (EnumHasAnyFlags(Mask, EClimbIKLimbMask::FootLeft)) Parts.Add(TEXT("LF"));
		if (EnumHasAnyFlags(Mask, EClimbIKLimbMask::FootRight)) Parts.Add(TEXT("RF"));
		LimbNames = FString::Join(Parts, TEXT("+"));
	}

	return FString::Printf(TEXT("Enable IK: %s (%.0f%%)"), *LimbNames, TargetWeight * 100.0f);
}
