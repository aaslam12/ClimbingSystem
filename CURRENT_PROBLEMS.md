# Climbing System - Comprehensive Problem Summary

**Session Date:** 2026-04-06  
**Build:** Milestones 22-30 committed

---

## 🔴 CRITICAL ISSUES (System Non-Functional)

### 1. ClimbUp Does Not Work
**Status:** BROKEN  
**Symptoms:**
- User can grab ledges successfully
- User is hanging from ledge
- Pressing ClimbUp button does nothing
- Log shows: `"Input_ClimbUp: No clearance above ledge! Cannot climb up."`
- **Even after Milestone 30 fix**, still reports no clearance

**Root Cause Analysis:**
- Milestone 30 fixed the clearance detection to ignore ledge components
- BUT user still getting "No clearance" warning
- Possible causes:
  1. **Clearance sweep is still hitting something** - need to check debug logs with bDrawDebug enabled
  2. **Ledge might be detecting as multiple separate components** - wall vs top
  3. **Capsule radius/height values might be too large** - sweep sphere hitting nearby geometry
  4. **Detection might be using wrong hit result** - ForwardHit vs DownHit component mismatch

**Expected Behavior:**
- Press ClimbUp while hanging → character pulls up and over ledge
- Should transition to ClimbingUp or ClimbingUpCrouch state
- Motion warping should move character to top of ledge

**Debug Steps Needed:**
1. Enable `bDrawDebug = true` in BP_ClimbingCharacter
2. Grab a ledge
3. Check Output Log for "Clearance blocked by: [ComponentName]"
4. Check if debug lines show clearance sweep (should be green if clear, red if blocked)
5. Verify OriginalCapsuleRadius and OriginalCapsuleHalfHeight values aren't too large

---

### 2. Shimmying Does Not Work
**Status:** PARTIALLY FIXED (Milestone 27) but user reports still broken  
**Symptoms:**
- User grabs ledge successfully
- Pressing WASD keys does nothing
- Character doesn't move left/right along ledge
- User reports "shimmy direction from the string printed also does not change"

**Root Cause Analysis:**
- Milestone 27 fixed Input_Move to update CurrentClimbMoveInput when climbing
- BUT the issue might be:
  1. **IA_ClimbMove not mapped in ClimbingIMC** - Input_Move fallback might not be working
  2. **Shimmy state transition not triggering** - TickHangingState checks movement input
  3. **ClimbingMovementComponent not applying shimmy movement** - TickShimmyingState calculates speed but might not move character
  4. **CurrentClimbMoveInput not being set** - Input_Move check might be failing

**Expected Behavior:**
- Press A/D (or Left/Right) while hanging → character shimmies along ledge
- Should transition from Hanging to Shimmying state
- CommittedShimmyDir should update in debug display
- Character should visibly move horizontally along wall

**Debug Steps Needed:**
1. Check if state transitions from Hanging to Shimmying (look at on-screen state display)
2. Check if CommittedShimmyDir value changes when pressing A/D
3. Verify Input_Move is being called (add log in Input_Move)
4. Check if TickShimmyingState is being called (state should show "Shimmying")

---

### 3. Mantling Does Not Work At All
**Status:** PARTIALLY FIXED (Milestone 29) but user reports still broken  
**Symptoms:**
- User approaches low obstacle (50-180cm)
- Presses Grab button
- Nothing happens OR character hangs instead of mantling
- User says "mantling does not work at all, neither does the mantle step, low mantle, high mantle"

**Root Cause Analysis:**
- Milestone 29 added fallback mantle detection for low obstacles
- BUT detection might be failing because:
  1. **Obstacle not tagged as Climbable** - needs "Climbable" tag on component
  2. **Obstacle outside height range** - check if between 50cm and 180cm from character feet
  3. **Clearance check failing** - even mantle needs ClearanceType != None
  4. **No montages assigned** - MantleLow/MantleHigh slots empty in Animation Set
  5. **Fallback detection not triggering** - normal ledge detection might be succeeding and choosing Hanging instead

**Expected Behavior:**
- Approach obstacle 50-100cm high → Grab triggers MantleLow
- Approach obstacle 100-180cm high → Grab triggers MantleHigh
- Character vaults over obstacle with animation
- Ends up on top of obstacle, back in locomotion mode

**Debug Steps Needed:**
1. Check obstacle height from character feet (should be 50-180cm)
2. Add "Climbable" tag to obstacle component
3. Check for "Mantle detection: Found obstacle at height X cm" log
4. Check for "Mantling: No montage assigned" error
5. Verify obstacle has full clearance above it (nothing blocking top)

---

## 🟡 MAJOR ISSUES (Features Not Working)

### 4. Directional Jumping Implementation Untested
**Status:** IMPLEMENTED (Milestone 27) but not verified  
**Symptoms:**
- User says "when i press space while hanging, i fall through map" (fixed in M26)
- User wants directional jumping: "if i press the move right key and press jump, it should jump to next ledge"
- Milestone 27 implemented this but user hasn't confirmed it works

**Expected Behavior:**
- Jump while hanging (no direction) → lache forward to next ledge
- Left+Jump → lache diagonally left
- Right+Jump → lache diagonally right
- If no valid ledge found → do nothing (don't fall)

**Issues:**
- User might not know Jump now triggers Lache when hanging
- IA_Lache might not be mapped OR Jump key might not be mapped
- LacheArc calculation might not be finding ledges (too strict)
- Character might be launching but missing catches

---

### 5. Input Mapping Context Not Configured
**Status:** LIKELY USER CONFIGURATION ISSUE  
**Symptoms:**
- ClimbUp doesn't work → might not be mapped
- ClimbMove might not work → might not be mapped
- User doesn't know what keys are mapped to what actions

**Root Cause:**
- ClimbingInputMappingContext is an asset that needs manual configuration
- User might not have assigned keys to IA_ClimbUp, IA_ClimbMove, etc.
- Without mappings, the input functions never fire

**Required Mappings:**
```
ClimbingInputMappingContext should have:
- IA_Grab → E key
- IA_Drop → Q key  
- IA_ClimbUp → Space or F key (CRITICAL - user doesn't have this)
- IA_ClimbMove → WASD or Gamepad Stick (2D Vector)
- IA_Lache → L key or other (optional)
- IA_Sprint → Shift (optional)
- IA_Crouch → Ctrl/C (optional)
```

**Debug Steps:**
1. Open ClimbingInputMappingContext asset in Content Browser
2. Verify each IA_* action has a key binding
3. Test each key while climbing and check logs for "Input_[ActionName]: ..."

---

## 🟢 ISSUES FIXED (But Need Verification)

### 6. Character Going Through Wall When Grabbing ✅
**Fixed:** Milestone 22  
**Status:** User confirmed working ("ledge grab and drop properly works")  
- Surface normal direction was inverted
- Changed `-DetectionResult.SurfaceNormal` to `+DetectionResult.SurfaceNormal`

### 7. Drop Positioning Wrong ✅
**Fixed:** Milestone 22  
**Status:** User confirmed working
- Added offset away from wall when dropping
- Character now drops in front of wall instead of behind/inside

### 8. Camera Rotation Issue ✅
**Fixed:** Milestone 22  
**Status:** Not explicitly tested but should be working
- Removed `bUseControllerRotationYaw = true` on climb exit
- Character should maintain proper orientation

### 9. Fall Through Map When Pressing Space ✅
**Fixed:** Milestone 26 + 27  
**Status:** Should be working
- M26: Added explicit collision re-enable on state exit
- M27: Made Jump trigger Lache when climbing instead of ACharacter::Jump

### 10. Multi-Ledge Detection Bugs ✅
**Fixed:** Milestone 24  
**Status:** Not explicitly tested
- Changed to multi-hit trace
- Added best-hit selection to prefer nearby height when climbing

---

## 🔵 CONFIGURATION ISSUES (Not Code Bugs)

### 11. Missing Animation Montages
**Status:** CONFIGURATION REQUIRED  
**Symptoms:**
- User sees warnings: "Slot 'X' unassigned on 'BP_ClimbingCharacter_C_0'"
- Most animations are assigned, but some ladder/ragdoll ones missing
- ClimbUp/Mantle montages might not be assigned

**Required Setup:**
1. Create/assign ClimbingAnimationSet data asset
2. Assign montages for critical slots:
   - GrabLedge, HangIdle
   - ShimmyLeft, ShimmyRight
   - ClimbUp, ClimbUpCrouch
   - MantleLow, MantleHigh
   - DropDown
3. Verify montages have correct SlotNames matching ABP

### 12. Motion Warping Not Set Up
**Status:** CONFIGURATION REQUIRED  
**Symptoms:**
- Character might teleport instead of animating smoothly
- Grab/ClimbUp/Mantle animations don't align to ledge
- Character might not move during montages

**Required Setup:**
1. Enable Motion Warping plugin (Edit → Plugins)
2. Add Motion Warping component to BP_ClimbingCharacter
3. In AnimBP: Add Motion Warping node after climbing slot
4. Verify montages have sync groups matching warp target names:
   - WarpTarget_LedgeGrab
   - WarpTarget_ClimbUp
   - WarpTarget_Mantle

### 13. Debug Visualization Disabled
**Status:** USER SHOULD ENABLE FOR TROUBLESHOOTING  
**Current:** bDrawDebug probably = false  
**Needed:** Set to true to see:
- Detection traces (forward/downward in green/red)
- Clearance checks (green/yellow/red)
- Ledge positions (cyan spheres)
- Surface normals (blue arrows)
- Lache arcs (cyan lines)

---

## 📋 TESTING CHECKLIST (What Works vs What Doesn't)

| Feature | Expected | User Reports | Status |
|---------|----------|--------------|--------|
| Grab tall ledge | Hang from ledge | ✅ WORKS | WORKING |
| Drop from ledge | Drop down safely | ✅ WORKS | WORKING |
| Shimmy left/right | Move along ledge | ❌ DOESN'T WORK | **BROKEN** |
| ClimbUp from hang | Pull up over ledge | ❌ DOESN'T WORK | **BROKEN** |
| Jump while hanging | Lache to next ledge | ❓ NOT TESTED | UNKNOWN |
| Grab low obstacle | Mantle over it | ❌ DOESN'T WORK | **BROKEN** |
| Multi-ledge stability | Stick to one ledge | ❓ NOT TESTED | UNKNOWN |
| Character positioning | In front of wall | ✅ WORKS | WORKING |

---

## 🎯 RECOMMENDED DEBUGGING ORDER

### Priority 1: Fix ClimbUp (Most Critical)
1. Enable `bDrawDebug = true` in BP_ClimbingCharacter
2. Grab a ledge
3. Check Output Log for "Clearance blocked by: [ComponentName]"
4. If blocked: identify what component is blocking and why
5. If not blocked but still says "No clearance": the ClearanceType is being set wrong
6. Check if IA_ClimbUp is actually mapped in ClimbingIMC
7. Test with F key, Space key, and any other potential mappings

### Priority 2: Fix Shimmying
1. Add log to Input_Move: `UE_LOG(LogClimbing, Warning, TEXT("Input_Move called: X=%.2f Y=%.2f Climbing=%d"), MovementVector.X, MovementVector.Y, ClimbingMovement ? 1 : 0);`
2. Press A/D while hanging and verify log appears
3. Check on-screen state display - does it change from Hanging to Shimmying?
4. If state doesn't change: TickHangingState transition logic is broken
5. If state changes but no movement: TickShimmyingState movement application is broken

### Priority 3: Fix Mantling
1. Check obstacle height: measure from character feet to top
2. Add "Climbable" tag to obstacle static mesh component
3. Check for "Mantle detection: Found obstacle" log when pressing Grab near obstacle
4. If detection works but no animation: check for "No montage assigned" error
5. Verify MantleLow/MantleHigh slots have montages in Animation Set

### Priority 4: Verify Input Mappings
1. Open ClimbingInputMappingContext asset
2. Screenshot all mappings and share
3. Verify IA_ClimbUp has a key binding
4. Test each key individually while climbing

---

## 💡 IMMEDIATE ACTION ITEMS FOR USER

1. **Enable Debug Visualization:**
   - Open BP_ClimbingCharacter
   - Find `bDrawDebug` variable
   - Set to `true`
   - Save and test

2. **Check Clearance Issue:**
   - Grab ledge with debug enabled
   - Look for colored lines above ledge (should be green if clear)
   - Check Output Log for "Clearance blocked by: [ComponentName]"
   - Share the component name

3. **Verify Input Mappings:**
   - Find ClimbingInputMappingContext in Content Browser
   - Open it
   - Screenshot the mappings list
   - Identify what key is bound to IA_ClimbUp

4. **Test Shimmy with Logs:**
   - While hanging, press A and D keys
   - Watch on-screen debug text - does "ShimmyDir" value change?
   - Does state change from "Hanging" to "Shimmying"?

5. **Test Mantle Detection:**
   - Stand near a box/platform 50-150cm tall
   - Add "Climbable" tag to the box's static mesh component
   - Press Grab
   - Check log for "Mantle detection: Found obstacle at height X cm"

---

## 🔧 POTENTIAL NEXT FIXES NEEDED

Based on symptoms, likely need to:

1. **Clearance Detection:** May need to increase the clearance check start distance or change the sweep size
2. **Shimmy Movement:** May need to verify ClimbingMovementComponent is actually moving the character
3. **Input Routing:** May need to add fallback for when ClimbingIMC actions aren't mapped
4. **Mantle Triggering:** May need to lower mantle priority vs hanging detection

---

**Last Updated:** Milestone 30 (2026-04-06)  
**Next Steps:** Wait for user debug info from bDrawDebug and Input Mapping verification

