// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNotify_DisableClimbIK.h"
#include "ClimbingAnimInstance.h"
#include "ClimbingCharacter.h"
#include "ClimbingTypes.h"
#include "Components/SkeletalMeshComponent.h"

UAnimNotify_DisableClimbIK::UAnimNotify_DisableClimbIK()
	: LimbMask(static_cast<int32>(EClimbIKLimbMask::BothHands))
{
}

void UAnimNotify_DisableClimbIK::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
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

	// Set target weights to zero based on limb mask
	// The AnimInstance will blend to 0 over IKBlendTimeOut
	const EClimbIKLimbMask Mask = static_cast<EClimbIKLimbMask>(LimbMask);

	if (EnumHasAnyFlags(Mask, EClimbIKLimbMask::HandLeft))
	{
		AnimInstance->TargetIKWeightHandLeft = 0.0f;
	}

	if (EnumHasAnyFlags(Mask, EClimbIKLimbMask::HandRight))
	{
		AnimInstance->TargetIKWeightHandRight = 0.0f;
	}

	if (EnumHasAnyFlags(Mask, EClimbIKLimbMask::FootLeft))
	{
		AnimInstance->TargetIKWeightFootLeft = 0.0f;
	}

	if (EnumHasAnyFlags(Mask, EClimbIKLimbMask::FootRight))
	{
		AnimInstance->TargetIKWeightFootRight = 0.0f;
	}

#if !UE_BUILD_SHIPPING
	// Debug logging
	if (AClimbingCharacter* Character = Cast<AClimbingCharacter>(MeshComp->GetOwner()))
	{
		if (Character->bDrawDebug)
		{
			UE_LOG(LogClimbing, Verbose, TEXT("DisableClimbIK: Mask=0x%02X on %s"),
				LimbMask, *Character->GetName());
		}
	}
#endif
}

FString UAnimNotify_DisableClimbIK::GetNotifyName_Implementation() const
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

	return FString::Printf(TEXT("Disable IK: %s"), *LimbNames);
}
