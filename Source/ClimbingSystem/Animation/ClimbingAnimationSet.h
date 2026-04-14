// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/ClimbingTypes.h"
#include "ClimbingAnimationSet.generated.h"

/**
 * Per-surface animation override set.
 * Mirrors every montage slot on AClimbingCharacter using identical field names.
 * Substitution is per-slot: a null field falls back to the character default for that slot only.
 * 
 * Assign to UClimbingSurfaceData::AnimationSetOverride for surface-specific animations.
 */
UCLASS(BlueprintType)
class CLIMBINGSYSTEM_API UClimbingAnimationSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UClimbingAnimationSet();

	// ========================================================================
	// Ledge Animations
	// ========================================================================

	/** Idle hang animation. Plays when hanging stationary on a ledge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Idle animation while hanging from ledge. If null, falls back to character default."))
	TObjectPtr<UAnimMontage> HangIdle;

	/** Idle hang with left lean. Triggered by idle variation system. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Idle variation with left lean. Selected randomly by idle variation system after IdleVariationDelay."))
	TObjectPtr<UAnimMontage> HangIdleLeft;

	/** Idle hang with right lean. Triggered by idle variation system. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Idle variation with right lean. Selected randomly by idle variation system after IdleVariationDelay."))
	TObjectPtr<UAnimMontage> HangIdleRight;

	/** Shimmy left animation. Plays when moving left along ledge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation for shimmying left along ledge. In-place animation — movement component drives lateral motion."))
	TObjectPtr<UAnimMontage> ShimmyLeft;

	/** Shimmy right animation. Plays when moving right along ledge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation for shimmying right along ledge. In-place animation — movement component drives lateral motion."))
	TObjectPtr<UAnimMontage> ShimmyRight;

	/** Inside corner transition (moving left). Character wraps around corner toward wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Corner transition animation moving left around an inside corner. Root motion with WarpTarget_CornerPivot rotation."))
	TObjectPtr<UAnimMontage> CornerInsideLeft;

	/** Inside corner transition (moving right). Character wraps around corner toward wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Corner transition animation moving right around an inside corner. Root motion with WarpTarget_CornerPivot rotation."))
	TObjectPtr<UAnimMontage> CornerInsideRight;

	/** Outside corner transition (moving left). Character swings around corner away from wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Corner transition animation moving left around an outside corner. Root motion with WarpTarget_CornerPivot rotation."))
	TObjectPtr<UAnimMontage> CornerOutsideLeft;

	/** Outside corner transition (moving right). Character swings around corner away from wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Corner transition animation moving right around an outside corner. Root motion with WarpTarget_CornerPivot rotation."))
	TObjectPtr<UAnimMontage> CornerOutsideRight;

	/** Climb up with full clearance. Character stands after reaching top. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation for climbing up onto ledge with full standing clearance. Root motion + WarpTarget_ClimbUpLand."))
	TObjectPtr<UAnimMontage> ClimbUp;

	/** Climb up with crouch clearance. Character remains crouched after reaching top. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation for climbing up onto ledge with crouch-only clearance. Root motion + WarpTarget_ClimbUpLand."))
	TObjectPtr<UAnimMontage> ClimbUpCrouch;

	/** Drop down from ledge. Intentional release animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation for intentionally dropping from ledge or braced wall. Root motion."))
	TObjectPtr<UAnimMontage> DropDown;

	/** Grab ledge animation. Plays when initiating hang from ground or air. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation for grabbing ledge from ground or air. Root motion + WarpTarget_LedgeGrab."))
	TObjectPtr<UAnimMontage> GrabLedge;

	/** Grab fail animation. Plays when server rejects grab attempt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation when grab attempt fails (server rejection). Brief recovery animation before returning to locomotion."))
	TObjectPtr<UAnimMontage> GrabFail;

	/** Shimmy reposition animation. Plays when continuous shimmy exceeds MaxContinuousShimmyDistance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation for repositioning hands during extended shimmy. Triggered when shimmy distance exceeds MaxContinuousShimmyDistance."))
	TObjectPtr<UAnimMontage> ShimmyReposition;

	// ========================================================================
	// Lache Animations
	// ========================================================================

	/** Lache launch animation. Initial push-off from ledge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Lache",
		meta = (ToolTip = "Animation for launching Lache jump. Root motion — character pushes off from ledge."))
	TObjectPtr<UAnimMontage> LacheLaunch;

	/** Lache flight animation. In-air pose during arc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Lache",
		meta = (ToolTip = "Looping animation during Lache flight. Root motion maintains arc trajectory."))
	TObjectPtr<UAnimMontage> LacheFlight;

	/** Lache catch animation. Grabbing target ledge at end of arc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Lache",
		meta = (ToolTip = "Animation for catching target ledge after Lache jump. Root motion + WarpTarget_LacheCatch."))
	TObjectPtr<UAnimMontage> LacheCatch;

	/** Lache miss animation. Failed to reach target — falling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Lache",
		meta = (ToolTip = "Animation when Lache target is invalidated mid-flight. Transitions to falling locomotion."))
	TObjectPtr<UAnimMontage> LacheMiss;

	// ========================================================================
	// Mantle Animations
	// ========================================================================

	/** Low mantle animation. For obstacles <= MantleLowMaxHeight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Mantle",
		meta = (ToolTip = "Animation for mantling low obstacles. Root motion + WarpTarget_MantleLow. Used when height <= MantleLowMaxHeight."))
	TObjectPtr<UAnimMontage> MantleLow;

	/** High mantle animation. For obstacles between MantleLowMaxHeight and MantleHighMaxHeight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Mantle",
		meta = (ToolTip = "Animation for mantling high obstacles. Root motion + WarpTarget_MantleHigh. Used when height > MantleLowMaxHeight."))
	TObjectPtr<UAnimMontage> MantleHigh;

	// ========================================================================
	// Ragdoll Animations
	// ========================================================================

	/** Get up from ragdoll — face down orientation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ragdoll",
		meta = (ToolTip = "Animation for recovering from ragdoll when face-down. Selected based on pelvis up-vector dot world up."))
	TObjectPtr<UAnimMontage> RagdollGetUpFaceDown;

	/** Get up from ragdoll — face up orientation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ragdoll",
		meta = (ToolTip = "Animation for recovering from ragdoll when face-up. Selected based on pelvis up-vector dot world up."))
	TObjectPtr<UAnimMontage> RagdollGetUpFaceUp;

	// ========================================================================
	// Braced Wall Animations
	// ========================================================================

	/** Braced wall idle animation. Feet planted, hands on wall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Braced",
		meta = (ToolTip = "Idle animation for braced wall climb. Feet planted on wall below hands."))
	TObjectPtr<UAnimMontage> BracedIdle;

	/** Braced shimmy left animation. Lateral movement while braced. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Braced",
		meta = (ToolTip = "Animation for shimmying left while in braced wall climb. In-place — movement component drives motion."))
	TObjectPtr<UAnimMontage> BracedShimmyLeft;

	/** Braced shimmy right animation. Lateral movement while braced. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Braced",
		meta = (ToolTip = "Animation for shimmying right while in braced wall climb. In-place — movement component drives motion."))
	TObjectPtr<UAnimMontage> BracedShimmyRight;

	/** Braced to hang transition. Moving up from braced to ledge hang. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Braced",
		meta = (ToolTip = "Animation for transitioning from braced wall to ledge hang when lip is detected above."))
	TObjectPtr<UAnimMontage> BracedToHang;

	// ========================================================================
	// Ladder Animations
	// ========================================================================

	/** Ladder idle animation. Stationary on ladder. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Idle animation while on ladder. In-place."))
	TObjectPtr<UAnimMontage> LadderIdle;

	/** Ladder climb up animation. Normal ascending speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for climbing up ladder at normal speed. In-place — movement component drives vertical motion."))
	TObjectPtr<UAnimMontage> LadderClimbUp;

	/** Ladder climb down animation. Normal descending speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for climbing down ladder at normal speed. In-place — movement component drives vertical motion."))
	TObjectPtr<UAnimMontage> LadderClimbDown;

	/** Ladder fast ascend animation. Sprint modifier active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for fast ladder ascent (IA_Sprint held). In-place with increased playback rate."))
	TObjectPtr<UAnimMontage> LadderFastAscend;

	/** Ladder fast descend animation. Crouch modifier active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for fast ladder descent (IA_Crouch held). In-place with increased playback rate."))
	TObjectPtr<UAnimMontage> LadderFastDescend;

	/** Ladder enter from bottom animation. Mounting ladder from ground. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for entering ladder from bottom. Root motion + WarpTarget_LadderEnterBottom."))
	TObjectPtr<UAnimMontage> LadderEnterBottom;

	/** Ladder enter from top animation. Mounting ladder from above. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for entering ladder from top (climbing down). Root motion + WarpTarget_LadderEnterTop."))
	TObjectPtr<UAnimMontage> LadderEnterTop;

	/** Ladder exit at bottom animation. Dismounting at ladder base. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for exiting ladder at bottom onto ground. Root motion + WarpTarget_LadderExitBottom."))
	TObjectPtr<UAnimMontage> LadderExitBottom;

	/** Ladder exit at top animation. Dismounting at ladder top. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for exiting ladder at top onto surface. Root motion + WarpTarget_LadderExitTop."))
	TObjectPtr<UAnimMontage> LadderExitTop;

	/** Ladder exit side animation. Intentional drop from ladder. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for intentionally dropping off side of ladder. Root motion."))
	TObjectPtr<UAnimMontage> LadderExitSide;

	// ========================================================================
	// Utility Functions
	// ========================================================================

	/**
	 * Returns the montage assigned to the given slot.
	 * Returns nullptr if the slot is not assigned in this override set.
	 * The character is responsible for falling back to defaults for null returns.
	 * @param Slot The animation slot to query
	 * @return The assigned montage, or nullptr if not overridden
	 */
	UFUNCTION(BlueprintCallable, Category = "Climbing")
	UAnimMontage* GetMontageForSlot(EClimbingAnimationSlot Slot) const;

	// ========================================================================
	// UPrimaryDataAsset Interface
	// ========================================================================

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
