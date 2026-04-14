// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "ClimbingTypes.generated.h"

// ============================================================================
// Log Category
// ============================================================================

DECLARE_LOG_CATEGORY_EXTERN(LogClimbing, Log, All);

// ============================================================================
// Enums
// ============================================================================

/**
 * Surface tier classification for climbing surfaces.
 * Determines whether a surface can be climbed and under what conditions.
 */
UENUM(BlueprintType)
enum class EClimbSurfaceTier : uint8
{
	/** Surface cannot be climbed under any circumstances */
	Unclimbable,
	/** Surface can be climbed normally, bypasses geometric validation */
	Climbable,
	/** Surface can only be climbed from one direction (approach vector validated) */
	ClimbableOneWay,
	/** Surface can only be used in ladder climbing state */
	LadderOnly,
	/** No tag assigned — use programmatic rules for validation */
	Untagged
};

/**
 * Clearance type above a ledge position.
 * Determines whether the character can climb up and in what stance.
 */
UENUM(BlueprintType)
enum class EClimbClearanceType : uint8
{
	/** Full clearance — character can stand after climbing up */
	Full,
	/** Crouch clearance only — character must crouch after climbing up */
	CrouchOnly,
	/** No clearance — cannot climb up at this position */
	None
};

/**
 * Sound event types for the climbing system.
 * Used as keys in the ClimbingSounds map for audio playback.
 */
UENUM(BlueprintType)
enum class EClimbSoundType : uint8
{
	/** Hand contacts ledge during grab */
	HandGrab,
	/** Foot plants on surface during shimmy */
	FootPlant,
	/** Hand contacts ladder rung */
	LadderRungHand,
	/** Foot contacts ladder rung */
	LadderRungFoot,
	/** Impact sound when landing on mantled surface */
	MantleImpact,
	/** Vocal grunt when launching a Lache jump */
	LacheLaunchGrunt,
	/** Impact sound when catching ledge after Lache */
	LacheCatchImpact,
	/** Sound when grab attempt fails or is rejected */
	GrabFail
};

/**
 * Animation slot identifiers for the climbing system.
 * Each slot maps to a specific climbing action or state.
 */
UENUM(BlueprintType)
enum class EClimbingAnimationSlot : uint8
{
	// Hang states
	HangIdle,
	HangIdleLeft,
	HangIdleRight,
	
	// Shimmy
	ShimmyLeft,
	ShimmyRight,
	
	// Corner transitions
	CornerInsideLeft,
	CornerInsideRight,
	CornerOutsideLeft,
	CornerOutsideRight,
	
	// Climb up
	ClimbUp,
	ClimbUpCrouch,
	
	// Misc ledge actions
	DropDown,
	GrabLedge,
	GrabFail,
	ShimmyReposition,
	
	// Lache
	LacheLaunch,
	LacheFlight,
	LacheCatch,
	LacheMiss,
	
	// Mantle
	MantleLow,
	MantleHigh,
	
	// Ragdoll recovery
	RagdollGetUpFaceDown,
	RagdollGetUpFaceUp,
	
	// Braced wall
	BracedIdle,
	BracedShimmyLeft,
	BracedShimmyRight,
	BracedToHang,
	
	// Ladder
	LadderIdle,
	LadderClimbUp,
	LadderClimbDown,
	LadderFastAscend,
	LadderFastDescend,
	LadderEnterBottom,
	LadderEnterTop,
	LadderExitBottom,
	LadderExitTop,
	LadderExitSide,

	// Must be last — used for iteration
	MAX UMETA(Hidden)
};

/**
 * State machine states for the climbing system.
 * Determines current climbing mode and available transitions.
 */
UENUM(BlueprintType)
enum class EClimbingState : uint8
{
	/** Not climbing — normal locomotion */
	None,
	/** Hanging from ledge, stationary */
	Hanging,
	/** Moving laterally along ledge */
	Shimmying,
	/** Pulling up onto surface (full clearance) */
	ClimbingUp,
	/** Pulling up onto surface (crouch clearance) */
	ClimbingUpCrouch,
	/** Intentionally releasing from ledge */
	DroppingDown,
	/** Transitioning around corner (inside or outside) */
	CornerTransition,
	/** Wall climb with feet braced, stationary */
	BracedWall,
	/** Wall climb with feet braced, moving laterally */
	BracedShimmying,
	/** Performing mantle animation */
	Mantling,
	/** Lache launch frame */
	Lache,
	/** In flight during Lache jump */
	LacheInAir,
	/** Catching ledge after Lache */
	LacheCatch,
	/** Missed Lache target, falling */
	LacheMiss,
	/** On ladder */
	OnLadder,
	/** Entering or exiting ladder */
	LadderTransition,
	/** Ragdoll physics active */
	Ragdoll,

	// Must be last — used for iteration
	MAX UMETA(Hidden)
};

// ============================================================================
// Structs
// ============================================================================

/**
 * Configuration for a climbing state's interruptibility.
 * Stored in a TMap keyed by EClimbingState — every state MUST have an entry.
 */
USTRUCT(BlueprintType)
struct FClimbingStateConfig
{
	GENERATED_BODY()

	/** If true, this state can be interrupted by player input at any time. Default true — the safe fallback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (ToolTip = "If true, this state can be interrupted by player input at any time. Default true — the safe fallback."))
	bool bInterruptible = true;

	/** Minimum montage completion (0.0–1.0) before this state can be cancelled. Only evaluated when bInterruptible is false. Default 0.0 = cancel immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (ToolTip = "Minimum montage completion (0.0–1.0) before this state can be cancelled. Only evaluated when bInterruptible is false. Default 0.0 = cancel immediately."))
	float MinCompletionBeforeCancel = 0.0f;

	FClimbingStateConfig()
		: bInterruptible(true)
		, MinCompletionBeforeCancel(0.0f)
	{
	}

	FClimbingStateConfig(bool bInInterruptible, float InMinCompletion = 0.0f)
		: bInterruptible(bInInterruptible)
		, MinCompletionBeforeCancel(InMinCompletion)
	{
	}
};

/**
 * Local-only detection result. Used for detection logic, IK, and state transitions on the local machine.
 * Never placed in a replicated UPROPERTY — TWeakObjectPtr is not natively replicable in UE5.
 * Simulated proxies receive FClimbingDetectionResultNet and resolve HitComponent locally via confirmation trace.
 */
USTRUCT(BlueprintType)
struct FClimbingDetectionResult
{
	GENERATED_BODY()

	/** World position of the detected ledge grab point */
	UPROPERTY(BlueprintReadOnly, Category = "Climbing")
	FVector LedgePosition = FVector::ZeroVector;

	/** Surface normal at the ledge position (points away from wall) */
	UPROPERTY(BlueprintReadOnly, Category = "Climbing")
	FVector SurfaceNormal = FVector::ZeroVector;

	/** Classification tier of the detected surface */
	UPROPERTY(BlueprintReadOnly, Category = "Climbing")
	EClimbSurfaceTier SurfaceTier = EClimbSurfaceTier::Untagged;

	/** Clearance type above the ledge (Full, CrouchOnly, None) */
	UPROPERTY(BlueprintReadOnly, Category = "Climbing")
	EClimbClearanceType ClearanceType = EClimbClearanceType::None;

	/** 
	 * Local only — NOT replicated. TWeakObjectPtr is not replicable in UE5.
	 * Simulated proxies resolve HitComponent locally via confirmation trace.
	 * @see AClimbingCharacter::ResolveHitComponentFromNet()
	 */
	TWeakObjectPtr<UPrimitiveComponent> HitComponent;

	/** Whether this detection result contains valid data */
	UPROPERTY(BlueprintReadOnly, Category = "Climbing")
	bool bValid = false;

	FClimbingDetectionResult() = default;

	/** Resets all fields to default invalid state */
	void Reset()
	{
		LedgePosition = FVector::ZeroVector;
		SurfaceNormal = FVector::ZeroVector;
		SurfaceTier = EClimbSurfaceTier::Untagged;
		ClearanceType = EClimbClearanceType::None;
		HitComponent.Reset();
		bValid = false;
	}

	/** Creates a network-safe version of this result (excludes HitComponent) */
	struct FClimbingDetectionResultNet ToNetResult() const;
};

/**
 * Network-safe detection result subset. No pointer types — safe to replicate.
 * HitComponent is excluded; each client resolves it locally via confirmation trace
 * from LedgePosition after receiving this struct.
 * 
 * Intentionally USTRUCT() without BlueprintType — this is a network serialization struct,
 * not a designer-facing type. Do NOT add BlueprintType.
 * 
 * Do NOT implement custom NetSerialize — the struct only replicates on state transitions
 * (not per-tick), so default field-by-field serialization is acceptable.
 * 
 * @see AClimbingCharacter::ResolveHitComponentFromNet()
 */
USTRUCT()
struct FClimbingDetectionResultNet
{
	GENERATED_BODY()

	/** World position of the detected ledge grab point (1cm precision via FVector_NetQuantize) */
	UPROPERTY()
	FVector_NetQuantize LedgePosition = FVector::ZeroVector;

	/** Surface normal at the ledge position (normalized, quantized for network) */
	UPROPERTY()
	FVector_NetQuantizeNormal SurfaceNormal = FVector::ZeroVector;

	/** Classification tier of the detected surface */
	UPROPERTY()
	EClimbSurfaceTier SurfaceTier = EClimbSurfaceTier::Untagged;

	/** Clearance type above the ledge (Full, CrouchOnly, None) */
	UPROPERTY()
	EClimbClearanceType ClearanceType = EClimbClearanceType::None;

	/** Whether this detection result contains valid data */
	UPROPERTY()
	bool bValid = false;

	FClimbingDetectionResultNet() = default;

	/** Constructs from a local FClimbingDetectionResult (drops HitComponent) */
	explicit FClimbingDetectionResultNet(const FClimbingDetectionResult& LocalResult)
		: LedgePosition(LocalResult.LedgePosition)
		, SurfaceNormal(LocalResult.SurfaceNormal)
		, SurfaceTier(LocalResult.SurfaceTier)
		, ClearanceType(LocalResult.ClearanceType)
		, bValid(LocalResult.bValid)
	{
	}

	/** Resets all fields to default invalid state */
	void Reset()
	{
		LedgePosition = FVector::ZeroVector;
		SurfaceNormal = FVector::ZeroVector;
		SurfaceTier = EClimbSurfaceTier::Untagged;
		ClearanceType = EClimbClearanceType::None;
		bValid = false;
	}
};

// Inline implementation of ToNetResult (after FClimbingDetectionResultNet is defined)
inline FClimbingDetectionResultNet FClimbingDetectionResult::ToNetResult() const
{
	return FClimbingDetectionResultNet(*this);
}
