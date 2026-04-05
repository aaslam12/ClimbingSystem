// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EnableClimbIK.generated.h"

/**
 * Limb bitmask flags for IK control.
 * Used by EnableClimbIK and DisableClimbIK notifies to specify which limbs to affect.
 */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EClimbIKLimbMask : uint8
{
	None		= 0,
	HandLeft	= 1 << 0,
	HandRight	= 1 << 1,
	FootLeft	= 1 << 2,
	FootRight	= 1 << 3,

	/** Both hands (convenience mask) */
	BothHands	= HandLeft | HandRight,
	/** Both feet (convenience mask) */
	BothFeet	= FootLeft | FootRight,
	/** All four limbs (convenience mask) */
	AllLimbs	= BothHands | BothFeet
};
ENUM_CLASS_FLAGS(EClimbIKLimbMask);

/**
 * Animation Notify that enables IK for specified limbs.
 * Place on animations at the frame where limbs should begin IK targeting.
 * 
 * Usage:
 * - Place on GrabLedge animation at the hand-contact frame
 * - Configure LimbMask to specify which limbs to enable
 * - TargetWeight specifies the target IK weight (default 1.0 = full IK)
 * - The AnimInstance blends to TargetWeight over IKBlendTimeIn
 * 
 * @see UAnimNotify_DisableClimbIK
 */
UCLASS(const, hidecategories = Object, collapsecategories, meta = (DisplayName = "Enable Climb IK"))
class CLIMBINGSYSTEM_API UAnimNotify_EnableClimbIK : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_EnableClimbIK();

	// UAnimNotify interface
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

#if WITH_EDITOR
	virtual bool CanBePlaced(UAnimSequenceBase* Animation) const override { return true; }
#endif

	/** Bitmask of limbs to enable IK for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|IK",
		meta = (Bitmask, BitmaskEnum = "/Script/ClimbingSystem.EClimbIKLimbMask",
		ToolTip = "Which limbs to enable IK for. Hands use Two-Bone IK, feet use FABRIK."))
	int32 LimbMask = static_cast<int32>(EClimbIKLimbMask::BothHands);

	/** Target IK weight for the enabled limbs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ClampMin = "0.0", ClampMax = "1.0",
		ToolTip = "Target IK weight for enabled limbs. 0 = animation only, 1 = full IK. Blended over IKBlendTimeIn."))
	float TargetWeight = 1.0f;
};
