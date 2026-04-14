// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "ClimbingTypes.h"
#include "ClimbingSurfaceData.generated.h"

class UClimbingAnimationSet;

/**
 * Per-surface metadata for the climbing system.
 * Assign to actors via component tags or direct reference.
 * AnimationSetOverride is loaded async on first surface contact.
 */
UCLASS(BlueprintType)
class CLIMBINGSYSTEM_API UClimbingSurfaceData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UClimbingSurfaceData();

	// ========================================================================
	// Surface Classification
	// ========================================================================

	/** Surface tier classification. Determines climbability rules. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Surface",
		meta = (ToolTip = "Surface tier classification. Climbable bypasses geometric validation; ClimbableOneWay validates approach vector; LadderOnly restricts to ladder state."))
	EClimbSurfaceTier SurfaceTier = EClimbSurfaceTier::Climbable;

	/** One-way approach direction in local space. Only used when SurfaceTier == ClimbableOneWay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Surface",
		meta = (ToolTip = "Valid approach direction in local space. Character must approach within ApproachAngleTolerance of this vector. Only evaluated when SurfaceTier is ClimbableOneWay.", 
		EditCondition = "SurfaceTier == EClimbSurfaceTier::ClimbableOneWay"))
	FVector OneWayApproachDirection = FVector::ForwardVector;

	/** Angle tolerance in degrees for one-way approach validation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Surface",
		meta = (ToolTip = "Maximum angle in degrees between character approach and OneWayApproachDirection for the grab to be valid. 45 = lenient, 15 = strict.",
		ClampMin = "0.0", ClampMax = "90.0",
		EditCondition = "SurfaceTier == EClimbSurfaceTier::ClimbableOneWay"))
	float ApproachAngleTolerance = 30.0f;

	// ========================================================================
	// Movement Modifiers
	// ========================================================================

	/** Multiplier applied to shimmy and ladder climb speeds on this surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Multiplier applied to base shimmy and ladder climb speeds. 1.0 = normal. 0.5 = half speed (icy/slippery). 1.5 = fast (grip-enhanced).",
		ClampMin = "0.1", ClampMax = "3.0"))
	float ClimbSpeedMultiplier = 1.0f;

	/** Stamina drain multiplier for this surface (if stamina system is implemented). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Multiplier for stamina drain rate while climbing this surface. 1.0 = normal, 2.0 = double drain (difficult surface). Ignored if no stamina system.",
		ClampMin = "0.0", ClampMax = "5.0"))
	float StaminaDrainMultiplier = 1.0f;

	/** Whether this surface allows Lache jumps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "If false, Lache input is ignored while hanging on this surface. Use for surfaces where jump is dangerous or nonsensical."))
	bool bAllowLache = true;

	// ========================================================================
	// Animation Override
	// ========================================================================

	/** Optional animation set override for this surface type. Loaded async on first contact. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations",
		meta = (ToolTip = "Optional animation set that overrides character defaults when climbing this surface. Null slots fall back to character defaults. Loaded asynchronously on first surface contact."))
	TSoftObjectPtr<UClimbingAnimationSet> AnimationSetOverride;

	// ========================================================================
	// Audio Override
	// ========================================================================

	/** Physical surface type for footstep/hand impact sounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Audio",
		meta = (ToolTip = "Physical material type used to select appropriate impact sounds. If None, uses default climbing sounds."))
	TEnumAsByte<EPhysicalSurface> PhysicalSurfaceType = EPhysicalSurface::SurfaceType_Default;

	/** Volume multiplier for climbing sounds on this surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Audio",
		meta = (ToolTip = "Volume multiplier for all climbing sounds on this surface. 1.0 = normal, 0.5 = quiet (soft material), 1.5 = loud (metal).",
		ClampMin = "0.0", ClampMax = "3.0"))
	float SoundVolumeMultiplier = 1.0f;

	// ========================================================================
	// Ladder-Specific (only used when SurfaceTier == LadderOnly)
	// ========================================================================

	/** Rung spacing for ladder IK (cm). Only used when SurfaceTier == LadderOnly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Ladder",
		meta = (ToolTip = "Vertical distance between ladder rungs in cm. Used for procedural IK foot/hand placement. Standard ladder = 30cm.",
		ClampMin = "15.0", ClampMax = "60.0",
		EditCondition = "SurfaceTier == EClimbSurfaceTier::LadderOnly"))
	float LadderRungSpacing = 30.0f;

	/** Ladder width for side-to-side IK offset (cm). Only used when SurfaceTier == LadderOnly. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Ladder",
		meta = (ToolTip = "Horizontal width of ladder for hand IK offset. Hands placed at +/- half this width from center.",
		ClampMin = "20.0", ClampMax = "100.0",
		EditCondition = "SurfaceTier == EClimbSurfaceTier::LadderOnly"))
	float LadderWidth = 40.0f;

	// ========================================================================
	// UPrimaryDataAsset Interface
	// ========================================================================

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
