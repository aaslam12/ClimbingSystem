// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ClimbingTypes.h"
#include "InputActionValue.h"
#include "ClimbingCharacter.generated.h"

// Forward declarations
class UClimbingMovementComponent;
class UClimbingAnimInstance;
class UClimbingAnimationSet;
class UClimbingSurfaceData;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UMotionWarpingComponent;
class UAnimMontage;
class USoundBase;

// Log category declared in ClimbingTypes.h: DECLARE_LOG_CATEGORY_EXTERN(LogClimbing, Log, All);

/**
 * Climbing character class implementing a full cinematic climbing system.
 * Supports listen-server multiplayer with server-authoritative state.
 * 
 * Key Features:
 * - Ledge hang with shimmy (hysteresis-gated direction)
 * - Corner transitions (inside/outside)
 * - Braced wall climbing
 * - Ladder climbing with fast ascent/descent
 * - Lache jumps with predictive arc tracing
 * - Mantle (low/high based on height)
 * - Freefall re-grab with coyote time
 * - IK for hands and feet
 * - Dynamic anchor support
 * - Physics grab-breaking with ragdoll recovery
 * 
 * All gameplay logic in C++. Blueprints are data containers only.
 */
UCLASS()
class CLIMBINGSYSTEM_API AClimbingCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AClimbingCharacter(const FObjectInitializer& ObjectInitializer);

	// ========================================================================
	// AActor Overrides
	// ========================================================================

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Destroyed() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ========================================================================
	// Components
	// ========================================================================

	/** Custom movement component for climbing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing|Components")
	TObjectPtr<UClimbingMovementComponent> ClimbingMovement;

	/** Motion warping component for root motion alignment. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing|Components")
	TObjectPtr<UMotionWarpingComponent> MotionWarping;

	/** Spring arm for camera. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing|Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Follow camera. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Climbing|Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	// ========================================================================
	// Input — Mapping Context
	// ========================================================================

	/** Input Mapping Context containing all climbing-specific actions. Added at higher priority on climb entry, removed on exit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
		meta = (ToolTip = "Input Mapping Context containing all climbing-specific actions (IA_Grab, IA_Drop, IA_Lache, IA_ClimbUp, IA_ClimbMove). Added to the subsystem at higher priority than the locomotion context on climbing entry, removed on climbing exit. Must be assigned — without it no climbing inputs will fire."))
	TObjectPtr<UInputMappingContext> ClimbingInputMappingContext;

	/** Priority of the climbing Input Mapping Context relative to locomotion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
		meta = (ToolTip = "Priority of the climbing Input Mapping Context relative to locomotion. Higher number = higher priority. Must be greater than the locomotion IMC priority to prevent locomotion inputs from shadowing climbing inputs. 1 recommended if locomotion IMC uses priority 0."))
	int32 ClimbingIMCPriority = 1;

	/** Default locomotion Input Mapping Context (for push/pop reference). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
		meta = (ToolTip = "Default locomotion Input Mapping Context. Used as reference for relative priority."))
	TObjectPtr<UInputMappingContext> LocomotionInputMappingContext;

	// ========================================================================
	// Input — Actions
	// ========================================================================

	/** Grab / initiate climb input action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
		meta = (ToolTip = "Grab / initiate climb. ETriggerEvent::Triggered (pressed)."))
	TObjectPtr<UInputAction> IA_Grab;

	/** Intentional drop from hang or ladder input action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
		meta = (ToolTip = "Intentional drop from hang or ladder. ETriggerEvent::Triggered (pressed)."))
	TObjectPtr<UInputAction> IA_Drop;

	/** Launch a Lache jump while hanging input action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
		meta = (ToolTip = "Launch a Lache jump while hanging. ETriggerEvent::Triggered (pressed)."))
	TObjectPtr<UInputAction> IA_Lache;

	/** Pull up from hang onto surface above input action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
		meta = (ToolTip = "Pull up from hang onto surface above. ETriggerEvent::Triggered (pressed)."))
	TObjectPtr<UInputAction> IA_ClimbUp;

	/** 2D axis for shimmy left/right and ladder up/down input action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
		meta = (ToolTip = "2D axis: X = shimmy left/right, Y = ladder up/down. ETriggerEvent::Ongoing (held). Shimmy only fires while held."))
	TObjectPtr<UInputAction> IA_ClimbMove;

	/** Fast ladder ascent modifier input action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
		meta = (ToolTip = "Fast ladder ascent modifier. ETriggerEvent::Ongoing (held)."))
	TObjectPtr<UInputAction> IA_Sprint;

	/** Fast ladder descent modifier input action. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
		meta = (ToolTip = "Fast ladder descent modifier. ETriggerEvent::Ongoing (held)."))
	TObjectPtr<UInputAction> IA_Crouch;

	// ========================================================================
	// Detection Settings
	// ========================================================================

	/** Seconds between detection scans during ground locomotion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Detection",
		meta = (ToolTip = "Seconds between detection scans during ground locomotion. 0.05 = 20Hz recommended. Never 0 — per-tick scanning is reserved for active climbing states.",
		ClampMin = "0.01", ClampMax = "0.5"))
	float DetectionScanInterval = 0.05f;

	/** Seconds between scans while airborne. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Detection",
		meta = (ToolTip = "Seconds between scans while airborne. Drives coyote time re-check rate. 0.05 recommended.",
		ClampMin = "0.01", ClampMax = "0.5"))
	float FallingGrabCheckInterval = 0.05f;

	/** Sphere radius used by the confirmation trace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Detection",
		meta = (ToolTip = "Sphere radius (cm) used by the confirmation trace that resolves HitComponent from a replicated ledge position. Should be slightly larger than the expected surface contact area. 16cm default.",
		ClampMin = "5.0", ClampMax = "50.0"))
	float ConfirmationTraceRadius = 16.0f;

	/** Forward reach distance for ledge detection (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Detection",
		meta = (ToolTip = "How far forward from character to trace for ledges in cm.",
		ClampMin = "30.0", ClampMax = "150.0"))
	float LedgeDetectionForwardReach = 75.0f;

	/** Vertical range for ledge detection above character (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Detection",
		meta = (ToolTip = "Vertical range above character to scan for ledges in cm.",
		ClampMin = "50.0", ClampMax = "250.0"))
	float LedgeDetectionVerticalReach = 150.0f;

	/** Detection sphere radius for the main ledge trace (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Detection",
		meta = (ToolTip = "Sphere radius for the main ledge detection trace in cm.",
		ClampMin = "5.0", ClampMax = "30.0"))
	float LedgeDetectionRadius = 12.0f;

	/** Minimum ledge depth to be considered climbable (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Detection",
		meta = (ToolTip = "Minimum horizontal depth of a ledge to be climbable. Prevents grabbing thin lips.",
		ClampMin = "5.0", ClampMax = "50.0"))
	float MinLedgeDepth = 15.0f;

	/** Maximum angle from vertical for a surface to be climbable (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Detection",
		meta = (ToolTip = "Maximum angle from vertical for a wall surface to be climbable. 0 = perfectly vertical only.",
		ClampMin = "0.0", ClampMax = "60.0"))
	float MaxClimbableSurfaceAngle = 30.0f;

	// ========================================================================
	// Movement Settings
	// ========================================================================

	/** Minimum absolute IA_ClimbMove.X value to update committed shimmy direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Minimum absolute IA_ClimbMove.X value required to update the committed shimmy direction. Prevents rapid Left/Right montage flipping from analog stick noise near center. Raw axis values below this threshold maintain the last committed direction. 0.1 recommended.",
		ClampMin = "0.0", ClampMax = "0.5"))
	float ShimmyDirectionDeadzone = 0.1f;

	/** If EffectiveShimmySpeed drops below this threshold, the shimmy montage is paused. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "If EffectiveShimmySpeed drops below this threshold, the shimmy montage is paused (PlaybackRate = 0). Prevents the animation playing slowly while stationary. Set 0 to disable.",
		ClampMin = "0.0", ClampMax = "50.0"))
	float ShimmySpeedDeadzone = 10.0f;

	/** Minimum shimmy montage playback rate at lowest effective speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Minimum shimmy montage playback rate at lowest effective speed. Values below 0.3 produce visible foot-sliding artifacts.",
		ClampMin = "0.1", ClampMax = "1.0"))
	float ShimmyPlaybackRateMin = 0.4f;

	/** Maximum shimmy montage playback rate at full effective speed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Maximum shimmy montage playback rate at full effective speed. Values above 2.0 cause hands to lose wall contact in the animation.",
		ClampMin = "0.5", ClampMax = "2.5"))
	float ShimmyPlaybackRateMax = 1.2f;

	/** Maximum continuous shimmy distance before reposition animation plays (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Maximum distance character can shimmy without stopping before ShimmyReposition animation plays. Simulates grip fatigue.",
		ClampMin = "100.0", ClampMax = "1000.0"))
	float MaxContinuousShimmyDistance = 300.0f;

	/** Corner angle threshold for transition vs shimmy rejection (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
		meta = (ToolTip = "Minimum angle between surfaces to trigger corner transition. Below this = shimmy rejected.",
		ClampMin = "15.0", ClampMax = "90.0"))
	float CornerAngleThreshold = 30.0f;

	// ========================================================================
	// Lache Settings
	// ========================================================================

	/** Launch speed scaling the character's look direction (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Lache",
		meta = (ToolTip = "Launch speed (cm/s) scaling the character's look direction. Higher = flatter arc.",
		ClampMin = "500.0", ClampMax = "2500.0"))
	float LacheLaunchSpeed = 1200.0f;

	/** Total simulated arc duration (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Lache",
		meta = (ToolTip = "Total simulated arc duration (seconds). Increase for longer jumps.",
		ClampMin = "0.5", ClampMax = "3.0"))
	float LacheTotalArcTime = 1.2f;

	/** Number of arc subdivisions for trace steps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Lache",
		meta = (ToolTip = "Number of arc subdivisions. Higher = more accurate mid-flight obstacle detection. Each step fires one sphere trace. 12 recommended.",
		ClampMin = "4", ClampMax = "30"))
	int32 LacheArcTraceSteps = 12;

	/** Sphere radius at each arc step (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Lache",
		meta = (ToolTip = "Sphere radius at each arc step (cm). Should approximate shoulder width. Used for both obstacle detection and landing ledge detection.",
		ClampMin = "10.0", ClampMax = "50.0"))
	float LacheArcTraceRadius = 24.0f;

	/** Whether to automatically engage cinematic camera during Lache. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Lache",
		meta = (ToolTip = "If true, automatically locks camera to cinematic frame during Lache jumps that exceed LacheCinematicDistanceThreshold."))
	bool bAutoLacheCinematic = true;

	/** Minimum Lache distance to trigger auto cinematic camera (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Lache",
		meta = (ToolTip = "Minimum horizontal distance of Lache jump to trigger automatic cinematic camera lock.",
		ClampMin = "100.0", ClampMax = "1000.0",
		EditCondition = "bAutoLacheCinematic"))
	float LacheCinematicDistanceThreshold = 300.0f;

	// ========================================================================
	// Mantle Settings
	// ========================================================================

	/** Maximum height for CMC step-up (no mantle state entered). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Mantle",
		meta = (ToolTip = "Maximum height that uses CMC step-up instead of mantle animation. Below this = automatic step.",
		ClampMin = "20.0", ClampMax = "100.0"))
	float MantleStepMaxHeight = 50.0f;

	/** Maximum height for low mantle animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Mantle",
		meta = (ToolTip = "Maximum height for MantleLow animation. Heights above this use MantleHigh.",
		ClampMin = "50.0", ClampMax = "150.0"))
	float MantleLowMaxHeight = 100.0f;

	/** Maximum height that can be mantled at all. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Mantle",
		meta = (ToolTip = "Maximum height that can be mantled. Above this, mantle is not possible — must climb.",
		ClampMin = "100.0", ClampMax = "250.0"))
	float MantleHighMaxHeight = 180.0f;

	// ========================================================================
	// Ladder Settings
	// ========================================================================

	/** Default rung spacing for ladders without surface data (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Ladder",
		meta = (ToolTip = "Vertical distance between ladder rungs in cm. Used for procedural IK when no UClimbingSurfaceData is present.",
		ClampMin = "15.0", ClampMax = "60.0"))
	float DefaultLadderRungSpacing = 30.0f;

	// ========================================================================
	// Freefall Re-Grab Settings
	// ========================================================================

	/** Whether coyote time is enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Freefall",
		meta = (ToolTip = "If true, allows re-grabbing ledge within CoyoteTimeWindow seconds after leaving."))
	bool bEnableCoyoteTime = true;

	/** Coyote time window in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Freefall",
		meta = (ToolTip = "Seconds after leaving ledge during which IA_Grab can re-trigger detection.",
		ClampMin = "0.05", ClampMax = "0.5",
		EditCondition = "bEnableCoyoteTime"))
	float CoyoteTimeWindow = 0.15f;

	/** Whether falling grab is enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Freefall",
		meta = (ToolTip = "If true, allows grabbing ledges while falling within FallingGrabReachDistance."))
	bool bEnableFallingGrab = true;

	/** Maximum distance to grab ledge while falling (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Freefall",
		meta = (ToolTip = "Maximum horizontal distance character can grab a ledge while falling.",
		ClampMin = "30.0", ClampMax = "150.0",
		EditCondition = "bEnableFallingGrab"))
	float FallingGrabReachDistance = 80.0f;

	// ========================================================================
	// IK Settings
	// ========================================================================

	/** Max climbing characters with active IK per frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Max climbing characters with active IK per frame, sorted by distance to local camera. Beyond this count, IK weights are zeroed for that frame.",
		ClampMin = "1", ClampMax = "16"))
	int32 MaxSimultaneousIKCharacters = 4;

	/** Simulated proxy IK cull distance (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Simulated proxy IK disabled beyond this distance (cm). Owning client IK is never culled.",
		ClampMin = "500.0", ClampMax = "5000.0"))
	float SimulatedProxyIKCullDistance = 1500.0f;

	/** Seconds between IK updates for simulated proxies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Seconds between IK updates for simulated proxies. 0.05 = 20Hz. Owning client always updates every tick.",
		ClampMin = "0.01", ClampMax = "0.2"))
	float SimulatedProxyIKUpdateInterval = 0.05f;

	/** Maximum reach distance before IK weight fades out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Maximum limb reach distance before IK weight fades to zero. Prevents hyperextension — without this limit the arm stretches to an anatomically impossible length.",
		ClampMin = "30.0", ClampMax = "120.0"))
	float MaxReachDistance = 80.0f;

	/** Time to fade out IK when max reach is exceeded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Seconds to blend IK weight to zero when limb exceeds MaxReachDistance.",
		ClampMin = "0.05", ClampMax = "0.5"))
	float IKFadeOutBlendTime = 0.15f;

	/** Hand IK offset from ledge position (local space). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Offset from ledge position to hand IK target in local character space."))
	FVector HandIKOffset = FVector(5.0f, 0.0f, -5.0f);

	/** Distance between left and right hand IK targets (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|IK",
		meta = (ToolTip = "Horizontal distance between hand IK targets. Character shoulder width.",
		ClampMin = "20.0", ClampMax = "80.0"))
	float HandIKSpacing = 40.0f;

	// ========================================================================
	// Camera Settings
	// ========================================================================

	/** Camera nudge strength toward wall during climbing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Camera",
		meta = (ToolTip = "Strength of camera rotation nudge toward wall normal during climbing. 0 = disabled.",
		ClampMin = "0.0", ClampMax = "2.0"))
	float CameraNudgeStrength = 0.5f;

	/** Angle from wall at which camera nudge activates (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Camera",
		meta = (ToolTip = "Camera angle from wall normal at which nudge begins. Prevents nudge when already facing wall.",
		ClampMin = "15.0", ClampMax = "90.0"))
	float CameraNudgeActivationAngle = 45.0f;

	/** Speed of camera blend during nudge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Camera",
		meta = (ToolTip = "Speed of camera rotation blend during nudge. Higher = snappier.",
		ClampMin = "0.5", ClampMax = "10.0"))
	float CameraNudgeBlendSpeed = 3.0f;

	/** Camera probe radius during climbing (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Camera",
		meta = (ToolTip = "Sphere radius for camera collision probe during climbing. Larger = more collision avoidance.",
		ClampMin = "5.0", ClampMax = "30.0"))
	float ClimbingCameraProbeRadius = 12.0f;

	// ========================================================================
	// Physics Settings
	// ========================================================================

	/** Impulse threshold to break grab (Newtons). 0 = disable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Physics",
		meta = (ToolTip = "Newtons to break a grab on physics impact while hanging or braced. 0 = disable.",
		ClampMin = "0.0", ClampMax = "10000.0"))
	float GrabBreakImpulseThreshold = 2000.0f;

	/** Multiplier on incoming impulse as ragdoll launch velocity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Physics",
		meta = (ToolTip = "Multiplier on incoming impulse as ragdoll launch velocity.",
		ClampMin = "0.0", ClampMax = "2.0"))
	float GrabBreakLaunchScale = 0.5f;

	/** Minimum ragdoll seconds before recovery. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Physics",
		meta = (ToolTip = "Minimum ragdoll seconds before recovery. Too low = re-stand jitter. 1.5 recommended.",
		ClampMin = "0.5", ClampMax = "5.0"))
	float RagdollRecoveryTime = 1.5f;

	/** Pelvis bone name for ragdoll detection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Physics",
		meta = (ToolTip = "Pelvis bone name used for face-up/face-down get-up detection and ragdoll camera. Override for non-standard skeleton hierarchies."))
	FName PelvisBoneName = FName("pelvis");

	/** Bone socket for spring arm during ragdoll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Physics",
		meta = (ToolTip = "Bone socket for spring arm during ragdoll. Default 'pelvis'. Override for non-standard skeletons."))
	FName RagdollCameraTargetSocket = FName("pelvis");

	// ========================================================================
	// State Machine Settings
	// ========================================================================

	/** Collision profile during climbing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|StateMachine",
		meta = (ToolTip = "Collision profile during climbing. Must ignore WorldStatic/WorldDynamic while retaining Pawn blocking. Prevents capsule-wall jitter."))
	FName ClimbingCollisionProfile = FName("ClimbingCapsule");

	/** Capsule half-height during climbing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|StateMachine",
		meta = (ToolTip = "Reduced capsule half-height during climbing states. Prevents wall clipping.",
		ClampMin = "20.0", ClampMax = "100.0"))
	float ClimbingCapsuleHalfHeight = 48.0f;

	/** Capsule radius during climbing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|StateMachine",
		meta = (ToolTip = "Reduced capsule radius during climbing states.",
		ClampMin = "10.0", ClampMax = "50.0"))
	float ClimbingCapsuleRadius = 24.0f;

	// ========================================================================
	// Multiplayer Settings
	// ========================================================================

	/** Tolerance for server position validation (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Multiplayer",
		meta = (ToolTip = "Maximum distance between client-predicted and server-validated ledge position before rejection. Larger = more lenient to lag.",
		ClampMin = "10.0", ClampMax = "100.0"))
	float ServerValidationPositionTolerance = 30.0f;

	/** Seconds to blend out rejected prediction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Multiplayer",
		meta = (ToolTip = "Seconds to blend out a rejected prediction and lerp to pre-prediction position. Shorter = more responsive but jarring. Longer = smoother but widens cheating correction window. 0.2 recommended.",
		ClampMin = "0.1", ClampMax = "0.5"))
	float PredictionRollbackBlendOut = 0.2f;

	// ========================================================================
	// Audio Settings
	// ========================================================================

	/** Map of sound events to assets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Audio",
		meta = (ToolTip = "Map of sound events to assets. Missing entries: log Warning once, cache null, never retry."))
	TMap<EClimbSoundType, TSoftObjectPtr<USoundBase>> ClimbingSounds;

	// ========================================================================
	// Animations — Montage Slot
	// ========================================================================

	/** Animation Blueprint slot name for all climbing montages. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations",
		meta = (ToolTip = "Animation Blueprint slot name for all climbing montages. Must match the ABP slot node name exactly. Mismatching causes montages to play with no visible output. Default 'FullBody'."))
	FName ClimbingMontageSlot = FName("FullBody");

	// ========================================================================
	// Animations — Ledge
	// ========================================================================

	/** Idle hang animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Idle animation while hanging from ledge. Required for hang state."))
	TObjectPtr<UAnimMontage> HangIdle;

	/** Idle hang with left lean (idle variation). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Idle variation with left lean. Selected randomly after IdleVariationDelay."))
	TObjectPtr<UAnimMontage> HangIdleLeft;

	/** Idle hang with right lean (idle variation). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Idle variation with right lean. Selected randomly after IdleVariationDelay."))
	TObjectPtr<UAnimMontage> HangIdleRight;

	/** Shimmy left animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation for shimmying left along ledge. In-place — movement component drives motion."))
	TObjectPtr<UAnimMontage> ShimmyLeft;

	/** Shimmy right animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation for shimmying right along ledge. In-place — movement component drives motion."))
	TObjectPtr<UAnimMontage> ShimmyRight;

	/** Inside corner left transition animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Corner transition animation moving left around an inside corner."))
	TObjectPtr<UAnimMontage> CornerInsideLeft;

	/** Inside corner right transition animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Corner transition animation moving right around an inside corner."))
	TObjectPtr<UAnimMontage> CornerInsideRight;

	/** Outside corner left transition animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Corner transition animation moving left around an outside corner."))
	TObjectPtr<UAnimMontage> CornerOutsideLeft;

	/** Outside corner right transition animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Corner transition animation moving right around an outside corner."))
	TObjectPtr<UAnimMontage> CornerOutsideRight;

	/** Climb up animation (full clearance). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation for climbing up onto ledge with full standing clearance."))
	TObjectPtr<UAnimMontage> ClimbUp;

	/** Climb up crouch animation (crouch clearance). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation for climbing up onto ledge with crouch-only clearance."))
	TObjectPtr<UAnimMontage> ClimbUpCrouch;

	/** Drop down animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation for intentionally dropping from ledge or braced wall."))
	TObjectPtr<UAnimMontage> DropDown;

	/** Grab ledge animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation for grabbing ledge from ground or air."))
	TObjectPtr<UAnimMontage> GrabLedge;

	/** Grab fail animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation when grab attempt fails (server rejection)."))
	TObjectPtr<UAnimMontage> GrabFail;

	/** Shimmy reposition animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ledge",
		meta = (ToolTip = "Animation for repositioning hands during extended shimmy."))
	TObjectPtr<UAnimMontage> ShimmyReposition;

	// ========================================================================
	// Animations — Idle Variations
	// ========================================================================

	/** Additional idle variations for hang state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|IdleVariation",
		meta = (ToolTip = "Array of additional idle variation montages. Selected randomly after IdleVariationDelay."))
	TArray<TObjectPtr<UAnimMontage>> HangIdleVariations;

	/** Delay before idle variation plays (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|IdleVariation",
		meta = (ToolTip = "Seconds of idle before randomly selecting an idle variation.",
		ClampMin = "1.0", ClampMax = "30.0"))
	float IdleVariationDelay = 5.0f;

	/** Blend time for idle variations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|IdleVariation",
		meta = (ToolTip = "Blend time when transitioning to/from idle variations.",
		ClampMin = "0.1", ClampMax = "1.0"))
	float IdleVariationBlendTime = 0.25f;

	/** Prevent consecutive repeats of same idle variation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|IdleVariation",
		meta = (ToolTip = "If true, the last-played idle variation is excluded from the random pool."))
	bool bPreventConsecutiveVariationRepeat = true;

	// ========================================================================
	// Animations — Lache
	// ========================================================================

	/** Lache launch animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Lache",
		meta = (ToolTip = "Animation for launching Lache jump."))
	TObjectPtr<UAnimMontage> LacheLaunch;

	/** Lache flight animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Lache",
		meta = (ToolTip = "Looping animation during Lache flight."))
	TObjectPtr<UAnimMontage> LacheFlight;

	/** Lache catch animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Lache",
		meta = (ToolTip = "Animation for catching target ledge after Lache jump."))
	TObjectPtr<UAnimMontage> LacheCatch;

	/** Lache miss animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Lache",
		meta = (ToolTip = "Animation when Lache target is invalidated mid-flight."))
	TObjectPtr<UAnimMontage> LacheMiss;

	// ========================================================================
	// Animations — Mantle
	// ========================================================================

	/** Low mantle animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Mantle",
		meta = (ToolTip = "Animation for mantling low obstacles."))
	TObjectPtr<UAnimMontage> MantleLow;

	/** High mantle animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Mantle",
		meta = (ToolTip = "Animation for mantling high obstacles."))
	TObjectPtr<UAnimMontage> MantleHigh;

	// ========================================================================
	// Animations — Ragdoll
	// ========================================================================

	/** Ragdoll get up (face down) animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ragdoll",
		meta = (ToolTip = "Animation for recovering from ragdoll when face-down."))
	TObjectPtr<UAnimMontage> RagdollGetUpFaceDown;

	/** Ragdoll get up (face up) animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ragdoll",
		meta = (ToolTip = "Animation for recovering from ragdoll when face-up."))
	TObjectPtr<UAnimMontage> RagdollGetUpFaceUp;

	// ========================================================================
	// Animations — Braced Wall
	// ========================================================================

	/** Braced wall idle animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Braced",
		meta = (ToolTip = "Idle animation for braced wall climb."))
	TObjectPtr<UAnimMontage> BracedIdle;

	/** Braced shimmy left animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Braced",
		meta = (ToolTip = "Animation for shimmying left while in braced wall climb."))
	TObjectPtr<UAnimMontage> BracedShimmyLeft;

	/** Braced shimmy right animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Braced",
		meta = (ToolTip = "Animation for shimmying right while in braced wall climb."))
	TObjectPtr<UAnimMontage> BracedShimmyRight;

	/** Braced to hang transition animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Braced",
		meta = (ToolTip = "Animation for transitioning from braced wall to ledge hang."))
	TObjectPtr<UAnimMontage> BracedToHang;

	// ========================================================================
	// Animations — Ladder
	// ========================================================================

	/** Ladder idle animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Idle animation while on ladder."))
	TObjectPtr<UAnimMontage> LadderIdle;

	/** Ladder climb up animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for climbing up ladder at normal speed."))
	TObjectPtr<UAnimMontage> LadderClimbUp;

	/** Ladder climb down animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for climbing down ladder at normal speed."))
	TObjectPtr<UAnimMontage> LadderClimbDown;

	/** Ladder fast ascend animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for fast ladder ascent (IA_Sprint held)."))
	TObjectPtr<UAnimMontage> LadderFastAscend;

	/** Ladder fast descend animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for fast ladder descent (IA_Crouch held)."))
	TObjectPtr<UAnimMontage> LadderFastDescend;

	/** Ladder enter bottom animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for entering ladder from bottom."))
	TObjectPtr<UAnimMontage> LadderEnterBottom;

	/** Ladder enter top animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for entering ladder from top."))
	TObjectPtr<UAnimMontage> LadderEnterTop;

	/** Ladder exit bottom animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for exiting ladder at bottom."))
	TObjectPtr<UAnimMontage> LadderExitBottom;

	/** Ladder exit top animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for exiting ladder at top."))
	TObjectPtr<UAnimMontage> LadderExitTop;

	/** Ladder exit side animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations|Ladder",
		meta = (ToolTip = "Animation for intentionally dropping off side of ladder."))
	TObjectPtr<UAnimMontage> LadderExitSide;

	// ========================================================================
	// Debug Settings
	// ========================================================================

	/** Enable runtime debug drawing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Debug",
		meta = (ToolTip = "Enable runtime debug drawing. Compiled out in Shipping builds."))
	bool bDrawDebug = false;

	// ========================================================================
	// Public Functions — Detection
	// ========================================================================

	/**
	 * Performs ledge detection from current position.
	 * @return Detection result with validity and hit information
	 */
	UFUNCTION(BlueprintCallable, Category = "Climbing|Detection")
	FClimbingDetectionResult PerformLedgeDetection() const;

	/**
	 * Performs ladder detection from current position.
	 * @return Detection result with validity and hit information
	 */
	UFUNCTION(BlueprintCallable, Category = "Climbing|Detection")
	FClimbingDetectionResult PerformLadderDetection() const;

	/**
	 * Performs braced wall detection from current position.
	 * @return Detection result with validity and hit information
	 */
	UFUNCTION(BlueprintCallable, Category = "Climbing|Detection")
	FClimbingDetectionResult PerformBracedWallDetection() const;

	/**
	 * Resolves HitComponent from a replicated network detection result.
	 * Casts a short sphere trace from LedgePosition inward along the negated SurfaceNormal.
	 * Cannot be BlueprintCallable since FClimbingDetectionResultNet is not BlueprintType.
	 * @param NetResult The network-safe detection result to resolve
	 * @return The resolved primitive component, or null if trace fails
	 */
	UPrimitiveComponent* ResolveHitComponentFromNet(const FClimbingDetectionResultNet& NetResult) const;

	// ========================================================================
	// Public Functions — Montage Access
	// ========================================================================

	/**
	 * Returns the active montage for the given slot.
	 * Priority: current surface AnimationSetOverride → character defaults.
	 * @param Slot The animation slot to query
	 * @return The assigned montage, or nullptr if not assigned
	 */
	UFUNCTION(BlueprintCallable, Category = "Climbing|Animations")
	UAnimMontage* GetMontageForSlot(EClimbingAnimationSlot Slot) const;

	// ========================================================================
	// Public Functions — Camera
	// ========================================================================

	/**
	 * Locks camera to a specific frame for cinematic moments.
	 * @param Location Target camera world location
	 * @param Rotation Target camera rotation
	 * @param BlendTime Seconds to blend to locked position
	 */
	UFUNCTION(BlueprintCallable, Category = "Climbing|Camera")
	void LockCameraToFrame(FVector Location, FRotator Rotation, float BlendTime);

	/**
	 * Releases camera lock and returns to normal following.
	 * @param BlendTime Seconds to blend back to normal
	 */
	UFUNCTION(BlueprintCallable, Category = "Climbing|Camera")
	void ReleaseCameraLock(float BlendTime);

	// ========================================================================
	// Public Functions — State
	// ========================================================================

	/** Called by UClimbingMovementComponent when state is replicated. */
	void OnClimbingStateReplicated(EClimbingState OldState, EClimbingState NewState);

	// ========================================================================
	// Server RPCs
	// ========================================================================

	/** Server RPC to attempt grab with client detection result. */
	UFUNCTION(Server, Reliable)
	void Server_AttemptGrab(FClimbingDetectionResultNet ClientResult);

	/** Server RPC to drop from current climbing state. */
	UFUNCTION(Server, Reliable)
	void Server_Drop();

	/** Server RPC to attempt Lache jump. */
	UFUNCTION(Server, Reliable)
	void Server_AttemptLache(FVector ClientArcTarget);

	/** Server RPC to attempt climb up. */
	UFUNCTION(Server, Reliable)
	void Server_AttemptClimbUp();

	/** Server RPC to update shimmy direction. */
	UFUNCTION(Server, Unreliable)
	void Server_UpdateShimmyDirection(FVector2D Direction);

	// ========================================================================
	// Client RPCs
	// ========================================================================

	/** Client RPC to reject a state transition and trigger rollback. */
	UFUNCTION(Client, Reliable)
	void Client_RejectStateTransition();

	/** Client RPC to confirm a state transition. */
	UFUNCTION(Client, Reliable)
	void Client_ConfirmStateTransition(EClimbingState ConfirmedState);

protected:
	// ========================================================================
	// Input Handlers
	// ========================================================================

	void Input_Grab(const FInputActionValue& Value);
	void Input_Drop(const FInputActionValue& Value);
	void Input_Lache(const FInputActionValue& Value);
	void Input_ClimbUp(const FInputActionValue& Value);
	void Input_ClimbMove(const FInputActionValue& Value);
	void Input_ClimbMoveCompleted(const FInputActionValue& Value);
	void Input_Sprint(const FInputActionValue& Value);
	void Input_SprintCompleted(const FInputActionValue& Value);
	void Input_Crouch(const FInputActionValue& Value);
	void Input_CrouchCompleted(const FInputActionValue& Value);

	// ========================================================================
	// State Management
	// ========================================================================

	/** Transitions to a new climbing state with proper cleanup and setup. */
	void TransitionToState(EClimbingState NewState, const FClimbingDetectionResult& DetectionResult);

	/** Handles state entry logic. */
	void OnStateEnter(EClimbingState NewState, const FClimbingDetectionResult& DetectionResult);

	/** Handles state exit logic. */
	void OnStateExit(EClimbingState OldState);

	/** Handles state-specific tick logic. */
	void TickClimbingState(float DeltaTime);

	/** Per-state tick functions. */
	void TickHangingState(float DeltaTime);
	void TickShimmyingState(float DeltaTime);
	void TickBracedWallState(float DeltaTime);
	void TickBracedShimmyingState(float DeltaTime);
	void TickLadderState(float DeltaTime);
	void TickLacheInAirState(float DeltaTime);

	/** Updates IK targets and weights. */
	void UpdateClimbingIK(float DeltaTime);

	// ========================================================================
	// Input Mapping Context Management
	// ========================================================================

	/** Adds the climbing IMC to the local player subsystem. */
	void AddClimbingInputMappingContext();

	/** Removes the climbing IMC from the local player subsystem. */
	void RemoveClimbingInputMappingContext();

	// ========================================================================
	// Detection Helpers
	// ========================================================================

	/** Checks if a surface tier allows climbing in the current context. */
	bool IsSurfaceClimbable(EClimbSurfaceTier Tier) const;

	/** Validates one-way approach direction. */
	bool ValidateOneWayApproach(const FVector& SurfaceNormal, const FVector& ApproachDirection, float Tolerance) const;

	/** Gets surface data from a hit component (via tag or reference). */
	const UClimbingSurfaceData* GetSurfaceDataFromComponent(UPrimitiveComponent* Component) const;

	// ========================================================================
	// IK Helpers
	// ========================================================================

	/** Updates IK targets for the current climbing state. */
	void UpdateIKTargets();

	/** Registers this character with the IK manager. */
	void RegisterWithIKManager();

	/** Unregisters this character from the IK manager. */
	void UnregisterFromIKManager();

	/** Sorts the IK manager array by distance to local camera. */
	static void SortIKManagerByDistance();

	/** Checks if this character is within the IK budget. */
	bool IsWithinIKBudget() const;

	/** Updates IK for ledge hang state (hands only). */
	void UpdateLedgeHangIK(float DeltaTime, class UClimbingAnimInstance* AnimInst);

	/** Updates IK for braced wall state (all four limbs). */
	void UpdateBracedWallIK(float DeltaTime, class UClimbingAnimInstance* AnimInst);

	/** Updates IK for ladder state (all four limbs with rung snapping). */
	void UpdateLadderIK(float DeltaTime, class UClimbingAnimInstance* AnimInst);

	/** Updates IK for corner transition (FABRIK blend). */
	void UpdateCornerIK(float DeltaTime, class UClimbingAnimInstance* AnimInst);

	// ========================================================================
	// Audio Helpers
	// ========================================================================

	/** Plays a climbing sound at the character's location. */
	void PlayClimbingSound(EClimbSoundType SoundType);

	/** Loads and caches a sound asset if not already loaded. */
	USoundBase* GetResolvedSound(EClimbSoundType SoundType);

	// ========================================================================
	// Cleanup Helpers
	// ========================================================================

	/** Common cleanup logic for EndPlay and Destroyed. */
	void PerformCleanup();

	// ========================================================================
	// Validation
	// ========================================================================

	/** Validates all animation slots are assigned. Called in BeginPlay. */
	void ValidateAnimationSlots();

	// ========================================================================
	// Internal State
	// ========================================================================

	/** Cached animation instance reference. */
	UPROPERTY()
	TWeakObjectPtr<UClimbingAnimInstance> CachedAnimInstance;

	/** Current detection result (local only, not replicated). */
	FClimbingDetectionResult CurrentDetectionResult;

	/** Current surface data (from detected surface). */
	UPROPERTY()
	TWeakObjectPtr<const UClimbingSurfaceData> CurrentSurfaceData;

	/** Current animation set override (from surface data). */
	UPROPERTY()
	TObjectPtr<UClimbingAnimationSet> CurrentAnimationSetOverride;

	/** Committed shimmy direction with hysteresis. */
	float CommittedShimmyDir = 0.0f;

	/** Current climb move input value. */
	FVector2D CurrentClimbMoveInput = FVector2D::ZeroVector;

	/** Whether sprint modifier is active. */
	bool bSprintModifierActive = false;

	/** Whether crouch modifier is active. */
	bool bCrouchModifierActive = false;

	/** Continuous shimmy distance since last stop. */
	float ContinuousShimmyDistance = 0.0f;

	/** Last played idle variation index. */
	int32 LastIdleVariationIndex = -1;

	/** Time since last idle for variation selection. */
	float IdleTimer = 0.0f;

	/** Locked Lache target (during Lache flight). */
	FClimbingDetectionResult LockedLacheTarget;

	/** Pre-prediction position for rollback. */
	FVector PrePredictionPosition = FVector::ZeroVector;

	/** Lache launch position (origin of arc). */
	FVector LacheLaunchPosition = FVector::ZeroVector;

	/** Lache launch direction (forward at launch). */
	FVector LacheLaunchDirection = FVector::ZeroVector;

	/** Time in flight during Lache. */
	float LacheFlightTime = 0.0f;

	/** Whether camera is currently locked. */
	bool bCameraLocked = false;

	/** Original capsule half-height before climbing. */
	float OriginalCapsuleHalfHeight = 0.0f;

	/** Original capsule radius before climbing. */
	float OriginalCapsuleRadius = 0.0f;

	/** Original collision profile before climbing. */
	FName OriginalCollisionProfile;

	/** Resolved sound cache. */
	UPROPERTY()
	TMap<EClimbSoundType, TObjectPtr<USoundBase>> ResolvedSounds;

	/** Timer handle for detection scanning. */
	FTimerHandle DetectionTimerHandle;

	/** Timer handle for falling grab checks. */
	FTimerHandle FallingGrabTimerHandle;

	/** Timer handle for ragdoll recovery. */
	FTimerHandle RagdollRecoveryTimerHandle;

	/** Timer handle for idle variations. */
	FTimerHandle IdleVariationTimerHandle;

	/** Coyote time remaining. */
	float CoyoteTimeRemaining = 0.0f;

	/** Inside corner flag for current corner transition. */
	bool bCurrentCornerIsInside = false;

	/** Simulated proxy IK update accumulator. */
	float SimulatedProxyIKAccumulator = 0.0f;

	// ========================================================================
	// Additional Helper Functions
	// ========================================================================

	/** Plays a random idle variation montage. */
	void PlayIdleVariation();

	/** Recovers from ragdoll state. */
	void RecoverFromRagdoll();

	/** Performs corner detection in shimmy direction. */
	FClimbingDetectionResult PerformCornerDetection(float ShimmyDirection) const;

	/** Checks for lip/ledge above current braced position. */
	bool CheckForLipAbove(FClimbingDetectionResult& OutLedgeResult) const;

	/** Calculates Lache arc and finds valid target. */
	FClimbingDetectionResult CalculateLacheArc() const;

	/** Performs ledge detection centered at a specific location (for Lache targeting). */
	FClimbingDetectionResult PerformLedgeDetectionAtLocation(const FVector& Location) const;

	// ========================================================================
	// Static IK Manager
	// ========================================================================

	/**
	 * Static array of active climbing characters for IK budget management.
	 * Game thread access only. Sorted by distance to local camera on state change.
	 */
	static TArray<TWeakObjectPtr<AClimbingCharacter>> ActiveClimbingCharacters;
};
