// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EnableClimbIK.h" // For EClimbIKLimbMask
#include "AnimNotify_DisableClimbIK.generated.h"

/**
 * Animation Notify that disables IK for specified limbs.
 * Place on animations at the frame where limbs should release from IK targeting.
 * 
 * Usage:
 * - Place on DropDown animation at the hand-release frame
 * - Configure LimbMask to specify which limbs to disable
 * - The AnimInstance blends to weight 0 over IKBlendTimeOut
 * 
 * @see UAnimNotify_EnableClimbIK
 */
UCLASS(const, hidecategories = Object, collapsecategories, meta = (DisplayName = "Disable Climb IK"))
class CLIMBINGSYSTEM_API UAnimNotify_DisableClimbIK : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_DisableClimbIK();

	// UAnimNotify interface
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

#if WITH_EDITOR
	virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override { return true; }
#endif

	/** Bitmask of limbs to disable IK for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|IK",
		meta = (Bitmask, BitmaskEnum = "/Script/ClimbingSystem.EClimbIKLimbMask",
		ToolTip = "Which limbs to disable IK for. Target weight is set to 0, blended over IKBlendTimeOut."))
	int32 LimbMask = static_cast<int32>(EClimbIKLimbMask::BothHands);
};
