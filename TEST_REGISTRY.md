# TEST_REGISTRY.md
# UE5 Multiplayer Climbing System — Test Case Registry
# Last updated: 2026-04-25T18:34:24-07:00
# Total registered: 424 | Implemented: 152 | Planned: 272 | Manual QA: 3

---

## RUN HISTORY
| Run # | Date | New TCs Added | Focus Areas | Agent Notes |
|-------|------|--------------|-------------|-------------|
| 1     | 2026-04-25 | 122 | Detection, Traces, StateMachine, Movement, Actions, Shimmy, Corner, BracedWall, Lache, Mantle, Ladder, Ragdoll, Physics, IK, Animation, Audio, Camera, Multiplayer, Input, Debug, Lifecycle | 5-agent coordinated fleet; each agent assigned non-overlapping systems and TC-ID ranges |
| 2     | 2026-04-25 | 150 | Mantle depth, Ladder depth, Ragdoll/Physics depth, IK/Camera/Audio/SurfaceData depth, Multiplayer RPCs, Freefall/CoyoteTime, ClimbableOneWay, Integration/Stress/Performance, MontageCallbacks, InputHandlers, Capsule, WarpTargets | 5-agent fleet Run 2; focused on ~30% coverage areas, failure/stress/edge cases, untested functions |

---

## DO NOT TEST — OUT OF SCOPE
<!-- Add systems explicitly excluded from testing. Future runs will not propose tests for these. -->
| Item | Reason |
|------|--------|
| ThirdPerson/ sample content | Third-party template content, not owned by project |
| Content/ Blueprint assets | Data-only containers; no gameplay logic to test |
| Config/Default*.ini | Engine configuration; validated by editor, not automation |

---

## MANUAL QA REQUIRED — CANNOT BE AUTOMATED
<!-- Blueprint-only systems, visual/audio checks, replication tests requiring multi-process PIE -->
| ID | System | What to Verify | Why Not Automatable |
|----|--------|---------------|---------------------|
| QA-001 | Animation Blueprint (ABP) | Verify climbing montages play visually correct in all states; check blend transitions between hang/shimmy/corner | Blueprint graph wiring; no C++ interface to validate visual output |
| QA-002 | Multi-Process Replication | Verify simulated proxy animations, IK, and state sync with 2+ PIE clients on listen server | Requires multi-process PIE; single-process approximation cannot fully validate network timing |
| QA-003 | Motion Warping Visual Alignment | Verify motion warp targets align character hands/feet to ledge geometry during GrabLedge, ClimbUp, Mantle, LacheCatch, Corner, Ladder transitions | Visual alignment quality cannot be asserted programmatically |

---

## IMPLEMENTED — DO NOT RE-PROPOSE
<!-- Concise entries. One line per test. Enough detail to detect duplicates. -->
<!-- Format: ID | Name | System | What it tests | Condition/Variant | Type -->

| ID | Test Name | System | Behavior Tested | Condition | Type |
|----|-----------|--------|----------------|-----------|------|
| TC-0001 | FClimbingEnumLayoutTest | Types | verifies EClimbingAnimationSlot and EClimbingState enum counts match expected layout | happy | Unit |
| TC-0002 | FClimbingStructBlueprintExposureTest | Types | verifies FClimbingDetectionResult is BlueprintType and FClimbingDetectionResultNet is not | happy | Unit |
| TC-0003 | FClimbingNetStructSafetyTest | Types | verifies FClimbingDetectionResultNet has no UObject pointer properties and no custom NetSerialize | happy | Unit |
| TC-0004 | FClimbingStateConfigDefaultsTest | Types | verifies FClimbingStateConfig default and custom constructor values | happy | Unit |
| TC-0005 | DetectionResultNetConversion | Types | verifies FClimbingDetectionResult converts to FClimbingDetectionResultNet correctly | happy | Unit |
| TC-0006 | DetectionResultNetResetClearsAllFields | Types | verifies FClimbingDetectionResultNet reset clears all fields to defaults | happy | Unit |
| TC-0007 | DetectionResultResetClearsAllFields | Types | verifies FClimbingDetectionResult reset clears all fields to defaults | happy | Unit |
| TC-0008 | SurfaceDataAssetIdReturnsValidId | SurfaceData | verifies UClimbingSurfaceData returns a valid primary asset ID | happy | Unit |
| TC-0009 | SurfaceDataDefaultsAllFieldsMatchSpec | SurfaceData | verifies all UClimbingSurfaceData default field values match spec | happy | Unit |
| TC-0010 | GridTracesCorrectColumnCount | Traces | verifies grid trace produces correct number of columns | happy | Unit |
| TC-0011 | GridTracesCorrectRowCount | Traces | verifies grid trace produces correct number of rows | happy | Unit |
| TC-0012 | GridTracesTerminateAtFirstVerticalHit | Traces | verifies grid trace stops at first vertical hit per column | happy | Unit |
| TC-0013 | HorizontalTraceValidatesWallNormal | Traces | verifies horizontal trace validates wall normal direction | happy | Unit |
| TC-0014 | OriginHeightIsAtChestLevel | Traces | verifies trace origin height is at chest level | happy | Unit |
| TC-0015 | BracedRejectsLadderOnly | Detection | verifies braced detection rejects LadderOnly tagged surfaces | failure | Component |
| TC-0016 | LadderOnlyTagDetected | Detection | verifies LadderOnly tag is detected by ladder detection | happy | Component |
| TC-0017 | LadderRejectsNonLadderSurface | Detection | verifies ladder detection rejects non-LadderOnly surfaces | failure | Component |
| TC-0018 | LedgeRejectsUnclimbableTag | Detection | verifies ledge detection rejects Unclimbable tagged surfaces | failure | Component |
| TC-0019 | WallBackingClassifiesAsBracedHang | Detection | verifies wall backing trace classifies as braced hang when wall present | happy | Component |
| TC-0020 | WallBackingClassifiesAsFreeHang | Detection | verifies wall backing trace classifies as free hang when no wall present | happy | Component |
| TC-0021 | WallBackingTraceDepthRespondsToParameter | Detection | verifies wall backing trace depth responds to parameter changes | edge | Component |
| TC-0022 | LedgeGrabRejectsAboveMaxHeight | Detection | verifies ledge grab rejects ledges above max height | failure | Component |
| TC-0023 | LedgeGrabRejectsBelowMinHeight | Detection | verifies ledge grab rejects ledges below min height | failure | Component |
| TC-0024 | LedgeGrabTakesPriorityOverMantle | Detection | verifies ledge grab takes priority over mantle detection | happy | Component |
| TC-0025 | TopConfirmationTraceFindsFlatSurface | Detection | verifies top confirmation trace finds flat surfaces | happy | Component |
| TC-0026 | TopConfirmationTraceRejectsMidWallFace | Detection | verifies top confirmation trace rejects mid-wall face hits | failure | Component |
| TC-0027 | LedgeGrabTriggersWithinValidHeightRange | Detection | verifies ledge grab triggers within valid height range | happy | Component |
| TC-0028 | MantleClearanceCheckRejectsBlockedLanding | Detection | verifies mantle clearance check rejects blocked landing zones | failure | Component |
| TC-0029 | MantleDoesNotFireAboveMaxHeight | Detection | verifies mantle does not fire above max mantle height | failure | Component |
| TC-0030 | MantleHeightThresholdExposedParameterRespected | Detection | verifies mantle height threshold exposed parameter is respected | happy | Component |
| TC-0031 | MantleTopSurfaceRequiresFlatSurface | Detection | verifies mantle top surface confirmation requires flat surface | failure | Component |
| TC-0032 | MantleTriggersOnThickWallAtValidHeight | Detection | verifies mantle triggers on thick wall at valid height | happy | Component |
| TC-0033 | MantleTriggersOnThinWallAtValidHeight | Detection | verifies mantle triggers on thin wall at valid height | happy | Component |
| TC-0034 | PriorityOrderLedgeGrabBeforeMantle | Priority | verifies ledge grab has priority over mantle in detection order | happy | Component |
| TC-0035 | PriorityOrderNothingFiredWhenBothFail | Priority | verifies nothing fires when both ledge grab and mantle fail | failure | Component |
| TC-0036 | PriorityOrderPassesAreSequentialNotParallel | Priority | verifies detection passes run sequentially not in parallel | happy | Component |
| TC-0037 | ClimbUpValidFromHanging | Movement | verifies ClimbUp is valid from Hanging state | happy | Component |
| TC-0038 | DropFromHanging | Movement | verifies Drop transitions from Hanging | happy | Component |
| TC-0039 | DropInvalidFromNone | Movement | verifies Drop is invalid from None state | failure | Component |
| TC-0040 | LacheInvalidFromNone | Movement | verifies Lache is invalid from None state | failure | Component |
| TC-0041 | LacheNoTargetStaysHanging | Movement | verifies Lache with no target stays in Hanging | failure | Component |
| TC-0042 | LacheValidTransitionFromHanging | Movement | verifies Lache valid transition from Hanging state | happy | Component |
| TC-0043 | ShimmyDirectionPersistsAcrossTicks | Movement | verifies shimmy direction persists across ticks | happy | Component |
| TC-0044 | ShimmyNoInputStaysHanging | Movement | verifies no shimmy input stays in Hanging state | happy | Component |
| TC-0045 | ShimmyTransitionFromHanging | Movement | verifies shimmy transition from Hanging state | happy | Component |
| TC-0046 | LadderSpeedModifiers | Movement | verifies ladder speed with sprint/crouch modifiers | happy | Unit |
| TC-0047 | LadderSpeedZeroMultiplierReturnsZero | Movement | verifies ladder speed returns zero with zero multiplier | edge | Unit |
| TC-0048 | MaxSpeedNoneStateReturnsWalkingSpeed | Movement | verifies None state returns walking speed | happy | Unit |
| TC-0049 | RPCPolicyPackedMovement | Movement | verifies ShouldUsePackedMovementRPCs policy | happy | Unit |
| TC-0050 | ShimmySpeedBoundaryAngleNoPenalty | Movement | verifies shimmy speed has no penalty at boundary angle | edge | Unit |
| TC-0051 | ShimmySpeedFullOverhang | Movement | verifies shimmy speed at full overhang | edge | Unit |
| TC-0052 | ShimmySpeedOverhangPenaltyBoundary | Movement | verifies shimmy speed overhang penalty at boundary | edge | Unit |
| TC-0053 | ShimmySpeedVerticalWall | Movement | verifies shimmy speed on vertical wall (no penalty) | happy | Unit |
| TC-0054 | ShimmySpeedZeroMultiplierReturnsZero | Movement | verifies shimmy speed returns zero with zero multiplier | edge | Unit |
| TC-0055 | MaxSpeedByState | Movement | verifies max speed values per climbing state | happy | Unit |
| TC-0056 | StateConfigCommittedStatesNotInterruptible | Movement | verifies committed states are not interruptible | happy | Unit |
| TC-0057 | StateConfigExhaustiveForAllStates | Movement | verifies StateConfigs has entry for every EClimbingState | happy | Unit |
| TC-0058 | StateConfigFreelyInterruptibleStates | Movement | verifies freely interruptible states are marked correctly | happy | Unit |
| TC-0059 | BracedWallToHangValidShimmyInvalid | Movement | verifies BracedWall to Hang valid but shimmy invalid | edge | Component |
| TC-0060 | DroppingDownExitsToNone | Movement | verifies DroppingDown exits to None state | happy | Component |
| TC-0061 | MantlingCommittedBlocksEarlyExit | Movement | verifies Mantling committed state blocks early exit | happy | Component |
| TC-0062 | RagdollOnlyExitsToNone | Movement | verifies Ragdoll only exits to None state | happy | Component |
| TC-0063 | CoreTransitionRules | Movement | verifies core state transition rules | happy | Component |
| TC-0064 | StateTransitionsInterruptibility | Movement | verifies state transition interruptibility rules | happy | Component |
| TC-0065 | StateTransitionsNoopSameState | Movement | verifies same-state transition is a no-op | edge | Component |
| TC-0066 | StateTransitionsPreviousStateTracking | Movement | verifies PreviousClimbingState tracking | happy | Component |
| TC-0067 | StateMachineDefaultValues | Character | verifies state machine default property values | happy | Unit |
| TC-0068 | HasClimbingMovementComponent | Character | verifies character has ClimbingMovementComponent | happy | World |
| TC-0069 | InitialStateIsNone | Character | verifies initial climbing state is None | happy | World |
| TC-0070 | SpawnSucceeds | Character | verifies character spawns successfully | happy | World |
| TC-0071 | ShimmyDirectionPersistsInsideDeadzone | Actions | verifies shimmy direction persists when input inside deadzone | edge | Component |
| TC-0072 | ShimmyDirectionUpdatesAboveDeadzone | Actions | verifies shimmy direction updates when input above deadzone | happy | Component |
| TC-0073 | HangingTransitionsToShimmying | Actions | verifies Hanging transitions to Shimmying with input | happy | Component |
| TC-0074 | ShimmyReleaseReturnsToHanging | Actions | verifies releasing shimmy returns to Hanging | happy | Component |
| TC-0075 | ClimbUpBlockedNoClearance | Actions | verifies ClimbUp blocked when no clearance | failure | Component |
| TC-0076 | ClimbUpRejectedFromInvalidState | Actions | verifies ClimbUp rejected from invalid state | failure | Component |
| TC-0077 | ClimbUpSelectsCrouchClearanceState | Actions | verifies ClimbUp selects crouch state for CrouchOnly clearance | happy | Component |
| TC-0078 | ClimbUpSelectsFullClearanceState | Actions | verifies ClimbUp selects full state for Full clearance | happy | Component |
| TC-0079 | ClimbUpUsesLastValidatedFallback | Actions | verifies ClimbUp uses last validated detection as fallback | edge | Component |
| TC-0080 | MantleDetectionFallbackInValidRange | Actions | verifies mantle detection fallback works in valid range | happy | Component |
| TC-0081 | MantleDetectionFallbackRejectsUnclimbable | Actions | verifies mantle detection fallback rejects unclimbable | failure | Component |
| TC-0082 | MantleDetectionFallbackTooHighNotMantling | Actions | verifies mantle detection fallback rejects too-high obstacles | failure | Component |
| TC-0083 | BracedWallFullStateChain | Integration | verifies braced wall full state chain lifecycle | happy | Integration |
| TC-0084 | ClimbUpFullCycle | Integration | verifies ClimbUp full cycle from hang to locomotion | happy | Integration |
| TC-0085 | CornerTransitionCommittedState | Integration | verifies corner transition committed state constraints | happy | Integration |
| TC-0086 | IKWeightLifecycle | Integration | verifies IK weight lifecycle through state changes | happy | Integration |
| TC-0087 | LacheFullStateChain | Integration | verifies Lache full state chain lifecycle | happy | Integration |
| TC-0088 | LadderFullStateChain | Integration | verifies ladder full state chain lifecycle | happy | Integration |
| TC-0089 | MantleCommittedStateConstraints | Integration | verifies mantle committed state constraints | happy | Integration |
| TC-0090 | RagdollRecoveryCycle | Integration | verifies ragdoll recovery cycle lifecycle | happy | Integration |
| TC-0091 | FullCycleHangShimmyDrop | Integration | verifies full cycle hang-shimmy-drop | happy | Integration |
| TC-0092 | SurfaceSpeedMultiplierAffectsShimmy | Integration | verifies surface speed multiplier affects shimmy speed | happy | Integration |
| TC-0093 | AnimationSetAllSlotsHandledWithoutCrash | Animation | verifies all animation slots handled without crash | happy | Unit |
| TC-0094 | AnimationSetAssetIdReturnsValidId | Animation | verifies animation set returns valid asset ID | happy | Unit |
| TC-0095 | AnimationSetGetMontageDefaultReturnsNull | Animation | verifies GetMontageForSlot default returns null | happy | Unit |
| TC-0096 | AnimationSetGetMontageInvalidSlotReturnsNull | Animation | verifies GetMontageForSlot invalid slot returns null | failure | Unit |
| TC-0097 | AnimBlendNegativeClampedToZero | Animation | verifies negative blend value clamped to zero | edge | Unit |
| TC-0098 | AnimBlendOvershootClampedToTarget | Animation | verifies overshoot blend value clamped to target | edge | Unit |
| TC-0099 | AnimIKBlendImmediate | Animation | verifies IK blend immediate mode | happy | Unit |
| TC-0100 | AnimIKBlendStepRate | Animation | verifies IK blend step rate | happy | Unit |
| TC-0101 | AnimIKMaskNotifyGating | Animation | verifies IK mask notify gating | happy | Unit |
| TC-0102 | AnimIKNotifyMultipleLimbsIndependent | Animation | verifies multiple IK limbs are independent | happy | Unit |
| TC-0103 | AnimIKResetWeights | Animation | verifies IK weight reset | happy | Unit |
| TC-0104 | SoundNotifyDefaultValues | Audio | verifies sound notify default values | happy | Unit |
| TC-0105 | SoundNotifyNameNonEmpty | Audio | verifies sound notify name is non-empty | happy | Unit |
| TC-0106 | CharacterAudioDefaultMapEmpty | Audio | verifies character audio default map is empty | happy | Unit |
| TC-0107 | CharacterCameraDefaultValues | Camera | verifies character camera default values | happy | Unit |
| TC-0108 | CharacterDetectionDefaultValues | Detection | verifies character detection default values | happy | Unit |
| TC-0109 | CharacterFreefallDefaultValues | Freefall | verifies character freefall default values | happy | Unit |
| TC-0110 | CharacterIKDefaultValues | IK | verifies character IK default values | happy | Unit |
| TC-0111 | CharacterLacheDefaultValues | Lache | verifies character Lache default values | happy | Unit |
| TC-0112 | CharacterMantleDefaultValues | Mantle | verifies character mantle default values | happy | Unit |
| TC-0113 | CharacterMultiplayerDefaultValues | Multiplayer | verifies character multiplayer default values | happy | Unit |
| TC-0114 | CharacterPhysicsDefaultValues | Physics | verifies character physics default values | happy | Unit |
| TC-0115 | IKNotifyDisableDefaultIsBothHands | IKNotify | verifies disable IK notify defaults to both hands | happy | Unit |
| TC-0116 | IKNotifyEnableDefaultIsBothHands | IKNotify | verifies enable IK notify defaults to both hands | happy | Unit |
| TC-0117 | IKNotifyLimbMaskBitmaskCombinations | IKNotify | verifies IK notify limb mask bitmask combinations | happy | Unit |
| TC-0118 | IKNotifyNamesNonEmpty | IKNotify | verifies IK notify names are non-empty | happy | Unit |
| TC-0119 | InputAddClimbingRequiresLocalControl | Input | verifies add climbing IMC requires local control | happy | Component |
| TC-0120 | InputAddLocomotionRequiresLocalControl | Input | verifies add locomotion IMC requires local control | happy | Component |
| TC-0121 | InputPawnClientRestartClearsIMCFlags | Input | verifies PawnClientRestart clears IMC flags | happy | Component |
| TC-0122 | InputPawnClientRestartPreservesClimbingState | Input | verifies PawnClientRestart preserves climbing state | happy | Component |
| TC-0123 | InputRemoveClimbingRequiresLocalControl | Input | verifies remove climbing IMC requires local control | happy | Component |
| TC-0124 | InputRemoveLocomotionRequiresLocalControl | Input | verifies remove locomotion IMC requires local control | happy | Component |
| TC-0125 | MultiplayerAnchorIsReplicableType | Multiplayer | verifies AnchorComponent is a replicable type (TObjectPtr) | happy | Unit |
| TC-0126 | MultiplayerAnchorLocalTransformIsReplicated | Multiplayer | verifies AnchorLocalTransform is replicated | happy | Unit |
| TC-0127 | MultiplayerConfirmationTraceInvalidPayloadReturnsNull | Multiplayer | verifies confirmation trace returns null for invalid payload | failure | Unit |
| TC-0128 | MultiplayerConfirmationTraceResolvesHitComponent | Multiplayer | verifies confirmation trace resolves HitComponent | happy | Component |
| TC-0129 | MultiplayerDetectionResultIsReplicated | Multiplayer | verifies detection result is replicated | happy | Unit |
| TC-0130 | MultiplayerInputMappingContextPropertiesExist | Multiplayer | verifies input mapping context properties exist | happy | Unit |
| TC-0131 | MultiplayerNetStructNoWeakObjectProperties | Multiplayer | verifies net struct has no weak object properties | happy | Unit |
| TC-0132 | MultiplayerRepNotifyOnRepFunctionExists | Multiplayer | verifies OnRep function exists | happy | Unit |
| TC-0133 | MultiplayerRPCsClientFunctionsExist | Multiplayer | verifies Client_ RPC functions exist | happy | Unit |
| TC-0134 | MultiplayerRPCsServerFunctionsExist | Multiplayer | verifies Server_ RPC functions exist | happy | Unit |
| TC-0135 | MultiplayerStateHasRepNotify | Multiplayer | verifies climbing state has RepNotify | happy | Unit |
| TC-0136 | MultiplayerValidationToleranceDefault | Multiplayer | verifies server validation tolerance default value | happy | Unit |
| TC-0137 | PerformanceLadderSpeedCalculation | Performance | verifies ladder speed calculation performance | stress | Performance |
| TC-0138 | PerformanceShimmySpeedCalculation | Performance | verifies shimmy speed calculation performance | stress | Performance |
| TC-0139 | PerformanceStateTransitionValidation | Performance | verifies state transition validation performance | stress | Performance |
| TC-0140 | PerformanceDetectionResultConversion | Performance | verifies detection result conversion performance | stress | Performance |
| TC-0141 | LacheDirectionalInputBlendsLaunchDirection | Lache | verifies directional input blends Lache launch direction | happy | Component |
| TC-0142 | LacheInAirCatchesWithinRadius | Lache | verifies Lache in-air catches within radius | happy | Component |
| TC-0143 | LacheInAirInvalidTargetMisses | Lache | verifies Lache in-air invalid target triggers miss | failure | Component |
| TC-0144 | LacheNoTargetStaysHanging | Lache | verifies Lache with no target stays hanging | failure | Component |
| TC-0145 | LacheValidTargetTransitionsToLache | Lache | verifies Lache with valid target transitions to Lache state | happy | Component |
| TC-0146 | CharacterContractsClimbUpDetectionFallsBackToCached | Contracts | verifies ClimbUp detection falls back to cached result | edge | Unit |
| TC-0147 | CharacterContractsClimbUpDetectionPrefersFresh | Contracts | verifies ClimbUp detection prefers fresh result | happy | Unit |
| TC-0148 | CharacterContractsDefaultValues | Contracts | verifies character contract default values | happy | Unit |
| TC-0149 | CharacterContractsReplicationFlags | Contracts | verifies character replication flags | happy | Unit |
| TC-0150 | ArcEarlyExitStopsAtFirstValidLedge | Arc | verifies arc early exit stops at first valid ledge | happy | Unit |
| TC-0151 | ArcSampleCountMatchesParameter | Arc | verifies arc sample count matches parameter | happy | Unit |
| TC-0152 | ArcSamplePointsRenderOnlyWhenDebugEnabled | Arc | verifies arc sample points render only when debug enabled | happy | Unit |


---

## PLANNED — NOT YET IMPLEMENTED
<!-- Full entries. One entry per test. -->
<!-- Implementer reads this section to know what to build. -->

### TC-0153
- **Name:** LedgeDetectionSurfaceNormalPointsTowardCharacter
- **Registry String:** `ClimbingSystem.Detection.LedgeGrab.SurfaceNormal_PointsTowardCharacter`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::PerformLedgeDetection()
- **Behavior Tested:** The SurfaceNormal in the detection result points toward the character (positive dot product with vector from ledge to character).
- **Preconditions:** Character spawned facing +X; wall with ledge top placed in front.
- **Action:** Spawn wall, call PerformLedgeDetection(), compute dot product of Result.SurfaceNormal with (CharacterLocation - LedgePosition).GetSafeNormal2D(), assert dot > 0.
- **Expected Outcome:** Dot product > 0 (normal faces character).
- **Edge/Failure Condition:** happy path
- **Why This Matters:** IK hand placement and hang offset both depend on the surface normal pointing away from the wall toward the character.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0154
- **Name:** LedgeDetectionArcUsesActualVelocityWhenFalling
- **Registry String:** `ClimbingSystem.Detection.Arc.UsesActualVelocity_WhenFalling`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::PerformLedgeDetection() arc velocity selection
- **Behavior Tested:** When the character is falling, PerformLedgeDetection uses the actual velocity for the arc, allowing detection of ledges along the fall trajectory.
- **Preconditions:** Character spawned; ClimbingMovement.IsFalling() == true; velocity set to a known downward+forward vector; ledge placed along that trajectory.
- **Action:** Set character velocity to FVector(200, 0, -400), place ledge along that arc, call PerformLedgeDetection(), assert bValid == true.
- **Expected Outcome:** Result.bValid == true (ledge found along fall arc).
- **Edge/Failure Condition:** edge case
- **Why This Matters:** The arc velocity branch is the key mechanism for mid-air ledge catching.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Requires setting ClimbingMovement velocity directly; may need to call SetMovementMode(MOVE_Falling) first.

### TC-0155
- **Name:** LedgeDetectionAtLocationReturnsValidForReachableLedge
- **Registry String:** `ClimbingSystem.Detection.AtLocation.ReturnsValid_ForReachableLedge`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::PerformLedgeDetectionAtLocation()
- **Behavior Tested:** PerformLedgeDetectionAtLocation returns a valid result when a ledge exists within reach of the specified location.
- **Preconditions:** Character spawned; ledge box placed above the query location.
- **Action:** Spawn ledge 100cm above a query point, call PerformLedgeDetectionAtLocation(queryPoint), assert bValid == true.
- **Expected Outcome:** Result.bValid == true.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** PerformLedgeDetectionAtLocation is used for Lache arc targeting; a broken return would cause all Lache jumps to miss.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0156
- **Name:** LedgeDetectionAtLocationReturnsInvalidWhenNoGeometry
- **Registry String:** `ClimbingSystem.Detection.AtLocation.ReturnsInvalid_WhenNoGeometry`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::PerformLedgeDetectionAtLocation()
- **Behavior Tested:** PerformLedgeDetectionAtLocation returns bValid == false when no geometry exists near the query location.
- **Preconditions:** Character spawned in empty test world.
- **Action:** Call PerformLedgeDetectionAtLocation(FVector(1000, 1000, 1000)) in an empty world, assert bValid == false.
- **Expected Outcome:** Result.bValid == false.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** A false positive here would cause Lache to lock onto a phantom target.
- **Extends:** TC-0155
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0157
- **Name:** LedgeDetectionAtLocationSurfaceTierFromSurfaceData
- **Registry String:** `ClimbingSystem.Detection.AtLocation.SurfaceTier_ReadFromSurfaceData`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::PerformLedgeDetectionAtLocation()
- **Behavior Tested:** When the hit component has a SurfaceData: tag, PerformLedgeDetectionAtLocation reads SurfaceTier from the UClimbingSurfaceData asset.
- **Preconditions:** Character spawned; ledge box with SurfaceData: tag pointing to a LadderOnly asset.
- **Action:** Spawn ledge with SurfaceData: tag, call PerformLedgeDetectionAtLocation(), assert SurfaceTier == LadderOnly.
- **Expected Outcome:** Result.SurfaceTier == EClimbSurfaceTier::LadderOnly.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Lache could land on a LadderOnly surface and enter the wrong state if tier is not read from SurfaceData.
- **Extends:** TC-0155
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Requires a loadable UClimbingSurfaceData asset.

### TC-0158
- **Name:** ResolveHitComponentFromNetUsesConfirmationRadius
- **Registry String:** `ClimbingSystem.Detection.ConfirmationTrace.UsesConfiguredRadius`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::ResolveHitComponentFromNet()
- **Behavior Tested:** ResolveHitComponentFromNet succeeds within ConfirmationTraceRadius and fails just outside it.
- **Preconditions:** Character spawned; wall component placed at known offset from LedgePosition.
- **Action:** Place wall 15cm from LedgePosition (within 16cm radius), assert resolution succeeds. Place wall 20cm away, assert resolution fails.
- **Expected Outcome:** Resolves at 15cm; fails at 20cm.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** ConfirmationTraceRadius = 16cm is spec-mandated; wrong radius breaks simulated proxy IK.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0159
- **Name:** ResolveHitComponentFromNetRejectsZeroNormal
- **Registry String:** `ClimbingSystem.Detection.ConfirmationTrace.RejectsZeroNormal`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::ResolveHitComponentFromNet()
- **Behavior Tested:** ResolveHitComponentFromNet returns nullptr when SurfaceNormal is zero (degenerate net payload).
- **Preconditions:** Character spawned in test world.
- **Action:** Construct FClimbingDetectionResultNet with bValid=true but SurfaceNormal=FVector::ZeroVector, call ResolveHitComponentFromNet(), assert nullptr.
- **Expected Outcome:** Returns nullptr without crashing.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** A zero normal would produce a zero-length trace direction, causing undefined behavior.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0160
- **Name:** LedgeDetectionNoResultWhenNoGeometry
- **Registry String:** `ClimbingSystem.Detection.LedgeGrab.NoResult_WhenNoGeometry`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::PerformLedgeDetection()
- **Behavior Tested:** PerformLedgeDetection returns bValid == false in an empty world with no geometry.
- **Preconditions:** Character spawned in empty test world; no geometry spawned.
- **Action:** Call PerformLedgeDetection() with no geometry present, assert bValid == false.
- **Expected Outcome:** Result.bValid == false.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** A false positive in an empty world would indicate detection is hitting the character's own capsule.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0161
- **Name:** BracedWallDetectionIgnoresUnclimbableTag
- **Registry String:** `ClimbingSystem.Detection.BracedWall.RejectsUnclimbableTag`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::PerformBracedWallDetection()
- **Behavior Tested:** PerformBracedWallDetection returns invalid when the wall component is tagged Unclimbable.
- **Preconditions:** Character spawned; vertical wall tagged Unclimbable placed in front.
- **Action:** Spawn wall tagged Unclimbable, call PerformBracedWallDetection(), assert bValid == false.
- **Expected Outcome:** Result.bValid == false.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Braced wall detection must respect Unclimbable tag.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0162
- **Name:** BracedWallDetectionSetsCorrectSurfaceTier
- **Registry String:** `ClimbingSystem.Detection.BracedWall.SetsCorrectSurfaceTier`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::PerformBracedWallDetection()
- **Behavior Tested:** PerformBracedWallDetection sets SurfaceTier to Climbable when tagged Climbable, and Untagged when no tag present.
- **Preconditions:** Character spawned; two test runs with Climbable-tagged and untagged walls.
- **Action:** Run against Climbable-tagged wall, assert SurfaceTier == Climbable. Repeat with untagged wall, assert SurfaceTier == Untagged.
- **Expected Outcome:** Climbable tag -> Climbable tier; no tag -> Untagged tier.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Tier classification in braced wall detection drives downstream validation rules.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0163
- **Name:** LedgeDetectionMaxSurfaceAngleRejectsSlope
- **Registry String:** `ClimbingSystem.Detection.LedgeGrab.MaxSurfaceAngle_RejectsExcessiveSlope`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::PerformLedgeDetection()
- **Behavior Tested:** A ledge surface whose normal angle from vertical exceeds MaxClimbableSurfaceAngle is rejected.
- **Preconditions:** Character spawned; steeply sloped box (normal > 30° from up) at valid grab height.
- **Action:** Spawn box rotated 45°, set MaxClimbableSurfaceAngle = 30.0f, call PerformLedgeDetection(), assert bValid == false.
- **Expected Outcome:** Result.bValid == false.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** MaxClimbableSurfaceAngle prevents grabbing sloped roofs.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Spawning a rotated box requires FRotator in SpawnActor.

### TC-0164
- **Name:** LedgeDetectionMaxSurfaceAngleAcceptsWithinLimit
- **Registry String:** `ClimbingSystem.Detection.LedgeGrab.MaxSurfaceAngle_AcceptsWithinLimit`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::PerformLedgeDetection()
- **Behavior Tested:** A ledge surface whose normal angle from vertical is within MaxClimbableSurfaceAngle is accepted.
- **Preconditions:** Character spawned; gently sloped box (normal 15° from up) at valid grab height; MaxClimbableSurfaceAngle = 30.0f.
- **Action:** Spawn box rotated 15°, call PerformLedgeDetection(), assert bValid == true.
- **Expected Outcome:** Result.bValid == true.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Complements TC-0163; confirms angle filter accepts valid gentle slopes.
- **Extends:** TC-0163
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0165
- **Name:** LedgeDetectionArcSamplesMinimumThreeNoCrash
- **Registry String:** `ClimbingSystem.Traces.ArcSamples_MinimumThree_NoCrash`
- **File:** `Source/ClimbingSystem/Tests/ClimbingTraceTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::PerformLedgeDetection() arc loop
- **Behavior Tested:** Setting LedgeArcSamples to the minimum clamped value of 3 does not crash and still detects a nearby ledge.
- **Preconditions:** Character spawned; ledge box at valid grab height.
- **Action:** Set LedgeArcSamples = 3, spawn ledge, call PerformLedgeDetection(), assert no crash and bValid == true.
- **Expected Outcome:** No crash; result.bValid == true.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** The arc loop runs from s=1 to s=LedgeArcSamples; minimum of 3 must work correctly.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0166
- **Name:** LedgeDetectionHitComponentIsValidOnSuccess
- **Registry String:** `ClimbingSystem.Detection.LedgeGrab.HitComponent_ValidOnSuccess`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::PerformLedgeDetection()
- **Behavior Tested:** When PerformLedgeDetection returns bValid == true, HitComponent is a non-null valid weak pointer pointing to the detected surface component.
- **Preconditions:** Character spawned; ledge box at valid grab height.
- **Action:** Spawn ledge, call PerformLedgeDetection(), assert result.bValid == true and result.HitComponent.IsValid() == true.
- **Expected Outcome:** HitComponent is valid and matches the spawned component.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** HitComponent is used for IK target placement and clearance sweep ignore list.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0167
- **Name:** LedgeDetectionIgnoresCharacterOwnCapsule
- **Registry String:** `ClimbingSystem.Detection.LedgeGrab.IgnoresOwnCapsule`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::PerformLedgeDetection()
- **Behavior Tested:** PerformLedgeDetection does not return a hit on the character's own capsule component.
- **Preconditions:** Character spawned in empty test world.
- **Action:** Call PerformLedgeDetection() with no external geometry, assert bValid == false (own capsule not detected).
- **Expected Outcome:** Result.bValid == false.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** AddIgnoredActor(this) must be set on QueryParams; if missing, the character would grab its own capsule.
- **Extends:** TC-0160
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Functionally overlaps TC-0160 but explicitly targets the self-ignore contract.

### TC-0168
- **Name:** MantleHighMontageSelectedAboveThreshold
- **Registry String:** `ClimbingSystem.StateMachine.Montage.MantleHighAboveThreshold`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateEnter (Mantling case)
- **Behavior Tested:** MantleHigh montage is selected when MantleHeight > MantleLowMaxHeight.
- **Preconditions:** Character with MantleLowMaxHeight=100; detection result with LedgePosition.Z such that MantleHeight = 120; MantleHigh montage assigned.
- **Action:** Transition to Mantling with that detection result.
- **Expected Outcome:** The playing montage is MantleHigh.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Heights above MantleLowMaxHeight must use MantleHigh; wrong selection misaligns the motion warp target.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0169
- **Name:** CornerInsideLeftMontageSelected
- **Registry String:** `ClimbingSystem.StateMachine.Montage.CornerInsideLeftSelected`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::OnStateEnter (CornerTransition case)
- **Behavior Tested:** CornerInsideLeft montage is selected when bCurrentCornerIsInside=true and CommittedShimmyDir < 0.
- **Preconditions:** Character in Shimmying state; bCurrentCornerIsInside=true; CommittedShimmyDir=-1.0; CornerInsideLeft montage assigned.
- **Action:** Transition to CornerTransition.
- **Expected Outcome:** The playing montage is CornerInsideLeft.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** CornerTransition has 4 montage slots; each combination must select the correct one.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** bCurrentCornerIsInside is a replicated private field.

### TC-0170
- **Name:** CornerOutsideRightMontageSelected
- **Registry String:** `ClimbingSystem.StateMachine.Montage.CornerOutsideRightSelected`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::OnStateEnter (CornerTransition case)
- **Behavior Tested:** CornerOutsideRight montage is selected when bCurrentCornerIsInside=false and CommittedShimmyDir >= 0.
- **Preconditions:** Character in Shimmying state; bCurrentCornerIsInside=false; CommittedShimmyDir=1.0; CornerOutsideRight montage assigned.
- **Action:** Transition to CornerTransition.
- **Expected Outcome:** The playing montage is CornerOutsideRight.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Validates the outside-right branch of the 4-way corner montage selection.
- **Extends:** TC-0169
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0171
- **Name:** LadderTransitionTopMontageWhenValidDetection
- **Registry String:** `ClimbingSystem.StateMachine.Montage.LadderTransitionTopWhenValidDetection`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::OnStateEnter (LadderTransition case)
- **Behavior Tested:** LadderExitTop montage is selected when DetectionResult.bValid is true (exiting at top).
- **Preconditions:** Character in OnLadder state; LadderExitTop montage assigned; detection result with bValid=true.
- **Action:** Transition to LadderTransition with that detection result.
- **Expected Outcome:** The playing montage is LadderExitTop.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** LadderTransition uses bValid to distinguish top vs bottom exit.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0172
- **Name:** LadderTransitionBottomMontageWhenInvalidDetection
- **Registry String:** `ClimbingSystem.StateMachine.Montage.LadderTransitionBottomWhenInvalidDetection`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::OnStateEnter (LadderTransition case)
- **Behavior Tested:** LadderExitBottom montage is selected when DetectionResult.bValid is false (exiting at bottom).
- **Preconditions:** Character in OnLadder state; LadderExitBottom montage assigned; detection result with bValid=false.
- **Action:** Transition to LadderTransition with that detection result.
- **Expected Outcome:** The playing montage is LadderExitBottom.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Bottom exit is the fallback path; must select the correct montage.
- **Extends:** TC-0171
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0173
- **Name:** MontageSlotMismatchLogsWarning
- **Registry String:** `ClimbingSystem.StateMachine.Montage.SlotMismatchLogsWarning`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::OnStateEnter (PlayStateMontage)
- **Behavior Tested:** When Montage_Play returns <= 0 (slot name mismatch), a Warning log is emitted referencing ClimbingMontageSlot.
- **Preconditions:** Character with ClimbingMontageSlot set to a name that does not match any ABP slot node.
- **Action:** Transition to Hanging from None (triggers GrabLedge play attempt).
- **Expected Outcome:** A LogClimbing Warning message containing "ClimbingMontageSlot" is emitted.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Silent failure leaves the character frozen with no animation.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Use AddExpectedError to capture log output.

### TC-0174
- **Name:** PhysFlyingZeroesVelocityForHanging
- **Registry String:** `ClimbingSystem.Movement.PhysFlying.ZeroesVelocityForHanging`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementComponentTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** UClimbingMovementComponent::PhysFlying
- **Behavior Tested:** PhysFlying zeroes Velocity and returns early when state is Hanging.
- **Preconditions:** Component with CurrentClimbingState=Hanging; Velocity set to a non-zero value.
- **Action:** Call PhysFlying(0.016f, 0).
- **Expected Outcome:** Velocity == FVector::ZeroVector after the call.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** PhysFlying must suppress all movement for static climbing states.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Also covers BracedWall and OnLadder which share the same branch.

### TC-0175
- **Name:** PhysFlyingDoesNotZeroVelocityForShimmying
- **Registry String:** `ClimbingSystem.Movement.PhysFlying.DoesNotZeroVelocityForShimmying`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementComponentTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** UClimbingMovementComponent::PhysFlying
- **Behavior Tested:** PhysFlying calls Super (does not zero velocity) when state is Shimmying.
- **Preconditions:** Component with CurrentClimbingState=Shimmying; Velocity set to a non-zero lateral value.
- **Action:** Call PhysFlying(0.016f, 0).
- **Expected Outcome:** Velocity is not zeroed by the override.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Shimmying is not in the zero-velocity branch; zeroing it would stop lateral movement.
- **Extends:** TC-0174
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0176
- **Name:** CanAttemptJumpBlockedDuringClimbing
- **Registry String:** `ClimbingSystem.Movement.Jump.CanAttemptJumpBlockedDuringClimbing`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementComponentTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** UClimbingMovementComponent::CanAttemptJump
- **Behavior Tested:** CanAttemptJump returns false for every non-None climbing state.
- **Preconditions:** Component initialized.
- **Action:** Set CurrentClimbingState to each of the 16 non-None states; call CanAttemptJump each time.
- **Expected Outcome:** Returns false for all 16 non-None states.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Normal jump must be suppressed during all climbing states.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0177
- **Name:** DoJumpBlockedDuringClimbing
- **Registry String:** `ClimbingSystem.Movement.Jump.DoJumpBlockedDuringClimbing`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementComponentTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** UClimbingMovementComponent::DoJump
- **Behavior Tested:** DoJump returns false for any non-None climbing state.
- **Preconditions:** Component with CurrentClimbingState=Hanging.
- **Action:** Call DoJump(false, 0.016f).
- **Expected Outcome:** Returns false; Velocity is unchanged.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** DoJump must be independently blocked even if CanAttemptJump is bypassed.
- **Extends:** TC-0176
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0178
- **Name:** PackedMovementRPCsFalseForAttachedStates
- **Registry String:** `ClimbingSystem.Movement.RPCPolicy.FalseForAttachedStates`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementComponentTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** UClimbingMovementComponent::ShouldUsePackedMovementRPCs
- **Behavior Tested:** ShouldUsePackedMovementRPCs returns false for Hanging, Shimmying, BracedWall, BracedShimmying, and OnLadder.
- **Preconditions:** Component initialized.
- **Action:** Set CurrentClimbingState to each of the 5 states; call ShouldUsePackedMovementRPCs each time.
- **Expected Outcome:** Returns false for all 5 states.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Packed RPCs must be disabled for in-place climbing to prevent base class interference.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0179
- **Name:** AnchorInvalidationTriggersClearAnchor
- **Registry String:** `ClimbingSystem.Movement.Anchor.InvalidationTriggersClearAnchor`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementComponentTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** UClimbingMovementComponent::UpdateAnchorFollowing
- **Behavior Tested:** When AnchorComponent becomes invalid during tick, ClearAnchor is called and AnchorComponent is set to nullptr.
- **Preconditions:** Component in Hanging state with a valid AnchorComponent.
- **Action:** Destroy the anchor actor; call TickComponent(0.016f, ...).
- **Expected Outcome:** AnchorComponent == nullptr after the tick.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Stale pointer would crash on next dereference.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Use IsValid() not IsPendingKill().

### TC-0180
- **Name:** IMCAddedOnlyOnFirstClimbEntry
- **Registry String:** `ClimbingSystem.StateMachine.IMC.AddedOnlyOnFirstClimbEntry`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateEnter
- **Behavior Tested:** AddClimbingInputMappingContext is called only when PreviousClimbingState == None, not on re-entry from another climbing state.
- **Preconditions:** Character in Hanging state (already climbed once); transition to Shimmying then back to Hanging.
- **Action:** Transition Hanging→Shimmying→Hanging; count IMC add calls.
- **Expected Outcome:** AddClimbingInputMappingContext is not called again on the second Hanging entry.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Double-adding at higher priority would shadow locomotion inputs permanently.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0181
- **Name:** CornerAngleThresholdExactly30Accepted
- **Registry String:** `ClimbingSystem.Actions.Corner.AngleExactly30DegreesTriggersTransition`
- **File:** `Source/ClimbingSystem/Tests/ClimbingShimmyRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::PerformCornerDetection
- **Behavior Tested:** A surface angle difference of exactly 30° (CornerAngleThreshold) produces a valid corner result.
- **Preconditions:** CurrentNormal=(0,1,0); SideHit normal at exactly 30° from current; CornerAngleThreshold=30.0.
- **Action:** Call PerformCornerDetection with shimmy direction; side trace returns normal at 30°.
- **Expected Outcome:** Result.bValid == true; corner transition is triggered.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Source uses AngleDeg >= CornerAngleThreshold; exactly 30° must be accepted (>=, not >).
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0182
- **Name:** CornerAngleBelow30Rejected
- **Registry String:** `ClimbingSystem.Actions.Corner.AngleBelow30DegreesRejectsTransition`
- **File:** `Source/ClimbingSystem/Tests/ClimbingShimmyRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::PerformCornerDetection
- **Behavior Tested:** A surface angle difference of 29.9° (just below CornerAngleThreshold) produces an invalid corner result.
- **Preconditions:** CurrentNormal=(0,1,0); SideHit normal at 29.9° from current.
- **Action:** Call PerformCornerDetection.
- **Expected Outcome:** Result.bValid == false; no corner transition.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Boundary test for the >= comparison in corner detection.
- **Extends:** TC-0181
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0183
- **Name:** CornerPivotWarpTargetSetOnTransition
- **Registry String:** `ClimbingSystem.Actions.Corner.WarpTargetCornerPivotRegisteredOnTransition`
- **File:** `Source/ClimbingSystem/Tests/ClimbingShimmyRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::TransitionToState(CornerTransition)
- **Behavior Tested:** On entering CornerTransition state, WarpTarget_CornerPivot is registered on MotionWarpingComponent with the corner impact position.
- **Preconditions:** Hanging state; valid corner detection result; MotionWarping component present.
- **Action:** Trigger corner transition via TickHangingState with shimmy input and valid PerformCornerDetection result.
- **Expected Outcome:** AddOrUpdateWarpTargetFromLocation called with "WarpTarget_CornerPivot" and corner position.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Warp target name must match exactly for motion warping to work.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Verify for both inside and outside corner variants.

### TC-0184
- **Name:** CornerInsideMontageSelectedLeft
- **Registry String:** `ClimbingSystem.Actions.Corner.InsideCornerLeftMontageSelectedWhenShimmyingLeft`
- **File:** `Source/ClimbingSystem/Tests/ClimbingShimmyRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::TransitionToState(CornerTransition)
- **Behavior Tested:** When bCurrentCornerIsInside=true and CommittedShimmyDir=-1 (left), CornerInsideLeft montage is selected.
- **Preconditions:** bCurrentCornerIsInside=true; CommittedShimmyDir=-1; CornerInsideLeft montage assigned.
- **Action:** Transition to CornerTransition state.
- **Expected Outcome:** CornerInsideLeft montage is played.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Validates inside-left branch of 4-way corner montage selection.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0185
- **Name:** CornerOutsideMontageSelectedRight
- **Registry String:** `ClimbingSystem.Actions.Corner.OutsideCornerRightMontageSelectedWhenShimmyingRight`
- **File:** `Source/ClimbingSystem/Tests/ClimbingShimmyRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::TransitionToState(CornerTransition)
- **Behavior Tested:** When bCurrentCornerIsInside=false and CommittedShimmyDir=+1 (right), CornerOutsideRight montage is selected.
- **Preconditions:** bCurrentCornerIsInside=false; CommittedShimmyDir=1; CornerOutsideRight montage assigned.
- **Action:** Transition to CornerTransition state.
- **Expected Outcome:** CornerOutsideRight montage is played.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Validates outside-right branch of 4-way corner montage selection.
- **Extends:** TC-0184
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0186
- **Name:** BracedWallSetBaseOnEntry
- **Registry String:** `ClimbingSystem.Actions.BracedWall.SetBaseCalledWithAnchorOnEntry`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateEnter(BracedWall)
- **Behavior Tested:** SetBase(AnchorComponent) is called when entering BracedWall state.
- **Preconditions:** Character not in BracedWall; valid braced detection result with AnchorComponent set.
- **Action:** TransitionToState(BracedWall, DetectionResult).
- **Expected Outcome:** Character's base is set to AnchorComponent (GetMovementBase() == AnchorComponent).
- **Edge/Failure Condition:** happy path
- **Why This Matters:** SetBase is critical for moving platform support during braced states.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Verify via GetMovementBase().

### TC-0187
- **Name:** BracedWallSetBaseNullOnExit
- **Registry String:** `ClimbingSystem.Actions.BracedWall.SetBaseNullCalledOnExitBracedWall`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateExit(BracedWall)
- **Behavior Tested:** SetBase(nullptr) is called when exiting BracedWall state.
- **Preconditions:** Character in BracedWall state with AnchorComponent as base.
- **Action:** TransitionToState(Hanging, ...) or TransitionToState(DroppingDown, ...).
- **Expected Outcome:** GetMovementBase() == nullptr after exit.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Failure to clear base causes character to inherit platform velocity after dismount.
- **Extends:** TC-0186
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0188
- **Name:** BracedShimmySetBaseNullOnExit
- **Registry String:** `ClimbingSystem.Actions.BracedWall.SetBaseNullCalledOnExitBracedShimmying`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::OnStateExit(BracedShimmying)
- **Behavior Tested:** SetBase(nullptr) is called when exiting BracedShimmying state.
- **Preconditions:** Character in BracedShimmying state with AnchorComponent as base.
- **Action:** Release shimmy input; state transitions to BracedWall or Hanging.
- **Expected Outcome:** GetMovementBase() == nullptr after full exit from braced states.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** BracedShimmying also uses SetBase; must clear on exit.
- **Extends:** TC-0187
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0189
- **Name:** LipDetectionTriggersHangTransition
- **Registry String:** `ClimbingSystem.Actions.BracedWall.LipAboveTransitionsToHanging`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::TickBracedWallState / CheckForLipAbove
- **Behavior Tested:** When CheckForLipAbove returns true during BracedWall tick, state transitions to Hanging.
- **Preconditions:** Character in BracedWall state; CheckForLipAbove returns true with valid LedgeResult.
- **Action:** Call TickBracedWallState(DeltaTime).
- **Expected Outcome:** State transitions to Hanging; BracedToHang montage played.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** BracedToHang transition is the primary path from braced to hanging.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0190
- **Name:** LipDetectionDuringBracedShimmy
- **Registry String:** `ClimbingSystem.Actions.BracedWall.LipAboveDuringShimmyTransitionsToHanging`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::TickBracedShimmyingState / CheckForLipAbove
- **Behavior Tested:** CheckForLipAbove is also checked during BracedShimmying; finding a lip transitions to Hanging mid-shimmy.
- **Preconditions:** Character in BracedShimmying state; CheckForLipAbove returns true.
- **Action:** Call TickBracedShimmyingState(DeltaTime).
- **Expected Outcome:** State transitions to Hanging; shimmy stops.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Lip detection must work during braced shimmy, not just braced idle.
- **Extends:** TC-0189
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0191
- **Name:** LipDetectionBlockedAboveNoTransition
- **Registry String:** `ClimbingSystem.Actions.BracedWall.LipDetectionFailsWhenBlockedAbove`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::CheckForLipAbove
- **Behavior Tested:** When the upward trace hits geometry (blocked above), CheckForLipAbove returns false and no transition occurs.
- **Preconditions:** BracedWall state; upward trace hits a ceiling/overhang.
- **Action:** Call TickBracedWallState(DeltaTime) with upward trace blocked.
- **Expected Outcome:** CheckForLipAbove returns false; state remains BracedWall.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Blocked above means no lip; must not transition.
- **Extends:** TC-0189
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0192
- **Name:** LipSurfaceAngleTooSteepNoTransition
- **Registry String:** `ClimbingSystem.Actions.BracedWall.LipRejectedWhenSurfaceAngleTooSteep`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::CheckForLipAbove
- **Behavior Tested:** When the downward trace finds a surface but its angle exceeds MaxClimbableSurfaceAngle, CheckForLipAbove returns false.
- **Preconditions:** BracedWall state; upward trace clear; downward trace hits surface with steep normal.
- **Action:** Call CheckForLipAbove.
- **Expected Outcome:** Returns false; no transition to Hanging.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Steep surface must be rejected for lip detection.
- **Extends:** TC-0191
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0193
- **Name:** BracedWallNoShimmyWithoutInput
- **Registry String:** `ClimbingSystem.Actions.BracedWall.NoShimmyTransitionWithoutInput`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::TickBracedWallState
- **Behavior Tested:** BracedWall state does not transition to BracedShimmying when CurrentClimbMoveInput.X is zero.
- **Preconditions:** BracedWall state; CurrentClimbMoveInput = (0,0); no lip above.
- **Action:** Call TickBracedWallState(DeltaTime).
- **Expected Outcome:** State remains BracedWall; no transition.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** No input should not trigger shimmy.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0194
- **Name:** ShimmyDirectionNotUpdatedAtExactDeadzone
- **Registry String:** `ClimbingSystem.Actions.Shimmy.DirectionNotUpdatedWhenInputExactlyAtDeadzone`
- **File:** `Source/ClimbingSystem/Tests/ClimbingShimmyRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::Input_ClimbMove
- **Behavior Tested:** When |IA_ClimbMove.X| == ShimmyDirectionDeadzone exactly (0.1), CommittedShimmyDir is NOT updated (strictly greater than).
- **Preconditions:** CommittedShimmyDir = 1.0; input X = 0.1 (exactly at deadzone).
- **Action:** Call Input_ClimbMove with X=0.1.
- **Expected Outcome:** CommittedShimmyDir remains 1.0 (unchanged).
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Source uses FMath::Abs(X) > ShimmyDirectionDeadzone (strictly greater); 0.1 must NOT update.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0195
- **Name:** ShimmyDirectionUpdatedJustAboveDeadzone
- **Registry String:** `ClimbingSystem.Actions.Shimmy.DirectionUpdatedWhenInputJustAboveDeadzone`
- **File:** `Source/ClimbingSystem/Tests/ClimbingShimmyRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Input_ClimbMove
- **Behavior Tested:** When |IA_ClimbMove.X| is 0.101 (just above deadzone), CommittedShimmyDir IS updated.
- **Preconditions:** CommittedShimmyDir = 1.0; input X = -0.101.
- **Action:** Call Input_ClimbMove with X=-0.101.
- **Expected Outcome:** CommittedShimmyDir == -1.0.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Boundary test for the > comparison in shimmy direction update.
- **Extends:** TC-0194
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0196
- **Name:** CornerDetectionNoTransitionBelowAngle
- **Registry String:** `ClimbingSystem.Actions.Corner.NoCornerTransitionWhenAngleBelowThreshold`
- **File:** `Source/ClimbingSystem/Tests/ClimbingShimmyRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::TickShimmyingState
- **Behavior Tested:** When PerformCornerDetection returns invalid (angle below threshold), shimmy continues without corner transition.
- **Preconditions:** Shimmying state; PerformCornerDetection returns bValid=false.
- **Action:** Call TickShimmyingState(DeltaTime) with shimmy input.
- **Expected Outcome:** State remains Shimmying; no CornerTransition entered.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Shimmy must continue when no corner is detected.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0197
- **Name:** BracedShimmyReleaseReturnsToBracedWall
- **Registry String:** `ClimbingSystem.Actions.BracedShimmy.ReleaseInputReturnsToBracedWall`
- **File:** `Source/ClimbingSystem/Tests/ClimbingShimmyRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::TickBracedShimmyingState
- **Behavior Tested:** Releasing shimmy input (X=0) from BracedShimmying transitions back to BracedWall.
- **Preconditions:** BracedShimmying state; CurrentClimbMoveInput.X = 0.
- **Action:** Call TickBracedShimmyingState(DeltaTime).
- **Expected Outcome:** State transitions to BracedWall.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Releasing input must return to braced idle.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0198
- **Name:** CommittedShimmyDirReplicatedToProxies
- **Registry String:** `ClimbingSystem.Actions.Shimmy.CommittedShimmyDirReplicatedToSimulatedProxy`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** AClimbingCharacter (Replicated property CommittedShimmyDir)
- **Behavior Tested:** CommittedShimmyDir is replicated to simulated proxies so they select the correct shimmy montage slot.
- **Preconditions:** Listen-server with one client; client character in Shimmying state; CommittedShimmyDir = -1 on client.
- **Action:** Server receives shimmy direction update; simulated proxy on server is ticked.
- **Expected Outcome:** Server-side proxy CommittedShimmyDir == -1; ShimmyLeft montage selected on proxy.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** CommittedShimmyDir is marked UPROPERTY(Replicated).
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** YES
- **Added In Run:** 1
- **Notes:** None.

### TC-0199
- **Name:** bCurrentCornerIsInsideReplicatedToProxies
- **Registry String:** `ClimbingSystem.Actions.Corner.CornerInsideFlagReplicatedToProxy`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** AClimbingCharacter (Replicated property bCurrentCornerIsInside)
- **Behavior Tested:** bCurrentCornerIsInside is replicated so simulated proxies play the correct inside/outside corner montage.
- **Preconditions:** Listen-server; client enters CornerTransition with bCurrentCornerIsInside=true.
- **Action:** Server receives state transition; proxy is ticked.
- **Expected Outcome:** Server proxy bCurrentCornerIsInside == true; CornerInside montage selected.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** bCurrentCornerIsInside is marked UPROPERTY(Replicated).
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** YES
- **Added In Run:** 1
- **Notes:** None.

### TC-0200
- **Name:** ClimbUpRejectedFromBracedWallState
- **Registry String:** `ClimbingSystem.Actions.ClimbUp.ClimbUpRejectedFromBracedWallState`
- **File:** `Source/ClimbingSystem/Tests/ClimbingClimbUpRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::Input_ClimbUp
- **Behavior Tested:** ClimbUp input is rejected when character is in BracedWall state (only Hanging/Shimmying are valid).
- **Preconditions:** Character in BracedWall state.
- **Action:** Call Input_ClimbUp.
- **Expected Outcome:** State remains BracedWall; rejection logged; no transition.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Source guard: CurrentState != Hanging && CurrentState != Shimmying → return.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0201
- **Name:** RepositionMontageResetsDistanceCounter
- **Registry String:** `ClimbingSystem.Actions.Shimmy.DistanceAccumulatesFromZeroAfterReposition`
- **File:** `Source/ClimbingSystem/Tests/ClimbingShimmyRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::TickShimmyingState
- **Behavior Tested:** After reposition resets ContinuousShimmyDistance to 0, the next tick begins accumulating from 0.
- **Preconditions:** ContinuousShimmyDistance just crossed MaxContinuousShimmyDistance; reposition triggered.
- **Action:** Call TickShimmyingState a second time after reposition.
- **Expected Outcome:** ContinuousShimmyDistance == EffectiveSpeed * DeltaTime (accumulated from 0, not from cap).
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Distance counter must reset after reposition to prevent immediate re-trigger.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0202
- **Name:** BracedWallAnchorComponentNullSafe
- **Registry String:** `ClimbingSystem.Actions.BracedWall.NullAnchorComponentDoesNotCrashOnEntry`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::OnStateEnter(BracedWall)
- **Behavior Tested:** Entering BracedWall state when AnchorComponent is null does not crash.
- **Preconditions:** Detection result with HitComponent = nullptr; ClimbingMovement->AnchorComponent = nullptr.
- **Action:** TransitionToState(BracedWall, DetectionResult).
- **Expected Outcome:** No crash; state enters BracedWall; SetBase called with nullptr (no-op or safe).
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Defensive test for null-safety of SetBase path during network edge cases.
- **Extends:** TC-0186
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0203
- **Name:** ArcFinalPositionAtTotalArcTime
- **Registry String:** `ClimbingSystem.Arc.Position.FinalStepPositionAtTotalArcTime`
- **File:** `Source/ClimbingSystem/Tests/ClimbingArcTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Lache arc parameterization
- **Behavior Tested:** The final arc step position at t=LacheTotalArcTime (1.2s) matches the formula exactly.
- **Preconditions:** LaunchOrigin=(0,0,0), ForwardVector=(1,0,0), LacheLaunchSpeed=1200, GravityZ=-980.
- **Action:** Evaluate StepPos at t=1.2.
- **Expected Outcome:** StepPos = (1440, 0, -705.6) — X=1200*1.2, Z=0.5*(-980)*1.44.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Validates the arc endpoint; if wrong, the catch target position is wrong.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Pure math; use exact expected values derived from spec constants.

### TC-0204
- **Name:** LadderNonLadderTagRejected
- **Registry String:** `ClimbingSystem.Ladder.Entry.NonLadderTaggedActorRejectedOnEntry`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** Ladder entry validation
- **Behavior Tested:** A climbable surface without the LadderOnly tag does not trigger ladder state entry.
- **Preconditions:** Character near a climbable actor that lacks the LadderOnly tag.
- **Action:** Trigger ladder entry input while overlapping the non-ladder actor.
- **Expected Outcome:** Character does NOT enter Ladder state.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** LadderOnly tag gates ladder state; without tag check, any climbable surface would trigger ladder logic.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0205
- **Name:** RagdollRecoveryNotInterruptedEarly
- **Registry String:** `ClimbingSystem.Ragdoll.Recovery.InputBeforeTimerExpiryDoesNotTriggerGetUp`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterPhysicsAudioTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** Ragdoll recovery timer
- **Behavior Tested:** Player input sent before RagdollRecoveryTime (1.5s) expires does not prematurely trigger the get-up sequence.
- **Preconditions:** Character in ragdoll state; timer at 0.5s elapsed.
- **Action:** Send jump/move input at t=0.5s; observe state.
- **Expected Outcome:** Character remains in ragdoll; get-up does NOT begin until t=1.5s.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Recovery time must be enforced regardless of input.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0206
- **Name:** AnchorWorldSpaceWithRotatedComponent
- **Registry String:** `ClimbingSystem.Physics.Anchor.WorldSpaceCorrectWhenAnchorComponentIsRotated`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterPhysicsAudioTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** Dynamic anchor world-space computation
- **Behavior Tested:** World-space anchor position is correctly computed when AnchorComponent has a non-identity rotation.
- **Preconditions:** AnchorComponent rotated 90° around Z; AnchorLocalTransform has a local X offset.
- **Action:** Compute world-space = AnchorComponent->GetComponentTransform() * AnchorLocalTransform; compare to manual calculation.
- **Expected Outcome:** World-space position accounts for rotation; local X offset maps to world Y.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Rotation is the most common source of error in transform composition.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Use exact 90° rotation for deterministic expected values.

### TC-0207
- **Name:** SetBasePreservedAcrossHangingToShimmy
- **Registry String:** `ClimbingSystem.Physics.SetBase.BasePreservedWhenTransitioningHangingToShimmying`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterPhysicsAudioTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** SetBase lifecycle
- **Behavior Tested:** When transitioning from Hanging to Shimmying, SetBase remains set to the anchor (not cleared between states).
- **Preconditions:** Character in Hanging state with SetBase active on anchor.
- **Action:** Transition to Shimmying state; observe SetBase calls during transition.
- **Expected Outcome:** SetBase is never called with nullptr between Hanging exit and Shimmying entry.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** A brief nullptr SetBase between states would cause a one-frame detach on moving platforms.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0208
- **Name:** GrabBreakImpulseDirectionPreserved
- **Registry String:** `ClimbingSystem.Ragdoll.GrabBreak.LaunchDirectionMatchesImpulseDirection`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterPhysicsAudioTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** Ragdoll grab break impulse
- **Behavior Tested:** The ragdoll launch direction after grab break matches the direction of the incoming impulse.
- **Preconditions:** Character in Hanging state; grab active.
- **Action:** Apply 3000N impulse in direction (0,1,0); measure ragdoll velocity direction.
- **Expected Outcome:** Ragdoll velocity direction is (0,1,0) within angular tolerance.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Direction preservation ensures ragdoll flies away from the hit source.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Normalize velocity before comparing direction.

### TC-0209
- **Name:** MantleCommittedBlocksLacheInput
- **Registry String:** `ClimbingSystem.Mantle.CommittedState.LacheInputIgnoredDuringMantle`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMantleRuntimeTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** Mantle committed state input constraints
- **Behavior Tested:** Lache input sent while a mantle montage is playing is ignored.
- **Preconditions:** Character mid-MantleLow montage.
- **Action:** Send IA_Lache input during montage playback.
- **Expected Outcome:** Lache state is NOT entered; mantle montage plays to completion.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Committed mantle state must block all interrupting inputs.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0210
- **Name:** LadderClimbMoveYAxisOnly
- **Registry String:** `ClimbingSystem.Ladder.Input.XAxisInputIgnoredOnLadder`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** Ladder input mapping
- **Behavior Tested:** IA_ClimbMove X-axis input does not move the character horizontally while on a ladder; only Y-axis drives movement.
- **Preconditions:** Character on ladder mid-rung.
- **Action:** Send IA_ClimbMove with X=1.0, Y=0.0; observe character movement.
- **Expected Outcome:** Character does not move horizontally.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: IA_ClimbMove Y-axis drives ladder movement; X-axis must be suppressed.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0211
- **Name:** RagdollFaceDetectionAtZeroDotProduct
- **Registry String:** `ClimbingSystem.Ragdoll.FaceDetection.ZeroDotProductHandledGracefully`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterPhysicsAudioTests.cpp`
- **Type:** Component
- **Priority:** P3
- **System Under Test:** Ragdoll face-up/face-down detection
- **Behavior Tested:** When pelvis up-vector is exactly perpendicular to world up (dot product = 0), the system picks a deterministic face classification without crash.
- **Preconditions:** Pelvis bone oriented so up-vector = (1,0,0).
- **Action:** Query face detection with dot product = 0.
- **Expected Outcome:** System returns either face-up or face-down deterministically; get-up montage fires.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Zero dot product is a degenerate case; undefined behavior could cause random montage selection.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Document which classification is chosen at zero as the expected behavior.

### TC-0212
- **Name:** LacheArcTraceRadiusUsedForDetection
- **Registry String:** `ClimbingSystem.Arc.Trace.RadiusUsedForSphereTraceAtEachStep`
- **File:** `Source/ClimbingSystem/Tests/ClimbingArcTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** Lache arc trace
- **Behavior Tested:** The sphere trace at each arc step uses LacheArcTraceRadius (24) as the sphere radius.
- **Preconditions:** Character in Lache state; a blocker placed 25 units from arc step 6 (just outside radius).
- **Action:** Tick through arc; observe whether blocker triggers LacheMiss.
- **Expected Outcome:** Blocker at 25 units is NOT detected; LacheMiss does NOT fire.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** LacheArcTraceRadius=24; using wrong radius causes false positives or missed detections.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Also test blocker at exactly 24 units (should detect) to bracket the radius boundary.

### TC-0213
- **Name:** LadderOnlyTagRequiredForLadderState
- **Registry String:** `ClimbingSystem.Ladder.Entry.LadderOnlyTagRequiredForLadderStateEntry`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** Ladder entry validation
- **Behavior Tested:** Adding the LadderOnly tag to an actor at runtime enables ladder state entry on the next overlap.
- **Preconditions:** Actor without LadderOnly tag; tag added at runtime.
- **Action:** Add LadderOnly tag to actor; trigger ladder entry input again.
- **Expected Outcome:** Character now enters Ladder state.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Validates that the tag check is dynamic and not cached at spawn.
- **Extends:** TC-0204
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0214
- **Name:** AnchorNullMidBracedShimmyDrops
- **Registry String:** `ClimbingSystem.Physics.Anchor.NullAnchorDuringBracedShimmyTriggersDroppedDown`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterPhysicsAudioTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** Dynamic anchor null handling
- **Behavior Tested:** AnchorComponent becoming null during BracedShimmying state triggers DroppingDown immediately.
- **Preconditions:** Character in BracedShimmying state with valid AnchorComponent.
- **Action:** Set AnchorComponent to nullptr; tick once.
- **Expected Outcome:** Character transitions to DroppingDown; no crash or hang.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Spec: AnchorComponent null -> DroppingDown immediately.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Completes null-anchor coverage for all four anchor states.

### TC-0215
- **Name:** ServerValidationAcceptsAtToleranceBoundary
- **Registry String:** `ClimbingSystem.Multiplayer.Validation.AcceptsAtToleranceBoundary`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerContractTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** Server_AttemptGrab validation
- **Behavior Tested:** Grab at exactly ServerValidationPositionTolerance distance is accepted.
- **Preconditions:** Server with known tolerance value; client result at exact tolerance distance.
- **Action:** Call Server_AttemptGrab with client result at tolerance boundary.
- **Expected Outcome:** Grab accepted; state transitions to climbing.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Boundary test for server validation tolerance.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0216
- **Name:** ServerValidationRejectsAboveToleranceBoundary
- **Registry String:** `ClimbingSystem.Multiplayer.Validation.RejectsAboveToleranceBoundary`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerContractTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** Server_AttemptGrab validation
- **Behavior Tested:** Grab at tolerance+1 triggers Client_RejectStateTransition.
- **Preconditions:** Server with known tolerance value; client result at tolerance+1 distance.
- **Action:** Call Server_AttemptGrab with client result above tolerance.
- **Expected Outcome:** Grab rejected; Client_RejectStateTransition called.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Server must reject grabs beyond tolerance to prevent cheating.
- **Extends:** TC-0215
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0217
- **Name:** PredictionRollbackStopsMontage
- **Registry String:** `ClimbingSystem.Multiplayer.Rollback.StopsMontageOnRejection`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerContractTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** Client_RejectStateTransition
- **Behavior Tested:** Client_RejectStateTransition stops any playing climbing montage.
- **Preconditions:** Client with active climbing montage playing.
- **Action:** Call Client_RejectStateTransition.
- **Expected Outcome:** Montage is stopped.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Rollback must stop the predicted montage before playing GrabFail.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0218
- **Name:** PredictionRollbackPlaysGrabFailMontage
- **Registry String:** `ClimbingSystem.Multiplayer.Rollback.PlaysGrabFailMontage`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerContractTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** Client_RejectStateTransition
- **Behavior Tested:** GrabFail montage plays after Client_RejectStateTransition.
- **Preconditions:** Client with GrabFail montage assigned.
- **Action:** Call Client_RejectStateTransition.
- **Expected Outcome:** GrabFail montage is playing.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: rejection triggers GrabFail montage.
- **Extends:** TC-0217
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0219
- **Name:** PredictionRollbackBlendOutDuration
- **Registry String:** `ClimbingSystem.Multiplayer.Rollback.BlendOutIs0Point2Seconds`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerContractTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** PredictionRollbackBlendOut property
- **Behavior Tested:** PredictionRollbackBlendOut property equals 0.2f.
- **Preconditions:** Default character instance.
- **Action:** Read PredictionRollbackBlendOut value.
- **Expected Outcome:** Value == 0.2f.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: 0.2 recommended.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0220
- **Name:** OnRepClimbingStatePlaysEntryMontage
- **Registry String:** `ClimbingSystem.Multiplayer.OnRep.PlaysEntryMontageImmediately`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** OnRep_ClimbingState
- **Behavior Tested:** OnRep_ClimbingState plays entry montage for new state.
- **Preconditions:** Simulated proxy character.
- **Action:** Set CurrentClimbingState to Hanging; call OnRep_ClimbingState.
- **Expected Outcome:** Entry montage for Hanging is playing.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Simulated proxies must play correct montage on state replication.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0221
- **Name:** OnRepClimbingStateRunsConfirmationTrace
- **Registry String:** `ClimbingSystem.Multiplayer.OnRep.RunsConfirmationTraceImmediately`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** OnRep_ClimbingState
- **Behavior Tested:** OnRep_ClimbingState calls ResolveHitComponentFromNet synchronously (not deferred).
- **Preconditions:** Simulated proxy character with valid LastValidatedDetectionResult.
- **Action:** Call OnRep_ClimbingState.
- **Expected Outcome:** ResolveHitComponentFromNet is called during OnRep execution.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: confirmation trace runs immediately in OnRep, not deferred to next IK tick.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0222
- **Name:** BeginPlayLogsWarningPerNullSlot
- **Registry String:** `ClimbingSystem.Lifecycle.BeginPlay.LogsWarningPerNullAnimSlot`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::BeginPlay slot validation
- **Behavior Tested:** One Warning log emitted per null EClimbingAnimationSlot during BeginPlay.
- **Preconditions:** Character with some animation slots unassigned.
- **Action:** Call BeginPlay; count LogClimbing Warning messages.
- **Expected Outcome:** Warning count matches number of null slots.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: BeginPlay iterates every slot and logs Warning for each null.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0223
- **Name:** BeginPlayNoWarningWhenAllSlotsPopulated
- **Registry String:** `ClimbingSystem.Lifecycle.BeginPlay.NoWarningWhenAllSlotsPopulated`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::BeginPlay slot validation
- **Behavior Tested:** No Warning logs when all animation slots are assigned.
- **Preconditions:** Character with all animation slots assigned.
- **Action:** Call BeginPlay; count LogClimbing Warning messages.
- **Expected Outcome:** Zero warnings.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Confirms validation only fires for actual gaps.
- **Extends:** TC-0222
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0224
- **Name:** EndPlayClearsAllSixItems
- **Registry String:** `ClimbingSystem.Lifecycle.EndPlay.ClearsAllSixCleanupItems`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::EndPlay
- **Behavior Tested:** EndPlay performs all 6 cleanup steps: unregister IK, SetBase(nullptr), Montage_Stop, clear LacheTarget, restore physics, remove IMC.
- **Preconditions:** Character in active climbing state with all systems engaged.
- **Action:** Call EndPlay.
- **Expected Outcome:** All 6 cleanup items verified.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: EndPlay must perform all 6 cleanup steps.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0225
- **Name:** EndPlaySetBaseNullptr
- **Registry String:** `ClimbingSystem.Lifecycle.EndPlay.SetBaseNullptr`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::EndPlay
- **Behavior Tested:** EndPlay calls SetBase(nullptr) on the character movement component.
- **Preconditions:** Character with active SetBase.
- **Action:** Call EndPlay.
- **Expected Outcome:** GetMovementBase() == nullptr.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Stale base reference after EndPlay causes crash.
- **Extends:** TC-0224
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0226
- **Name:** EndPlayRestoresPhysics
- **Registry String:** `ClimbingSystem.Lifecycle.EndPlay.RestoresPhysics`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::EndPlay
- **Behavior Tested:** EndPlay restores physics simulation state to pre-climb defaults.
- **Preconditions:** Character with mesh simulating physics (ragdoll state).
- **Action:** Call EndPlay.
- **Expected Outcome:** Mesh is no longer simulating physics.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: if mesh simulating physics → SetSimulatePhysics(false).
- **Extends:** TC-0224
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0227
- **Name:** EndPlayRemovesIMC
- **Registry String:** `ClimbingSystem.Lifecycle.EndPlay.RemovesClimbingIMC`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::EndPlay
- **Behavior Tested:** EndPlay removes ClimbingIMC from the Enhanced Input subsystem.
- **Preconditions:** Character with ClimbingIMC active.
- **Action:** Call EndPlay.
- **Expected Outcome:** ClimbingIMC is no longer in the active mapping stack.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: remove ClimbingInputMappingContext in EndPlay.
- **Extends:** TC-0224
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0228
- **Name:** EndPlayClearsLacheTarget
- **Registry String:** `ClimbingSystem.Lifecycle.EndPlay.ClearsLacheTarget`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::EndPlay
- **Behavior Tested:** EndPlay sets LacheTarget to null/invalid.
- **Preconditions:** Character with active LockedLacheTarget.
- **Action:** Call EndPlay.
- **Expected Outcome:** LockedLacheTarget is cleared.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: clear LockedLacheTarget in EndPlay.
- **Extends:** TC-0224
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0229
- **Name:** DestroyedAlsoUnregistersFromIKManager
- **Registry String:** `ClimbingSystem.Lifecycle.Destroyed.UnregistersFromIKManager`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Destroyed
- **Behavior Tested:** Destroyed callback removes character from ActiveClimbingCharacters independently of EndPlay.
- **Preconditions:** Character registered in ActiveClimbingCharacters.
- **Action:** Call Destroyed.
- **Expected Outcome:** Character is no longer in ActiveClimbingCharacters.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Dual-unregister handles multiplayer edge cases where EndPlay may not fire.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0230
- **Name:** ClimbingIMCAddedOnClimbEntry
- **Registry String:** `ClimbingSystem.Input.IMC.AddedAtClimbEntry`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** Input Mapping Context management
- **Behavior Tested:** ClimbingIMC is pushed at ClimbingIMCPriority when state transitions from None to any climbing state.
- **Preconditions:** Character in None state with ClimbingIMC assigned.
- **Action:** Transition to Hanging.
- **Expected Outcome:** ClimbingIMC is in the active mapping stack at ClimbingIMCPriority.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: AddMappingContext on climbing entry.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0231
- **Name:** ClimbingIMCRemovedOnReturnToNone
- **Registry String:** `ClimbingSystem.Input.IMC.RemovedOnReturnToNone`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** Input Mapping Context management
- **Behavior Tested:** ClimbingIMC is removed when climbing state returns to None.
- **Preconditions:** Character in Hanging state with ClimbingIMC active.
- **Action:** Transition to None.
- **Expected Outcome:** ClimbingIMC is no longer in the active mapping stack.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: RemoveMappingContext on return to None.
- **Extends:** TC-0230
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0232
- **Name:** ClimbingIMCPriorityValue
- **Registry String:** `ClimbingSystem.Input.IMC.PriorityValueIsCorrect`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Input Mapping Context management
- **Behavior Tested:** ClimbingIMC is added at the configured ClimbingIMCPriority value, not a hardcoded default.
- **Preconditions:** Character with ClimbingIMCPriority = 1.
- **Action:** Transition to climbing; verify IMC priority.
- **Expected Outcome:** IMC added at priority 1.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Priority must be configurable per spec.
- **Extends:** TC-0230
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0233
- **Name:** IMCNotAddedForNonLocalController
- **Registry String:** `ClimbingSystem.Input.IMC.NotAddedForNonLocalController`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** Input Mapping Context management
- **Behavior Tested:** IMC push is skipped when the controller is not locally controlled (simulated proxy).
- **Preconditions:** Character with non-local controller.
- **Action:** Transition to climbing.
- **Expected Outcome:** ClimbingIMC is NOT added.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** IMC is client-only; adding on server would be a no-op at best, crash at worst.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0234
- **Name:** IdleVariationDelayTimerFires
- **Registry String:** `ClimbingSystem.Anim.IdleVariation.DelayTimerFires`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimationEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Idle variation system
- **Behavior Tested:** Idle variation montage does not play before the configured delay timer elapses.
- **Preconditions:** Character in Hanging state; IdleVariationDelay = 5.0s.
- **Action:** Tick for 3 seconds; verify no variation montage plays.
- **Expected Outcome:** No idle variation montage playing at t=3s.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: idle variation plays after IdleVariationDelay seconds.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0235
- **Name:** IdleVariationNoRepeatSameVariation
- **Registry String:** `ClimbingSystem.Anim.IdleVariation.NoRepeatSameVariation`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimationEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Idle variation system
- **Behavior Tested:** Idle variation selection never picks the same variation twice in a row when bPreventConsecutiveVariationRepeat is true.
- **Preconditions:** bPreventConsecutiveVariationRepeat = true; HangIdleVariations has 3+ entries.
- **Action:** Trigger idle variation 10 times; record selected indices.
- **Expected Outcome:** No two consecutive indices are the same.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: bPreventConsecutiveVariationRepeat excludes the last-played index from the pool.
- **Extends:** TC-0234
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0236
- **Name:** IdleVariationResetsTimerAfterPlay
- **Registry String:** `ClimbingSystem.Anim.IdleVariation.ResetsTimerAfterPlay`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimationEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Idle variation system
- **Behavior Tested:** After an idle variation plays, the delay timer resets for the next cycle.
- **Preconditions:** Character in Hanging state; idle variation just played.
- **Action:** Verify timer resets to 0 after variation plays.
- **Expected Outcome:** Next variation does not play until IdleVariationDelay elapses again.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Timer must reset to prevent rapid-fire variations.
- **Extends:** TC-0234
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0237
- **Name:** BlendTimeZeroIsImmediate
- **Registry String:** `ClimbingSystem.Anim.Blend.ZeroBlendTimeIsImmediate`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimInstanceTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** UClimbingAnimInstance blend system
- **Behavior Tested:** A blend time of exactly 0.0f results in an immediate weight change with no interpolation.
- **Preconditions:** IK weight at 0.0; target weight 1.0; blend time 0.0.
- **Action:** Call blend update.
- **Expected Outcome:** Weight immediately equals 1.0 after one update.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Zero blend time must be handled as immediate, not divide-by-zero.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0238
- **Name:** DebugDrawShippingGuardPreventsExecution
- **Registry String:** `ClimbingSystem.Debug.ShippingGuard.DrawCallSkippedInShipping`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterDetectionCameraTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** Debug visualization shipping guard
- **Behavior Tested:** Debug draw code inside #if !UE_BUILD_SHIPPING is not reachable in shipping builds.
- **Preconditions:** Shipping build configuration.
- **Action:** Verify debug draw functions are compiled out.
- **Expected Outcome:** No debug draw calls in shipping.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: all drawing in #if !UE_BUILD_SHIPPING.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** This is a compile-time check; test verifies the guard exists in source.

### TC-0239
- **Name:** DebugDrawOnlyWhenFlagSet
- **Registry String:** `ClimbingSystem.Debug.bDrawDebug.DrawOnlyWhenEnabled`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterDetectionCameraTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Debug visualization flag
- **Behavior Tested:** Debug visualization only executes when bDrawDebug is true.
- **Preconditions:** bDrawDebug = false.
- **Action:** Tick character; verify no debug draw calls.
- **Expected Outcome:** No debug primitives submitted.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Debug draw must be gated by bDrawDebug flag.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0240
- **Name:** EditorLacheArcGatedByIsSelected
- **Registry String:** `ClimbingSystem.Debug.LacheArc.GatedByIsSelected`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterDetectionCameraTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Editor Lache arc preview
- **Behavior Tested:** Editor Lache arc preview only draws when the actor IsSelected() returns true.
- **Preconditions:** Editor build; bDrawDebug = true; actor not selected.
- **Action:** Tick character.
- **Expected Outcome:** No Lache arc drawn when not selected.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: IsSelected() gates the draw to the selected actor only.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0241
- **Name:** SimulatedProxyIKWeightsZeroedBeyondCullDistance
- **Registry String:** `ClimbingSystem.Multiplayer.SimulatedProxy.IKZeroedBeyondCullDistance`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** Simulated proxy IK culling
- **Behavior Tested:** Simulated proxy beyond SimulatedProxyIKCullDistance has all IK weights zeroed.
- **Preconditions:** Simulated proxy at distance > 1500cm from camera.
- **Action:** Update IK for proxy.
- **Expected Outcome:** All 4 IK weights == 0.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: SimulatedProxyIKCullDistance = 1500cm.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0242
- **Name:** OnRepClimbingStateNoOpForNoneState
- **Registry String:** `ClimbingSystem.Multiplayer.OnRep.NoOpForNoneState`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** OnRep_ClimbingState
- **Behavior Tested:** OnRep_ClimbingState does not play a montage or run a confirmation trace when new state is None.
- **Preconditions:** Simulated proxy; CurrentClimbingState set to None.
- **Action:** Call OnRep_ClimbingState.
- **Expected Outcome:** No montage played; no confirmation trace run.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** None state should not trigger any climbing behavior on proxies.
- **Extends:** TC-0220
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0243
- **Name:** ServerValidationZeroToleranceRejectsAnyOffset
- **Registry String:** `ClimbingSystem.Multiplayer.Validation.ZeroToleranceRejectsAnyOffset`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerContractTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Server_AttemptGrab validation
- **Behavior Tested:** With ServerValidationPositionTolerance=0, any non-zero client offset is rejected.
- **Preconditions:** ServerValidationPositionTolerance = 0; client result offset by 1cm.
- **Action:** Call Server_AttemptGrab.
- **Expected Outcome:** Grab rejected.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Zero tolerance is the strictest setting; must reject any offset.
- **Extends:** TC-0216
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0244
- **Name:** RollbackRestoresClimbingStateToNone
- **Registry String:** `ClimbingSystem.Multiplayer.Rollback.RestoresStateToNone`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerContractTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** Client_RejectStateTransition
- **Behavior Tested:** After Client_RejectStateTransition, local ClimbingState reverts to None.
- **Preconditions:** Client in predicted Hanging state.
- **Action:** Call Client_RejectStateTransition.
- **Expected Outcome:** CurrentClimbingState == None.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Rollback must restore to None to prevent stuck climbing state.
- **Extends:** TC-0217
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0245
- **Name:** IKWeightHandLeftIsBlueprintReadWrite
- **Registry String:** `ClimbingSystem.IK.Properties.HandLeftIsBlueprintReadWrite`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimInstanceTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** UClimbingAnimInstance IK weight properties
- **Behavior Tested:** IKWeightHandLeft on UClimbingAnimInstance has BlueprintReadWrite specifier.
- **Preconditions:** UClimbingAnimInstance class exists.
- **Action:** Reflect on IKWeightHandLeft property flags.
- **Expected Outcome:** Property has BlueprintReadWrite flag.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: all IK weights BlueprintReadWrite.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0246
- **Name:** IKWeightHandRightIsBlueprintReadWrite
- **Registry String:** `ClimbingSystem.IK.Properties.HandRightIsBlueprintReadWrite`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimInstanceTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** UClimbingAnimInstance IK weight properties
- **Behavior Tested:** IKWeightHandRight on UClimbingAnimInstance has BlueprintReadWrite specifier.
- **Preconditions:** UClimbingAnimInstance class exists.
- **Action:** Reflect on IKWeightHandRight property flags.
- **Expected Outcome:** Property has BlueprintReadWrite flag.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: all IK weights BlueprintReadWrite.
- **Extends:** TC-0245
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0247
- **Name:** IKWeightFootLeftIsBlueprintReadWrite
- **Registry String:** `ClimbingSystem.IK.Properties.FootLeftIsBlueprintReadWrite`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimInstanceTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** UClimbingAnimInstance IK weight properties
- **Behavior Tested:** IKWeightFootLeft on UClimbingAnimInstance has BlueprintReadWrite specifier.
- **Preconditions:** UClimbingAnimInstance class exists.
- **Action:** Reflect on IKWeightFootLeft property flags.
- **Expected Outcome:** Property has BlueprintReadWrite flag.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: all IK weights BlueprintReadWrite.
- **Extends:** TC-0245
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0248
- **Name:** IKWeightFootRightIsBlueprintReadWrite
- **Registry String:** `ClimbingSystem.IK.Properties.FootRightIsBlueprintReadWrite`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimInstanceTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** UClimbingAnimInstance IK weight properties
- **Behavior Tested:** IKWeightFootRight on UClimbingAnimInstance has BlueprintReadWrite specifier.
- **Preconditions:** UClimbingAnimInstance class exists.
- **Action:** Reflect on IKWeightFootRight property flags.
- **Expected Outcome:** Property has BlueprintReadWrite flag.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: all IK weights BlueprintReadWrite.
- **Extends:** TC-0245
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0249
- **Name:** IKManagerRegistersOnBeginPlay
- **Registry String:** `ClimbingSystem.IK.Manager.RegistersOnBeginPlay`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** IK manager static array
- **Behavior Tested:** Character is added to ActiveClimbingCharacters during BeginPlay.
- **Preconditions:** Empty ActiveClimbingCharacters array.
- **Action:** Spawn character; call BeginPlay.
- **Expected Outcome:** ActiveClimbingCharacters contains the character.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: register in BeginPlay.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0250
- **Name:** IKManagerEmptyAfterAllUnregister
- **Registry String:** `ClimbingSystem.IK.Manager.EmptyAfterAllUnregister`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** IK manager static array
- **Behavior Tested:** ActiveClimbingCharacters is empty after all registered characters call EndPlay.
- **Preconditions:** Multiple characters registered.
- **Action:** Call EndPlay on all characters.
- **Expected Outcome:** ActiveClimbingCharacters is empty.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Stale entries in the array would cause crashes on next IK update.
- **Extends:** TC-0249
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0251
- **Name:** IKManagerStaleWeakPtrsPurgedOnUpdate
- **Registry String:** `ClimbingSystem.IK.Manager.StaleWeakPtrsPurged`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** IK manager static array
- **Behavior Tested:** Expired weak pointers are removed from ActiveClimbingCharacters during the next update pass.
- **Preconditions:** Character registered; then destroyed without calling EndPlay.
- **Action:** Trigger IK manager update.
- **Expected Outcome:** Stale entry is purged.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Weak pointer cleanup prevents accessing destroyed objects.
- **Extends:** TC-0250
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0252
- **Name:** AudioSoundMapKeyCoversAllSoundTypes
- **Registry String:** `ClimbingSystem.Audio.SoundMap.CoversAllEClimbSoundTypes`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSoundNotifyTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Audio sound map
- **Behavior Tested:** ClimbingSounds TMap can hold an entry for every EClimbSoundType enum value without collision.
- **Preconditions:** Default character instance.
- **Action:** Add one entry per EClimbSoundType; verify count matches enum count.
- **Expected Outcome:** Map size equals EClimbSoundType enum count.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Ensures no enum hash collisions in the sound map.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0253
- **Name:** AudioNullSoftPtrDoesNotDispatchLoad
- **Registry String:** `ClimbingSystem.Audio.AsyncLoad.NullSoftPtrSkipsLoad`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSoundNotifyTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** Audio async loading
- **Behavior Tested:** A null TSoftObjectPtr in ClimbingSounds does not trigger an async load attempt.
- **Preconditions:** ClimbingSounds has entry with null TSoftObjectPtr for HandGrab.
- **Action:** Request sound for HandGrab.
- **Expected Outcome:** No async load dispatched; null cached; subsequent calls skip.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Spec: missing entries log Warning once, cache null, never retry.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0254
- **Name:** CameraProbeRadiusDefaultValue
- **Registry String:** `ClimbingSystem.Camera.ProbeRadius.DefaultValue`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterDetectionCameraTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Camera system defaults
- **Behavior Tested:** ClimbingCameraProbeRadius has a positive non-zero default value.
- **Preconditions:** Default character instance.
- **Action:** Read ClimbingCameraProbeRadius.
- **Expected Outcome:** Value > 0.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Zero probe radius would cause camera clipping through geometry.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0255
- **Name:** CameraNudgeBlendSpeedPositive
- **Registry String:** `ClimbingSystem.Camera.Nudge.BlendSpeedIsPositive`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterDetectionCameraTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Camera nudge system
- **Behavior Tested:** CameraNudgeBlendSpeed default value is greater than zero.
- **Preconditions:** Default character instance.
- **Action:** Read CameraNudgeBlendSpeed.
- **Expected Outcome:** Value > 0.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Zero blend speed would prevent camera nudge from ever completing.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0256
- **Name:** EndPlayMontageStopCalled
- **Registry String:** `ClimbingSystem.Lifecycle.EndPlay.MontageStopCalled`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::EndPlay
- **Behavior Tested:** EndPlay calls Montage_Stop on the AnimInstance to halt any playing montage.
- **Preconditions:** Character with active climbing montage.
- **Action:** Call EndPlay.
- **Expected Outcome:** No montage playing after EndPlay.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: Montage_Stop(0.0f) in EndPlay.
- **Extends:** TC-0224
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0257
- **Name:** EndPlayIdempotentOnDoubleCall
- **Registry String:** `ClimbingSystem.Lifecycle.EndPlay.IdempotentOnDoubleCall`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::EndPlay
- **Behavior Tested:** Calling EndPlay twice does not crash or corrupt state.
- **Preconditions:** Character that has already had EndPlay called.
- **Action:** Call EndPlay a second time.
- **Expected Outcome:** No crash; no state corruption.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Multiplayer edge cases may trigger EndPlay multiple times.
- **Extends:** TC-0224
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0258
- **Name:** PawnClientRestartDoesNotAddIMCTwice
- **Registry String:** `ClimbingSystem.Input.Runtime.PawnClientRestart.DoesNotAddIMCTwice`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** PawnClientRestart IMC handling
- **Behavior Tested:** PawnClientRestart does not result in duplicate IMC entries if called while climbing.
- **Preconditions:** Character in climbing state with ClimbingIMC already active.
- **Action:** Call PawnClientRestart.
- **Expected Outcome:** ClimbingIMC appears exactly once in the mapping stack.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Duplicate IMC entries would shadow other contexts.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0259
- **Name:** ClimbingIMCNotPresentAfterLocomotionRestore
- **Registry String:** `ClimbingSystem.Input.IMC.AbsentAfterLocomotionRestore`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** Input Mapping Context management
- **Behavior Tested:** After locomotion IMC is restored, ClimbingIMC is no longer in the active mapping stack.
- **Preconditions:** Character returned to None state from climbing.
- **Action:** Verify IMC stack.
- **Expected Outcome:** ClimbingIMC absent; locomotion IMC present.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Stale ClimbingIMC would shadow locomotion inputs.
- **Extends:** TC-0231
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0260
- **Name:** AllAnimationSlotsEnumeratedInBeginPlay
- **Registry String:** `ClimbingSystem.Anim.BeginPlay.AllSlotsEnumerated`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimationEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::BeginPlay slot validation
- **Behavior Tested:** BeginPlay iterates every value of EClimbingAnimationSlot exactly once.
- **Preconditions:** Character with known slot count.
- **Action:** Count iterations during BeginPlay validation.
- **Expected Outcome:** Iteration count equals EClimbingAnimationSlot enum count.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Missing a slot in validation means a null montage won't be caught.
- **Extends:** TC-0222
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0261
- **Name:** NullMontageSlotDoesNotCrashOnPlay
- **Registry String:** `ClimbingSystem.Anim.MontageSlot.NullDoesNotCrash`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimationEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** GetMontageForSlot null handling
- **Behavior Tested:** Attempting to play a montage for a null slot logs a warning and returns without crashing.
- **Preconditions:** Character with a null montage slot.
- **Action:** Call GetMontageForSlot for the null slot; attempt to play it.
- **Expected Outcome:** No crash; warning logged.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Null montage must be handled gracefully, not crash.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0262
- **Name:** IKBlendWeightClampedBetweenZeroAndOne
- **Registry String:** `ClimbingSystem.Anim.IKBlend.WeightClampedZeroToOne`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimInstanceTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** UClimbingAnimInstance IK blend
- **Behavior Tested:** IK blend weight is always clamped to [0.0, 1.0] regardless of input delta.
- **Preconditions:** IK weight at 0.9; large positive delta applied.
- **Action:** Apply delta that would push weight to 1.5.
- **Expected Outcome:** Weight clamped to 1.0.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Unclamped weights cause visual artifacts in the animation blueprint.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0263
- **Name:** DebugDrawFalseProducesNoDrawCalls
- **Registry String:** `ClimbingSystem.Debug.bDrawDebug.FalseProducesNoDrawCalls`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterDetectionCameraTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Debug visualization flag
- **Behavior Tested:** When bDrawDebug is false, no debug draw primitives are submitted.
- **Preconditions:** bDrawDebug = false; character ticking.
- **Action:** Tick character; count debug draw calls.
- **Expected Outcome:** Zero debug draw calls.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Debug draw must be fully gated.
- **Extends:** TC-0239
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0264
- **Name:** ServerRPCFunctionsAreReliable
- **Registry String:** `ClimbingSystem.Multiplayer.RPCs.ServerFunctionsAreReliable`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerContractTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Server RPC metadata
- **Behavior Tested:** Server_ prefixed RPC functions have the Reliable specifier in their UFUNCTION metadata.
- **Preconditions:** AClimbingCharacter class exists.
- **Action:** Reflect on Server_ function metadata.
- **Expected Outcome:** All Server_ RPCs are Reliable.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Unreliable climbing RPCs would cause state desync.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0265
- **Name:** ClientRPCFunctionsAreReliable
- **Registry String:** `ClimbingSystem.Multiplayer.RPCs.ClientFunctionsAreReliable`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerContractTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Client RPC metadata
- **Behavior Tested:** Client_ prefixed RPC functions have the Reliable specifier.
- **Preconditions:** AClimbingCharacter class exists.
- **Action:** Reflect on Client_ function metadata.
- **Expected Outcome:** All Client_ RPCs are Reliable.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Unreliable rejection RPCs would leave clients in wrong state.
- **Extends:** TC-0264
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0266
- **Name:** IdleVariationNotPlayedWhileActivelyClimbing
- **Registry String:** `ClimbingSystem.Anim.IdleVariation.NotPlayedWhileMoving`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimationEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Idle variation system
- **Behavior Tested:** Idle variation timer does not fire while the character is actively moving on a surface.
- **Preconditions:** Character in Shimmying state (actively moving).
- **Action:** Wait for IdleVariationDelay; verify no variation plays.
- **Expected Outcome:** No idle variation montage during active movement.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Idle variations should only play during idle states.
- **Extends:** TC-0234
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0267
- **Name:** MontageSlotDefaultIsNullForNewAnimSet
- **Registry String:** `ClimbingSystem.Anim.MontageSlot.DefaultNullForNewSet`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimationSetTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** UClimbingAnimationSet
- **Behavior Tested:** A freshly constructed animation set returns null for every slot query.
- **Preconditions:** Newly constructed UClimbingAnimationSet.
- **Action:** Query every slot.
- **Expected Outcome:** All slots return null.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Default null ensures per-slot fallback to character defaults works correctly.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0268
- **Name:** BeginPlayWarningCountMatchesNullSlotCount
- **Registry String:** `ClimbingSystem.Lifecycle.BeginPlay.WarningCountMatchesNullSlots`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::BeginPlay slot validation
- **Behavior Tested:** Number of Warning logs equals the exact count of null slots, not more or fewer.
- **Preconditions:** Character with exactly 5 null slots.
- **Action:** Call BeginPlay; count warnings.
- **Expected Outcome:** Exactly 5 warnings.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Over-counting or under-counting would mislead developers.
- **Extends:** TC-0222
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0269
- **Name:** EndPlayWithNoIMCDoesNotCrash
- **Registry String:** `ClimbingSystem.Lifecycle.EndPlay.NoIMCDoesNotCrash`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::EndPlay
- **Behavior Tested:** EndPlay does not crash when ClimbingIMC was never added (e.g., non-local controller).
- **Preconditions:** Character that never entered climbing (IMC never pushed).
- **Action:** Call EndPlay.
- **Expected Outcome:** No crash.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Non-local controllers never push IMC; EndPlay must handle this gracefully.
- **Extends:** TC-0224
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0270
- **Name:** DestroyedCalledWithoutBeginPlayDoesNotCrash
- **Registry String:** `ClimbingSystem.Lifecycle.Destroyed.WithoutBeginPlayDoesNotCrash`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Destroyed
- **Behavior Tested:** Destroyed callback on a character that never completed BeginPlay does not crash or corrupt the IK manager.
- **Preconditions:** Character constructed but BeginPlay never called.
- **Action:** Call Destroyed.
- **Expected Outcome:** No crash; IK manager unaffected.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Edge case during rapid spawn/destroy cycles in multiplayer.
- **Extends:** TC-0229
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0271
- **Name:** SimulatedProxyOnRepStateUpdatesTick
- **Registry String:** `ClimbingSystem.Multiplayer.SimulatedProxy.OnRepUpdatesAnimState`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** OnRep_ClimbingState on simulated proxy
- **Behavior Tested:** OnRep_ClimbingState on a simulated proxy correctly updates animation state without requiring a local controller.
- **Preconditions:** Simulated proxy character.
- **Action:** Set state to Hanging; call OnRep_ClimbingState.
- **Expected Outcome:** Animation state updated; montage playing.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Proxies must update visuals without local input.
- **Extends:** TC-0220
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0272
- **Name:** LacheArcPreviewOnlyInEditor
- **Registry String:** `ClimbingSystem.Debug.LacheArc.OnlyInEditorBuild`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterDetectionCameraTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Editor Lache arc preview
- **Behavior Tested:** Lache arc preview code is compiled out or gated in non-editor builds.
- **Preconditions:** Non-editor build.
- **Action:** Verify arc preview code is behind #if WITH_EDITOR.
- **Expected Outcome:** Code not compiled in non-editor builds.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: editor-only tick for Lache arc preview.
- **Extends:** TC-0240
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** Compile-time check.

### TC-0273
- **Name:** EditorLacheArcNotDrawnWhenNotSelected
- **Registry String:** `ClimbingSystem.Debug.LacheArc.NotDrawnWhenNotSelected`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterDetectionCameraTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Editor Lache arc preview
- **Behavior Tested:** Lache arc draw call is skipped when IsSelected() returns false.
- **Preconditions:** Editor build; bDrawDebug = true; actor not selected.
- **Action:** Tick character.
- **Expected Outcome:** No Lache arc drawn.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Without IsSelected() gate, all AClimbingCharacter instances draw arcs simultaneously.
- **Extends:** TC-0240
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0274
- **Name:** AnimNotifyMontageSlotMatchesState
- **Registry String:** `ClimbingSystem.Anim.Notify.MontageSlotMatchesClimbingState`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimationEdgeCaseTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AnimNotify firing context
- **Behavior Tested:** AnimNotify fires only when the montage is playing in the correct slot for the current climbing state.
- **Preconditions:** Character in Hanging state; montage playing in ClimbingMontageSlot.
- **Action:** Trigger AnimNotify; verify it fires.
- **Expected Outcome:** Notify fires and processes correctly.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Notify in wrong slot would enable/disable IK at wrong time.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 1
- **Notes:** None.

### TC-0275
- **Name:** MantleStepUpAtExactThreshold
- **Registry String:** `ClimbingSystem.Mantle.Height.StepUpAtExactStepMaxHeight`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMantleDetectionTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter mantle height classification
- **Behavior Tested:** Height exactly at MantleStepMaxHeight(50cm) triggers CMC step-up, not Mantling state.
- **Preconditions:** Character near obstacle at exactly 50cm height.
- **Action:** Trigger grab; check state.
- **Expected Outcome:** No Mantling state entered; CMC step-up handles it.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: <=MantleStepMaxHeight -> CMC step-up, no state entered.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0276
- **Name:** MantleLowAt51cm
- **Registry String:** `ClimbingSystem.Mantle.Height.LowMantleAt51cm`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMantleDetectionTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter mantle height classification
- **Behavior Tested:** Height 51cm (1 above step threshold) selects MantleLow slot.
- **Preconditions:** Character near obstacle at 51cm height.
- **Action:** Trigger grab; check montage.
- **Expected Outcome:** MantleLow montage selected; Mantling state entered.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: MantleStepMaxHeight<h<=MantleLowMaxHeight -> MantleLow.
- **Extends:** TC-0275
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0277
- **Name:** MantleHighAt101cm
- **Registry String:** `ClimbingSystem.Mantle.Height.HighMantleAt101cm`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMantleDetectionTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter mantle height classification
- **Behavior Tested:** Height 101cm (1 above LowMax) selects MantleHigh slot.
- **Preconditions:** Character near obstacle at 101cm height.
- **Action:** Trigger grab; check montage.
- **Expected Outcome:** MantleHigh montage selected.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: MantleLowMaxHeight<h<=MantleHighMaxHeight -> MantleHigh.
- **Extends:** TC-0276
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0278
- **Name:** MantleRejectedAt181cm
- **Registry String:** `ClimbingSystem.Mantle.Height.RejectedAboveHighMaxHeight`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMantleDetectionTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter mantle height classification
- **Behavior Tested:** Height 181cm (1 above HighMax=180) is not mantleable; height 180cm is mantleable.
- **Preconditions:** Character near obstacles at 180cm and 181cm.
- **Action:** Test both heights.
- **Expected Outcome:** 180cm -> MantleHigh; 181cm -> no mantle.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: >MantleHighMaxHeight -> not mantleable.
- **Extends:** TC-0277
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0279
- **Name:** MantleLowWarpTargetRegistered
- **Registry String:** `ClimbingSystem.Mantle.WarpTarget.MantleLowRegistered`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMantleRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateEnter(Mantling)
- **Behavior Tested:** MantleLow entry registers WarpTarget_MantleLow at ledge position.
- **Preconditions:** Valid mantle detection in low range; MotionWarping component present.
- **Action:** Transition to Mantling with low height.
- **Expected Outcome:** WarpTarget_MantleLow registered on MotionWarpingComponent.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: WarpTarget_MantleLow used for MantleLow animation.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0280
- **Name:** MantleHighWarpTargetRegistered
- **Registry String:** `ClimbingSystem.Mantle.WarpTarget.MantleHighRegistered`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMantleRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateEnter(Mantling)
- **Behavior Tested:** MantleHigh entry registers WarpTarget_MantleHigh at ledge position.
- **Preconditions:** Valid mantle detection in high range; MotionWarping component present.
- **Action:** Transition to Mantling with high height.
- **Expected Outcome:** WarpTarget_MantleHigh registered on MotionWarpingComponent.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: WarpTarget_MantleHigh used for MantleHigh animation.
- **Extends:** TC-0279
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0281
- **Name:** MantleBelowStepNoStateEntered
- **Registry String:** `ClimbingSystem.Mantle.Height.BelowStepMaxNoMantlingState`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMantleDetectionTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter mantle detection
- **Behavior Tested:** Height below MantleStepMaxHeight never enters Mantling state or plays montage.
- **Preconditions:** Character near obstacle at 30cm height.
- **Action:** Trigger grab.
- **Expected Outcome:** No Mantling state; no mantle montage; CMC step-up handles it.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Low obstacles should use engine step-up, not climbing system.
- **Extends:** TC-0275
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0282
- **Name:** LadderEnterBottomMontageAndWarp
- **Registry String:** `ClimbingSystem.Ladder.Entry.BottomMontageAndWarpTarget`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateEnter(LadderTransition)
- **Behavior Tested:** OnLadder entry from bottom plays LadderEnterBottom montage and registers WarpTarget_LadderEnterBottom.
- **Preconditions:** Character at bottom of ladder; LadderEnterBottom montage assigned.
- **Action:** Transition to LadderTransition from bottom.
- **Expected Outcome:** LadderEnterBottom montage playing; WarpTarget_LadderEnterBottom registered.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: WarpTarget_LadderEnterBottom used for ladder entry animation.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0283
- **Name:** LadderSprintAndCrouchSimultaneous
- **Registry String:** `ClimbingSystem.Ladder.Speed.SprintAndCrouchSimultaneous`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementComponentTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** UClimbingMovementComponent ladder speed
- **Behavior Tested:** Sprint+Crouch held simultaneously applies both multipliers to ladder speed.
- **Preconditions:** OnLadder state; bSprintHeld=true; bCrouchHeld=true.
- **Action:** Compute ladder speed.
- **Expected Outcome:** Speed = base * sprintMultiplier * crouchMultiplier.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec does not explicitly forbid simultaneous modifiers; behavior must be deterministic.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0284
- **Name:** LadderRungSpacingDefault
- **Registry String:** `ClimbingSystem.Ladder.RungSpacing.DefaultValue`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementComponentTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::DefaultLadderRungSpacing
- **Behavior Tested:** DefaultLadderRungSpacing defaults to 30cm and is EditAnywhere.
- **Preconditions:** Default character instance.
- **Action:** Read DefaultLadderRungSpacing.
- **Expected Outcome:** Value == 30.0f.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: LadderRungSpacing=30 drives vertical snap grid.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0285
- **Name:** LadderRungSpacingAffectsIKInterval
- **Registry String:** `ClimbingSystem.Ladder.RungSpacing.AffectsIKTraceInterval`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::UpdateLadderIK
- **Behavior Tested:** Changing DefaultLadderRungSpacing changes the vertical interval of IK traces.
- **Preconditions:** Character on ladder; DefaultLadderRungSpacing=60 (doubled).
- **Action:** Run UpdateLadderIK; measure trace spacing.
- **Expected Outcome:** IK traces spaced at 60cm intervals instead of 30cm.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Rung spacing drives procedural IK; wrong spacing misaligns hands/feet.
- **Extends:** TC-0284
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0286
- **Name:** GrabBreakBelowThresholdNoRagdoll
- **Registry String:** `ClimbingSystem.Ragdoll.GrabBreak.BelowThresholdNoRagdoll`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterPhysicsAudioTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::NotifyHit
- **Behavior Tested:** 1999N impulse (below 2000 threshold) does not trigger ragdoll.
- **Preconditions:** Character in Hanging state.
- **Action:** Apply 1999N impulse via NotifyHit.
- **Expected Outcome:** State remains Hanging; no ragdoll.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: GrabBreakImpulseThreshold=2000N.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0287
- **Name:** GrabBreakAtExactThresholdTriggersRagdoll
- **Registry String:** `ClimbingSystem.Ragdoll.GrabBreak.AtExactThresholdTriggersRagdoll`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterPhysicsAudioTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::NotifyHit
- **Behavior Tested:** Exactly 2000N impulse triggers ragdoll (>= boundary).
- **Preconditions:** Character in Hanging state.
- **Action:** Apply exactly 2000N impulse via NotifyHit.
- **Expected Outcome:** State transitions to Ragdoll.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Boundary test for grab break threshold.
- **Extends:** TC-0286
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0288
- **Name:** NotifyHitDuringNoneStateNoRagdoll
- **Registry String:** `ClimbingSystem.Ragdoll.GrabBreak.NoneStateIgnoresImpulse`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterPhysicsAudioTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::NotifyHit
- **Behavior Tested:** 5000N impulse in None state does not trigger ragdoll (non-climbing guard).
- **Preconditions:** Character in None state (locomotion).
- **Action:** Apply 5000N impulse via NotifyHit.
- **Expected Outcome:** State remains None; no ragdoll.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Grab break only applies during climbing states.
- **Extends:** TC-0287
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0289
- **Name:** PelvisBoneNameOverrideUsedInRecovery
- **Registry String:** `ClimbingSystem.Ragdoll.PelvisBone.OverrideUsedInRecovery`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterPhysicsAudioTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter ragdoll recovery
- **Behavior Tested:** Custom PelvisBoneName override is used for face detection during recovery.
- **Preconditions:** PelvisBoneName set to custom bone name; character in ragdoll.
- **Action:** Trigger recovery; verify custom bone queried.
- **Expected Outcome:** GetBoneQuaternion called with custom bone name, not "pelvis".
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: PelvisBoneName exposed for non-standard skeletons.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0290
- **Name:** RagdollFaceUpMontageSelected
- **Registry String:** `ClimbingSystem.Ragdoll.FaceDetection.FaceUpMontageSelected`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterPhysicsAudioTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter ragdoll face detection
- **Behavior Tested:** Pelvis up-vector dot > 0 selects RagdollGetUpFaceUp montage.
- **Preconditions:** Character in ragdoll; pelvis up-vector pointing up.
- **Action:** Trigger recovery.
- **Expected Outcome:** RagdollGetUpFaceUp montage plays.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: pelvis up-vector dot world up > 0 -> FaceUp.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0291
- **Name:** RagdollFaceDownMontageSelected
- **Registry String:** `ClimbingSystem.Ragdoll.FaceDetection.FaceDownMontageSelected`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterPhysicsAudioTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter ragdoll face detection
- **Behavior Tested:** Pelvis up-vector dot <= 0 selects RagdollGetUpFaceDown montage.
- **Preconditions:** Character in ragdoll; pelvis up-vector pointing down.
- **Action:** Trigger recovery.
- **Expected Outcome:** RagdollGetUpFaceDown montage plays.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: pelvis up-vector dot world up <= 0 -> FaceDown.
- **Extends:** TC-0290
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0292
- **Name:** GrabBreakLaunchVelocityScaled
- **Registry String:** `ClimbingSystem.Ragdoll.GrabBreak.LaunchVelocityScaledCorrectly`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterPhysicsAudioTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::NotifyHit ragdoll launch
- **Behavior Tested:** Launch velocity = NormalImpulse * GrabBreakLaunchScale (3000*0.5=1500).
- **Preconditions:** Character in Hanging; GrabBreakLaunchScale=0.5.
- **Action:** Apply 3000N impulse.
- **Expected Outcome:** Ragdoll launch velocity magnitude ~1500.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: GrabBreakLaunchScale=0.5 multiplier on impulse.
- **Extends:** TC-0287
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0293
- **Name:** AnchorMovesCharacterFollowsNextTick
- **Registry String:** `ClimbingSystem.Physics.Anchor.CharacterFollowsMovingAnchor`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** Dynamic anchor following
- **Behavior Tested:** When anchor moves, character world position updates next tick.
- **Preconditions:** Character in Hanging with valid anchor.
- **Action:** Move anchor 100cm; tick once.
- **Expected Outcome:** Character position updated to match new anchor world-space.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: world-space = Anchor->GetComponentTransform() * AnchorLocalTransform per tick.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0294
- **Name:** AnchorMovingPlatformTracking
- **Registry String:** `ClimbingSystem.Physics.Anchor.MovingPlatformTrackingAcrossTicks`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** Dynamic anchor following
- **Behavior Tested:** Character tracks moving platform across 5 ticks within 1cm tolerance.
- **Preconditions:** Character in Hanging; anchor moving 20cm/tick.
- **Action:** Tick 5 times; verify position each tick.
- **Expected Outcome:** Character within 1cm of expected position each tick.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Moving platform support is critical for gameplay.
- **Extends:** TC-0293
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0295
- **Name:** HangingEntryCallsSetBase
- **Registry String:** `ClimbingSystem.Physics.SetBase.HangingEntryCallsSetBase`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementEdgeCaseTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateEnter(Hanging)
- **Behavior Tested:** Hanging entry calls SetBase on HitComponent.
- **Preconditions:** Character in None; valid detection result.
- **Action:** Transition to Hanging.
- **Expected Outcome:** GetMovementBase() == HitComponent.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: SetBase on entry to Hanging.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0296
- **Name:** ExitClimbingClearsMovementBase
- **Registry String:** `ClimbingSystem.Physics.SetBase.ExitClimbingClearsBase`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementEdgeCaseTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateExit
- **Behavior Tested:** Exiting any climbing state to None clears movement base to nullptr.
- **Preconditions:** Character in Hanging with SetBase active.
- **Action:** Transition to None.
- **Expected Outcome:** GetMovementBase() == nullptr.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: SetBase(nullptr) on all exits.
- **Extends:** TC-0295
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0297
- **Name:** NullAnchorDuringHangingDrops
- **Registry String:** `ClimbingSystem.Physics.Anchor.NullAnchorDuringHangingTriggersDroppedDown`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementEdgeCaseTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** Dynamic anchor null handling
- **Behavior Tested:** AnchorComponent becoming null during Hanging triggers DroppingDown.
- **Preconditions:** Character in Hanging with valid anchor.
- **Action:** Set AnchorComponent to nullptr; tick.
- **Expected Outcome:** State transitions to DroppingDown.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Spec: AnchorComponent null -> DroppingDown immediately.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0298
- **Name:** ServerRejectsStreamingSublevelAnchor
- **Registry String:** `ClimbingSystem.Multiplayer.ServerAttemptGrab.RejectsStreamingSublevelAnchor`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Server_AttemptGrab
- **Behavior Tested:** Server_AttemptGrab rejects when HitComponent owner is in streaming sublevel.
- **Preconditions:** Server; component in streaming sublevel.
- **Action:** Call Server_AttemptGrab.
- **Expected Outcome:** Grab rejected; Client_RejectStateTransition called.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Spec: anchors restricted to persistent level actors only.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** APPROX
- **Added In Run:** 2
- **Notes:** None.

### TC-0299
- **Name:** ServerAcceptsPersistentLevelAnchor
- **Registry String:** `ClimbingSystem.Multiplayer.ServerAttemptGrab.AcceptsPersistentLevelAnchor`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::Server_AttemptGrab
- **Behavior Tested:** Server_AttemptGrab accepts when component is in persistent level.
- **Preconditions:** Server; component in persistent level; position within tolerance.
- **Action:** Call Server_AttemptGrab.
- **Expected Outcome:** Grab accepted; state transitions.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Complement to TC-0298.
- **Extends:** TC-0298
- **Requires World:** YES
- **Requires Network:** APPROX
- **Added In Run:** 2
- **Notes:** None.

### TC-0300
- **Name:** LadderExitTopMontageAndWarp
- **Registry String:** `ClimbingSystem.Ladder.Exit.TopMontageAndWarpTarget`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateEnter(LadderTransition)
- **Behavior Tested:** LadderTransition with valid ledge plays LadderExitTop and registers WarpTarget_LadderExitTop.
- **Preconditions:** Character on ladder at top; valid detection result.
- **Action:** Transition to LadderTransition.
- **Expected Outcome:** LadderExitTop montage; WarpTarget_LadderExitTop registered.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: WarpTarget_LadderExitTop for top exit.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0301
- **Name:** LadderExitBottomNoWarpTarget
- **Registry String:** `ClimbingSystem.Ladder.Exit.BottomMontageNoWarpTarget`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateEnter(LadderTransition)
- **Behavior Tested:** LadderTransition with invalid ledge plays LadderExitBottom; no warp target.
- **Preconditions:** Character on ladder at bottom; invalid detection result.
- **Action:** Transition to LadderTransition.
- **Expected Outcome:** LadderExitBottom montage; no warp target registered.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Bottom exit is fallback path.
- **Extends:** TC-0300
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0302
- **Name:** LandedWithinCoyoteWindowTriggersRescan
- **Registry String:** `ClimbingSystem.CoyoteTime.Landed.WithinWindowTriggersRescan`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::Landed
- **Behavior Tested:** Landed within CoyoteTimeWindow triggers re-grab detection scan.
- **Preconditions:** Character just left climbing; within 0.15s window.
- **Action:** Trigger Landed at t=0.08s post-leave.
- **Expected Outcome:** Detection scan fires.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: CoyoteTimeWindow seconds post-leave.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0303
- **Name:** LandedAfterCoyoteWindowNoRescan
- **Registry String:** `ClimbingSystem.CoyoteTime.Landed.AfterWindowNoRescan`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::Landed
- **Behavior Tested:** Landed after CoyoteTimeWindow expires does not trigger re-grab scan.
- **Preconditions:** Character left climbing; 0.16s elapsed.
- **Action:** Trigger Landed at t=0.16s.
- **Expected Outcome:** No detection scan fires.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Window must expire correctly.
- **Extends:** TC-0302
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0304
- **Name:** GrabBreakThresholdZeroDisablesBreak
- **Registry String:** `ClimbingSystem.Ragdoll.GrabBreak.ThresholdZeroDisablesBreak`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterPhysicsAudioTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::NotifyHit
- **Behavior Tested:** GrabBreakImpulseThreshold=0 disables grab-break entirely.
- **Preconditions:** Character in Hanging; GrabBreakImpulseThreshold=0.
- **Action:** Apply 10000N impulse.
- **Expected Outcome:** State remains Hanging; no ragdoll.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: 0 = disable.
- **Extends:** TC-0286
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

---


### TC-0305
- **Name:** IKMaxReachExceededFadesWeight
- **Registry String:** `ClimbingSystem.IK.MaxReach.ExceededFadesWeightToZero`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimInstanceTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::UpdateClimbingIK
- **Behavior Tested:** When hand target exceeds MaxReachDistance(80cm), IK weight fades to 0 over IKFadeOutBlendTime.
- **Preconditions:** Character climbing; hand target at 90cm from shoulder.
- **Action:** Update IK; check weight over time.
- **Expected Outcome:** Weight reaches 0 after IKFadeOutBlendTime(0.15s).
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: exceeding MaxReachDistance fades weight to zero.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0306
- **Name:** IKMaxReachWithinKeepsWeight
- **Registry String:** `ClimbingSystem.IK.MaxReach.WithinKeepsWeightActive`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimInstanceTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::UpdateClimbingIK
- **Behavior Tested:** When hand target is within MaxReachDistance(80cm), IK weight stays at 1.0.
- **Preconditions:** Character climbing; hand target at 70cm from shoulder.
- **Action:** Update IK.
- **Expected Outcome:** Weight remains 1.0.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Complement to TC-0305.
- **Extends:** TC-0305
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0307
- **Name:** IKBudgetFifthCharacterZeroed
- **Registry String:** `ClimbingSystem.IK.Budget.FifthCharacterWeightsZeroed`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** IK manager budget enforcement
- **Behavior Tested:** With MaxSimultaneousIKCharacters=4, the 5th climbing character has all IK weights zeroed.
- **Preconditions:** 5 characters climbing simultaneously.
- **Action:** Sort by camera distance; update IK.
- **Expected Outcome:** 4 nearest have active IK; 5th has all weights=0.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: MaxSimultaneousIKCharacters=4.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0308
- **Name:** IKManagerSortedByCameraDistance
- **Registry String:** `ClimbingSystem.IK.Manager.SortedByCameraDistanceOnStateChange`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** IK manager sorted insert
- **Behavior Tested:** ActiveClimbingCharacters sorted by camera distance on state change, not per-tick.
- **Preconditions:** 3 characters at different distances from camera.
- **Action:** Trigger state change; verify sort order.
- **Expected Outcome:** Array sorted nearest-first.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: sorted insert by camera distance on state change.
- **Extends:** TC-0307
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0309
- **Name:** IKProxyUpdateIntervalThrottled
- **Registry String:** `ClimbingSystem.IK.Proxy.UpdateIntervalThrottled`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** Simulated proxy IK update
- **Behavior Tested:** Proxy IK updates throttled to SimulatedProxyIKUpdateInterval(0.05s).
- **Preconditions:** Simulated proxy climbing.
- **Action:** Tick at 0.03s (no update), then at 0.06s (update fires).
- **Expected Outcome:** No IK update at 0.03s; one update at 0.06s.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: SimulatedProxyIKUpdateInterval=0.05s.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0310
- **Name:** HandIKOffsetApplied
- **Registry String:** `ClimbingSystem.IK.Hand.OffsetAppliedToTarget`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimInstanceTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::UpdateLedgeHangIK
- **Behavior Tested:** HandIKOffset(5,0,-5) is applied to computed hand IK target position.
- **Preconditions:** Character hanging; IK active.
- **Action:** Compute hand IK target; verify offset applied.
- **Expected Outcome:** Target = base + FVector(5,0,-5).
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: HandIKOffset=FVector(5,0,-5).
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0311
- **Name:** HandIKSpacingBetweenLeftRight
- **Registry String:** `ClimbingSystem.IK.Hand.SpacingBetweenLeftAndRight`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimInstanceTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::UpdateLedgeHangIK
- **Behavior Tested:** Left and right hand IK targets are separated by HandIKSpacing(40cm).
- **Preconditions:** Character hanging; IK active.
- **Action:** Compute both hand targets; measure distance.
- **Expected Outcome:** Horizontal distance between hands ~40cm.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: HandIKSpacing=40cm.
- **Extends:** TC-0310
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0312
- **Name:** CameraNudgeAtExactActivationAngle
- **Registry String:** `ClimbingSystem.Camera.Nudge.AtExactActivationAngleBoundary`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterDetectionCameraTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter camera nudge
- **Behavior Tested:** Camera nudge activates at exactly CameraNudgeActivationAngle(45°).
- **Preconditions:** Character climbing; camera at 45° from wall normal.
- **Action:** Tick camera update.
- **Expected Outcome:** Nudge activates (rotation begins).
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: CameraNudgeActivationAngle=45°.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0313
- **Name:** CameraNudgeBelowAngleNoNudge
- **Registry String:** `ClimbingSystem.Camera.Nudge.BelowActivationAngleNoNudge`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterDetectionCameraTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter camera nudge
- **Behavior Tested:** Camera nudge does not activate below CameraNudgeActivationAngle.
- **Preconditions:** Character climbing; camera at 30° from wall normal.
- **Action:** Tick camera update.
- **Expected Outcome:** No nudge rotation applied.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Complement to TC-0312.
- **Extends:** TC-0312
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0314
- **Name:** LockCameraToFrameZeroBlendTime
- **Registry String:** `ClimbingSystem.Camera.Lock.ZeroBlendTimeImmediate`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterDetectionCameraTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::LockCameraToFrame
- **Behavior Tested:** LockCameraToFrame with BlendTime=0 applies immediately.
- **Preconditions:** Camera in default position.
- **Action:** Call LockCameraToFrame(Loc, Rot, 0.0f).
- **Expected Outcome:** Camera at target position/rotation immediately.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Zero blend must not cause divide-by-zero.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0315
- **Name:** ReleaseCameraLockDuringNonLockedNoOp
- **Registry String:** `ClimbingSystem.Camera.Lock.ReleaseWhenNotLockedIsNoOp`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterDetectionCameraTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::ReleaseCameraLock
- **Behavior Tested:** ReleaseCameraLock during non-locked state is a no-op (no crash).
- **Preconditions:** Camera not locked.
- **Action:** Call ReleaseCameraLock(0.5f).
- **Expected Outcome:** No crash; no state change.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Defensive test for double-release.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0316
- **Name:** RagdollCameraSocketSwitch
- **Registry String:** `ClimbingSystem.Camera.Ragdoll.SocketSwitchOnRagdoll`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterDetectionCameraTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter ragdoll camera
- **Behavior Tested:** On ragdoll entry, spring arm switches to RagdollCameraTargetSocket(pelvis).
- **Preconditions:** Character climbing; camera locked.
- **Action:** Trigger ragdoll.
- **Expected Outcome:** Spring arm AttachSocketName = "pelvis"; camera lock released.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: ReleaseCameraLock(0.1f) -> switch to pelvis socket.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0317
- **Name:** RagdollCameraSocketRestoredOnRecovery
- **Registry String:** `ClimbingSystem.Camera.Ragdoll.SocketRestoredOnRecovery`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterDetectionCameraTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter ragdoll recovery
- **Behavior Tested:** Spring arm socket restored to pre-ragdoll value on recovery.
- **Preconditions:** In ragdoll; spring arm on pelvis.
- **Action:** Trigger recovery.
- **Expected Outcome:** AttachSocketName = original socket.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Camera must return to normal after ragdoll.
- **Extends:** TC-0316
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0318
- **Name:** AudioResolvedCacheHitSecondCall
- **Registry String:** `ClimbingSystem.Audio.ResolvedCache.HitOnSecondCall`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSoundNotifyTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** Audio resolved cache
- **Behavior Tested:** Second dispatch for same EClimbSoundType uses cache, no new async load.
- **Preconditions:** HandGrab cached from prior call.
- **Action:** Dispatch HandGrab again.
- **Expected Outcome:** Async load count unchanged; cached ptr used.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: resolved cache prevents redundant loads.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0319
- **Name:** AudioAsyncLoadSuccessCachesPtr
- **Registry String:** `ClimbingSystem.Audio.AsyncLoad.SuccessCachesResolvedPtr`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSoundNotifyTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** Audio async load callback
- **Behavior Tested:** Successful async load stores resolved ptr in cache under correct key.
- **Preconditions:** Cache empty; valid soft ptr for FootPlant.
- **Action:** Simulate load completion with valid USoundBase.
- **Expected Outcome:** Cache[FootPlant] != null.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: success path caches resolved ptr.
- **Extends:** TC-0318
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0320
- **Name:** AudioAsyncLoadFailureCachesNullNoRetry
- **Registry String:** `ClimbingSystem.Audio.AsyncLoad.FailureCachesNullNoRetry`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSoundNotifyTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** Audio async load failure
- **Behavior Tested:** Failed load caches null, logs once, never retries.
- **Preconditions:** Cache empty; invalid soft ptr for MantleImpact.
- **Action:** Dispatch twice; simulate null load result.
- **Expected Outcome:** Load attempted once; log emitted once; second call skips.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Spec: failure logs once, caches null, never retries.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0321
- **Name:** AudioGrabFailSoundDispatched
- **Registry String:** `ClimbingSystem.Audio.GrabFail.DispatchedOnMissedGrab`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSoundNotifyTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Audio GrabFail dispatch
- **Behavior Tested:** GrabFail sound dispatched when grab attempt finds no valid surface.
- **Preconditions:** GrabFail cached; no surface in range.
- **Action:** Trigger grab attempt with no hit.
- **Expected Outcome:** GrabFail dispatched once.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Spec: EClimbSoundType::GrabFail.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0322
- **Name:** AudioLacheSoundsDispatched
- **Registry String:** `ClimbingSystem.Audio.Lache.LaunchAndCatchSoundsDispatched`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSoundNotifyTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** Audio Lache sounds
- **Behavior Tested:** LacheLaunchGrunt dispatched on launch, LacheCatchImpact on catch.
- **Preconditions:** Both sounds cached.
- **Action:** Simulate launch then catch events.
- **Expected Outcome:** Each sound dispatched exactly once at correct phase.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: EClimbSoundType covers Lache sounds.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0323
- **Name:** SurfaceDataAnimSetOverrideAsyncLoad
- **Registry String:** `ClimbingSystem.SurfaceData.AnimSetOverride.AsyncLoadOnFirstContact`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSurfaceDataTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** UClimbingSurfaceData AnimationSetOverride
- **Behavior Tested:** Async load triggered exactly once on first surface contact.
- **Preconditions:** Valid soft ptr; no prior contact.
- **Action:** Simulate first contact.
- **Expected Outcome:** Async load requested once.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: AnimationSetOverride loaded async on first surface contact.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0324
- **Name:** SurfaceDataPerSlotFallback
- **Registry String:** `ClimbingSystem.SurfaceData.AnimSetOverride.NullSlotFallsBackToCharacterDefault`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSurfaceDataTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** UClimbingSurfaceData per-slot fallback
- **Behavior Tested:** Null slot in AnimationSetOverride falls back to character default.
- **Preconditions:** Override has null for one slot; character has default.
- **Action:** Query that slot.
- **Expected Outcome:** Returns character default, not null.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: per-slot fallback.
- **Extends:** TC-0323
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0325
- **Name:** SurfaceDataClimbSpeedMultiplierAffectsShimmy
- **Registry String:** `ClimbingSystem.SurfaceData.ClimbSpeedMultiplier.AffectsShimmySpeed`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSurfaceDataTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** UClimbingSurfaceData ClimbSpeedMultiplier
- **Behavior Tested:** Shimmy speed is scaled by ClimbSpeedMultiplier from surface data.
- **Preconditions:** Character shimmying; surface multiplier=0.5.
- **Action:** Compute shimmy speed.
- **Expected Outcome:** Result = base speed * 0.5.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: ClimbSpeedMultiplier affects shimmy.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0326
- **Name:** SurfaceDataClimbSpeedMultiplierZero
- **Registry String:** `ClimbingSystem.SurfaceData.ClimbSpeedMultiplier.ZeroHaltsShimmy`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSurfaceDataTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** UClimbingSurfaceData ClimbSpeedMultiplier
- **Behavior Tested:** ClimbSpeedMultiplier=0 results in zero shimmy speed without crash.
- **Preconditions:** Surface multiplier=0.
- **Action:** Compute shimmy speed.
- **Expected Outcome:** Speed=0; no crash/NaN.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Zero multiplier must not cause divide-by-zero.
- **Extends:** TC-0325
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0327
- **Name:** SurfaceDataAudioOverrideUsed
- **Registry String:** `ClimbingSystem.SurfaceData.AudioOverride.SurfaceSpecificSoundUsed`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSurfaceDataTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** UClimbingSurfaceData surface-specific audio
- **Behavior Tested:** Surface-specific audio override dispatched instead of character default.
- **Preconditions:** Surface has HandGrab override; character has different default.
- **Action:** Dispatch HandGrab on that surface.
- **Expected Outcome:** Surface override sound used.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: per-surface audio override.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0328
- **Name:** SurfaceDataAudioOverrideNullFallback
- **Registry String:** `ClimbingSystem.SurfaceData.AudioOverride.NullFallsBackToCharacterDefault`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSurfaceDataTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** UClimbingSurfaceData surface audio fallback
- **Behavior Tested:** Null audio override falls back to character default sound.
- **Preconditions:** Surface has null for FootPlant; character has default.
- **Action:** Dispatch FootPlant on that surface.
- **Expected Outcome:** Character default FootPlant sound dispatched.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Per-slot audio fallback.
- **Extends:** TC-0327
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0329
- **Name:** IKCullDistanceBoundaryTest
- **Registry String:** `ClimbingSystem.IK.CullDistance.AtExactBoundary`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** Simulated proxy IK culling
- **Behavior Tested:** Proxy at exactly SimulatedProxyIKCullDistance(1500cm) has IK zeroed; at 1499cm has IK active.
- **Preconditions:** Two proxies at 1499cm and 1500cm.
- **Action:** Update IK.
- **Expected Outcome:** 1499cm proxy has active IK; 1500cm proxy has zeroed IK.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: SimulatedProxyIKCullDistance=1500cm boundary.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0330
- **Name:** IKCornerFABRIKAllFourLimbs
- **Registry String:** `ClimbingSystem.IK.Corner.FABRIKAllFourLimbs`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimInstanceTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** AClimbingCharacter corner IK
- **Behavior Tested:** During CornerTransition, FABRIK is applied to all four limbs.
- **Preconditions:** Character in CornerTransition state.
- **Action:** Check IK weights for all 4 limbs.
- **Expected Outcome:** All 4 IK weights > 0 during corner transition.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: FABRIK for all four limbs during corner transitions.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0331
- **Name:** SpringArmRotatesOnClimbEntry
- **Registry String:** `ClimbingSystem.Camera.SpringArm.RotatesOnClimbEntry`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterDetectionCameraTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter spring arm
- **Behavior Tested:** Spring arm rotation aligns to surface normal on climb state entry.
- **Preconditions:** Locomotion state; wall normal=(0,1,0).
- **Action:** Trigger climb entry.
- **Expected Outcome:** Spring arm yaw reflects wall normal.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: assisted mode rotates spring arm toward wall on climb entry.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0332
- **Name:** IKFadeOutBlendTimeDefault
- **Registry String:** `ClimbingSystem.IK.FadeOut.BlendTimeDefaultValue`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimInstanceTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter IK properties
- **Behavior Tested:** IKFadeOutBlendTime defaults to 0.15s.
- **Preconditions:** Default character instance.
- **Action:** Read IKFadeOutBlendTime.
- **Expected Outcome:** Value == 0.15f.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: IKFadeOutBlendTime=0.15s.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0333
- **Name:** MaxReachDistanceDefault
- **Registry String:** `ClimbingSystem.IK.MaxReach.DefaultValue`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimInstanceTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter IK properties
- **Behavior Tested:** MaxReachDistance defaults to 80cm.
- **Preconditions:** Default character instance.
- **Action:** Read MaxReachDistance.
- **Expected Outcome:** Value == 80.0f.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: MaxReachDistance=80cm.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0334
- **Name:** HandIKSpacingDefault
- **Registry String:** `ClimbingSystem.IK.Hand.SpacingDefaultValue`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimInstanceTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter IK properties
- **Behavior Tested:** HandIKSpacing defaults to 40cm.
- **Preconditions:** Default character instance.
- **Action:** Read HandIKSpacing.
- **Expected Outcome:** Value == 40.0f.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: HandIKSpacing=40cm.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0335
- **Name:** ServerAttemptGrabRejectsStreamingSublevel
- **Registry String:** `ClimbingSystem.Multiplayer.ServerGrab.RejectsStreamingSublevel`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Server_AttemptGrab
- **Behavior Tested:** Server_AttemptGrab rejects when hit component is in streaming sublevel.
- **Preconditions:** Server; component in streaming sublevel.
- **Action:** Call Server_AttemptGrab with sublevel component.
- **Expected Outcome:** Client_RejectStateTransition fired.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Spec: anchors restricted to persistent level.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** APPROX
- **Added In Run:** 2
- **Notes:** None.

### TC-0336
- **Name:** ServerAttemptGrabAcceptsLoadedLevel
- **Registry String:** `ClimbingSystem.Multiplayer.ServerGrab.AcceptsLoadedLevel`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Server_AttemptGrab
- **Behavior Tested:** Server_AttemptGrab accepts when sublevel is fully loaded and position within tolerance.
- **Preconditions:** Server; component in loaded persistent level; position within tolerance.
- **Action:** Call Server_AttemptGrab.
- **Expected Outcome:** State transitions; anchor replicated.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Complement to TC-0335.
- **Extends:** TC-0335
- **Requires World:** YES
- **Requires Network:** APPROX
- **Added In Run:** 2
- **Notes:** None.

### TC-0337
- **Name:** ServerDropFromHangingTransitionsToNone
- **Registry String:** `ClimbingSystem.Multiplayer.ServerDrop.FromHangingToNone`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Server_Drop
- **Behavior Tested:** Server_Drop from Hanging transitions to None; MOVE_Falling restored.
- **Preconditions:** Server; character in Hanging.
- **Action:** Call Server_Drop.
- **Expected Outcome:** State = None; movement mode = Falling.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Server_Drop requires no validation.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** APPROX
- **Added In Run:** 2
- **Notes:** None.

### TC-0338
- **Name:** ServerDropFromShimmyTransitionsToNone
- **Registry String:** `ClimbingSystem.Multiplayer.ServerDrop.FromShimmyToNone`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::Server_Drop
- **Behavior Tested:** Server_Drop from Shimmying transitions to None.
- **Preconditions:** Server; character in Shimmying.
- **Action:** Call Server_Drop.
- **Expected Outcome:** State = None.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Drop valid from all interruptible states.
- **Extends:** TC-0337
- **Requires World:** YES
- **Requires Network:** APPROX
- **Added In Run:** 2
- **Notes:** None.

### TC-0339
- **Name:** ServerAttemptLacheRejectsOutOfRange
- **Registry String:** `ClimbingSystem.Multiplayer.ServerLache.RejectsOutOfRangeTarget`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Server_AttemptLache
- **Behavior Tested:** Arc target beyond max reach triggers Client_RejectStateTransition.
- **Preconditions:** Server; character in Hanging; arc target far away.
- **Action:** Call Server_AttemptLache with out-of-range target.
- **Expected Outcome:** Grab rejected.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Server validates arc target.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** APPROX
- **Added In Run:** 2
- **Notes:** None.

### TC-0340
- **Name:** ServerAttemptLacheAcceptsValidTarget
- **Registry String:** `ClimbingSystem.Multiplayer.ServerLache.AcceptsValidTarget`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Server_AttemptLache
- **Behavior Tested:** Arc target within reach triggers lache launch state.
- **Preconditions:** Server; character in Hanging; valid arc target.
- **Action:** Call Server_AttemptLache.
- **Expected Outcome:** State = Lache; no rejection.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Lache server validation happy path.
- **Extends:** TC-0339
- **Requires World:** YES
- **Requires Network:** APPROX
- **Added In Run:** 2
- **Notes:** None.

### TC-0341
- **Name:** ServerAttemptClimbUpRejectsNoClearance
- **Registry String:** `ClimbingSystem.Multiplayer.ServerClimbUp.RejectsInsufficientClearance`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Server_AttemptClimbUp
- **Behavior Tested:** Clearance blocked on server triggers Client_RejectStateTransition.
- **Preconditions:** Server; character in Hanging; clearance blocked.
- **Action:** Call Server_AttemptClimbUp.
- **Expected Outcome:** Rejected; Client_RejectStateTransition called.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Server re-runs clearance check.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** APPROX
- **Added In Run:** 2
- **Notes:** None.

### TC-0342
- **Name:** ServerAttemptClimbUpAcceptsClear
- **Registry String:** `ClimbingSystem.Multiplayer.ServerClimbUp.AcceptsClearLedge`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Server_AttemptClimbUp
- **Behavior Tested:** Clearance passes triggers ClimbingUp state.
- **Preconditions:** Server; character in Hanging; clearance clear.
- **Action:** Call Server_AttemptClimbUp.
- **Expected Outcome:** State = ClimbingUp; replicated.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** ClimbUp server validation happy path.
- **Extends:** TC-0341
- **Requires World:** YES
- **Requires Network:** APPROX
- **Added In Run:** 2
- **Notes:** None.

### TC-0343
- **Name:** ServerUpdateShimmyDirectionReplicates
- **Registry String:** `ClimbingSystem.Multiplayer.ServerShimmy.DirectionReplicatesToProxy`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Server_UpdateShimmyDirection
- **Behavior Tested:** FVector2D(1,0) stored in replicated shimmy direction property.
- **Preconditions:** Server; character shimmying.
- **Action:** Call Server_UpdateShimmyDirection(FVector2D(1,0)).
- **Expected Outcome:** Replicated direction = (1,0).
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Shimmy direction must replicate for proxy montage selection.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** APPROX
- **Added In Run:** 2
- **Notes:** None.

### TC-0344
- **Name:** ServerUpdateShimmyZeroVectorAccepted
- **Registry String:** `ClimbingSystem.Multiplayer.ServerShimmy.ZeroVectorAccepted`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::Server_UpdateShimmyDirection
- **Behavior Tested:** FVector2D::ZeroVector accepted cleanly (stop shimmy).
- **Preconditions:** Server; character shimmying.
- **Action:** Call Server_UpdateShimmyDirection(FVector2D::ZeroVector).
- **Expected Outcome:** No crash; direction zeroed.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Zero input must not crash.
- **Extends:** TC-0343
- **Requires World:** YES
- **Requires Network:** APPROX
- **Added In Run:** 2
- **Notes:** None.

### TC-0345
- **Name:** ClientConfirmMatchesPrediction
- **Registry String:** `ClimbingSystem.Multiplayer.ClientConfirm.MatchesPredictionNoRollback`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Client_ConfirmStateTransition
- **Behavior Tested:** Confirmation matching optimistic prediction keeps state; no rollback.
- **Preconditions:** Client predicted Hanging; server confirms Hanging.
- **Action:** Call Client_ConfirmStateTransition(Hanging).
- **Expected Outcome:** State remains Hanging; no GrabFail montage.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: confirmation matching prediction = no rollback.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** APPROX
- **Added In Run:** 2
- **Notes:** None.

### TC-0346
- **Name:** OptimisticPredictionStateSetBeforeAck
- **Registry String:** `ClimbingSystem.Multiplayer.Prediction.StateSetBeforeServerAck`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** Optimistic client prediction
- **Behavior Tested:** Client enters climbing state immediately on input before server ack.
- **Preconditions:** Client in None; valid surface detected.
- **Action:** Trigger IA_Grab.
- **Expected Outcome:** Local state = Hanging immediately; Server_AttemptGrab sent.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: optimistic client prediction.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** APPROX
- **Added In Run:** 2
- **Notes:** None.

### TC-0347
- **Name:** SimulatedProxyMontageOnRepNotify
- **Registry String:** `ClimbingSystem.Multiplayer.SimProxy.MontageSelectedOnRepNotify`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** OnRep_ClimbingState proxy montage
- **Behavior Tested:** Rep-notify for BracedWall plays braced entry montage, not shimmy/climb-up.
- **Preconditions:** Simulated proxy; state replicated to BracedWall.
- **Action:** Call OnRep_ClimbingState.
- **Expected Outcome:** BracedIdle montage playing.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Proxy must play correct montage per state.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0348
- **Name:** FreefallGrabSucceedsWithinReach
- **Registry String:** `ClimbingSystem.Freefall.FallingGrab.SucceedsWithinReachDistance`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** Freefall re-grab
- **Behavior Tested:** Ledge at 79cm + bEnableFallingGrab=true + IA_Grab triggers grab.
- **Preconditions:** Character falling; ledge at 79cm; bEnableFallingGrab=true.
- **Action:** Trigger IA_Grab.
- **Expected Outcome:** Character grabs ledge; enters climbing state.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: FallingGrabReachDistance=80cm.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0349
- **Name:** FreefallGrabFailsBeyondReach
- **Registry String:** `ClimbingSystem.Freefall.FallingGrab.FailsBeyondReachDistance`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** Freefall re-grab
- **Behavior Tested:** Ledge at 81cm fails grab; stays MOVE_Falling.
- **Preconditions:** Character falling; ledge at 81cm.
- **Action:** Trigger IA_Grab.
- **Expected Outcome:** No grab; stays falling.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Boundary test for FallingGrabReachDistance.
- **Extends:** TC-0348
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0350
- **Name:** FreefallGrabDisabledByToggle
- **Registry String:** `ClimbingSystem.Freefall.FallingGrab.DisabledByToggle`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** Freefall re-grab toggle
- **Behavior Tested:** bEnableFallingGrab=false prevents grab even with ledge at 40cm.
- **Preconditions:** Character falling; ledge at 40cm; bEnableFallingGrab=false.
- **Action:** Trigger IA_Grab.
- **Expected Outcome:** No grab.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Spec: bEnableFallingGrab toggle.
- **Extends:** TC-0348
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0351
- **Name:** FreefallCheckIntervalRate
- **Registry String:** `ClimbingSystem.Freefall.FallingGrab.CheckIntervalRate`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** FallingGrabCheckInterval timer
- **Behavior Tested:** Timer fires at 0.05s interval; no fire at 0.04s, one fire at 0.06s.
- **Preconditions:** Character in MOVE_Falling.
- **Action:** Tick at 0.04s and 0.06s.
- **Expected Outcome:** Zero fires at 0.04s; one fire at 0.06s.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: FallingGrabCheckInterval=0.05s.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0352
- **Name:** CoyoteGrabSucceedsWithinWindow
- **Registry String:** `ClimbingSystem.CoyoteTime.Grab.SucceedsWithinWindow`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** Coyote time re-grab
- **Behavior Tested:** IA_Grab at t=0.10s post-leave triggers detection re-run and grab.
- **Preconditions:** Character just left climbing; within 0.15s window; ledge nearby.
- **Action:** Trigger IA_Grab at t=0.10s.
- **Expected Outcome:** Detection re-runs; character grabs.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: CoyoteTimeWindow=0.15s.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0353
- **Name:** CoyoteGrabFailsAfterWindow
- **Registry String:** `ClimbingSystem.CoyoteTime.Grab.FailsAfterWindowExpiry`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** Coyote time window expiry
- **Behavior Tested:** IA_Grab at t=0.16s (past window) does not trigger coyote path.
- **Preconditions:** Character left climbing; 0.16s elapsed.
- **Action:** Trigger IA_Grab at t=0.16s.
- **Expected Outcome:** Coyote path not entered; stays None.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Window must expire correctly.
- **Extends:** TC-0352
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0354
- **Name:** CoyoteGrabDisabledByToggle
- **Registry String:** `ClimbingSystem.CoyoteTime.Grab.DisabledByToggle`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** Coyote time toggle
- **Behavior Tested:** bEnableCoyoteTime=false prevents coyote grab at t=0.05s.
- **Preconditions:** bEnableCoyoteTime=false; character just left climbing.
- **Action:** Trigger IA_Grab at t=0.05s.
- **Expected Outcome:** No coyote grab.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Spec: bEnableCoyoteTime toggle.
- **Extends:** TC-0352
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0355
- **Name:** CoyoteLandedClearsWindow
- **Registry String:** `ClimbingSystem.CoyoteTime.Landed.ClearsWindow`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** World
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::Landed coyote interaction
- **Behavior Tested:** Landed at t=0.08s invalidates coyote window; subsequent IA_Grab fails coyote path.
- **Preconditions:** Character left climbing; Landed at t=0.08s.
- **Action:** Trigger IA_Grab at t=0.10s (within original window but after Landed).
- **Expected Outcome:** Coyote path not entered.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Landing should clear the coyote window.
- **Extends:** TC-0352
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0356
- **Name:** ClimbableOneWayRejectsWrongApproach
- **Registry String:** `ClimbingSystem.Detection.ClimbableOneWay.RejectsWrongApproachDirection`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::ValidateOneWayApproach
- **Behavior Tested:** Approach parallel to normal (back face) returns false.
- **Preconditions:** SurfaceNormal=(0,1,0); ApproachDirection=(0,1,0).
- **Action:** Call ValidateOneWayApproach.
- **Expected Outcome:** Returns false.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Spec: ClimbableOneWay approach vector validated.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0357
- **Name:** ClimbableOneWayAcceptsCorrectApproach
- **Registry String:** `ClimbingSystem.Detection.ClimbableOneWay.AcceptsCorrectApproachDirection`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::ValidateOneWayApproach
- **Behavior Tested:** Approach opposing normal (front face, dot < 0) returns true.
- **Preconditions:** SurfaceNormal=(0,1,0); ApproachDirection=(0,-1,0).
- **Action:** Call ValidateOneWayApproach.
- **Expected Outcome:** Returns true.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: ClimbableOneWay approach validated.
- **Extends:** TC-0356
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0358
- **Name:** GetSurfaceDataParsesValidTag
- **Registry String:** `ClimbingSystem.Detection.SurfaceData.ParsesValidTag`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSurfaceDataTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::GetSurfaceDataFromComponent
- **Behavior Tested:** Tag "SurfaceData:/Game/Path/Asset.Asset" returns valid UClimbingSurfaceData.
- **Preconditions:** Component with valid SurfaceData: tag.
- **Action:** Call GetSurfaceDataFromComponent.
- **Expected Outcome:** Returns non-null UClimbingSurfaceData.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: SurfaceData:<AssetPath> tag parsing.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0359
- **Name:** GetSurfaceDataMalformedTagReturnsNull
- **Registry String:** `ClimbingSystem.Detection.SurfaceData.MalformedTagReturnsNull`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSurfaceDataTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::GetSurfaceDataFromComponent
- **Behavior Tested:** Tag "SurfaceData:" with no path returns nullptr without crash.
- **Preconditions:** Component with malformed SurfaceData: tag.
- **Action:** Call GetSurfaceDataFromComponent.
- **Expected Outcome:** Returns nullptr; no crash.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Defensive test for malformed tags.
- **Extends:** TC-0358
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0360
- **Name:** GetSurfaceDataMissingAssetReturnsNull
- **Registry String:** `ClimbingSystem.Detection.SurfaceData.MissingAssetReturnsNull`
- **File:** `Source/ClimbingSystem/Tests/ClimbingSurfaceDataTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::GetSurfaceDataFromComponent
- **Behavior Tested:** Well-formed path to non-existent asset returns nullptr without crash.
- **Preconditions:** Component with SurfaceData: tag pointing to non-existent asset.
- **Action:** Call GetSurfaceDataFromComponent.
- **Expected Outcome:** Returns nullptr; no crash.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Missing asset must not crash.
- **Extends:** TC-0358
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0361
- **Name:** IsSurfaceClimbableClimbableTiersTrue
- **Registry String:** `ClimbingSystem.Detection.IsSurfaceClimbable.ClimbableTiersReturnTrue`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::IsSurfaceClimbable
- **Behavior Tested:** Climbable, ClimbableOneWay, and Untagged all return true.
- **Preconditions:** None.
- **Action:** Call IsSurfaceClimbable for each tier.
- **Expected Outcome:** True for Climbable, ClimbableOneWay, Untagged.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: tier-based filtering.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0362
- **Name:** IsSurfaceClimbableUnclimbableTiersFalse
- **Registry String:** `ClimbingSystem.Detection.IsSurfaceClimbable.UnclimbableTiersReturnFalse`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::IsSurfaceClimbable
- **Behavior Tested:** Unclimbable and LadderOnly both return false.
- **Preconditions:** None.
- **Action:** Call IsSurfaceClimbable for each tier.
- **Expected Outcome:** False for Unclimbable, LadderOnly.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: tier-based filtering.
- **Extends:** TC-0361
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0363
- **Name:** SimulatedProxyIKUpdateIntervalThrottles
- **Registry String:** `ClimbingSystem.Multiplayer.SimProxy.IKUpdateIntervalThrottles`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** Simulated proxy IK update interval
- **Behavior Tested:** IK updates throttled to configured interval; no update at half-interval.
- **Preconditions:** Simulated proxy climbing; interval=0.05s.
- **Action:** Tick at 0.025s (no update), then at 0.06s (update).
- **Expected Outcome:** No IK update at 0.025s; one at 0.06s.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: SimulatedProxyIKUpdateInterval=0.05s.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0364
- **Name:** DetectionScanIntervalOnGround
- **Registry String:** `ClimbingSystem.Detection.ScanInterval.GroundLocomotionRate`
- **File:** `Source/ClimbingSystem/Tests/ClimbingDetectionRuntimeTests.cpp`
- **Type:** Component
- **Priority:** P2
- **System Under Test:** Detection scan timer
- **Behavior Tested:** Detection scan fires at DetectionScanInterval(0.05s) during ground locomotion.
- **Preconditions:** Character on ground; None state.
- **Action:** Tick at 0.04s (no scan), then at 0.06s (scan fires).
- **Expected Outcome:** Zero scans at 0.04s; one scan at 0.06s.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: DetectionScanInterval=0.05s on ground.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0365
- **Name:** IntegrationLacheLaunchFlightCatchHangChain
- **Registry String:** `ClimbingSystem.Integration.Lache.LaunchFlightCatchHangChain`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** Full Lache lifecycle
- **Behavior Tested:** Full Lache launch->flight->catch->hang chain with callback ordering verified.
- **Preconditions:** Character hanging; valid Lache target.
- **Action:** Trigger Lache; tick through flight; verify catch and hang.
- **Expected Outcome:** States: Lache->LacheInAir->LacheCatch->Hanging in order.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Full Lache lifecycle integration.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0366
- **Name:** IntegrationLacheMissResultsInFall
- **Registry String:** `ClimbingSystem.Integration.Lache.MissResultsInFall`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** Lache miss path
- **Behavior Tested:** Lache flight with no catch anchor exits to None/falling; capsule restored.
- **Preconditions:** Character in LacheInAir; no valid catch target.
- **Action:** Tick through flight; target invalidated.
- **Expected Outcome:** State = LacheMiss then None; capsule restored.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Miss path must clean up correctly.
- **Extends:** TC-0365
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0367
- **Name:** IntegrationGrabShimmyCornerShimmyDropChain
- **Registry String:** `ClimbingSystem.Integration.StateMachine.GrabShimmyCornerShimmyDropChain`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** Multi-state chain
- **Behavior Tested:** Full None->Hanging->Shimmy->Corner->Hanging->Shimmy->None chain.
- **Preconditions:** Character in None; ledge with corner nearby.
- **Action:** Execute full chain.
- **Expected Outcome:** All transitions valid; OnCornerTransitionMontageBlendingOut fires once.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Multi-state integration.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0368
- **Name:** IntegrationClimbUpCapsuleRestore
- **Registry String:** `ClimbingSystem.Integration.ClimbUp.CapsuleRestoreOnLocomotion`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** Capsule lifecycle
- **Behavior Tested:** After OnClimbUpMontageBlendingOut, capsule dims match pre-climb values.
- **Preconditions:** Character climbed up; montage blending out.
- **Action:** Verify capsule after blend-out.
- **Expected Outcome:** HalfHeight and Radius match original values.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Capsule must restore on exit.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0369
- **Name:** StressRapidGrabSpam
- **Registry String:** `ClimbingSystem.Stress.Input.RapidGrabSpam`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** Input spam resilience
- **Behavior Tested:** 10 IA_Grab presses in 0.5s; character enters Hanging exactly once; no corruption.
- **Preconditions:** Character in None; ledge nearby.
- **Action:** Spam IA_Grab 10 times in 0.5s.
- **Expected Outcome:** State = Hanging; no double-entry; no crash.
- **Edge/Failure Condition:** stress
- **Why This Matters:** Input spam must not corrupt state.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0370
- **Name:** StressRapidNoneHangingCycle
- **Registry String:** `ClimbingSystem.Stress.StateMachine.RapidNoneHangingCycle`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** State machine stress
- **Behavior Tested:** 100x None->Hanging->None cycles; no timer leak, capsule mismatch, or IK residue.
- **Preconditions:** Character in None.
- **Action:** Cycle 100 times.
- **Expected Outcome:** Final state = None; capsule original; IK weights = 0; no timer leaks.
- **Edge/Failure Condition:** stress
- **Why This Matters:** Rapid cycling must not leak resources.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0371
- **Name:** StressFiveClimbersIKBudget
- **Registry String:** `ClimbingSystem.Stress.IK.FiveClimbersExceedsBudget`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** IK budget enforcement
- **Behavior Tested:** 5 simultaneous climbers; exactly 4 have IK active; 5th excluded; no crash.
- **Preconditions:** 5 characters climbing.
- **Action:** Update IK for all.
- **Expected Outcome:** 4 with active IK; 1 with zeroed weights.
- **Edge/Failure Condition:** stress
- **Why This Matters:** Spec: MaxSimultaneousIKCharacters=4.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0372
- **Name:** PerformanceDetectionScanUnder1ms
- **Registry String:** `ClimbingSystem.Performance.Detection.ScanUnder1ms`
- **File:** `Source/ClimbingSystem/Tests/ClimbingPerformanceExtendedTests.cpp`
- **Type:** Performance
- **Priority:** P2
- **System Under Test:** Detection scan performance
- **Behavior Tested:** Single detection scan median < 1ms over 100 samples.
- **Preconditions:** Character in test world with geometry.
- **Action:** Run 100 detection scans; measure time.
- **Expected Outcome:** Median < 1ms; no spike > 5ms.
- **Edge/Failure Condition:** stress
- **Why This Matters:** Detection runs at 20Hz; must be fast.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0373
- **Name:** IntegrationTransitionDuringMontageBlendOut
- **Registry String:** `ClimbingSystem.Integration.StateMachine.TransitionDuringMontageBlendOut`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** State transition during blend-out
- **Behavior Tested:** Drop input during ClimbUp blend-out; callback fires once; final state deterministic.
- **Preconditions:** Character in ClimbingUp; montage blending out.
- **Action:** Inject drop input during blend-out.
- **Expected Outcome:** OnClimbUpMontageBlendingOut fires once; state deterministic.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Blend-out race condition must be handled.
- **Extends:** TC-0368
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0374
- **Name:** NetworkDoubleAttemptGrabBeforeResponse
- **Registry String:** `ClimbingSystem.Network.Server.DoubleAttemptGrabBeforeResponse`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** Server RPC dedup
- **Behavior Tested:** Two Server_AttemptGrab RPCs before first processed; server commits one; second rejected.
- **Preconditions:** Server; two rapid grab RPCs.
- **Action:** Send two Server_AttemptGrab before processing.
- **Expected Outcome:** One Hanging; second rejected.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Duplicate RPC handling.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** YES
- **Added In Run:** 2
- **Notes:** None.

### TC-0375
- **Name:** IntegrationCornerAnchorDestroyedMidTransition
- **Registry String:** `ClimbingSystem.Integration.Corner.AnchorDestroyedMidTransition`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** Anchor destruction during committed state
- **Behavior Tested:** Anchor destroyed during CornerTransition; character exits to None safely.
- **Preconditions:** Character in CornerTransition; anchor valid.
- **Action:** Destroy anchor actor.
- **Expected Outcome:** State = None; no crash.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Committed state must handle anchor loss.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0376
- **Name:** IntegrationTimerPauseUnpause
- **Registry String:** `ClimbingSystem.Integration.Timer.PauseUnpauseRespectsBIgnorePause`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** Timer pause behavior
- **Behavior Tested:** Detection and FallingGrab timers do not fire during game pause; resume on unpause.
- **Preconditions:** Character with active timers.
- **Action:** Pause game; tick; unpause; tick.
- **Expected Outcome:** No timer fires during pause; fires resume after unpause.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: all timers bIgnorePause=false.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0377
- **Name:** IntegrationCapsuleSizeAtEachTransition
- **Registry String:** `ClimbingSystem.Integration.Capsule.SizeVerifiedAtEachTransition`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** Capsule lifecycle
- **Behavior Tested:** Capsule dims recorded at every state boundary; resized on entry, restored on exit.
- **Preconditions:** Character in None.
- **Action:** Full lifecycle: grab->shimmy->drop; verify capsule at each transition.
- **Expected Outcome:** Climbing dims during climb; original dims after exit.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Capsule must be correct at every state.
- **Extends:** TC-0368
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0378
- **Name:** IntegrationIMCStackFullLifecycle
- **Registry String:** `ClimbingSystem.Integration.Input.IMCStackFullLifecycle`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** IMC lifecycle
- **Behavior Tested:** Climbing IMC pushed on grab, popped on every exit path.
- **Preconditions:** Character in None.
- **Action:** Grab->drop; grab->climbUp; grab->lache miss; verify IMC after each.
- **Expected Outcome:** Base IMC only after each exit; no climbing IMC residue.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** IMC must be clean after every exit path.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0379
- **Name:** NetworkClientPredictServerConfirmProxyUpdate
- **Registry String:** `ClimbingSystem.Network.Replication.ClientPredictServerConfirmProxyUpdate`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** Full replication round-trip
- **Behavior Tested:** Client predicts Hanging; server confirms; proxy receives replicated state.
- **Preconditions:** Listen server with client.
- **Action:** Client grabs; server confirms; check proxy.
- **Expected Outcome:** All three (client, server, proxy) in Hanging; no rollback.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Full replication flow.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** YES
- **Added In Run:** 2
- **Notes:** None.

### TC-0380
- **Name:** PerformanceStateTransitionThroughput
- **Registry String:** `ClimbingSystem.Performance.StateMachine.TransitionThroughput`
- **File:** `Source/ClimbingSystem/Tests/ClimbingPerformanceExtendedTests.cpp`
- **Type:** Performance
- **Priority:** P2
- **System Under Test:** State transition performance
- **Behavior Tested:** 1000 state transitions complete in < 10ms total.
- **Preconditions:** Character initialized.
- **Action:** Run 1000 transitions; measure time.
- **Expected Outcome:** Total < 10ms (< 10us each).
- **Edge/Failure Condition:** stress
- **Why This Matters:** Transitions must be fast.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0381
- **Name:** IntegrationLadderTransitionMontageGatesState
- **Registry String:** `ClimbingSystem.Integration.Ladder.TransitionMontageGatesState`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P2
- **System Under Test:** Ladder transition committed state
- **Behavior Tested:** Movement input during LadderTransition does not prematurely exit committed state.
- **Preconditions:** Character in LadderTransition; montage playing.
- **Action:** Send movement input during montage.
- **Expected Outcome:** State remains LadderTransition until OnLadderTransitionMontageBlendingOut.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Committed state must block input.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0382
- **Name:** IntegrationRagdollIMCRestored
- **Registry String:** `ClimbingSystem.Integration.Ragdoll.IMCRestoredAfterRecovery`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P2
- **System Under Test:** IMC after ragdoll recovery
- **Behavior Tested:** After ragdoll recovery from Hanging, IMC stack has only base locomotion IMC.
- **Preconditions:** Character was climbing; entered ragdoll; recovered.
- **Action:** Check IMC stack after recovery.
- **Expected Outcome:** No climbing IMC; only locomotion IMC.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Ragdoll recovery must clean up IMC.
- **Extends:** TC-0378
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0383
- **Name:** IntegrationMantleAnchorDestroyedDuringCommitted
- **Registry String:** `ClimbingSystem.Integration.Mantle.AnchorDestroyedDuringCommittedState`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P2
- **System Under Test:** Anchor destruction during mantle
- **Behavior Tested:** Anchor destroyed during committed mantle montage; safe exit to None.
- **Preconditions:** Character in Mantling; anchor valid.
- **Action:** Destroy anchor actor.
- **Expected Outcome:** State = None; no crash.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Committed state must handle anchor loss.
- **Extends:** TC-0375
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0384
- **Name:** NetworkServerDetectionReRunOnEveryRPC
- **Registry String:** `ClimbingSystem.Network.Server.DetectionReRunOnEveryRPC`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** Server detection re-run
- **Behavior Tested:** Server re-runs full detection scan on each Server_AttemptGrab call.
- **Preconditions:** Server; stale client data.
- **Action:** Call Server_AttemptGrab with stale data; verify server re-runs detection.
- **Expected Outcome:** Server uses its own detection result, not client's.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: server re-runs detection.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** YES
- **Added In Run:** 2
- **Notes:** None.

### TC-0385
- **Name:** IntegrationAllStatesEntryExitSymmetry
- **Registry String:** `ClimbingSystem.Integration.StateMachine.AllStatesEntryExitSymmetry`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** State machine symmetry
- **Behavior Tested:** Entry+exit called exactly once per transition for all 17 EClimbingState values.
- **Preconditions:** Character initialized.
- **Action:** Script transitions through all 17 states; count entry/exit calls.
- **Expected Outcome:** Each state has exactly 1 entry and 1 exit call.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Entry/exit symmetry prevents resource leaks.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0386
- **Name:** StressNetworkRapidServerGrabSpam
- **Registry String:** `ClimbingSystem.Stress.Network.RapidServerAttemptGrabSpam`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P2
- **System Under Test:** Server RPC spam resilience
- **Behavior Tested:** 20 Server_AttemptGrab RPCs in 1s; server remains stable.
- **Preconditions:** Server; rapid RPCs from one client.
- **Action:** Send 20 RPCs in 1s.
- **Expected Outcome:** No crash; no state corruption; at most 1 accepted.
- **Edge/Failure Condition:** stress
- **Why This Matters:** Server must handle RPC spam.
- **Extends:** TC-0374
- **Requires World:** YES
- **Requires Network:** YES
- **Added In Run:** 2
- **Notes:** None.

### TC-0387
- **Name:** IntegrationIKWeightZeroOnStateExit
- **Registry String:** `ClimbingSystem.Integration.IK.WeightZeroOnStateExit`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P1
- **System Under Test:** IK weight cleanup
- **Behavior Tested:** IK weight reaches 0 on every exit path (drop, ClimbUp, ragdoll, anchor destroy).
- **Preconditions:** Character climbing with active IK.
- **Action:** Test each exit path; verify IK weights after.
- **Expected Outcome:** All 4 IK weights = 0 after each exit.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Residual IK weights cause visual artifacts.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0388
- **Name:** IntegrationLacheCapsuleRestoredOnMiss
- **Registry String:** `ClimbingSystem.Integration.Lache.CapsuleRestoredOnMiss`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P2
- **System Under Test:** Capsule restore after Lache miss
- **Behavior Tested:** Capsule dimensions restored after Lache miss/fall path.
- **Preconditions:** Character in LacheMiss.
- **Action:** Complete miss; verify capsule.
- **Expected Outcome:** Original capsule dims restored.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Lache miss must restore capsule.
- **Extends:** TC-0366
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0389
- **Name:** PerformanceIKSixClimbersGracefulDegradation
- **Registry String:** `ClimbingSystem.Performance.IK.SixClimbersGracefulDegradation`
- **File:** `Source/ClimbingSystem/Tests/ClimbingPerformanceExtendedTests.cpp`
- **Type:** Performance
- **Priority:** P2
- **System Under Test:** IK budget with excess climbers
- **Behavior Tested:** 6 climbers; IK active on exactly 4; tick budget still met.
- **Preconditions:** 6 characters climbing.
- **Action:** Update IK; measure time.
- **Expected Outcome:** 4 active IK; 2 zeroed; no crash; budget met.
- **Edge/Failure Condition:** stress
- **Why This Matters:** Graceful degradation beyond budget.
- **Extends:** TC-0371
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0390
- **Name:** IntegrationIMCNotDoubledOnRapidGrabDrop
- **Registry String:** `ClimbingSystem.Integration.Input.IMCNotDoubledOnRapidGrabDrop`
- **File:** `Source/ClimbingSystem/Tests/ClimbingIntegrationExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P2
- **System Under Test:** IMC stack integrity
- **Behavior Tested:** Rapid grab+drop 10 times; IMC stack depth never exceeds 1 climbing context.
- **Preconditions:** Character in None.
- **Action:** Grab+drop 10 times rapidly.
- **Expected Outcome:** IMC stack has at most 1 climbing context at any point.
- **Edge/Failure Condition:** stress
- **Why This Matters:** IMC double-add would shadow locomotion.
- **Extends:** TC-0378
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0391
- **Name:** NetworkProxyMontageNotPlayedTwice
- **Registry String:** `ClimbingSystem.Network.Replication.ProxyMontageNotPlayedTwice`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P2
- **System Under Test:** Proxy montage dedup
- **Behavior Tested:** Proxy receives replicated state; montage plays exactly once; no double-play.
- **Preconditions:** Simulated proxy; state replicated.
- **Action:** Trigger OnRep; verify montage play count.
- **Expected Outcome:** Montage plays exactly once.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Double-play causes animation glitches.
- **Extends:** TC-0379
- **Requires World:** YES
- **Requires Network:** YES
- **Added In Run:** 2
- **Notes:** None.

### TC-0392
- **Name:** NetworkTwoClimbersIndependentState
- **Registry String:** `ClimbingSystem.Network.MultiClient.TwoClimbersIndependentState`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMultiplayerExtendedTests.cpp`
- **Type:** Integration
- **Priority:** P2
- **System Under Test:** Multi-client independence
- **Behavior Tested:** Two clients climbing simultaneously; independent states; no cross-contamination.
- **Preconditions:** Listen server; two clients climbing.
- **Action:** One client drops; verify other still climbing.
- **Expected Outcome:** Dropped client = None; other = Hanging; no cross-contamination.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Multi-client state independence.
- **Extends:** TC-0379
- **Requires World:** YES
- **Requires Network:** YES
- **Added In Run:** 2
- **Notes:** None.

### TC-0393
- **Name:** PerformanceIKTickBudgetFourClimbers
- **Registry String:** `ClimbingSystem.Performance.IK.TickBudgetFourClimbers`
- **File:** `Source/ClimbingSystem/Tests/ClimbingPerformanceExtendedTests.cpp`
- **Type:** Performance
- **Priority:** P2
- **System Under Test:** IK tick performance
- **Behavior Tested:** Combined IK tick for 4 simultaneous climbers < 2ms median.
- **Preconditions:** 4 characters climbing.
- **Action:** Measure IK tick time over 100 frames.
- **Expected Outcome:** Median < 2ms; no frame > 5ms.
- **Edge/Failure Condition:** stress
- **Why This Matters:** IK budget must be met.
- **Extends:** TC-0371
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0394
- **Name:** PerformanceFallingGrabCheckIntervalDrift
- **Registry String:** `ClimbingSystem.Performance.Detection.FallingGrabCheckIntervalNoDrift`
- **File:** `Source/ClimbingSystem/Tests/ClimbingPerformanceExtendedTests.cpp`
- **Type:** Performance
- **Priority:** P2
- **System Under Test:** FallingGrabCheck timer accuracy
- **Behavior Tested:** FallingGrabCheck fires at correct 0.05s interval over 100 cycles; no drift.
- **Preconditions:** Character falling.
- **Action:** Measure 100 timer fires.
- **Expected Outcome:** Median interval ~0.05s; no drift > 0.01s.
- **Edge/Failure Condition:** stress
- **Why This Matters:** Timer drift would affect grab responsiveness.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0395
- **Name:** ClimbUpBlendOutTransitionsToNone
- **Registry String:** `ClimbingSystem.MontageCallback.ClimbUp.BlendOutTransitionsToNone`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnClimbUpMontageBlendingOut
- **Behavior Tested:** bInterrupted=false transitions to EClimbingState::None.
- **Preconditions:** Character in ClimbingUp; montage blending out normally.
- **Action:** Trigger blend-out callback with bInterrupted=false.
- **Expected Outcome:** State = None.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: ClimbUp montage completion exits to None.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0396
- **Name:** ClimbUpBlendOutInterruptedNoTransition
- **Registry String:** `ClimbingSystem.MontageCallback.ClimbUp.InterruptedNoTransition`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnClimbUpMontageBlendingOut
- **Behavior Tested:** bInterrupted=true does not transition state.
- **Preconditions:** Character in ClimbingUp; montage interrupted.
- **Action:** Trigger blend-out callback with bInterrupted=true.
- **Expected Outcome:** State unchanged (still ClimbingUp or whatever interrupted it to).
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Interrupted montage must not double-transition.
- **Extends:** TC-0395
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0397
- **Name:** CornerBlendOutTransitionsToShimmying
- **Registry String:** `ClimbingSystem.MontageCallback.Corner.BlendOutTransitionsToShimmying`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnCornerTransitionMontageBlendingOut
- **Behavior Tested:** Corner montage done transitions to Shimmying (resume shimmy).
- **Preconditions:** Character in CornerTransition; montage blending out.
- **Action:** Trigger blend-out callback.
- **Expected Outcome:** State = Shimmying or Hanging (resume).
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: corner completion resumes shimmy.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0398
- **Name:** LadderTransitionBlendOutToOnLadder
- **Registry String:** `ClimbingSystem.MontageCallback.LadderTransition.BlendOutToOnLadder`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnLadderTransitionMontageBlendingOut
- **Behavior Tested:** Valid anchor after ladder transition montage transitions to OnLadder.
- **Preconditions:** Character in LadderTransition; anchor valid.
- **Action:** Trigger blend-out callback.
- **Expected Outcome:** State = OnLadder.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: ladder transition -> OnLadder.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0399
- **Name:** LadderTransitionBlendOutToNoneWhenInvalid
- **Registry String:** `ClimbingSystem.MontageCallback.LadderTransition.BlendOutToNoneWhenInvalid`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnLadderTransitionMontageBlendingOut
- **Behavior Tested:** Invalidated anchor after ladder transition transitions to None.
- **Preconditions:** Character in LadderTransition; anchor invalidated.
- **Action:** Trigger blend-out callback.
- **Expected Outcome:** State = None.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Invalid anchor must exit cleanly.
- **Extends:** TC-0398
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0400
- **Name:** LacheLaunchBlendOutToLacheInAir
- **Registry String:** `ClimbingSystem.MontageCallback.LacheLaunch.BlendOutToLacheInAir`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnLacheLaunchMontageBlendingOut
- **Behavior Tested:** Launch montage done transitions to LacheInAir.
- **Preconditions:** Character in Lache; launch montage blending out.
- **Action:** Trigger blend-out callback.
- **Expected Outcome:** State = LacheInAir.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: launch -> LacheInAir.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0401
- **Name:** LacheCatchBlendOutToHanging
- **Registry String:** `ClimbingSystem.MontageCallback.LacheCatch.BlendOutToHanging`
- **File:** `Source/ClimbingSystem/Tests/ClimbingMovementFlowTests.cpp`
- **Type:** Component
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnLacheCatchMontageBlendingOut
- **Behavior Tested:** Catch montage done transitions to Hanging.
- **Preconditions:** Character in LacheCatch; montage blending out.
- **Action:** Trigger blend-out callback.
- **Expected Outcome:** State = Hanging.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: catch -> Hanging.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0402
- **Name:** InputGrabRejectedDuringCommittedState
- **Registry String:** `ClimbingSystem.Input.Grab.RejectedDuringCommittedState`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Input_Grab
- **Behavior Tested:** IA_Grab rejected during ClimbingUp (committed state).
- **Preconditions:** Character in ClimbingUp.
- **Action:** Call Input_Grab.
- **Expected Outcome:** No state change; no Server_AttemptGrab sent.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Committed states block input.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0403
- **Name:** InputGrabCallsServerAttemptGrab
- **Registry String:** `ClimbingSystem.Input.Grab.CallsServerAttemptGrabWhenClimbable`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Input_Grab
- **Behavior Tested:** None state + climbable surface triggers Server_AttemptGrab.
- **Preconditions:** Character in None; valid climbable surface detected.
- **Action:** Call Input_Grab.
- **Expected Outcome:** Server_AttemptGrab called; local state predicted.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Grab input happy path.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0404
- **Name:** InputDropFromHangingCallsServerDrop
- **Registry String:** `ClimbingSystem.Input.Drop.FromHangingCallsServerDrop`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Input_Drop
- **Behavior Tested:** IA_Drop from Hanging calls Server_Drop.
- **Preconditions:** Character in Hanging.
- **Action:** Call Input_Drop.
- **Expected Outcome:** Server_Drop called.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Drop from Hanging is primary exit.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0405
- **Name:** InputDropFromCommittedStateRejected
- **Registry String:** `ClimbingSystem.Input.Drop.FromCommittedStateRejected`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Input_Drop
- **Behavior Tested:** IA_Drop from ClimbingUp (committed) is rejected.
- **Preconditions:** Character in ClimbingUp.
- **Action:** Call Input_Drop.
- **Expected Outcome:** No Server_Drop; state unchanged.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Committed states block drop.
- **Extends:** TC-0404
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0406
- **Name:** InputLacheOnlyFromHangingOrShimmying
- **Registry String:** `ClimbingSystem.Input.Lache.OnlyFromHangingOrShimmying`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Input_Lache
- **Behavior Tested:** IA_Lache from OnLadder is rejected.
- **Preconditions:** Character in OnLadder.
- **Action:** Call Input_Lache.
- **Expected Outcome:** No Lache; state unchanged.
- **Edge/Failure Condition:** failure mode
- **Why This Matters:** Spec: Lache only from Hanging/Shimmying.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0407
- **Name:** InputJumpDuringHangingTriggersLache
- **Registry String:** `ClimbingSystem.Input.Jump.DuringHangingTriggersLache`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Input_JumpStarted
- **Behavior Tested:** Jump during Hanging triggers Lache, not ACharacter::Jump.
- **Preconditions:** Character in Hanging.
- **Action:** Call Input_JumpStarted.
- **Expected Outcome:** Lache arc initiated; no normal jump.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: Jump while hanging triggers Lache.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0408
- **Name:** InputSprintSetsBSprintHeld
- **Registry String:** `ClimbingSystem.Input.Sprint.SetsBSprintHeld`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::Input_Sprint
- **Behavior Tested:** Input_Sprint sets bSprintHeld=true.
- **Preconditions:** bSprintHeld=false.
- **Action:** Call Input_Sprint.
- **Expected Outcome:** bSprintHeld=true.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: IA_Sprint fast ladder ascent modifier.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0409
- **Name:** InputSprintCompletedClearsBSprintHeld
- **Registry String:** `ClimbingSystem.Input.Sprint.CompletedClearsBSprintHeld`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::Input_SprintCompleted
- **Behavior Tested:** Input_SprintCompleted clears bSprintHeld=false.
- **Preconditions:** bSprintHeld=true.
- **Action:** Call Input_SprintCompleted.
- **Expected Outcome:** bSprintHeld=false.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Sprint release must clear flag.
- **Extends:** TC-0408
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0410
- **Name:** InputCrouchSetsBCrouchHeld
- **Registry String:** `ClimbingSystem.Input.Crouch.SetsBCrouchHeld`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::Input_Crouch
- **Behavior Tested:** Input_Crouch sets bCrouchHeld=true.
- **Preconditions:** bCrouchHeld=false.
- **Action:** Call Input_Crouch.
- **Expected Outcome:** bCrouchHeld=true.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: IA_Crouch fast ladder descent modifier.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0411
- **Name:** InputCrouchCompletedClearsBCrouchHeld
- **Registry String:** `ClimbingSystem.Input.Crouch.CompletedClearsBCrouchHeld`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::Input_CrouchCompleted
- **Behavior Tested:** Input_CrouchCompleted clears bCrouchHeld=false.
- **Preconditions:** bCrouchHeld=true.
- **Action:** Call Input_CrouchCompleted.
- **Expected Outcome:** bCrouchHeld=false.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Crouch release must clear flag.
- **Extends:** TC-0410
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0412
- **Name:** InputClimbMoveCompletedZerosInput
- **Registry String:** `ClimbingSystem.Input.ClimbMove.CompletedZerosCurrentInput`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::Input_ClimbMoveCompleted
- **Behavior Tested:** Input_ClimbMoveCompleted zeros CurrentClimbMoveInput.
- **Preconditions:** CurrentClimbMoveInput = (0.5, 0.3).
- **Action:** Call Input_ClimbMoveCompleted.
- **Expected Outcome:** CurrentClimbMoveInput = (0, 0).
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Releasing input must zero the vector.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0413
- **Name:** CapsuleResizedOnClimbEntry
- **Registry String:** `ClimbingSystem.Capsule.Resize.OnClimbEntry`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateEnter capsule
- **Behavior Tested:** Capsule HalfHeight=48, Radius=24 on Hanging entry.
- **Preconditions:** Character in None; original capsule dims.
- **Action:** Transition to Hanging.
- **Expected Outcome:** HalfHeight=48; Radius=24.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: ClimbingCapsuleHalfHeight=48, Radius=24.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0414
- **Name:** CapsuleRestoredOnExitToNone
- **Registry String:** `ClimbingSystem.Capsule.Restore.OnExitToNone`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateExit capsule
- **Behavior Tested:** Original capsule dimensions restored on None transition.
- **Preconditions:** Character in Hanging with climbing capsule.
- **Action:** Transition to None.
- **Expected Outcome:** HalfHeight and Radius match original values.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Capsule must restore for locomotion.
- **Extends:** TC-0413
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0415
- **Name:** ClimbingCollisionProfileSetOnEntry
- **Registry String:** `ClimbingSystem.Capsule.CollisionProfile.SetOnEntry`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateEnter collision profile
- **Behavior Tested:** ClimbingCollisionProfile="ClimbingCapsule" set on climb entry; restored on exit.
- **Preconditions:** Character in None; default collision profile.
- **Action:** Transition to Hanging; check profile; transition to None; check profile.
- **Expected Outcome:** "ClimbingCapsule" during climb; original after exit.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: ClimbingCollisionProfile switches on entry.
- **Extends:** TC-0413
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0416
- **Name:** HangingCharacterOffsetApplied
- **Registry String:** `ClimbingSystem.StateMachine.Hanging.CharacterOffsetApplied`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateEnter(Hanging)
- **Behavior Tested:** HangingCharacterOffset(0,0,-30) applied on Hanging entry; removed on exit.
- **Preconditions:** Character in None.
- **Action:** Transition to Hanging; check mesh offset; transition to None; check offset.
- **Expected Outcome:** Mesh Z offset = -30 during Hanging; 0 after exit.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: HangingCharacterOffset=FVector(0,0,-30).
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0417
- **Name:** GetMontageForSlotReturnsOverride
- **Registry String:** `ClimbingSystem.Animation.GetMontageForSlot.ReturnsOverrideWhenActive`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimationSetTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::GetMontageForSlot
- **Behavior Tested:** AnimationSetOverride takes precedence over character defaults.
- **Preconditions:** Override has HangIdle montage; character has different default.
- **Action:** Call GetMontageForSlot(HangIdle).
- **Expected Outcome:** Returns override montage, not character default.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: override priority over character defaults.
- **Extends:** NONE
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0418
- **Name:** GetMontageForSlotFallsBackToDefault
- **Registry String:** `ClimbingSystem.Animation.GetMontageForSlot.FallsBackToCharacterDefault`
- **File:** `Source/ClimbingSystem/Tests/ClimbingAnimationSetTests.cpp`
- **Type:** Unit
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::GetMontageForSlot
- **Behavior Tested:** Null override slot returns character default montage.
- **Preconditions:** Override has null for ShimmyLeft; character has default ShimmyLeft.
- **Action:** Call GetMontageForSlot(ShimmyLeft).
- **Expected Outcome:** Returns character default montage.
- **Edge/Failure Condition:** edge case
- **Why This Matters:** Spec: per-slot fallback.
- **Extends:** TC-0417
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0419
- **Name:** WarpTargetLedgeGrabRegistered
- **Registry String:** `ClimbingSystem.WarpTarget.LedgeGrab.RegisteredOnGrab`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateEnter(Hanging)
- **Behavior Tested:** WarpTarget_LedgeGrab registered before GrabLedge montage plays.
- **Preconditions:** Character in None; valid detection result; MotionWarping present.
- **Action:** Transition to Hanging.
- **Expected Outcome:** WarpTarget_LedgeGrab registered on MotionWarpingComponent.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: WarpTarget_LedgeGrab for GrabLedge animation.
- **Extends:** NONE
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0420
- **Name:** WarpTargetClimbUpLandRegistered
- **Registry String:** `ClimbingSystem.WarpTarget.ClimbUpLand.RegisteredOnClimbUp`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateEnter(ClimbingUp)
- **Behavior Tested:** WarpTarget_ClimbUpLand registered before ClimbUp montage plays.
- **Preconditions:** Character in Hanging; valid clearance.
- **Action:** Transition to ClimbingUp.
- **Expected Outcome:** WarpTarget_ClimbUpLand registered.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: WarpTarget_ClimbUpLand for ClimbUp animation.
- **Extends:** TC-0419
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0421
- **Name:** WarpTargetLacheCatchRegistered
- **Registry String:** `ClimbingSystem.WarpTarget.LacheCatch.RegisteredOnCatch`
- **File:** `Source/ClimbingSystem/Tests/ClimbingCharacterWorldTests.cpp`
- **Type:** World
- **Priority:** P1
- **System Under Test:** AClimbingCharacter::OnStateEnter(LacheCatch)
- **Behavior Tested:** WarpTarget_LacheCatch registered before LacheCatch montage plays.
- **Preconditions:** Character in LacheInAir; valid catch target.
- **Action:** Transition to LacheCatch.
- **Expected Outcome:** WarpTarget_LacheCatch registered.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Spec: WarpTarget_LacheCatch for catch animation.
- **Extends:** TC-0419
- **Requires World:** YES
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0422
- **Name:** InputDropFromOnLadderCallsServerDrop
- **Registry String:** `ClimbingSystem.Input.Drop.FromOnLadderCallsServerDrop`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::Input_Drop
- **Behavior Tested:** IA_Drop from OnLadder calls Server_Drop.
- **Preconditions:** Character in OnLadder.
- **Action:** Call Input_Drop.
- **Expected Outcome:** Server_Drop called.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Drop valid from OnLadder (interruptible).
- **Extends:** TC-0404
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0423
- **Name:** InputJumpDuringLocomotionTriggersNormalJump
- **Registry String:** `ClimbingSystem.Input.Jump.DuringLocomotionTriggersNormalJump`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::Input_JumpStarted
- **Behavior Tested:** Jump during None state triggers ACharacter::Jump, not Lache.
- **Preconditions:** Character in None.
- **Action:** Call Input_JumpStarted.
- **Expected Outcome:** Normal jump; no Lache arc.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Jump must behave normally outside climbing.
- **Extends:** TC-0407
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

### TC-0424
- **Name:** InputDropFromShimmyingCallsServerDrop
- **Registry String:** `ClimbingSystem.Input.Drop.FromShimmyingCallsServerDrop`
- **File:** `Source/ClimbingSystem/Tests/ClimbingInputContextRuntimeTests.cpp`
- **Type:** Unit
- **Priority:** P2
- **System Under Test:** AClimbingCharacter::Input_Drop
- **Behavior Tested:** IA_Drop from Shimmying calls Server_Drop.
- **Preconditions:** Character in Shimmying.
- **Action:** Call Input_Drop.
- **Expected Outcome:** Server_Drop called.
- **Edge/Failure Condition:** happy path
- **Why This Matters:** Drop valid from Shimmying (interruptible).
- **Extends:** TC-0404
- **Requires World:** NO
- **Requires Network:** NO
- **Added In Run:** 2
- **Notes:** None.

---

## COVERAGE SUMMARY
<!-- Updated every run -->

| System | TCs Implemented | TCs Planned | Coverage Est. | Last Updated |
|--------|----------------|-------------|---------------|--------------|
| Types/Structs/Enums | 7 | 0 | ~100% | Run 1 |
| SurfaceData | 2 | 8 | ~70% | Run 2 |
| Traces | 5 | 1 | ~70% | Run 1 |
| Detection (Ledge/Mantle/Braced/Wall) | 19 | 25 | ~70% | Run 2 |
| Priority | 3 | 0 | ~70% | Run 1 |
| Movement Component | 18 | 8 | ~70% | Run 2 |
| State Machine / Montage Callbacks | 4 | 14 | ~70% | Run 2 |
| Actions (Shimmy) | 4 | 8 | ~70% | Run 1 |
| Actions (ClimbUp) | 5 | 1 | ~70% | Run 1 |
| Actions (Corner) | 1 | 7 | ~70% | Run 1 |
| Actions (BracedWall) | 1 | 11 | ~70% | Run 1 |
| Actions (Lache) | 5 | 2 | ~70% | Run 1 |
| Actions (Mantle) | 3 | 8 | ~70% | Run 2 |
| Ladder | 1 | 8 | ~70% | Run 2 |
| Ragdoll/Physics | 2 | 18 | ~70% | Run 2 |
| IK System | 3 | 17 | ~70% | Run 2 |
| Animation (AnimInstance/Set/Notifies) | 11 | 12 | ~70% | Run 2 |
| Audio | 3 | 7 | ~70% | Run 2 |
| Camera | 1 | 8 | ~70% | Run 2 |
| Multiplayer (RPCs/Replication) | 12 | 30 | ~70% | Run 2 |
| Input (IMC/Handlers) | 6 | 20 | ~70% | Run 2 |
| Debug Visualization | 0 | 6 | ~30% | Run 1 |
| Lifecycle (BeginPlay/EndPlay/Destroyed) | 0 | 13 | ~70% | Run 1 |
| Freefall / Coyote Time | 1 | 8 | ~70% | Run 2 |
| Integration | 10 | 20 | ~70% | Run 2 |
| Performance / Stress | 4 | 10 | ~70% | Run 2 |
| Contracts | 4 | 0 | ~70% | Run 1 |
| Arc | 3 | 1 | ~70% | Run 1 |
| Capsule Management | 0 | 4 | ~70% | Run 2 |
| Warp Targets | 0 | 5 | ~30% | Run 2 |

---