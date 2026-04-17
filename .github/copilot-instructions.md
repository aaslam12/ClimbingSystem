# Copilot Instructions for ClimbingSystem

## Build, test, and lint commands

### Build
```bash
# Open the project in editor
UnrealEditor ClimbingSystem.uproject

# Regenerate project files
make configure
# or
UnrealBuildTool -ProjectFiles -Project="$PWD/ClimbingSystem.uproject" -Game

# Build game/editor targets on Linux (generated Makefile)
make ClimbingSystem
make ClimbingSystemEditor
```

Equivalent direct UBT commands:
```bash
"$UNREALROOTPATH/Engine/Build/BatchFiles/RunUBT.sh" ClimbingSystem Linux Development -Project="$PWD/ClimbingSystem.uproject"
"$UNREALROOTPATH/Engine/Build/BatchFiles/RunUBT.sh" ClimbingSystemEditor Linux Development -Project="$PWD/ClimbingSystem.uproject"
```

### Test (Unreal Automation)
Automation tests live in `Source/ClimbingSystem/Tests/*.cpp` and are wrapped in `#if WITH_DEV_AUTOMATION_TESTS`.

```bash
# Run all climbing automation tests
UnrealEditor-Cmd "$PWD/ClimbingSystem.uproject" -unattended -nop4 -nosplash -NullRHI \
  -ExecCmds="Automation RunTests ClimbingSystem;Quit" \
  -TestExit="Automation Test Queue Empty"

# Run one subsystem
UnrealEditor-Cmd "$PWD/ClimbingSystem.uproject" -unattended -nop4 -nosplash -NullRHI \
  -ExecCmds="Automation RunTests ClimbingSystem.Movement;Quit" \
  -TestExit="Automation Test Queue Empty"

# Run a single test (example)
UnrealEditor-Cmd "$PWD/ClimbingSystem.uproject" -unattended -nop4 -nosplash -NullRHI \
  -ExecCmds="Automation RunTests ClimbingSystem.Movement.ShimmySpeed.VerticalWall;Quit" \
  -TestExit="Automation Test Queue Empty"
```

### Lint
No dedicated lint command/config is defined in this repository.

## High-level architecture

- Single runtime module: `Source/ClimbingSystem`.
- Core runtime ownership is centered on `AClimbingCharacter` + `UClimbingMovementComponent` + `UClimbingAnimInstance`.
- `AClimbingCharacter` is intentionally split by concern (`ClimbingCharacter_*.cpp`), with state machine, detection, actions, multiplayer, IK, camera, physics, audio, and debug separated.
- Runtime flow is:
  1. Input handlers in `ClimbingCharacter_Actions.cpp` perform local prediction (`TransitionToState`) and call server RPCs for authoritative validation.
  2. Server RPCs in `ClimbingCharacter_Multiplayer.cpp` re-run detection, validate tolerance, reject/confirm, and replicate `LastValidatedDetectionResult`.
  3. `UClimbingMovementComponent` replicates climbing state/anchor info and applies movement mode per state.
  4. Simulated proxies handle `OnRep_ClimbingState` → `OnClimbingStateReplicated`, then resolve `HitComponent` from replicated net data via confirmation trace.
  5. `ClimbingCharacter_StateMachine.cpp` drives montage selection, capsule/collision profile switching, input context swaps, motion warping targets, and state tick routing.
- Surface behavior is data-driven through `UClimbingSurfaceData` (tier/rules/speed/audio/optional animation set), and per-surface animation override uses `UClimbingAnimationSet` with per-slot fallback to character defaults.

## Key conventions specific to this codebase

- **Replication contract is strict:** use `FClimbingDetectionResultNet` for replicated data (no UObject pointers); keep `FClimbingDetectionResult` local-only (contains `TWeakObjectPtr`).
- **State transition authority model:** clients predict locally, server re-runs detection and validates tolerance, then confirms or rejects; rejection triggers rollback with `Client_RejectStateTransition`.
- **State maps/switches must stay exhaustive:** when adding states, update `EClimbingState`, `InitializeStateConfigs()`, transition rules (`IsValidStateTransition`), movement-mode mapping, and state machine entry/exit/tick handling.
- **Animation slot changes are cross-cutting:** keep `EClimbingAnimationSlot`, `AClimbingCharacter` slot properties/getter, `UClimbingAnimationSet::GetMontageForSlot`, and tests synchronized.
- **Surface metadata convention:** surfaces are discovered via component/actor tags using `SurfaceData:<AssetPath>` and climb tags (`Climbable`, `Unclimbable`, `ClimbableOneWay`).
- **Input context flow matters:** locomotion IMC is applied at startup; climbing IMC is added on first climb-state entry and removed on return to `None`.
- **Attached climbing states block standard locomotion input:** `IA_ClimbMove` is authoritative while attached; standard move input is intentionally ignored in attached states.
- **Collision profile dependency:** climbing states switch capsule to `ClimbingCapsule` (defined in `Config/DefaultEngine.ini`), then restore original profile/size on exit.
- **Animation slot name must match ABP slot node:** `ClimbingMontageSlot` defaults to `FullBody`; mismatches cause montages to play with no visible output.
- **Anchor constraint:** server rejects grab anchors in streaming sublevels; climbable anchors are expected in the persistent level.
- **Pause behavior:** climbing timers are configured so the system pauses with game time (default timer pause behavior is intentional).
- **Coding style:** follow existing Unreal style in this repo (tabs, braces on new lines, PascalCase identifiers, `Climbing|...` category names).
