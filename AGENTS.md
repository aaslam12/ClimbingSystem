# 🧗 UE5 Cinematic Climbing System — Master Prompt v7

```
System:       UE5 Cinematic Climbing System
Version:      7.0
Engine Target: UE5.3+
Multiplayer:  Listen-server, server-authoritative
GAS:          No
Audio:        UGameplayStatics::SpawnSoundAtLocation
```

> You are an expert Unreal Engine 5 C++ developer. Your task is to architect and implement a complete, production-quality climbing system for a **listen-server multiplayer game**. Follow every specification below exactly. Do not skip sections. Do not summarize — provide full implementation code.
>
> **If you are approaching your context limit at any point, stop immediately and output only the text `CONTINUE IN NEXT MESSAGE` before truncating anything. Each class must be fully complete before moving to the next. Never summarize, abbreviate, or truncate a class body.**
>
> **Before beginning each new milestone, output a single line in this format: `[MILESTONE N: <title> — modifying <files>]`. This confirms context is maintained across message breaks.**

---

## 💾 Version Control — Local Commits

After completing each milestone, create a local git commit before continuing. **Do not push. Do not create a remote branch. Local commits only.**

Milestones in order:

1. Log category definition; all struct and enum definitions (`FClimbingDetectionResult`, `FClimbingDetectionResultNet`, `EClimbingState`, `EClimbSurfaceTier`, `EClimbClearanceType`, `EClimbSoundType`, `EClimbingAnimationSlot`, `FClimbingStateConfig`)
2. `UClimbingSurfaceData` and `UClimbingAnimationSet` data asset classes (`.h` and `.cpp`)
3. `UClimbingAnimInstance` (`.h` and `.cpp`)
4. `UClimbingMovementComponent` (`.h` and `.cpp`)
5. `AClimbingCharacter` header (`.h` only — all UPROPERTY declarations, no function bodies)
6. Detection system implementation (detection functions in `AClimbingCharacter.cpp`)
7a. State machine core — transition validation and `FClimbingDetectionResult` consumption
7b. State machine actions — shimmy (deadzone + hysteresis), corner, and braced wall logic
7c. State machine actions — Lache, ladder, mantle, and freefall re-grab logic
8. IK computation and the IK static manager
9. Camera system, cinematic lock, and ragdoll camera handling
10. Physics interaction — dynamic anchors, `SetBase` (hang and braced), grab breaking, ragdoll recovery
11. Audio system — soft pointer loading, caching, failure path, and `UAnimNotify_ClimbSoundCue`
12. `UAnimNotify_EnableClimbIK` and `UAnimNotify_DisableClimbIK`
13. Multiplayer — all `Server_` and `Client_` RPCs, replication setup, confirmation trace, simulated proxy IK
14. Input Mapping Context management (climbing context push/pop on state entry/exit)
15. Debug visualization system (runtime + `#if WITH_EDITOR` Lache arc preview with `IsSelected()` gate)
16. `BeginPlay` slot validation and `EndPlay` / `Destroyed` cleanup (IK manager, `SetBase`, Lache target, physics)

Each commit message must follow this format exactly:

```
[ClimbingSystem] <milestone title>

- <specific thing implemented>
- <specific thing implemented>
- <specific thing implemented>

Refs: UE5.3+, listen-server, no GAS
```

Do not batch milestones. Do not commit partial implementations. Each commit must be a fully compilable state.

---

## 📁 Architecture

- `AClimbingCharacter` — inherits from UE5 Third Person template `ACharacter` subclass.
- `UClimbingAnimInstance : UAnimInstance` — Animation Blueprint parent class.
- `UClimbingMovementComponent : UCharacterMovementComponent` — bound to `AClimbingCharacter`.
- `UClimbingAnimationSet : UPrimaryDataAsset` — per-surface animation override set.
- `UClimbingSurfaceData : UPrimaryDataAsset` — per-surface metadata.
- All classes fully modular. No gameplay logic in Blueprint — Blueprints are data containers only.

---

## 📋 Log Category

Define in `ClimbingCharacter.h`:
```cpp
DECLARE_LOG_CATEGORY_EXTERN(LogClimbing, Log, All);
```
Define in `ClimbingCharacter.cpp`:
```cpp
DEFINE_LOG_CATEGORY(LogClimbing);
```
Use `LogClimbing` for all climbing system log output. Never use `LogTemp` for climbing messages.

---

## 🗂️ Core Structs & Enums

### Replication Split

`TWeakObjectPtr` is not natively replicable in UE5. The detection result is therefore split into two structs with distinct roles:

**`FClimbingDetectionResult`** — local-only. Used for detection logic, IK, and state transitions on the local machine. Never placed in a replicated UPROPERTY.

```cpp
USTRUCT(BlueprintType)
struct FClimbingDetectionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FVector LedgePosition = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector SurfaceNormal = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    EClimbSurfaceTier SurfaceTier = EClimbSurfaceTier::Untagged;

    UPROPERTY(BlueprintReadOnly)
    EClimbClearanceType ClearanceType = EClimbClearanceType::None;

    /** Local only — NOT replicated. TWeakObjectPtr is not replicable in UE5.
     *  Simulated proxies resolve HitComponent locally via confirmation trace. */
    TWeakObjectPtr<UPrimitiveComponent> HitComponent;

    UPROPERTY(BlueprintReadOnly)
    bool bValid = false;
};
```

**`FClimbingDetectionResultNet`** — replication-safe. Contains no pointer types. Used as the payload for `Server_AttemptGrab` and as `LastValidatedDetectionResult` (replicated UPROPERTY). Intentionally `USTRUCT()` without `BlueprintType` — it is a network serialization struct, not a designer-facing type. Do not add `BlueprintType` to it.

Do **not** implement a custom `NetSerialize` on `FClimbingDetectionResultNet`. The struct only replicates on state transitions (not per-tick), so default field-by-field serialization is acceptable and a custom serializer adds complexity for negligible bandwidth gain.

```cpp
/** Network-safe detection result subset. No pointer types — safe to replicate.
 *  HitComponent is excluded; each client resolves it locally via confirmation trace
 *  from LedgePosition after receiving this struct. See ResolveHitComponentFromNet(). */
USTRUCT()
struct FClimbingDetectionResultNet
{
    GENERATED_BODY()

    UPROPERTY()
    FVector_NetQuantize LedgePosition = FVector::ZeroVector;  // 1cm precision, sufficient for ledge positions

    UPROPERTY()
    FVector_NetQuantizeNormal SurfaceNormal = FVector::ZeroVector;

    UPROPERTY()
    EClimbSurfaceTier SurfaceTier = EClimbSurfaceTier::Untagged;

    UPROPERTY()
    EClimbClearanceType ClearanceType = EClimbClearanceType::None;

    UPROPERTY()
    bool bValid = false;
};
```

### Confirmation Trace — `ResolveHitComponentFromNet`

After receiving a replicated `FClimbingDetectionResultNet`, clients and simulated proxies must resolve `HitComponent` locally. Implement this as a member function on `AClimbingCharacter`:

```cpp
/** Resolves a local UPrimitiveComponent* from a replicated FClimbingDetectionResultNet
 *  by casting a short sphere trace from LedgePosition inward along the negated SurfaceNormal.
 *  Trace shape: sphere, radius = ConfirmationTraceRadius, channel = ECC_WorldStatic + ECC_WorldDynamic.
 *  If the trace finds no component: returns null and logs LogClimbing Warning once per miss.
 *    On the owning client during active climbing: the confirmation trace is skipped entirely —
 *    the local detection scan result is used directly (HitComponent is already known).
 *    On simulated proxies: confirmation trace runs immediately in OnRep_ClimbingState,
 *    not deferred to the next IK tick.
 *  If the confirmation trace fails on a simulated proxy: HitComponent is left null,
 *    IK is disabled for that frame (IK weights set to 0), and the trace retries next IK update. */
UPrimitiveComponent* ResolveHitComponentFromNet(const FClimbingDetectionResultNet& NetResult) const;
```

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Detection",
    meta = (ToolTip = "Sphere radius (cm) used by the confirmation trace that resolves HitComponent from a replicated ledge position. Should be slightly larger than the expected surface contact area. 16cm default."))
float ConfirmationTraceRadius = 16.0f;
```

### Remaining Enums

```cpp
UENUM(BlueprintType)
enum class EClimbSurfaceTier : uint8
{
    Unclimbable, Climbable, ClimbableOneWay, LadderOnly, Untagged
};

UENUM(BlueprintType)
enum class EClimbClearanceType : uint8
{
    Full, CrouchOnly, None
};

UENUM(BlueprintType)
enum class EClimbSoundType : uint8
{
    HandGrab, FootPlant, LadderRungHand, LadderRungFoot,
    MantleImpact, LacheLaunchGrunt, LacheCatchImpact, GrabFail
};

UENUM(BlueprintType)
enum class EClimbingAnimationSlot : uint8
{
    HangIdle, HangIdleLeft, HangIdleRight,
    ShimmyLeft, ShimmyRight,
    CornerInsideLeft, CornerInsideRight, CornerOutsideLeft, CornerOutsideRight,
    ClimbUp, ClimbUpCrouch,
    DropDown, GrabLedge, GrabFail, ShimmyReposition,
    LacheLaunch, LacheFlight, LacheCatch, LacheMiss,
    MantleLow, MantleHigh,
    RagdollGetUpFaceDown, RagdollGetUpFaceUp,
    BracedIdle, BracedShimmyLeft, BracedShimmyRight, BracedToHang,
    LadderIdle, LadderClimbUp, LadderClimbDown,
    LadderFastAscend, LadderFastDescend,
    LadderEnterBottom, LadderEnterTop,
    LadderExitBottom, LadderExitTop, LadderExitSide
};

UENUM(BlueprintType)
enum class EClimbingState : uint8
{
    None, Hanging, Shimmying, ClimbingUp, ClimbingUpCrouch,
    DroppingDown, CornerTransition, BracedWall, BracedShimmying,
    Mantling, Lache, LacheInAir, LacheCatch, LacheMiss,
    OnLadder, LadderTransition, Ragdoll
};
```

### `FClimbingStateConfig` — Explicit Defaults

```cpp
USTRUCT(BlueprintType)
struct FClimbingStateConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (ToolTip = "If true, this state can be interrupted by player input at any time. Default true — the safe fallback."))
    bool bInterruptible = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (ToolTip = "Minimum montage completion (0.0–1.0) before this state can be cancelled. Only evaluated when bInterruptible is false. Default 0.0 = cancel immediately."))
    float MinCompletionBeforeCancel = 0.0f;
};
```

`TMap<EClimbingState, FClimbingStateConfig> StateConfigs` must be initialized in the constructor with an entry for every value in `EClimbingState`. No state may be absent — a missing key produces an uninitialized `bInterruptible` (undefined behavior).

---

## 🗺️ State-to-Slot Mapping

`GetMontageForSlot(EClimbingAnimationSlot)` is the canonical lookup. Implement this mapping exactly:

| State | Context | Slot |
|---|---|---|
| `Hanging` | Idle | `HangIdle` |
| `Hanging` | Leaning left | `HangIdleLeft` |
| `Hanging` | Leaning right | `HangIdleRight` |
| `Hanging` | Grab initiation | `GrabLedge` |
| `Shimmying` | `CommittedShimmyDir < 0` | `ShimmyLeft` |
| `Shimmying` | `CommittedShimmyDir > 0` | `ShimmyRight` |
| `Shimmying` | Speed below `ShimmySpeedDeadzone` | Montage rate = 0; no slot change |
| `Shimmying` | Distance cap reached | `ShimmyReposition` |
| `BracedWall` | Idle | `BracedIdle` |
| `BracedWall` | Lip detected above | `BracedToHang` |
| `BracedShimmying` | `CommittedShimmyDir < 0` | `BracedShimmyLeft` |
| `BracedShimmying` | `CommittedShimmyDir > 0` | `BracedShimmyRight` |
| `ClimbingUp` | `ClearanceType == Full` | `ClimbUp` |
| `ClimbingUpCrouch` | `ClearanceType == CrouchOnly` | `ClimbUpCrouch` |
| `DroppingDown` | `PreviousState` is hang or braced | `DropDown` |
| `DroppingDown` | `PreviousState == OnLadder` | `LadderExitSide` |
| `CornerTransition` | Inside corner, `CommittedShimmyDir < 0` | `CornerInsideLeft` |
| `CornerTransition` | Inside corner, `CommittedShimmyDir > 0` | `CornerInsideRight` |
| `CornerTransition` | Outside corner, `CommittedShimmyDir < 0` | `CornerOutsideLeft` |
| `CornerTransition` | Outside corner, `CommittedShimmyDir > 0` | `CornerOutsideRight` |
| `Mantling` | `h <= MantleLowMaxHeight` | `MantleLow` |
| `Mantling` | `h > MantleLowMaxHeight` | `MantleHigh` |
| `Lache` | Launch frame | `LacheLaunch` |
| `LacheInAir` | In flight | `LacheFlight` |
| `LacheCatch` | Catching ledge | `LacheCatch` |
| `LacheMiss` | Miss and fall | `LacheMiss` |
| `OnLadder` | Idle | `LadderIdle` |
| `OnLadder` | Up, normal speed | `LadderClimbUp` |
| `OnLadder` | Down, normal speed | `LadderClimbDown` |
| `OnLadder` | Up + `IA_Sprint` | `LadderFastAscend` |
| `OnLadder` | Down + `IA_Crouch` | `LadderFastDescend` |
| `LadderTransition` | Entering bottom | `LadderEnterBottom` |
| `LadderTransition` | Entering top | `LadderEnterTop` |
| `LadderTransition` | Exiting bottom | `LadderExitBottom` |
| `LadderTransition` | Exiting top | `LadderExitTop` |
| `Ragdoll` | Pelvis up dot world up > 0 | `RagdollGetUpFaceUp` |
| `Ragdoll` | Pelvis up dot world up <= 0 | `RagdollGetUpFaceDown` |
| `None` | Grab rejected by server | `GrabFail` |

`CommittedShimmyDir` is the hysteresis-gated shimmy direction value described in the shimmy section. The mapping table references it explicitly so the model cannot apply raw axis input directly.

**Corner inside/outside classification**: compute before entering corner state.
```cpp
float Dot = FVector::DotProduct(CurrentSurfaceNormal, NewSurfaceNormal);
bool bInsideCorner = Dot > 0.0f;  // new wall curves toward character's current facing
// Outside corner: Dot <= 0 (wall curves away)
```

**Ragdoll get-up face detection**: do not use pelvis bone Z rotation — this is ambiguous on skeletons where the pelvis reference pose has a non-trivial rotation offset. Use the pelvis bone's local up vector projected onto world space:
```cpp
FQuat PelvisQuat = GetMesh()->GetBoneQuaternion(FName("pelvis"), EBoneSpaces::WorldSpace);
FVector PelvisUp = PelvisQuat.GetUpVector();
bool bFaceUp = FVector::DotProduct(PelvisUp, FVector::UpVector) > 0.0f;
EClimbingAnimationSlot GetUpSlot = bFaceUp ? EClimbingAnimationSlot::RagdollGetUpFaceUp
                                           : EClimbingAnimationSlot::RagdollGetUpFaceDown;
```
Expose the pelvis bone name as a Blueprint-configurable `FName PelvisBoneName = FName("pelvis")` so projects with non-standard skeleton hierarchies can override it.

---

## 🎞 Animation Set Override — `UClimbingAnimationSet`

`UClimbingAnimationSet : UPrimaryDataAsset` mirrors every montage slot on `AClimbingCharacter` using identical field names and UPROPERTY categories. Substitution is per-slot: a null field in the override asset falls back to the character default for that slot only.

```cpp
/** Returns the active montage for the given slot.
 *  Priority: current surface AnimationSetOverride → character defaults.
 *  Per-slot fallback: null fields in the override fall back to character defaults individually.
 *  Logs UE_LOG(LogClimbing, Warning) once per missing slot at BeginPlay validation.
 *  Never returns null if BeginPlay validation passed without warnings. */
UAnimMontage* GetMontageForSlot(EClimbingAnimationSlot Slot) const;
```

---

## 🎮 Input System

### Input Mapping Contexts

The climbing system uses a **dedicated `UInputMappingContext`** that is pushed and popped on the player's `UEnhancedInputLocalPlayerSubsystem` as the character enters and exits climbing states. This prevents conflicts with the base locomotion mapping context (e.g., `IA_Sprint` triggering both fast ladder ascent and locomotion sprint simultaneously).

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
    meta = (ToolTip = "Input Mapping Context containing all climbing-specific actions (IA_Grab, IA_Drop, IA_Lache, IA_ClimbUp, IA_ClimbMove). Added to the subsystem at higher priority than the locomotion context on climbing entry, removed on climbing exit. Must be assigned — without it no climbing inputs will fire."))
TObjectPtr<UInputMappingContext> ClimbingInputMappingContext;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
    meta = (ToolTip = "Priority of the climbing Input Mapping Context relative to locomotion. Higher number = higher priority. Must be greater than the locomotion IMC priority to prevent locomotion inputs from shadowing climbing inputs. 1 recommended if locomotion IMC uses priority 0."))
int32 ClimbingIMCPriority = 1;
```

Call `AddMappingContext(ClimbingInputMappingContext, ClimbingIMCPriority)` on the `UEnhancedInputLocalPlayerSubsystem` when entering any climbing state from `None`. Call `RemoveMappingContext(ClimbingInputMappingContext)` when returning to `None` from any climbing state. Guard both calls with a null check on the subsystem.

`IA_Sprint` and `IA_Crouch` are bound in the **locomotion** mapping context for their standard locomotion behavior and in the **climbing** mapping context for their ladder modifier behavior. The priority system ensures the climbing binding takes precedence while climbing.

### Input Action Properties

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
    meta = (ToolTip = "Grab / initiate climb. ETriggerEvent::Triggered (pressed)."))
TObjectPtr<UInputAction> IA_Grab;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
    meta = (ToolTip = "Intentional drop from hang or ladder. ETriggerEvent::Triggered (pressed)."))
TObjectPtr<UInputAction> IA_Drop;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
    meta = (ToolTip = "Launch a Lache jump while hanging. ETriggerEvent::Triggered (pressed)."))
TObjectPtr<UInputAction> IA_Lache;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
    meta = (ToolTip = "Pull up from hang onto surface above. ETriggerEvent::Triggered (pressed)."))
TObjectPtr<UInputAction> IA_ClimbUp;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
    meta = (ToolTip = "2D axis: X = shimmy left/right, Y = ladder up/down. ETriggerEvent::Ongoing (held). Shimmy only fires while held."))
TObjectPtr<UInputAction> IA_ClimbMove;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
    meta = (ToolTip = "Fast ladder ascent modifier. ETriggerEvent::Ongoing (held)."))
TObjectPtr<UInputAction> IA_Sprint;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Input",
    meta = (ToolTip = "Fast ladder descent modifier. ETriggerEvent::Ongoing (held)."))
TObjectPtr<UInputAction> IA_Crouch;
```

All `Triggered` for tap inputs; `Ongoing` for held inputs. Bound in `SetupPlayerInputComponent`. Zero legacy `BindAxis` / `BindAction` calls anywhere.

---

## 🔍 Ledge & Surface Detection

### Detection Scan Frequency

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Detection",
    meta = (ToolTip = "Seconds between detection scans during ground locomotion. 0.05 = 20Hz recommended. Never 0 — per-tick scanning is reserved for active climbing states."))
float DetectionScanInterval = 0.05f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Detection",
    meta = (ToolTip = "Seconds between scans while airborne. Drives coyote time re-check rate. 0.05 recommended."))
float FallingGrabCheckInterval = 0.05f;
```

Schedule:
- **Ground locomotion**: `FTimerManager` timer at `DetectionScanInterval`, `bIgnorePause = false`. Document with comment at `SetTimer` call.
- **`MOVE_Falling`**: timer at `FallingGrabCheckInterval`, `bIgnorePause = false`. Begins immediately on `MOVE_Falling` entry.
- **Active climbing** (`Hanging`, `Shimmying`, `BracedWall`, `OnLadder`): every tick.
- **Committed states** (`CornerTransition`, `Mantling`, `LacheCatch`, `LadderTransition`, `Ragdoll`): no scan.

**Pause behavior**: all climbing timers use `bIgnorePause = false` (the `FTimerManager` default). The climbing system fully pauses with the game. No timer uses `bIgnorePause = true`. Document this with a comment at each `SetTimer` call.

### Level Streaming Constraint

Anchors restricted to persistent level actors only. In `Server_AttemptGrab`, after geometry validation, verify `HitComponent->GetOwner()` is not in a streaming sublevel. If it is, reject and log:
```cpp
UE_LOG(LogClimbing, Warning, TEXT("ClimbingSystem: Grab rejected — '%s' is in a streaming sublevel. Climbable surfaces must be in the persistent level."), *HitComponent->GetOwner()->GetName());
```

### Collision Tag System

| Tag | Behavior |
|---|---|
| `Unclimbable` | Always excluded |
| `Climbable` | Always included, bypasses geometric validation |
| `ClimbableOneWay` | Approach vector validated |
| `LadderOnly` | Ladder state only |
| *(untagged)* | Programmatic rules |

### Surface Metadata — `UClimbingSurfaceData`

All fields Blueprint-exposed with tooltips. `AnimationSetOverride` loaded async on first surface contact.

---

## 🎮 Climbing Actions

### 1. Ledge Hang
`HangIdle` on entry. After `IdleVariationDelay` seconds, select randomly from `HangIdleVariations`. `bPreventConsecutiveVariationRepeat` excludes the last-played index from the pool.

### 2. Shimmy

**Speed formula**:
```
EffectiveShimmySpeed = BaseShimmySpeed * ClimbSpeedMultiplier * OverhangPenalty

OverhangAngleDeg = angle(SurfaceNormal, WorldUp) - 90.0f
if (OverhangAngleDeg <= OverhangPenaltyStartAngle)
    OverhangPenalty = 1.0f
else
    OverhangPenalty = Lerp(1.0f, OverhangMaxPenaltyScalar,
        Clamp((OverhangAngleDeg - OverhangPenaltyStartAngle) / OverhangPenaltyRangeAngle, 0, 1))
```

**Direction hysteresis — `CommittedShimmyDir`**:

Analog sticks produce noisy values near zero. Applying raw `IA_ClimbMove.X` directly to direction selection causes rapid `ShimmyLeft` ↔ `ShimmyRight` flipping at rest, producing visible animation jitter. Use a committed direction variable with a deadzone gate:

```cpp
/** Last committed shimmy direction. -1 = left, +1 = right, 0 = uncommitted (entry state only).
 *  Updated only when |IA_ClimbMove.X| > ShimmyDirectionDeadzone.
 *  Used by GetMontageForSlot and the state-to-slot table instead of raw axis input. */
float CommittedShimmyDir = 0.0f;
```

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
    meta = (ToolTip = "Minimum absolute IA_ClimbMove.X value required to update the committed shimmy direction. Prevents rapid Left/Right montage flipping from analog stick noise near center. Raw axis values below this threshold maintain the last committed direction. 0.1 recommended."))
float ShimmyDirectionDeadzone = 0.1f;
```

Direction update logic (called each tick during `Shimmying` and `BracedShimmying`):
```cpp
float RawX = IA_ClimbMove.X;  // current frame raw axis value
if (FMath::Abs(RawX) > ShimmyDirectionDeadzone)
{
    CommittedShimmyDir = FMath::Sign(RawX);
}
// else: CommittedShimmyDir unchanged — last direction maintained
```

**Shimmy deadzone — stationary handling**:
```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
    meta = (ToolTip = "If EffectiveShimmySpeed drops below this threshold, the shimmy montage is paused (PlaybackRate = 0). Prevents the animation playing slowly while stationary, e.g. during ShimmyReposition or at the MaxContinuousShimmyDistance cap. Set 0 to disable."))
float ShimmySpeedDeadzone = 10.0f;
```

When `EffectiveShimmySpeed < ShimmySpeedDeadzone`: set montage PlaybackRate to 0. When `>= ShimmySpeedDeadzone`: PlaybackRate = `Lerp(ShimmyPlaybackRateMin, ShimmyPlaybackRateMax, NormalizedSpeed)`.

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
    meta = (ToolTip = "Minimum shimmy montage playback rate at lowest effective speed. Values below 0.3 produce visible foot-sliding artifacts."))
float ShimmyPlaybackRateMin = 0.4f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Movement",
    meta = (ToolTip = "Maximum shimmy montage playback rate at full effective speed. Values above 2.0 cause hands to lose wall contact in the animation."))
float ShimmyPlaybackRateMax = 1.2f;
```

**Continuous distance cap**: shimmy beyond `MaxContinuousShimmyDistance` without stopping → play `ShimmyReposition` (montage rate = 0 during this) → resume.

**Owning client optimization**: during active climbing states, the owning client uses its local detection scan result directly to update `HitComponent` — the confirmation trace (`ResolveHitComponentFromNet`) is skipped. The confirmation trace runs only on simulated proxies that receive replicated data without running local detection scans.

### 3. Corner Transitions
Predictive trace in shimmy direction. Angle threshold default 30°; below = shimmy rejected; at or above = corner transition. FABRIK for all four limbs. Corner inside/outside classification uses dot product formula from the mapping table section. `CommittedShimmyDir` drives slot selection.

### 4. Climb Up
`ClimbUp` when `ClearanceType == Full`; `ClimbUpCrouch` when `ClearanceType == CrouchOnly`.

### 5. Drop Down
`IA_Drop`. Slot per mapping table using `PreviousClimbingState`.

### 6. Lache

**Arc parameterization — gravity sign convention**: `UCharacterMovementComponent::GetGravityZ()` returns a **negative float** in standard UE5 gravity (e.g. `-980.0f`). Use it directly without `FMath::Abs`. The negative sign is intentional — it produces the correct downward arc displacement when multiplied against `FVector::UpVector`:

```
// Correct — arc curves downward:
position.Z += 0.5f * GetCharacterMovement()->GetGravityZ() * t * t;
// GetGravityZ() = -980  →  term is negative  →  Z decreases over time  ✓

// WRONG — do not negate or take Abs:
position.Z += 0.5f * FMath::Abs(GetGravityZ()) * t * t;  // arc goes UP — broken
position.Z -= 0.5f * GetGravityZ() * t * t;               // same mistake, different form
```

Full arc step formula:
```cpp
FVector ArcVelocity = GetActorForwardVector() * LacheLaunchSpeed;
float GravityZ = GetCharacterMovement()->GetGravityZ();  // negative value, use directly

for (int32 i = 0; i <= LacheArcTraceSteps; ++i)
{
    float t = i * (LacheTotalArcTime / LacheArcTraceSteps);
    FVector StepPos = LaunchOrigin
        + ArcVelocity * t
        + FVector(0.f, 0.f, 0.5f * GravityZ * t * t);  // GravityZ is negative — correct

    // Sphere trace at StepPos with LacheArcTraceRadius
    // Intermediate steps: check for non-climbable blockers → reject launch if hit
    // Final step: check for FClimbingDetectionResult → commit launch if valid
}
```

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Lache",
    meta = (ToolTip = "Launch speed (cm/s) scaling the character's look direction. Higher = flatter arc."))
float LacheLaunchSpeed = 1200.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Lache",
    meta = (ToolTip = "Total simulated arc duration (seconds). Increase for longer jumps."))
float LacheTotalArcTime = 1.2f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Lache",
    meta = (ToolTip = "Number of arc subdivisions. Higher = more accurate mid-flight obstacle detection. Each step fires one sphere trace. 12 recommended."))
int32 LacheArcTraceSteps = 12;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Lache",
    meta = (ToolTip = "Sphere radius at each arc step (cm). Should approximate shoulder width. Used for both obstacle detection and landing ledge detection."))
float LacheArcTraceRadius = 24.0f;
```

Mid-flight obstacle detection at every intermediate step. Target locked on launch. Mid-flight invalidation → `LacheMiss`. `bAutoLacheCinematic` + `LacheCinematicDistanceThreshold` auto-call `LockCameraToFrame`.

`LockCameraToFrame` affects only the local player's camera. Spectator and observer systems are out of scope.

### 7. Ladder Climbing
`LadderOnly` tag or geometric profile. `IA_ClimbMove` Y-axis. `IA_Sprint` fast ascent; `IA_Crouch` fast descent. Procedural rung IK via inward traces along ladder local X axis; `LadderRungSpacing` drives vertical snap grid.

### 8. Braced Wall

`SetBase(AnchorComponent.Get())` called on entry to both `BracedWall` and `BracedShimmying`. `SetBase(nullptr)` on all exits. Without `SetBase` during braced states, a character braced against a moving wall drifts away from the surface. Shimmy uses `CommittedShimmyDir` with the same hysteresis logic as ledge shimmy.

**Downward path from Hanging**: past valid lip → check braced geometry → `BracedWall`. If not valid → `DroppingDown`. Grab never silently dropped.

### 9. Mantle

| Height | Behavior |
|---|---|
| `<= MantleStepMaxHeight` | CMC step-up, no state entered |
| `MantleStepMaxHeight < h <= MantleLowMaxHeight` | `Mantling` + `MantleLow` + `WarpTarget_MantleLow` |
| `MantleLowMaxHeight < h <= MantleHighMaxHeight` | `Mantling` + `MantleHigh` + `WarpTarget_MantleHigh` |
| `> MantleHighMaxHeight` | Not mantleable |

### 10. Freefall Re-Grab

**Coyote time**: `CoyoteTimeWindow` seconds post-leave, `IA_Grab` re-triggers detection. `bEnableCoyoteTime` toggle.

**Falling grab**: `MOVE_Falling` + `bEnableFallingGrab` + `IA_Grab` + valid ledge within `FallingGrabReachDistance` → grab + `GrabLedge`.

---

## 🔧 Motion Warping

### Named Warp Targets

| Warp Target Name | Used In | Warp Type |
|---|---|---|
| `WarpTarget_LedgeGrab` | `GrabLedge` | Translation + Rotation |
| `WarpTarget_ClimbUpLand` | `ClimbUp`, `ClimbUpCrouch` | Translation + Rotation |
| `WarpTarget_MantleLow` | `MantleLow` | Translation + Rotation |
| `WarpTarget_MantleHigh` | `MantleHigh` | Translation + Rotation |
| `WarpTarget_LadderEnterBottom` | `LadderEnterBottom` | Translation + Rotation |
| `WarpTarget_LadderEnterTop` | `LadderEnterTop` | Translation + Rotation |
| `WarpTarget_LadderExitBottom` | `LadderExitBottom` | Translation + Rotation |
| `WarpTarget_LadderExitTop` | `LadderExitTop` | Translation + Rotation |
| `WarpTarget_LacheCatch` | `LacheCatch` | Translation + Rotation |
| `WarpTarget_CornerPivot` | All four corner animations | Rotation only |

Warp windows are curve-driven via a `MotionWarp` float curve in each animation. No fixed-time windows. Document in a comment in `BeginPlay` where `UMotionWarpingComponent` is validated.

### Root Motion vs In-Place

| Animation Group | Motion Model |
|---|---|
| `GrabLedge`, `ClimbUp`, `ClimbUpCrouch` | Root motion + Motion Warping |
| `ShimmyLeft`, `ShimmyRight` | In-place; movement component drives lateral |
| `BracedShimmyLeft`, `BracedShimmyRight` | In-place |
| `LacheLaunch`, `LacheFlight`, `LacheCatch`, `LacheMiss` | Root motion |
| `MantleLow`, `MantleHigh` | Root motion + Motion Warping |
| All ladder climb/transition animations | In-place; movement component drives vertical |
| All corner transitions | Root motion; rotation via `WarpTarget_CornerPivot` |
| `DropDown`, `LadderExitSide` | Root motion |
| All idle animations | In-place |

`UClimbingMovementComponent` overrides `ShouldUsePackedMovementRPCs` and `ApplyRootMotionToVelocity` to prevent base class interference during in-place states.

---

## 🌐 Multiplayer — Listen-Server Architecture

Replication contract comment block at the top of `UClimbingMovementComponent.h`:

```cpp
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
```

### Replicated State

```cpp
UPROPERTY(ReplicatedUsing = OnRep_ClimbingState)
EClimbingState CurrentClimbingState;

UPROPERTY(Replicated)
FClimbingDetectionResultNet LastValidatedDetectionResult;

UPROPERTY(Replicated)
TObjectPtr<UPrimitiveComponent> AnchorComponent;  // TObjectPtr — replicable; TWeakObjectPtr is not

UPROPERTY(Replicated)
FTransform AnchorLocalTransform;
```

### Server RPCs

| Input | RPC |
|---|---|
| `IA_Grab` | `Server_AttemptGrab(FClimbingDetectionResultNet ClientResult)` |
| `IA_Drop` | `Server_Drop()` |
| `IA_Lache` | `Server_AttemptLache(FVector ClientArcTarget)` |
| `IA_ClimbUp` | `Server_AttemptClimbUp()` |
| `IA_ClimbMove` | `Server_UpdateShimmyDirection(FVector2D Direction)` |

### Client Prediction Rollback

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Multiplayer",
    meta = (ToolTip = "Seconds to blend out a rejected prediction and lerp to pre-prediction position. Shorter = more responsive but jarring. Longer = smoother but widens cheating correction window. 0.2 recommended."))
float PredictionRollbackBlendOut = 0.2f;
```

On `Client_RejectStateTransition`: `Montage_Stop(PredictionRollbackBlendOut)` → play `GrabFail` → lerp back to pre-prediction position over `PredictionRollbackBlendOut` → `EClimbingState::None`.

### Simulated Proxy Animation

- `OnRep_ClimbingState` plays entry montage via `GetMontageForSlot`
- Runs `ResolveHitComponentFromNet()` immediately (not deferred)
- IK culled beyond `SimulatedProxyIKCullDistance`; updated at `SimulatedProxyIKUpdateInterval`
- Camera, detection, audio: owning client only

---

## 🦾 Inverse Kinematics (IK)

- **Hands** — Two-Bone IK; targets from local `FClimbingDetectionResult` (owning client) or resolved via `ResolveHitComponentFromNet` (proxies).
- **Feet** — FABRIK; irregular surface positions.
- **Corner transitions** — FABRIK for all four limbs.

Per-limb `MaxReachDistance`; exceeding it fades weight to zero over `IKFadeOutBlendTime`. Tooltip: "prevents hyperextension — without this limit the arm stretches to an anatomically impossible length."

`IKWeightHandLeft`, `IKWeightHandRight`, `IKWeightFootLeft`, `IKWeightFootRight` on `UClimbingAnimInstance` — all `BlueprintReadWrite`. Per-limb blend in/out times Blueprint-exposed.

### IK Performance Budget

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|IK",
    meta = (ToolTip = "Max climbing characters with active IK per frame, sorted by distance to local camera. Beyond this count, IK weights are zeroed for that frame."))
int32 MaxSimultaneousIKCharacters = 4;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|IK",
    meta = (ToolTip = "Simulated proxy IK disabled beyond this distance (cm). Owning client IK is never culled."))
float SimulatedProxyIKCullDistance = 1500.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|IK",
    meta = (ToolTip = "Seconds between IK updates for simulated proxies. 0.05 = 20Hz. Owning client always updates every tick."))
float SimulatedProxyIKUpdateInterval = 0.05f;
```

**IK manager**: `static TArray<TWeakObjectPtr<AClimbingCharacter>> ActiveClimbingCharacters`. Register in `BeginPlay`; unregister in both `EndPlay` and `Destroyed`. Sorted insert by camera distance on state change — not per-tick rebuild. Game thread access only (document with comment). Owning client only — server does not run IK or maintain this array.

### Montage Slot

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Animations",
    meta = (ToolTip = "Animation Blueprint slot name for all climbing montages. Must match the ABP slot node name exactly. Mismatching causes montages to play with no visible output. Default 'FullBody'."))
FName ClimbingMontageSlot = FName("FullBody");
```

### AnimNotify Classes

| Class | Purpose |
|---|---|
| `UAnimNotify_EnableClimbIK` | Enables IK for a limb bitmask; place on `GrabLedge` at hand-contact frame |
| `UAnimNotify_DisableClimbIK` | Disables IK for a limb bitmask; place on `DropDown` at hand-release frame |
| `UAnimNotify_ClimbSoundCue` | `SpawnSoundAtLocation` via `EClimbSoundType` lookup |

---

## 🎬 Camera System

Assisted mode: spring arm rotates toward wall on climb entry; nudge back beyond `CameraNudgeActivationAngle`. Expose: `CameraNudgeStrength`, `CameraNudgeActivationAngle`, `CameraNudgeBlendSpeed`, `ClimbingCameraProbeRadius`.

```cpp
void LockCameraToFrame(FVector Location, FRotator Rotation, float BlendTime);
void ReleaseCameraLock(float BlendTime);
```

`LockCameraToFrame` affects local player camera only. Spectator systems out of scope.

**Camera during ragdoll**: `ReleaseCameraLock(0.1f)` → switch spring arm to `RagdollCameraTargetSocket` → restore to root on recovery.

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Physics",
    meta = (ToolTip = "Bone socket for spring arm during ragdoll. Default 'pelvis'. Override for non-standard skeletons."))
FName RagdollCameraTargetSocket = FName("pelvis");
```

---

## ⚡ Physics Interaction

### Dynamic Anchors

```cpp
UPROPERTY(Replicated)
TObjectPtr<UPrimitiveComponent> AnchorComponent;

UPROPERTY(Replicated)
FTransform AnchorLocalTransform;
```

World-space grab point = `AnchorComponent->GetComponentTransform() * AnchorLocalTransform` per tick.

`SetBase(AnchorComponent.Get())` on entry to `Hanging`, `Shimmying`, `BracedWall`, `BracedShimmying`. `SetBase(nullptr)` on all exits. If `AnchorComponent` null → `DroppingDown` immediately.

### Grab Break

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Physics",
    meta = (ToolTip = "Newtons to break a grab on physics impact while hanging or braced. 0 = disable."))
float GrabBreakImpulseThreshold = 2000.0f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Physics",
    meta = (ToolTip = "Multiplier on incoming impulse as ragdoll launch velocity."))
float GrabBreakLaunchScale = 0.5f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Physics",
    meta = (ToolTip = "Minimum ragdoll seconds before recovery. Too low = re-stand jitter. 1.5 recommended."))
float RagdollRecoveryTime = 1.5f;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Physics",
    meta = (ToolTip = "Pelvis bone name used for face-up/face-down get-up detection and ragdoll camera. Override for non-standard skeleton hierarchies."))
FName PelvisBoneName = FName("pelvis");
```

On break: `Montage_Stop(0.1f)` → `SetSimulatePhysics(true)` → apply launch → after `RagdollRecoveryTime`: use pelvis up-vector dot product (not Z rotation) to select get-up slot → re-enable movement component → restore capsule.

---

## 🎞 Animation System — Plug & Play

Every slot on `AClimbingCharacter` and mirrored in `UClimbingAnimationSet`. All with tooltips: when it plays, what it looks like, what breaks if unassigned.

**Ledge**: `HangIdle`, `HangIdleLeft`, `HangIdleRight`, `ShimmyLeft`, `ShimmyRight`, `CornerInsideLeft`, `CornerInsideRight`, `CornerOutsideLeft`, `CornerOutsideRight`, `ClimbUp`, `ClimbUpCrouch`, `DropDown`, `GrabLedge`, `GrabFail`, `ShimmyReposition`

**Idle variation**: `HangIdleVariations`, `IdleVariationDelay`, `IdleVariationBlendTime`, `bPreventConsecutiveVariationRepeat`

**Lache**: `LacheLaunch`, `LacheFlight`, `LacheCatch`, `LacheMiss`

**Mantle**: `MantleLow`, `MantleHigh`

**Ragdoll**: `RagdollGetUpFaceDown`, `RagdollGetUpFaceUp`

**Braced**: `BracedIdle`, `BracedShimmyLeft`, `BracedShimmyRight`, `BracedToHang`

**Ladder**: `LadderIdle`, `LadderClimbUp`, `LadderClimbDown`, `LadderFastAscend`, `LadderFastDescend`, `LadderEnterBottom`, `LadderEnterTop`, `LadderExitBottom`, `LadderExitTop`, `LadderExitSide`

Per-slot: blend-in time, blend-out time, playback rate — all Blueprint-exposed with tooltips.

---

## 🔊 Audio

No GAS. `UGameplayStatics::SpawnSoundAtLocation` only.

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Audio",
    meta = (ToolTip = "Map of sound events to assets. Missing entries: log Warning once, cache null, never retry."))
TMap<EClimbSoundType, TSoftObjectPtr<USoundBase>> ClimbingSounds;
```

Resolved cache: `TMap<EClimbSoundType, TObjectPtr<USoundBase>> ResolvedSounds`. Async load failure: log once → cache null → skip subsequent calls for that key.

---

## 🔄 State Machine

### Interrupt Priority
- **Freely interruptible**: `Hanging`, `Shimmying`, `BracedWall`, `OnLadder`
- **Committed**: `CornerTransition`, `LadderTransition`, `Mantling`, `LacheCatch`, `Ragdoll`
- **Conditionally interruptible**: `ClimbingUp`, `ClimbingUpCrouch`, `LacheInAir`, `LacheMiss`

### Capsule During Climbing

Reduce to `ClimbingCapsuleHalfHeight` / `ClimbingCapsuleRadius` and switch to `ClimbingCollisionProfile` on climbing entry. Restore on exit.

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|StateMachine",
    meta = (ToolTip = "Collision profile during climbing. Must ignore WorldStatic/WorldDynamic while retaining Pawn blocking. Prevents capsule-wall jitter."))
FName ClimbingCollisionProfile = FName("ClimbingCapsule");
```

### Map Transition & `EndPlay` Cleanup

In both `EndPlay` and `Destroyed`:
1. Remove self from `ActiveClimbingCharacters`
2. `SetBase(nullptr)`
3. `Montage_Stop(0.0f)`
4. Clear `LockedLacheTarget`; if `CurrentClimbingState == LacheInAir` → transition to `DroppingDown`
5. If mesh simulating physics → `SetSimulatePhysics(false)`
6. Remove `ClimbingInputMappingContext` from `UEnhancedInputLocalPlayerSubsystem` if present

---

## ⚙️ Blueprint Exposure Standards

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|[Subcategory]",
    meta = (ToolTip = "What this does, what happens if increased, what breaks if zero or missing."))
```

Categories: `Climbing|Detection`, `Climbing|Movement`, `Climbing|Animations`, `Climbing|IK`, `Climbing|Camera`, `Climbing|Ladder`, `Climbing|Lache`, `Climbing|Mantle`, `Climbing|StateMachine`, `Climbing|Input`, `Climbing|Physics`, `Climbing|Audio`, `Climbing|Debug`, `Climbing|Multiplayer`

---

## 🐛 Debug Visualization

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Debug",
    meta = (ToolTip = "Enable runtime debug drawing. Compiled out in Shipping builds."))
bool bDrawDebug = false;
```

All drawing in `#if !UE_BUILD_SHIPPING`. When `bDrawDebug` true: detection traces (green/red/yellow), IK targets (white spheres), anchor (cyan), corner predictive trace (blue), capsule override bounds (orange), current + previous state (on-screen), freefall grab window (shoulder-height sphere), `CommittedShimmyDir` as on-screen text during shimmy.

### Editor Lache Arc Preview — `#if WITH_EDITOR`

```cpp
#if WITH_EDITOR
/** Editor-only tick that draws the Lache arc in the viewport when bDrawDebug is true
 *  and THIS specific actor instance is selected (IsSelected() == true).
 *  Uses the same parabola formula as the runtime arc: LacheLaunchSpeed, LacheTotalArcTime,
 *  LacheArcTraceSteps, GetGravityZ().
 *  IsSelected() gates the draw to the selected actor only — without this gate, all
 *  AClimbingCharacter instances in the level draw arcs simultaneously when any one is selected. */
virtual void Tick(float DeltaTime) override;  // route editor-only draw through existing Tick
#endif
```

Gate in `Tick`:
```cpp
#if WITH_EDITOR
if (bDrawDebug && IsSelected() && !GetWorld()->IsGameWorld())
{
    // Draw Lache arc preview using LacheLaunchSpeed, LacheTotalArcTime, LacheArcTraceSteps,
    // GetCharacterMovement()->GetGravityZ() (negative — use directly, do not Abs or negate)
}
#endif
```

---

## ✅ BeginPlay Slot Validation

In `AClimbingCharacter::BeginPlay`, iterate every `EClimbingAnimationSlot` value and call `GetMontageForSlot`. For each null result:
```cpp
UE_LOG(LogClimbing, Warning, TEXT("ClimbingSystem: Slot '%s' unassigned on '%s'. The action requiring this animation will fail silently at runtime."),
    *UEnum::GetValueAsString(Slot), *GetName());
```
Validation only — does not prevent the game from running.

---

## ✅ Constraints & Quality Bar

- **No Blueprint logic** — all gameplay logic in C++
- **No magic numbers** — every behavior-affecting literal is a named UPROPERTY
- **No unguarded raw pointers** — `TObjectPtr<>` with null checks in replicated paths; `TWeakObjectPtr<>` in local-only paths only
- **No `TWeakObjectPtr` in replicated UPROPERTYs** — use `TObjectPtr` or `FClimbingDetectionResultNet`
- **No `NetSerialize` on `FClimbingDetectionResultNet`** — default serialization is sufficient at this update frequency
- **No legacy input** — Enhanced Input only
- **No GAS** — audio via `UGameplayStatics::SpawnSoundAtLocation`
- **No streaming sublevel anchors** — reject and log actor name
- **No hardcoded gravity** — use `GetCharacterMovement()->GetGravityZ()` directly; never `FMath::Abs` it
- **No `BlueprintType` on `FClimbingDetectionResultNet`** — it is a network serialization struct only
- **No pelvis Z-rotation for face detection** — use pelvis up-vector dot world up
- Compile cleanly against **UE5.3+**
- `/** */` doc comments on every non-trivial function
- Complete `.h` and `.cpp` for every class — do not truncate or summarize
- All `Server_` and `Client_` RPCs with `_Implementation` in `.cpp`
- `LogClimbing` for all log output — never `LogTemp`
- `UE_BUILD_SHIPPING` guards on all debug drawing and per-tick `UE_LOG`
- All timers: `bIgnorePause = false`; document with comment at each `SetTimer` call
- IK manager static array: game thread access only; document with comment
- **Milestone checkpoint**: output `[MILESTONE N: <title> — modifying <files>]` before each milestone
- **If approaching context limit: output `CONTINUE IN NEXT MESSAGE` and stop. Never summarize or truncate a class body.**

---

## 📋 What Changed From v6

| Gap | Fix Applied |
|---|---|
| Confirmation trace underspecified | `ResolveHitComponentFromNet()` defined with shape, channel, radius, failure path, proxy-vs-owning-client scheduling |
| `NetSerialize` intent unspecified | Explicitly forbidden with stated reason: state-transition-only replication makes it unnecessary |
| Shimmy direction hysteresis missing | `CommittedShimmyDir` + `ShimmyDirectionDeadzone`; mapping table uses `CommittedShimmyDir` not raw axis |
| Confirmation vs detection redundancy | Owning client skips confirmation trace during active climbing; uses local scan result directly |
| Input Mapping Context unspecified | `ClimbingInputMappingContext` + `ClimbingIMCPriority`; push on climb entry, pop on exit; Sprint/Crouch conflict resolved |
| `FClimbingDetectionResultNet BlueprintType` | Explicitly forbidden with stated reason |
| Ragdoll face detection simplified | Pelvis up-vector dot world up mandated; pelvis Z rotation forbidden; `PelvisBoneName` exposed |
| Lache target cleanup on map transition | `LockedLacheTarget` cleared in `EndPlay` + `Destroyed`; `LacheInAir` → `DroppingDown` on transition |
| Editor arc preview `IsSelected()` scope | `IsSelected()` + `!GetWorld()->IsGameWorld()` gate specified; reason documented |
| `GetGravityZ()` sign convention | Explicit formula showing correct negative usage; two common wrong forms shown and labeled |
| Milestone checkpoint output | `[MILESTONE N: title — modifying files]` instruction added to preamble and constraints |
| IMC cleanup in `EndPlay` | `RemoveMappingContext` added to `EndPlay` + `Destroyed` cleanup list |
