// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Data/ClimbingTypes.h"
#include "ClimbingAnimInstance.generated.h"

class AClimbingCharacter;

/**
 * Animation Blueprint parent class for the climbing system.
 * Exposes IK weights and climbing state to the Animation Blueprint.
 * All properties are BlueprintReadWrite for Animation Blueprint access.
 */
UCLASS()
class CLIMBINGSYSTEM_API UClimbingAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UClimbingAnimInstance();

	// ========================================================================
	// UAnimInstance Interface
	// ========================================================================

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// ========================================================================
	// Climbing State (set by AClimbingCharacter)
	// ========================================================================

	/** Current climbing state. Used by state machine and blend logic in ABP. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|State",
		meta = (ToolTip = "Current climbing state. Updated by AClimbingCharacter each tick."))
	EClimbingState CurrentClimbingState = EClimbingState::None;

	/** Previous climbing state. Used for transition logic in ABP. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|State",
		meta = (ToolTip = "Previous climbing state before last transition. Useful for exit animations."))
	EClimbingState PreviousClimbingState = EClimbingState::None;

	/** Whether any climbing state is active (CurrentClimbingState != None). */
	UPROPERTY(BlueprintReadOnly, Category = "Climbing|State",
		meta = (ToolTip = "True when character is in any climbing state (not None)."))
	bool bIsClimbing = false;

	// ========================================================================
	// IK Weights (set by IK system, read by ABP)
	// ========================================================================

	/** Left hand IK weight. 0 = animation only, 1 = full IK. Blended in ABP. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Left hand IK weight. Blended with IKBlendTimeIn/Out. Set by EnableClimbIK/DisableClimbIK notifies."))
	float IKWeightHandLeft = 0.0f;

	/** Right hand IK weight. 0 = animation only, 1 = full IK. Blended in ABP. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Right hand IK weight. Blended with IKBlendTimeIn/Out. Set by EnableClimbIK/DisableClimbIK notifies."))
	float IKWeightHandRight = 0.0f;

	/** Left foot IK weight. 0 = animation only, 1 = full IK. Blended in ABP. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Left foot IK weight. Used for braced wall and ladder states."))
	float IKWeightFootLeft = 0.0f;

	/** Right foot IK weight. 0 = animation only, 1 = full IK. Blended in ABP. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Right foot IK weight. Used for braced wall and ladder states."))
	float IKWeightFootRight = 0.0f;

	// ========================================================================
	// IK Targets (set by IK system, read by ABP)
	// ========================================================================

	/** Left hand IK target in world space. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Left hand IK effector target in world space."))
	FVector IKTargetHandLeft = FVector::ZeroVector;

	/** Right hand IK target in world space. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Right hand IK effector target in world space."))
	FVector IKTargetHandRight = FVector::ZeroVector;

	/** Left foot IK target in world space. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Left foot IK effector target in world space."))
	FVector IKTargetFootLeft = FVector::ZeroVector;

	/** Right foot IK target in world space. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Right foot IK effector target in world space."))
	FVector IKTargetFootRight = FVector::ZeroVector;

	// ========================================================================
	// IK Blend Settings
	// ========================================================================

	/** Time to blend IK weight from 0 to target when enabling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Seconds to blend IK weight from 0 to target when IK is enabled. Prevents popping.",
		ClampMin = "0.0", ClampMax = "1.0"))
	float IKBlendTimeIn = 0.15f;

	/** Time to blend IK weight from current to 0 when disabling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Seconds to blend IK weight to 0 when IK is disabled. Prevents popping.",
		ClampMin = "0.0", ClampMax = "1.0"))
	float IKBlendTimeOut = 0.1f;

	// ========================================================================
	// Movement Data (set by AClimbingCharacter)
	// ========================================================================

	/** Current shimmy speed normalized (0-1). Used for animation playback rate. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Normalized shimmy speed for playback rate scaling. 0 = stationary, 1 = max speed."))
	float NormalizedShimmySpeed = 0.0f;

	/** Current ladder climb speed normalized (0-1). Used for animation playback rate. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Normalized ladder climb speed for playback rate scaling."))
	float NormalizedLadderSpeed = 0.0f;

	/** Committed shimmy direction. -1 = left, +1 = right, 0 = uncommitted. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Hysteresis-gated shimmy direction. -1 = left, +1 = right. Used for slot selection."))
	float CommittedShimmyDir = 0.0f;

	/** Ladder climb direction. -1 = down, +1 = up, 0 = stationary. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Current ladder climb direction. -1 = descending, +1 = ascending."))
	float LadderClimbDir = 0.0f;

	/** Whether sprint modifier is active (fast ladder ascent). */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "True when IA_Sprint is held during ladder climbing."))
	bool bIsLadderSprinting = false;

	/** Whether crouch modifier is active (fast ladder descent). */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "True when IA_Crouch is held during ladder climbing."))
	bool bIsLadderFastDescending = false;

	// ========================================================================
	// Surface Data
	// ========================================================================

	/** Surface normal of the currently grabbed surface. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|Surface",
		meta = (ToolTip = "Surface normal at current grab point. Used for character rotation alignment."))
	FVector CurrentSurfaceNormal = FVector::ForwardVector;

	/** Ledge position of the current grab. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|Surface",
		meta = (ToolTip = "World position of current ledge grab point."))
	FVector CurrentLedgePosition = FVector::ZeroVector;

	// ========================================================================
	// Corner Transition Data
	// ========================================================================

	/** Whether the current corner is an inside corner (true) or outside corner (false). */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|Corner",
		meta = (ToolTip = "True for inside corner, false for outside corner. Used for animation selection."))
	bool bIsInsideCorner = false;

	// ========================================================================
	// Ragdoll Recovery Data
	// ========================================================================

	/** Whether character is face up (true) or face down (false) during ragdoll. */
	UPROPERTY(BlueprintReadWrite, Category = "Climbing|Ragdoll",
		meta = (ToolTip = "True if pelvis up-vector dot world up > 0. Determines get-up animation."))
	bool bRagdollFaceUp = false;

	// ========================================================================
	// Helper Functions
	// ========================================================================

	/**
	 * Sets all IK weights to zero immediately. Used during state transitions.
	 */
	UFUNCTION(BlueprintCallable, Category = "Climbing|IK")
	void ResetAllIKWeights();

	/**
	 * Blends a single IK weight toward a target value over time.
	 * @param CurrentWeight The current weight value (modified in place)
	 * @param TargetWeight The target weight to blend toward
	 * @param DeltaTime Frame delta time
	 * @param BlendTime Total blend duration
	 */
	UFUNCTION(BlueprintCallable, Category = "Climbing|IK")
	static float BlendIKWeight(float CurrentWeight, float TargetWeight, float DeltaTime, float BlendTime);

	/** Clears notify-driven limb gating and returns to state-driven IK weighting. */
	void ResetIKNotifyMask();

	/** Enables/disables notify-driven IK gating for the provided limb bitmask. */
	void SetIKNotifyLimbState(int32 LimbMask, bool bEnable);

	/** Applies notify-driven limb gating to a proposed IK target weight. */
	float ApplyNotifyMaskToWeight(uint8 LimbBit, float ProposedWeight) const;

protected:
	/** Cached reference to the owning climbing character. */
	UPROPERTY(BlueprintReadOnly, Category = "Climbing")
	TWeakObjectPtr<AClimbingCharacter> OwningClimbingCharacter;

	/** Target IK weights for blending. */
	float TargetIKWeightHandLeft = 0.0f;
	float TargetIKWeightHandRight = 0.0f;
	float TargetIKWeightFootLeft = 0.0f;
	float TargetIKWeightFootRight = 0.0f;

	/** True once any enable/disable notify has explicitly gated IK limbs. */
	bool bHasIKNotifyMask = false;

	/** Bitmask of limbs currently allowed by IK enable/disable notifies. */
	uint8 IKNotifyEnabledMask = 0;

	/** Updates IK weight blending based on targets and blend times. */
	void UpdateIKBlending(float DeltaSeconds);

	/** Friend class for direct property access from animation notifies. */
	friend class UAnimNotify_EnableClimbIK;
	friend class UAnimNotify_DisableClimbIK;
	friend class AClimbingCharacter;
};
