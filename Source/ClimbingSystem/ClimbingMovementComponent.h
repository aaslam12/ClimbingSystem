// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ClimbingTypes.h"
#include "ClimbingMovementComponent.generated.h"

/**
 * REPLICATION CONTRACT — READ BEFORE MODIFYING
 *
 * Authority model: listen-server authoritative, optimistic client prediction.
 *
 * Replicated state:
 *   - CurrentClimbingState          (ReplicatedUsing = OnRep_ClimbingState)
 *   - LastValidatedDetectionResult  (FClimbingDetectionResultNet — no pointer types; safe to replicate)
 *   - AnchorComponent               (TObjectPtr — replicable; NOT TWeakObjectPtr)
 *   - AnchorLocalTransform
 *
 * NOT replicated (computed locally from replicated data):
 *   - FClimbingDetectionResult.HitComponent (resolved via ResolveHitComponentFromNet())
 *   - IK targets, camera state, audio, detection scan results
 *
 * Confirmation trace:
 *   - Owning client: skips confirmation trace during active climbing; uses local scan HitComponent directly
 *   - Simulated proxies: runs ResolveHitComponentFromNet() in OnRep_ClimbingState, not deferred
 *   - Failure: IK disabled for that frame; retries next IK update
 *
 * Server RPC flow:
 *   1. Client input → predicted state locally → Server_ RPC with FClimbingDetectionResultNet
 *   2. Server re-runs detection → validates LedgePosition within ServerValidationPositionTolerance
 *   3. Accept → replicate CurrentClimbingState → proxies update via OnRep
 *   4. Reject → Client_RejectStateTransition → client rollback
 *
 * Input Mapping Context:
 *   - ClimbingInputMappingContext added at ClimbingIMCPriority on climbing entry
 *   - Removed on return to None
 *   - Higher priority than locomotion IMC prevents Sprint/Crouch conflicts on ladder
 *
 * Anchor constraint: persistent level actors only. Streaming sublevel = rejected + logged.
 * Pause behavior: all timers bIgnorePause = false. System fully pauses with game.
 * Client-only: camera, IK, audio, detection, confirmation trace.
 * Map transition: BeginPlay registers; EndPlay AND Destroyed both unregister from
 *   ActiveClimbingCharacters, call SetBase(nullptr), stop montages, clear Lache target,
 *   restore physics. Dual-unregister handles multiplayer edge cases.
 */
UCLASS()
class CLIMBINGSYSTEM_API UClimbingMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UClimbingMovementComponent();

	// ========================================================================
	// UCharacterMovementComponent Overrides
	// ========================================================================

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual float GetMaxSpeed() const override;
	virtual bool CanAttemptJump() const override;
	virtual bool DoJump(bool bReplayingMoves) override;
	
	/**
	 * Override to prevent base class from interfering with in-place climbing animations.
	 * Returns false during climbing states where root motion should not affect velocity.
	 */
	virtual bool ShouldUsePackedMovementRPCs() const override;

	// ========================================================================
	// Climbing State (Replicated)
	// ========================================================================

	/** Current climbing state. Replicated with OnRep for proxy animation updates. */
	UPROPERTY(ReplicatedUsing = OnRep_ClimbingState, BlueprintReadOnly, Category = "Climbing|State")
	EClimbingState CurrentClimbingState = EClimbingState::None;

	/** Previous climbing state before last transition. Used for exit animation selection. */
	UPROPERTY(BlueprintReadOnly, Category = "Climbing|State")
	EClimbingState PreviousClimbingState = EClimbingState::None;

	/** Last validated detection result from server. Replicated for proxy IK. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Climbing|State")
	FClimbingDetectionResultNet LastValidatedDetectionResult;

	// ========================================================================
	// Anchor (Replicated)
	// ========================================================================

	/** Component the character is currently anchored to. Replicated for physics sync. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Climbing|Anchor")
	TObjectPtr<UPrimitiveComponent> AnchorComponent;

	/** Local-space transform relative to AnchorComponent. World position = Anchor.Transform * This. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Climbing|Anchor")
	FTransform AnchorLocalTransform;

	// ========================================================================
	// Climbing Movement Settings
	// ========================================================================

	/** Base shimmy speed in cm/s. Modified by surface ClimbSpeedMultiplier and overhang penalty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Base lateral movement speed while shimmying in cm/s. Modified by surface multipliers and overhang penalty.",
		ClampMin = "50.0", ClampMax = "500.0"))
	float BaseShimmySpeed = 150.0f;

	/** Base ladder climb speed in cm/s. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Base vertical climb speed on ladders in cm/s.",
		ClampMin = "50.0", ClampMax = "400.0"))
	float BaseLadderClimbSpeed = 100.0f;

	/** Fast ladder ascent multiplier (when IA_Sprint held). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Multiplier applied to ladder climb speed when IA_Sprint is held.",
		ClampMin = "1.0", ClampMax = "3.0"))
	float LadderSprintMultiplier = 1.5f;

	/** Fast ladder descent multiplier (when IA_Crouch held). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Multiplier applied to ladder descent speed when IA_Crouch is held.",
		ClampMin = "1.0", ClampMax = "3.0"))
	float LadderFastDescentMultiplier = 1.75f;

	// ========================================================================
	// Overhang Penalty
	// ========================================================================

	/** Angle at which overhang speed penalty begins (degrees from vertical). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Surface angle (degrees from vertical) at which overhang speed penalty begins. Below this = no penalty.",
		ClampMin = "0.0", ClampMax = "45.0"))
	float OverhangPenaltyStartAngle = 10.0f;

	/** Angle range over which penalty scales from 1.0 to max penalty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Degrees over which overhang penalty scales from 1.0 to OverhangMaxPenaltyScalar.",
		ClampMin = "5.0", ClampMax = "60.0"))
	float OverhangPenaltyRangeAngle = 30.0f;

	/** Maximum overhang speed multiplier (lower = slower on overhangs). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Speed multiplier at maximum overhang angle. 0.5 = half speed on extreme overhangs.",
		ClampMin = "0.1", ClampMax = "1.0"))
	float OverhangMaxPenaltyScalar = 0.5f;

	// ========================================================================
	// State-Specific Movement
	// ========================================================================

	/**
	 * Calculates the effective shimmy speed with all modifiers applied.
	 * @param SurfaceNormal Normal of the current climbing surface
	 * @param ClimbSpeedMultiplier Surface-specific speed multiplier
	 * @return Effective shimmy speed in cm/s
	 */
	UFUNCTION(BlueprintCallable, Category = "Climbing|Movement")
	float CalculateEffectiveShimmySpeed(const FVector& SurfaceNormal, float ClimbSpeedMultiplier = 1.0f) const;

	/**
	 * Calculates the effective ladder climb speed with modifiers.
	 * @param bSprinting True if IA_Sprint is held
	 * @param bFastDescending True if IA_Crouch is held
	 * @param ClimbSpeedMultiplier Surface-specific speed multiplier
	 * @return Effective ladder climb speed in cm/s
	 */
	UFUNCTION(BlueprintCallable, Category = "Climbing|Movement")
	float CalculateEffectiveLadderSpeed(bool bSprinting, bool bFastDescending, float ClimbSpeedMultiplier = 1.0f) const;

	// ========================================================================
	// State Transitions
	// ========================================================================

	/**
	 * Sets the climbing state with proper transition handling.
	 * Called by AClimbingCharacter when state changes.
	 * @param NewState The new climbing state
	 */
	void SetClimbingState(EClimbingState NewState);

	/**
	 * Checks if the current state allows interruption by player input.
	 * @return True if the current state can be interrupted
	 */
	UFUNCTION(BlueprintCallable, Category = "Climbing|State")
	bool CanInterruptCurrentState() const;

	/**
	 * Checks if a transition from current state to new state is valid.
	 * @param NewState The proposed new state
	 * @return True if the transition is allowed
	 */
	UFUNCTION(BlueprintCallable, Category = "Climbing|State")
	bool IsValidStateTransition(EClimbingState NewState) const;

	// ========================================================================
	// Anchor Management
	// ========================================================================

	/**
	 * Sets the anchor component and calculates local transform.
	 * @param NewAnchor The component to anchor to (can be null to clear)
	 * @param WorldGrabPoint World position of the grab point
	 */
	void SetAnchor(UPrimitiveComponent* NewAnchor, const FVector& WorldGrabPoint);

	/**
	 * Clears the current anchor.
	 */
	void ClearAnchor();

	/**
	 * Returns the current world-space grab position based on anchor and local transform.
	 * @return World position of grab point, or zero vector if no anchor
	 */
	UFUNCTION(BlueprintCallable, Category = "Climbing|Anchor")
	FVector GetWorldGrabPosition() const;

	/**
	 * Updates character position to follow moving anchor.
	 * Called during tick when anchored.
	 */
	void UpdateAnchorFollowing();

protected:
	// ========================================================================
	// Replication Callbacks
	// ========================================================================

	/** Called on clients when CurrentClimbingState is replicated. */
	UFUNCTION()
	void OnRep_ClimbingState();

	// ========================================================================
	// Internal State
	// ========================================================================

	/** State configs map — initialized in constructor. Must have entry for every EClimbingState. */
	UPROPERTY()
	TMap<EClimbingState, FClimbingStateConfig> StateConfigs;

	/** Current montage completion percentage (for conditional interruptibility). */
	float CurrentMontageCompletion = 0.0f;

	/**
	 * Initializes the StateConfigs map with default values for all states.
	 * Called in constructor.
	 */
	void InitializeStateConfigs();

	/**
	 * Calculates overhang penalty from surface normal.
	 * @param SurfaceNormal Normal of the climbing surface
	 * @return Penalty multiplier (1.0 = no penalty, lower = slower)
	 */
	float CalculateOverhangPenalty(const FVector& SurfaceNormal) const;
};
