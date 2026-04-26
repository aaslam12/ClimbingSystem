# TEST_REGISTRY.md
# UE5 Multiplayer Climbing System — Test Case Registry
# Last updated: 2026-04-25T21:40:28-07:00
# Total registered: 620 | Implemented: 620 | Planned: 0 | Manual QA: 3

---

## RUN HISTORY
| Run # | Date | New TCs Added | Focus Areas | Agent Notes |
|-------|------|--------------|-------------|-------------|
| 1     | 2026-04-25 | 122 | Detection, Traces, StateMachine, Movement, Actions, Shimmy, Corner, BracedWall, Lache, Mantle, Ladder, Ragdoll, Physics, IK, Animation, Audio, Camera, Multiplayer, Input, Debug, Lifecycle | 5-agent coordinated fleet; each agent assigned non-overlapping systems and TC-ID ranges |
| 2     | 2026-04-25 | 150 | Mantle depth, Ladder depth, Ragdoll/Physics depth, IK/Camera/Audio/SurfaceData depth, Multiplayer RPCs, Freefall/CoyoteTime, ClimbableOneWay, Integration/Stress/Performance, MontageCallbacks, InputHandlers, Capsule, WarpTargets | 5-agent fleet Run 2; focused on ~30% coverage areas, failure/stress/edge cases, untested functions |
| 3     | 2026-04-25 | 136 | Uncovered functions (ClassifyHangType, TickLadderState, TickLacheInAirState, Input_Move/Look, bAutoLacheCinematic, MinLedgeDepth, ShimmyReposition, OverhangPenalty, NativeInit/Update, OnClimbingStateReplicated, UpdateBracedWallIK), all 10 warp targets, debug visualization depth, animation gaps (HangIdleLeft/Right, DropDown slots, LadderFastAscend/Descend, GrabFail, ClimbingMontageSlot), IK failure modes, shimmy playback rate, ladder tick/exit, detection frequency, zero-parameter edge cases, full lifecycle integrations, network round-trips | 5-agent fleet Run 3; pushed all systems toward ~100% coverage |
| 4     | 2026-04-25 | 60 | Priority (equal-distance, OneWay, LadderOnly, multi-ledge, Unclimbable, boundary, determinism), ClimbUp (CrouchFullFlow, FromShimmying, CapsuleSweep, NullMontage, WarpPosition/Rotation, ServerReRun, CrouchMontage, InterruptedBlendOut), Corner (OutsideLeft, InsideRight, DotZero, FromBraced, NullMontage, TraceDistance, DistanceReset, Replication, BracedAngle), Lifecycle (NullMotionWarping, ScanTimer, StateConfigs, DestroyedDuringMontage, EndPlayLache/Ragdoll, EndPlayIKManager, DestroyedCorner), Performance/Stress (HeavyGeometry, RpcThroughput, MemoryStability, MontageThroughput, IK4Limbs, MaxGrid, FullCleanup, AnchorPerf, AudioPerf, 10Climbers, ShimmyStress, LacheStress, GetMontagePerf, ResolveHitPerf, OneWayPerf) | 3-agent fleet Run 4; closed final 5 gaps to reach ~100% planned coverage across all 30 systems |
| SYNC  | 2026-04-25 | 468 | ALL | All planned TCs synced to implemented | Registry synced from source scan — 468 TCs moved, 24 batch files verified |

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
| TC-0153 | LedgeDetectionSurfaceNormalPointsTowardCharacter | Detection | The SurfaceNormal in the detection result points toward the character (positive  | happy path | Component |
| TC-0154 | LedgeDetectionArcUsesActualVelocityWhenFalling | Detection | When the character is falling, PerformLedgeDetection uses the actual velocity fo | edge case | Component |
| TC-0155 | LedgeDetectionAtLocationReturnsValidForReachableLedge | Detection | PerformLedgeDetectionAtLocation returns a valid result when a ledge exists withi | happy path | Component |
| TC-0156 | LedgeDetectionAtLocationReturnsInvalidWhenNoGeometry | Detection | PerformLedgeDetectionAtLocation returns bValid == false when no geometry exists  | failure mode | Component |
| TC-0157 | LedgeDetectionAtLocationSurfaceTierFromSurfaceData | Detection | When the hit component has a SurfaceData: tag, PerformLedgeDetectionAtLocation r | edge case | Component |
| TC-0158 | ResolveHitComponentFromNetUsesConfirmationRadius | Detection | ResolveHitComponentFromNet succeeds within ConfirmationTraceRadius and fails jus | edge case | Component |
| TC-0159 | ResolveHitComponentFromNetRejectsZeroNormal | Detection | ResolveHitComponentFromNet returns nullptr when SurfaceNormal is zero (degenerat | failure mode | Component |
| TC-0160 | LedgeDetectionNoResultWhenNoGeometry | Detection | PerformLedgeDetection returns bValid == false in an empty world with no geometry | failure mode | Component |
| TC-0161 | BracedWallDetectionIgnoresUnclimbableTag | Detection | PerformBracedWallDetection returns invalid when the wall component is tagged Unc | failure mode | Component |
| TC-0162 | BracedWallDetectionSetsCorrectSurfaceTier | Detection | PerformBracedWallDetection sets SurfaceTier to Climbable when tagged Climbable,  | happy path | Component |
| TC-0163 | LedgeDetectionMaxSurfaceAngleRejectsSlope | Detection | A ledge surface whose normal angle from vertical exceeds MaxClimbableSurfaceAngl | edge case | Component |
| TC-0164 | LedgeDetectionMaxSurfaceAngleAcceptsWithinLimit | Detection | A ledge surface whose normal angle from vertical is within MaxClimbableSurfaceAn | happy path | Component |
| TC-0165 | LedgeDetectionArcSamplesMinimumThreeNoCrash | Traces | Setting LedgeArcSamples to the minimum clamped value of 3 does not crash and sti | edge case | Component |
| TC-0166 | LedgeDetectionHitComponentIsValidOnSuccess | Detection | When PerformLedgeDetection returns bValid == true, HitComponent is a non-null va | happy path | Component |
| TC-0167 | LedgeDetectionIgnoresCharacterOwnCapsule | Detection | PerformLedgeDetection does not return a hit on the character's own capsule compo | failure mode | Component |
| TC-0168 | MantleHighMontageSelectedAboveThreshold | StateMachine | MantleHigh montage is selected when MantleHeight > MantleLowMaxHeight. | happy path | World |
| TC-0169 | CornerInsideLeftMontageSelected | StateMachine | CornerInsideLeft montage is selected when bCurrentCornerIsInside=true and Commit | happy path | World |
| TC-0170 | CornerOutsideRightMontageSelected | StateMachine | CornerOutsideRight montage is selected when bCurrentCornerIsInside=false and Com | happy path | World |
| TC-0171 | LadderTransitionTopMontageWhenValidDetection | StateMachine | LadderExitTop montage is selected when DetectionResult.bValid is true (exiting a | happy path | World |
| TC-0172 | LadderTransitionBottomMontageWhenInvalidDetection | StateMachine | LadderExitBottom montage is selected when DetectionResult.bValid is false (exiti | edge case | World |
| TC-0173 | MontageSlotMismatchLogsWarning | StateMachine | When Montage_Play returns <= 0 (slot name mismatch), a Warning log is emitted re | failure mode | World |
| TC-0174 | PhysFlyingZeroesVelocityForHanging | Movement | PhysFlying zeroes Velocity and returns early when state is Hanging. | happy path | Unit |
| TC-0175 | PhysFlyingDoesNotZeroVelocityForShimmying | Movement | PhysFlying calls Super (does not zero velocity) when state is Shimmying. | edge case | Unit |
| TC-0176 | CanAttemptJumpBlockedDuringClimbing | Movement | CanAttemptJump returns false for every non-None climbing state. | happy path | Unit |
| TC-0177 | DoJumpBlockedDuringClimbing | Movement | DoJump returns false for any non-None climbing state. | happy path | Unit |
| TC-0178 | PackedMovementRPCsFalseForAttachedStates | Movement | ShouldUsePackedMovementRPCs returns false for Hanging, Shimmying, BracedWall, Br | happy path | Unit |
| TC-0179 | AnchorInvalidationTriggersClearAnchor | Movement | When AnchorComponent becomes invalid during tick, ClearAnchor is called and Anch | failure mode | World |
| TC-0180 | IMCAddedOnlyOnFirstClimbEntry | StateMachine | AddClimbingInputMappingContext is called only when PreviousClimbingState == None | edge case | World |
| TC-0181 | CornerAngleThresholdExactly30Accepted | Actions | A surface angle difference of exactly 30° (CornerAngleThreshold) produces a val | edge case | Unit |
| TC-0182 | CornerAngleBelow30Rejected | Actions | A surface angle difference of 29.9° (just below CornerAngleThreshold) produces  | edge case | Unit |
| TC-0183 | CornerPivotWarpTargetSetOnTransition | Actions | On entering CornerTransition state, WarpTarget_CornerPivot is registered on Moti | happy path | Component |
| TC-0184 | CornerInsideMontageSelectedLeft | Actions | When bCurrentCornerIsInside=true and CommittedShimmyDir=-1 (left), CornerInsideL | happy path | Unit |
| TC-0185 | CornerOutsideMontageSelectedRight | Actions | When bCurrentCornerIsInside=false and CommittedShimmyDir=+1 (right), CornerOutsi | happy path | Unit |
| TC-0186 | BracedWallSetBaseOnEntry | Actions | SetBase(AnchorComponent) is called when entering BracedWall state. | happy path | Unit |
| TC-0187 | BracedWallSetBaseNullOnExit | Actions | SetBase(nullptr) is called when exiting BracedWall state. | happy path | Unit |
| TC-0188 | BracedShimmySetBaseNullOnExit | Actions | SetBase(nullptr) is called when exiting BracedShimmying state. | edge case | Unit |
| TC-0189 | LipDetectionTriggersHangTransition | Actions | When CheckForLipAbove returns true during BracedWall tick, state transitions to  | happy path | Unit |
| TC-0190 | LipDetectionDuringBracedShimmy | Actions | CheckForLipAbove is also checked during BracedShimmying; finding a lip transitio | edge case | Unit |
| TC-0191 | LipDetectionBlockedAboveNoTransition | Actions | When the upward trace hits geometry (blocked above), CheckForLipAbove returns fa | failure mode | Unit |
| TC-0192 | LipSurfaceAngleTooSteepNoTransition | Actions | When the downward trace finds a surface but its angle exceeds MaxClimbableSurfac | edge case | Unit |
| TC-0193 | BracedWallNoShimmyWithoutInput | Actions | BracedWall state does not transition to BracedShimmying when CurrentClimbMoveInp | happy path | Unit |
| TC-0194 | ShimmyDirectionNotUpdatedAtExactDeadzone | Actions | When |IA_ClimbMove.X| == ShimmyDirectionDeadzone exactly (0.1), CommittedShimmyD | edge case | Unit |
| TC-0195 | ShimmyDirectionUpdatedJustAboveDeadzone | Actions | When |IA_ClimbMove.X| is 0.101 (just above deadzone), CommittedShimmyDir IS upda | edge case | Unit |
| TC-0196 | CornerDetectionNoTransitionBelowAngle | Actions | When PerformCornerDetection returns invalid (angle below threshold), shimmy cont | happy path | Unit |
| TC-0197 | BracedShimmyReleaseReturnsToBracedWall | Actions | Releasing shimmy input (X=0) from BracedShimmying transitions back to BracedWall | happy path | Unit |
| TC-0198 | CommittedShimmyDirReplicatedToProxies | Actions | CommittedShimmyDir is replicated to simulated proxies so they select the correct | happy path | World |
| TC-0199 | bCurrentCornerIsInsideReplicatedToProxies | Actions | bCurrentCornerIsInside is replicated so simulated proxies play the correct insid | happy path | World |
| TC-0200 | ClimbUpRejectedFromBracedWallState | Actions | ClimbUp input is rejected when character is in BracedWall state (only Hanging/Sh | failure mode | Unit |
| TC-0201 | RepositionMontageResetsDistanceCounter | Actions | After reposition resets ContinuousShimmyDistance to 0, the next tick begins accu | edge case | Unit |
| TC-0202 | BracedWallAnchorComponentNullSafe | Actions | Entering BracedWall state when AnchorComponent is null does not crash. | failure mode | Unit |
| TC-0203 | ArcFinalPositionAtTotalArcTime | Arc | The final arc step position at t=LacheTotalArcTime (1.2s) matches the formula ex | happy path | Unit |
| TC-0204 | LadderNonLadderTagRejected | Ladder | A climbable surface without the LadderOnly tag does not trigger ladder state ent | failure mode | World |
| TC-0205 | RagdollRecoveryNotInterruptedEarly | Ragdoll | Player input sent before RagdollRecoveryTime (1.5s) expires does not prematurely | edge case | World |
| TC-0206 | AnchorWorldSpaceWithRotatedComponent | Physics | World-space anchor position is correctly computed when AnchorComponent has a non | edge case | Component |
| TC-0207 | SetBasePreservedAcrossHangingToShimmy | Physics | When transitioning from Hanging to Shimmying, SetBase remains set to the anchor  | edge case | Component |
| TC-0208 | GrabBreakImpulseDirectionPreserved | Ragdoll | The ragdoll launch direction after grab break matches the direction of the incom | happy path | Component |
| TC-0209 | MantleCommittedBlocksLacheInput | Mantle | Lache input sent while a mantle montage is playing is ignored. | edge case | World |
| TC-0210 | LadderClimbMoveYAxisOnly | Ladder | IA_ClimbMove X-axis input does not move the character horizontally while on a la | edge case | World |
| TC-0211 | RagdollFaceDetectionAtZeroDotProduct | Ragdoll | When pelvis up-vector is exactly perpendicular to world up (dot product = 0), th | edge case | Component |
| TC-0212 | LacheArcTraceRadiusUsedForDetection | Arc | The sphere trace at each arc step uses LacheArcTraceRadius (24) as the sphere ra | edge case | World |
| TC-0213 | LadderOnlyTagRequiredForLadderState | Ladder | Adding the LadderOnly tag to an actor at runtime enables ladder state entry on t | happy path | World |
| TC-0214 | AnchorNullMidBracedShimmyDrops | Physics | AnchorComponent becoming null during BracedShimmying state triggers DroppingDown | failure mode | Component |
| TC-0215 | ServerValidationAcceptsAtToleranceBoundary | Multiplayer | Grab at exactly ServerValidationPositionTolerance distance is accepted. | edge case | Unit |
| TC-0216 | ServerValidationRejectsAboveToleranceBoundary | Multiplayer | Grab at tolerance+1 triggers Client_RejectStateTransition. | failure mode | Unit |
| TC-0217 | PredictionRollbackStopsMontage | Multiplayer | Client_RejectStateTransition stops any playing climbing montage. | happy path | Unit |
| TC-0218 | PredictionRollbackPlaysGrabFailMontage | Multiplayer | GrabFail montage plays after Client_RejectStateTransition. | happy path | Unit |
| TC-0219 | PredictionRollbackBlendOutDuration | Multiplayer | PredictionRollbackBlendOut property equals 0.2f. | happy path | Unit |
| TC-0220 | OnRepClimbingStatePlaysEntryMontage | Multiplayer | OnRep_ClimbingState plays entry montage for new state. | happy path | Unit |
| TC-0221 | OnRepClimbingStateRunsConfirmationTrace | Multiplayer | OnRep_ClimbingState calls ResolveHitComponentFromNet synchronously (not deferred | happy path | Unit |
| TC-0222 | BeginPlayLogsWarningPerNullSlot | Lifecycle | One Warning log emitted per null EClimbingAnimationSlot during BeginPlay. | happy path | Unit |
| TC-0223 | BeginPlayNoWarningWhenAllSlotsPopulated | Lifecycle | No Warning logs when all animation slots are assigned. | happy path | Unit |
| TC-0224 | EndPlayClearsAllSixItems | Lifecycle | EndPlay performs all 6 cleanup steps: unregister IK, SetBase(nullptr), Montage_S | happy path | Unit |
| TC-0225 | EndPlaySetBaseNullptr | Lifecycle | EndPlay calls SetBase(nullptr) on the character movement component. | happy path | Unit |
| TC-0226 | EndPlayRestoresPhysics | Lifecycle | EndPlay restores physics simulation state to pre-climb defaults. | happy path | Unit |
| TC-0227 | EndPlayRemovesIMC | Lifecycle | EndPlay removes ClimbingIMC from the Enhanced Input subsystem. | happy path | Unit |
| TC-0228 | EndPlayClearsLacheTarget | Lifecycle | EndPlay sets LacheTarget to null/invalid. | happy path | Unit |
| TC-0229 | DestroyedAlsoUnregistersFromIKManager | Lifecycle | Destroyed callback removes character from ActiveClimbingCharacters independently | happy path | Unit |
| TC-0230 | ClimbingIMCAddedOnClimbEntry | Input | ClimbingIMC is pushed at ClimbingIMCPriority when state transitions from None to | happy path | Unit |
| TC-0231 | ClimbingIMCRemovedOnReturnToNone | Input | ClimbingIMC is removed when climbing state returns to None. | happy path | Unit |
| TC-0232 | ClimbingIMCPriorityValue | Input | ClimbingIMC is added at the configured ClimbingIMCPriority value, not a hardcode | happy path | Unit |
| TC-0233 | IMCNotAddedForNonLocalController | Input | IMC push is skipped when the controller is not locally controlled (simulated pro | edge case | Unit |
| TC-0234 | IdleVariationDelayTimerFires | Anim | Idle variation montage does not play before the configured delay timer elapses. | happy path | Unit |
| TC-0235 | IdleVariationNoRepeatSameVariation | Anim | Idle variation selection never picks the same variation twice in a row when bPre | edge case | Unit |
| TC-0236 | IdleVariationResetsTimerAfterPlay | Anim | After an idle variation plays, the delay timer resets for the next cycle. | happy path | Unit |
| TC-0237 | BlendTimeZeroIsImmediate | Anim | A blend time of exactly 0.0f results in an immediate weight change with no inter | edge case | Unit |
| TC-0238 | DebugDrawShippingGuardPreventsExecution | Debug | Debug draw code inside #if !UE_BUILD_SHIPPING is not reachable in shipping build | happy path | Unit |
| TC-0239 | DebugDrawOnlyWhenFlagSet | Debug | Debug visualization only executes when bDrawDebug is true. | happy path | Unit |
| TC-0240 | EditorLacheArcGatedByIsSelected | Debug | Editor Lache arc preview only draws when the actor IsSelected() returns true. | edge case | Unit |
| TC-0241 | SimulatedProxyIKWeightsZeroedBeyondCullDistance | Multiplayer | Simulated proxy beyond SimulatedProxyIKCullDistance has all IK weights zeroed. | happy path | Unit |
| TC-0242 | OnRepClimbingStateNoOpForNoneState | Multiplayer | OnRep_ClimbingState does not play a montage or run a confirmation trace when new | edge case | Unit |
| TC-0243 | ServerValidationZeroToleranceRejectsAnyOffset | Multiplayer | With ServerValidationPositionTolerance=0, any non-zero client offset is rejected | edge case | Unit |
| TC-0244 | RollbackRestoresClimbingStateToNone | Multiplayer | After Client_RejectStateTransition, local ClimbingState reverts to None. | happy path | Unit |
| TC-0245 | IKWeightHandLeftIsBlueprintReadWrite | IK | IKWeightHandLeft on UClimbingAnimInstance has BlueprintReadWrite specifier. | happy path | Unit |
| TC-0246 | IKWeightHandRightIsBlueprintReadWrite | IK | IKWeightHandRight on UClimbingAnimInstance has BlueprintReadWrite specifier. | happy path | Unit |
| TC-0247 | IKWeightFootLeftIsBlueprintReadWrite | IK | IKWeightFootLeft on UClimbingAnimInstance has BlueprintReadWrite specifier. | happy path | Unit |
| TC-0248 | IKWeightFootRightIsBlueprintReadWrite | IK | IKWeightFootRight on UClimbingAnimInstance has BlueprintReadWrite specifier. | happy path | Unit |
| TC-0249 | IKManagerRegistersOnBeginPlay | IK | Character is added to ActiveClimbingCharacters during BeginPlay. | happy path | Unit |
| TC-0250 | IKManagerEmptyAfterAllUnregister | IK | ActiveClimbingCharacters is empty after all registered characters call EndPlay. | happy path | Unit |
| TC-0251 | IKManagerStaleWeakPtrsPurgedOnUpdate | IK | Expired weak pointers are removed from ActiveClimbingCharacters during the next  | edge case | Unit |
| TC-0252 | AudioSoundMapKeyCoversAllSoundTypes | Audio | ClimbingSounds TMap can hold an entry for every EClimbSoundType enum value witho | happy path | Unit |
| TC-0253 | AudioNullSoftPtrDoesNotDispatchLoad | Audio | A null TSoftObjectPtr in ClimbingSounds does not trigger an async load attempt. | failure mode | Unit |
| TC-0254 | CameraProbeRadiusDefaultValue | Camera | ClimbingCameraProbeRadius has a positive non-zero default value. | happy path | Unit |
| TC-0255 | CameraNudgeBlendSpeedPositive | Camera | CameraNudgeBlendSpeed default value is greater than zero. | happy path | Unit |
| TC-0256 | EndPlayMontageStopCalled | Lifecycle | EndPlay calls Montage_Stop on the AnimInstance to halt any playing montage. | happy path | Unit |
| TC-0257 | EndPlayIdempotentOnDoubleCall | Lifecycle | Calling EndPlay twice does not crash or corrupt state. | edge case | Unit |
| TC-0258 | PawnClientRestartDoesNotAddIMCTwice | Input | PawnClientRestart does not result in duplicate IMC entries if called while climb | edge case | Unit |
| TC-0259 | ClimbingIMCNotPresentAfterLocomotionRestore | Input | After locomotion IMC is restored, ClimbingIMC is no longer in the active mapping | happy path | Unit |
| TC-0260 | AllAnimationSlotsEnumeratedInBeginPlay | Anim | BeginPlay iterates every value of EClimbingAnimationSlot exactly once. | happy path | Unit |
| TC-0261 | NullMontageSlotDoesNotCrashOnPlay | Anim | Attempting to play a montage for a null slot logs a warning and returns without  | failure mode | Unit |
| TC-0262 | IKBlendWeightClampedBetweenZeroAndOne | Anim | IK blend weight is always clamped to [0.0, 1.0] regardless of input delta. | edge case | Unit |
| TC-0263 | DebugDrawFalseProducesNoDrawCalls | Debug | When bDrawDebug is false, no debug draw primitives are submitted. | happy path | Unit |
| TC-0264 | ServerRPCFunctionsAreReliable | Multiplayer | Server_ prefixed RPC functions have the Reliable specifier in their UFUNCTION me | happy path | Unit |
| TC-0265 | ClientRPCFunctionsAreReliable | Multiplayer | Client_ prefixed RPC functions have the Reliable specifier. | happy path | Unit |
| TC-0266 | IdleVariationNotPlayedWhileActivelyClimbing | Anim | Idle variation timer does not fire while the character is actively moving on a s | edge case | Unit |
| TC-0267 | MontageSlotDefaultIsNullForNewAnimSet | Anim | A freshly constructed animation set returns null for every slot query. | happy path | Unit |
| TC-0268 | BeginPlayWarningCountMatchesNullSlotCount | Lifecycle | Number of Warning logs equals the exact count of null slots, not more or fewer. | happy path | Unit |
| TC-0269 | EndPlayWithNoIMCDoesNotCrash | Lifecycle | EndPlay does not crash when ClimbingIMC was never added (e.g., non-local control | failure mode | Unit |
| TC-0270 | DestroyedCalledWithoutBeginPlayDoesNotCrash | Lifecycle | Destroyed callback on a character that never completed BeginPlay does not crash  | failure mode | Unit |
| TC-0271 | SimulatedProxyOnRepStateUpdatesTick | Multiplayer | OnRep_ClimbingState on a simulated proxy correctly updates animation state witho | happy path | Unit |
| TC-0272 | LacheArcPreviewOnlyInEditor | Debug | Lache arc preview code is compiled out or gated in non-editor builds. | happy path | Unit |
| TC-0273 | EditorLacheArcNotDrawnWhenNotSelected | Debug | Lache arc draw call is skipped when IsSelected() returns false. | edge case | Unit |
| TC-0274 | AnimNotifyMontageSlotMatchesState | Anim | AnimNotify fires only when the montage is playing in the correct slot for the cu | happy path | Unit |
| TC-0275 | MantleStepUpAtExactThreshold | Mantle | Height exactly at MantleStepMaxHeight(50cm) triggers CMC step-up, not Mantling s | edge case | Unit |
| TC-0276 | MantleLowAt51cm | Mantle | Height 51cm (1 above step threshold) selects MantleLow slot. | edge case | Unit |
| TC-0277 | MantleHighAt101cm | Mantle | Height 101cm (1 above LowMax) selects MantleHigh slot. | edge case | Unit |
| TC-0278 | MantleRejectedAt181cm | Mantle | Height 181cm (1 above HighMax=180) is not mantleable; height 180cm is mantleable | edge case | Component |
| TC-0279 | MantleLowWarpTargetRegistered | Mantle | MantleLow entry registers WarpTarget_MantleLow at ledge position. | happy path | Component |
| TC-0280 | MantleHighWarpTargetRegistered | Mantle | MantleHigh entry registers WarpTarget_MantleHigh at ledge position. | happy path | Component |
| TC-0281 | MantleBelowStepNoStateEntered | Mantle | Height below MantleStepMaxHeight never enters Mantling state or plays montage. | happy path | Component |
| TC-0282 | LadderEnterBottomMontageAndWarp | Ladder | OnLadder entry from bottom plays LadderEnterBottom montage and registers WarpTar | happy path | Component |
| TC-0283 | LadderSprintAndCrouchSimultaneous | Ladder | Sprint+Crouch held simultaneously applies both multipliers to ladder speed. | edge case | Unit |
| TC-0284 | LadderRungSpacingDefault | Ladder | DefaultLadderRungSpacing defaults to 30cm and is EditAnywhere. | happy path | Unit |
| TC-0285 | LadderRungSpacingAffectsIKInterval | Ladder | Changing DefaultLadderRungSpacing changes the vertical interval of IK traces. | edge case | Component |
| TC-0286 | GrabBreakBelowThresholdNoRagdoll | Ragdoll | 1999N impulse (below 2000 threshold) does not trigger ragdoll. | edge case | Component |
| TC-0287 | GrabBreakAtExactThresholdTriggersRagdoll | Ragdoll | Exactly 2000N impulse triggers ragdoll (>= boundary). | edge case | Component |
| TC-0288 | NotifyHitDuringNoneStateNoRagdoll | Ragdoll | 5000N impulse in None state does not trigger ragdoll (non-climbing guard). | failure mode | Component |
| TC-0289 | PelvisBoneNameOverrideUsedInRecovery | Ragdoll | Custom PelvisBoneName override is used for face detection during recovery. | edge case | Component |
| TC-0290 | RagdollFaceUpMontageSelected | Ragdoll | Pelvis up-vector dot > 0 selects RagdollGetUpFaceUp montage. | happy path | Component |
| TC-0291 | RagdollFaceDownMontageSelected | Ragdoll | Pelvis up-vector dot <= 0 selects RagdollGetUpFaceDown montage. | happy path | Component |
| TC-0292 | GrabBreakLaunchVelocityScaled | Ragdoll | Launch velocity = NormalImpulse * GrabBreakLaunchScale (3000*0.5=1500). | happy path | Component |
| TC-0293 | AnchorMovesCharacterFollowsNextTick | Physics | When anchor moves, character world position updates next tick. | happy path | World |
| TC-0294 | AnchorMovingPlatformTracking | Physics | Character tracks moving platform across 5 ticks within 1cm tolerance. | happy path | World |
| TC-0295 | HangingEntryCallsSetBase | Physics | Hanging entry calls SetBase on HitComponent. | happy path | Component |
| TC-0296 | ExitClimbingClearsMovementBase | Physics | Exiting any climbing state to None clears movement base to nullptr. | happy path | Component |
| TC-0297 | NullAnchorDuringHangingDrops | Physics | AnchorComponent becoming null during Hanging triggers DroppingDown. | failure mode | Component |
| TC-0298 | ServerRejectsStreamingSublevelAnchor | Multiplayer | Server_AttemptGrab rejects when HitComponent owner is in streaming sublevel. | failure mode | World |
| TC-0299 | ServerAcceptsPersistentLevelAnchor | Multiplayer | Server_AttemptGrab accepts when component is in persistent level. | happy path | World |
| TC-0300 | LadderExitTopMontageAndWarp | Ladder | LadderTransition with valid ledge plays LadderExitTop and registers WarpTarget_L | happy path | Component |
| TC-0301 | LadderExitBottomNoWarpTarget | Ladder | LadderTransition with invalid ledge plays LadderExitBottom; no warp target. | edge case | Component |
| TC-0302 | LandedWithinCoyoteWindowTriggersRescan | CoyoteTime | Landed within CoyoteTimeWindow triggers re-grab detection scan. | happy path | World |
| TC-0303 | LandedAfterCoyoteWindowNoRescan | CoyoteTime | Landed after CoyoteTimeWindow expires does not trigger re-grab scan. | edge case | World |
| TC-0304 | GrabBreakThresholdZeroDisablesBreak | Ragdoll | GrabBreakImpulseThreshold=0 disables grab-break entirely. | edge case | Component |
| TC-0305 | IKMaxReachExceededFadesWeight | IK | When hand target exceeds MaxReachDistance(80cm), IK weight fades to 0 over IKFad | edge case | Unit |
| TC-0306 | IKMaxReachWithinKeepsWeight | IK | When hand target is within MaxReachDistance(80cm), IK weight stays at 1.0. | happy path | Unit |
| TC-0307 | IKBudgetFifthCharacterZeroed | IK | With MaxSimultaneousIKCharacters=4, the 5th climbing character has all IK weight | edge case | World |
| TC-0308 | IKManagerSortedByCameraDistance | IK | ActiveClimbingCharacters sorted by camera distance on state change, not per-tick | happy path | World |
| TC-0309 | IKProxyUpdateIntervalThrottled | IK | Proxy IK updates throttled to SimulatedProxyIKUpdateInterval(0.05s). | happy path | Component |
| TC-0310 | HandIKOffsetApplied | IK | HandIKOffset(5,0,-5) is applied to computed hand IK target position. | happy path | Unit |
| TC-0311 | HandIKSpacingBetweenLeftRight | IK | Left and right hand IK targets are separated by HandIKSpacing(40cm). | happy path | Unit |
| TC-0312 | CameraNudgeAtExactActivationAngle | Camera | Camera nudge activates at exactly CameraNudgeActivationAngle(45°). | edge case | Component |
| TC-0313 | CameraNudgeBelowAngleNoNudge | Camera | Camera nudge does not activate below CameraNudgeActivationAngle. | happy path | Component |
| TC-0314 | LockCameraToFrameZeroBlendTime | Camera | LockCameraToFrame with BlendTime=0 applies immediately. | edge case | Component |
| TC-0315 | ReleaseCameraLockDuringNonLockedNoOp | Camera | ReleaseCameraLock during non-locked state is a no-op (no crash). | failure mode | Component |
| TC-0316 | RagdollCameraSocketSwitch | Camera | On ragdoll entry, spring arm switches to RagdollCameraTargetSocket(pelvis). | happy path | Component |
| TC-0317 | RagdollCameraSocketRestoredOnRecovery | Camera | Spring arm socket restored to pre-ragdoll value on recovery. | happy path | Component |
| TC-0318 | AudioResolvedCacheHitSecondCall | Audio | Second dispatch for same EClimbSoundType uses cache, no new async load. | happy path | Unit |
| TC-0319 | AudioAsyncLoadSuccessCachesPtr | Audio | Successful async load stores resolved ptr in cache under correct key. | happy path | Unit |
| TC-0320 | AudioAsyncLoadFailureCachesNullNoRetry | Audio | Failed load caches null, logs once, never retries. | failure mode | Unit |
| TC-0321 | AudioGrabFailSoundDispatched | Audio | GrabFail sound dispatched when grab attempt finds no valid surface. | failure mode | Unit |
| TC-0322 | AudioLacheSoundsDispatched | Audio | LacheLaunchGrunt dispatched on launch, LacheCatchImpact on catch. | happy path | Unit |
| TC-0323 | SurfaceDataAnimSetOverrideAsyncLoad | SurfaceData | Async load triggered exactly once on first surface contact. | happy path | Unit |
| TC-0324 | SurfaceDataPerSlotFallback | SurfaceData | Null slot in AnimationSetOverride falls back to character default. | edge case | Unit |
| TC-0325 | SurfaceDataClimbSpeedMultiplierAffectsShimmy | SurfaceData | Shimmy speed is scaled by ClimbSpeedMultiplier from surface data. | happy path | Unit |
| TC-0326 | SurfaceDataClimbSpeedMultiplierZero | SurfaceData | ClimbSpeedMultiplier=0 results in zero shimmy speed without crash. | edge case | Unit |
| TC-0327 | SurfaceDataAudioOverrideUsed | SurfaceData | Surface-specific audio override dispatched instead of character default. | happy path | Unit |
| TC-0328 | SurfaceDataAudioOverrideNullFallback | SurfaceData | Null audio override falls back to character default sound. | edge case | Unit |
| TC-0329 | IKCullDistanceBoundaryTest | IK | Proxy at exactly SimulatedProxyIKCullDistance(1500cm) has IK zeroed; at 1499cm h | edge case | Component |
| TC-0330 | IKCornerFABRIKAllFourLimbs | IK | During CornerTransition, FABRIK is applied to all four limbs. | happy path | Component |
| TC-0331 | SpringArmRotatesOnClimbEntry | Camera | Spring arm rotation aligns to surface normal on climb state entry. | happy path | Component |
| TC-0332 | IKFadeOutBlendTimeDefault | IK | IKFadeOutBlendTime defaults to 0.15s. | happy path | Unit |
| TC-0333 | MaxReachDistanceDefault | IK | MaxReachDistance defaults to 80cm. | happy path | Unit |
| TC-0334 | HandIKSpacingDefault | IK | HandIKSpacing defaults to 40cm. | happy path | Unit |
| TC-0335 | ServerAttemptGrabRejectsStreamingSublevel | Multiplayer | Server_AttemptGrab rejects when hit component is in streaming sublevel. | failure mode | World |
| TC-0336 | ServerAttemptGrabAcceptsLoadedLevel | Multiplayer | Server_AttemptGrab accepts when sublevel is fully loaded and position within tol | happy path | World |
| TC-0337 | ServerDropFromHangingTransitionsToNone | Multiplayer | Server_Drop from Hanging transitions to None; MOVE_Falling restored. | happy path | World |
| TC-0338 | ServerDropFromShimmyTransitionsToNone | Multiplayer | Server_Drop from Shimmying transitions to None. | happy path | World |
| TC-0339 | ServerAttemptLacheRejectsOutOfRange | Multiplayer | Arc target beyond max reach triggers Client_RejectStateTransition. | failure mode | World |
| TC-0340 | ServerAttemptLacheAcceptsValidTarget | Multiplayer | Arc target within reach triggers lache launch state. | happy path | World |
| TC-0341 | ServerAttemptClimbUpRejectsNoClearance | Multiplayer | Clearance blocked on server triggers Client_RejectStateTransition. | failure mode | World |
| TC-0342 | ServerAttemptClimbUpAcceptsClear | Multiplayer | Clearance passes triggers ClimbingUp state. | happy path | World |
| TC-0343 | ServerUpdateShimmyDirectionReplicates | Multiplayer | FVector2D(1,0) stored in replicated shimmy direction property. | happy path | World |
| TC-0344 | ServerUpdateShimmyZeroVectorAccepted | Multiplayer | FVector2D::ZeroVector accepted cleanly (stop shimmy). | edge case | World |
| TC-0345 | ClientConfirmMatchesPrediction | Multiplayer | Confirmation matching optimistic prediction keeps state; no rollback. | happy path | World |
| TC-0346 | OptimisticPredictionStateSetBeforeAck | Multiplayer | Client enters climbing state immediately on input before server ack. | happy path | World |
| TC-0347 | SimulatedProxyMontageOnRepNotify | Multiplayer | Rep-notify for BracedWall plays braced entry montage, not shimmy/climb-up. | happy path | Component |
| TC-0348 | FreefallGrabSucceedsWithinReach | Freefall | Ledge at 79cm + bEnableFallingGrab=true + IA_Grab triggers grab. | happy path | World |
| TC-0349 | FreefallGrabFailsBeyondReach | Freefall | Ledge at 81cm fails grab; stays MOVE_Falling. | edge case | World |
| TC-0350 | FreefallGrabDisabledByToggle | Freefall | bEnableFallingGrab=false prevents grab even with ledge at 40cm. | failure mode | World |
| TC-0351 | FreefallCheckIntervalRate | Freefall | Timer fires at 0.05s interval; no fire at 0.04s, one fire at 0.06s. | happy path | Component |
| TC-0352 | CoyoteGrabSucceedsWithinWindow | CoyoteTime | IA_Grab at t=0.10s post-leave triggers detection re-run and grab. | happy path | World |
| TC-0353 | CoyoteGrabFailsAfterWindow | CoyoteTime | IA_Grab at t=0.16s (past window) does not trigger coyote path. | edge case | World |
| TC-0354 | CoyoteGrabDisabledByToggle | CoyoteTime | bEnableCoyoteTime=false prevents coyote grab at t=0.05s. | failure mode | World |
| TC-0355 | CoyoteLandedClearsWindow | CoyoteTime | Landed at t=0.08s invalidates coyote window; subsequent IA_Grab fails coyote pat | edge case | World |
| TC-0356 | ClimbableOneWayRejectsWrongApproach | Detection | Approach parallel to normal (back face) returns false. | failure mode | Unit |
| TC-0357 | ClimbableOneWayAcceptsCorrectApproach | Detection | Approach opposing normal (front face, dot < 0) returns true. | happy path | Unit |
| TC-0358 | GetSurfaceDataParsesValidTag | Detection | Tag "SurfaceData:/Game/Path/Asset.Asset" returns valid UClimbingSurfaceData. | happy path | Unit |
| TC-0359 | GetSurfaceDataMalformedTagReturnsNull | Detection | Tag "SurfaceData:" with no path returns nullptr without crash. | failure mode | Unit |
| TC-0360 | GetSurfaceDataMissingAssetReturnsNull | Detection | Well-formed path to non-existent asset returns nullptr without crash. | failure mode | Unit |
| TC-0361 | IsSurfaceClimbableClimbableTiersTrue | Detection | Climbable, ClimbableOneWay, and Untagged all return true. | happy path | Unit |
| TC-0362 | IsSurfaceClimbableUnclimbableTiersFalse | Detection | Unclimbable and LadderOnly both return false. | happy path | Unit |
| TC-0363 | SimulatedProxyIKUpdateIntervalThrottles | Multiplayer | IK updates throttled to configured interval; no update at half-interval. | happy path | Component |
| TC-0364 | DetectionScanIntervalOnGround | Detection | Detection scan fires at DetectionScanInterval(0.05s) during ground locomotion. | happy path | Component |
| TC-0365 | IntegrationLacheLaunchFlightCatchHangChain | Integration | Full Lache launch->flight->catch->hang chain with callback ordering verified. | happy path | Integration |
| TC-0366 | IntegrationLacheMissResultsInFall | Integration | Lache flight with no catch anchor exits to None/falling; capsule restored. | failure mode | Integration |
| TC-0367 | IntegrationGrabShimmyCornerShimmyDropChain | Integration | Full None->Hanging->Shimmy->Corner->Hanging->Shimmy->None chain. | happy path | Integration |
| TC-0368 | IntegrationClimbUpCapsuleRestore | Integration | After OnClimbUpMontageBlendingOut, capsule dims match pre-climb values. | happy path | Integration |
| TC-0369 | StressRapidGrabSpam | Stress | 10 IA_Grab presses in 0.5s; character enters Hanging exactly once; no corruption | stress | Integration |
| TC-0370 | StressRapidNoneHangingCycle | Stress | 100x None->Hanging->None cycles; no timer leak, capsule mismatch, or IK residue. | stress | Integration |
| TC-0371 | StressFiveClimbersIKBudget | Stress | 5 simultaneous climbers; exactly 4 have IK active; 5th excluded; no crash. | stress | Integration |
| TC-0372 | PerformanceDetectionScanUnder1ms | Performance | Single detection scan median < 1ms over 100 samples. | stress | Performance |
| TC-0373 | IntegrationTransitionDuringMontageBlendOut | Integration | Drop input during ClimbUp blend-out; callback fires once; final state determinis | edge case | Integration |
| TC-0374 | NetworkDoubleAttemptGrabBeforeResponse | Network | Two Server_AttemptGrab RPCs before first processed; server commits one; second r | edge case | Integration |
| TC-0375 | IntegrationCornerAnchorDestroyedMidTransition | Integration | Anchor destroyed during CornerTransition; character exits to None safely. | failure mode | Integration |
| TC-0376 | IntegrationTimerPauseUnpause | Integration | Detection and FallingGrab timers do not fire during game pause; resume on unpaus | edge case | Integration |
| TC-0377 | IntegrationCapsuleSizeAtEachTransition | Integration | Capsule dims recorded at every state boundary; resized on entry, restored on exi | happy path | Integration |
| TC-0378 | IntegrationIMCStackFullLifecycle | Integration | Climbing IMC pushed on grab, popped on every exit path. | happy path | Integration |
| TC-0379 | NetworkClientPredictServerConfirmProxyUpdate | Network | Client predicts Hanging; server confirms; proxy receives replicated state. | happy path | Integration |
| TC-0380 | PerformanceStateTransitionThroughput | Performance | 1000 state transitions complete in < 10ms total. | stress | Performance |
| TC-0381 | IntegrationLadderTransitionMontageGatesState | Integration | Movement input during LadderTransition does not prematurely exit committed state | edge case | Integration |
| TC-0382 | IntegrationRagdollIMCRestored | Integration | After ragdoll recovery from Hanging, IMC stack has only base locomotion IMC. | edge case | Integration |
| TC-0383 | IntegrationMantleAnchorDestroyedDuringCommitted | Integration | Anchor destroyed during committed mantle montage; safe exit to None. | failure mode | Integration |
| TC-0384 | NetworkServerDetectionReRunOnEveryRPC | Network | Server re-runs full detection scan on each Server_AttemptGrab call. | happy path | Integration |
| TC-0385 | IntegrationAllStatesEntryExitSymmetry | Integration | Entry+exit called exactly once per transition for all 17 EClimbingState values. | happy path | Integration |
| TC-0386 | StressNetworkRapidServerGrabSpam | Stress | 20 Server_AttemptGrab RPCs in 1s; server remains stable. | stress | Integration |
| TC-0387 | IntegrationIKWeightZeroOnStateExit | Integration | IK weight reaches 0 on every exit path (drop, ClimbUp, ragdoll, anchor destroy). | happy path | Integration |
| TC-0388 | IntegrationLacheCapsuleRestoredOnMiss | Integration | Capsule dimensions restored after Lache miss/fall path. | edge case | Integration |
| TC-0389 | PerformanceIKSixClimbersGracefulDegradation | Performance | 6 climbers; IK active on exactly 4; tick budget still met. | stress | Performance |
| TC-0390 | IntegrationIMCNotDoubledOnRapidGrabDrop | Integration | Rapid grab+drop 10 times; IMC stack depth never exceeds 1 climbing context. | stress | Integration |
| TC-0391 | NetworkProxyMontageNotPlayedTwice | Network | Proxy receives replicated state; montage plays exactly once; no double-play. | edge case | Integration |
| TC-0392 | NetworkTwoClimbersIndependentState | Network | Two clients climbing simultaneously; independent states; no cross-contamination. | happy path | Integration |
| TC-0393 | PerformanceIKTickBudgetFourClimbers | Performance | Combined IK tick for 4 simultaneous climbers < 2ms median. | stress | Performance |
| TC-0394 | PerformanceFallingGrabCheckIntervalDrift | Performance | FallingGrabCheck fires at correct 0.05s interval over 100 cycles; no drift. | stress | Performance |
| TC-0395 | ClimbUpBlendOutTransitionsToNone | MontageCallback | bInterrupted=false transitions to EClimbingState::None. | happy path | Component |
| TC-0396 | ClimbUpBlendOutInterruptedNoTransition | MontageCallback | bInterrupted=true does not transition state. | edge case | Component |
| TC-0397 | CornerBlendOutTransitionsToShimmying | MontageCallback | Corner montage done transitions to Shimmying (resume shimmy). | happy path | Component |
| TC-0398 | LadderTransitionBlendOutToOnLadder | MontageCallback | Valid anchor after ladder transition montage transitions to OnLadder. | happy path | Component |
| TC-0399 | LadderTransitionBlendOutToNoneWhenInvalid | MontageCallback | Invalidated anchor after ladder transition transitions to None. | failure mode | Component |
| TC-0400 | LacheLaunchBlendOutToLacheInAir | MontageCallback | Launch montage done transitions to LacheInAir. | happy path | Component |
| TC-0401 | LacheCatchBlendOutToHanging | MontageCallback | Catch montage done transitions to Hanging. | happy path | Component |
| TC-0402 | InputGrabRejectedDuringCommittedState | Input | IA_Grab rejected during ClimbingUp (committed state). | failure mode | Unit |
| TC-0403 | InputGrabCallsServerAttemptGrab | Input | None state + climbable surface triggers Server_AttemptGrab. | happy path | Unit |
| TC-0404 | InputDropFromHangingCallsServerDrop | Input | IA_Drop from Hanging calls Server_Drop. | happy path | Unit |
| TC-0405 | InputDropFromCommittedStateRejected | Input | IA_Drop from ClimbingUp (committed) is rejected. | failure mode | Unit |
| TC-0406 | InputLacheOnlyFromHangingOrShimmying | Input | IA_Lache from OnLadder is rejected. | failure mode | Unit |
| TC-0407 | InputJumpDuringHangingTriggersLache | Input | Jump during Hanging triggers Lache, not ACharacter::Jump. | happy path | Unit |
| TC-0408 | InputSprintSetsBSprintHeld | Input | Input_Sprint sets bSprintHeld=true. | happy path | Unit |
| TC-0409 | InputSprintCompletedClearsBSprintHeld | Input | Input_SprintCompleted clears bSprintHeld=false. | happy path | Unit |
| TC-0410 | InputCrouchSetsBCrouchHeld | Input | Input_Crouch sets bCrouchHeld=true. | happy path | Unit |
| TC-0411 | InputCrouchCompletedClearsBCrouchHeld | Input | Input_CrouchCompleted clears bCrouchHeld=false. | happy path | Unit |
| TC-0412 | InputClimbMoveCompletedZerosInput | Input | Input_ClimbMoveCompleted zeros CurrentClimbMoveInput. | happy path | Unit |
| TC-0413 | CapsuleResizedOnClimbEntry | Capsule | Capsule HalfHeight=48, Radius=24 on Hanging entry. | happy path | World |
| TC-0414 | CapsuleRestoredOnExitToNone | Capsule | Original capsule dimensions restored on None transition. | happy path | World |
| TC-0415 | ClimbingCollisionProfileSetOnEntry | Capsule | ClimbingCollisionProfile="ClimbingCapsule" set on climb entry; restored on exit. | happy path | World |
| TC-0416 | HangingCharacterOffsetApplied | StateMachine | HangingCharacterOffset(0,0,-30) applied on Hanging entry; removed on exit. | happy path | World |
| TC-0417 | GetMontageForSlotReturnsOverride | Animation | AnimationSetOverride takes precedence over character defaults. | happy path | Unit |
| TC-0418 | GetMontageForSlotFallsBackToDefault | Animation | Null override slot returns character default montage. | edge case | Unit |
| TC-0419 | WarpTargetLedgeGrabRegistered | WarpTarget | WarpTarget_LedgeGrab registered before GrabLedge montage plays. | happy path | World |
| TC-0420 | WarpTargetClimbUpLandRegistered | WarpTarget | WarpTarget_ClimbUpLand registered before ClimbUp montage plays. | happy path | World |
| TC-0421 | WarpTargetLacheCatchRegistered | WarpTarget | WarpTarget_LacheCatch registered before LacheCatch montage plays. | happy path | World |
| TC-0422 | InputDropFromOnLadderCallsServerDrop | Input | IA_Drop from OnLadder calls Server_Drop. | happy path | Unit |
| TC-0423 | InputJumpDuringLocomotionTriggersNormalJump | Input | Jump during None state triggers ACharacter::Jump, not Lache. | happy path | Unit |
| TC-0424 | InputDropFromShimmyingCallsServerDrop | Input | IA_Drop from Shimmying calls Server_Drop. | happy path | Unit |
| TC-0425 | ClassifyHangTypeBracedWhenWallPresent | Detection | ClassifyHangType sets result to braced hang when wall backing trace finds wall. | happy path | Component |
| TC-0426 | ClassifyHangTypeFreeWhenNoWall | Detection | ClassifyHangType sets result to free hang when no wall found behind character. | happy path | Component |
| TC-0427 | ClassifyHangTypeNoSideEffects | Detection | ClassifyHangType only modifies hang type field; does not alter LedgePosition, Su | edge case | Unit |
| TC-0428 | TickLadderStateUpwardMovement | Ladder | Positive Y input moves character upward at ladder climb speed. | happy path | Component |
| TC-0429 | TickLadderStateRungSnap | Ladder | Character position snaps to nearest rung grid interval when stopping. | edge case | Component |
| TC-0430 | TickLadderStateTopExit | Ladder | Reaching ladder top triggers LadderTransition state. | happy path | Component |
| TC-0431 | TickLadderStateBottomExit | Ladder | Reaching ladder bottom triggers LadderTransition state. | happy path | Component |
| TC-0432 | TickLacheInAirFollowsArc | Lache | Character position follows arc formula each tick during LacheInAir. | happy path | Component |
| TC-0433 | TickLacheInAirCatchTransition | Lache | When arc reaches catch target, state transitions to LacheCatch. | happy path | Component |
| TC-0434 | TickLacheInAirMissTransition | Lache | When arc completes without catch, state transitions to LacheMiss. | failure mode | Component |
| TC-0435 | InputMoveUpdatesClimbMoveInputDuringClimbing | Input | During climbing states, Input_Move updates CurrentClimbMoveInput from movement v | happy path | Unit |
| TC-0436 | InputMoveNormalLocomotionDuringNone | Input | During None state, Input_Move drives normal locomotion, not climbing input. | happy path | Unit |
| TC-0437 | InputLookSuppressedDuringCinematicLock | Input | During cinematic lock (bCameraLocked), look input is suppressed. | edge case | Unit |
| TC-0438 | InputLookAppliedWhenUnlocked | Input | When camera is not locked, look input is applied normally. | happy path | Unit |
| TC-0439 | AutoLacheCinematicWithinThreshold | Lache | When bAutoLacheCinematic=true and target within 300cm, LockCameraToFrame is call | happy path | Component |
| TC-0440 | AutoLacheCinematicBeyondThreshold | Lache | When target beyond 300cm, LockCameraToFrame is NOT called. | edge case | Component |
| TC-0441 | MinLedgeDepthRejectsShallowLedge | Detection | Ledge thinner than MinLedgeDepth(15cm) is rejected. | failure mode | Component |
| TC-0442 | MinLedgeDepthAcceptsAtThreshold | Detection | Ledge at exactly MinLedgeDepth(15cm) is accepted. | edge case | Component |
| TC-0443 | ShimmyRepositionTriggeredAtLimit | Shimmy | ShimmyReposition montage triggered when ContinuousShimmyDistance exceeds MaxCont | edge case | Component |
| TC-0444 | ShimmyRepositionMontageRateZero | Shimmy | During ShimmyReposition, montage rate is set to 0 (character stationary). | happy path | Component |
| TC-0445 | OverhangPenaltyNoPenaltyBelowStart | Shimmy | OverhangAngleDeg <= OverhangPenaltyStartAngle results in penalty = 1.0 (no penal | happy path | Unit |
| TC-0446 | OverhangPenaltyLerpAcrossRange | Shimmy | Penalty lerps from 1.0 to OverhangMaxPenaltyScalar across the range. | happy path | Unit |
| TC-0447 | OverhangPenaltyClampedAtMax | Shimmy | Angle beyond start+range clamps penalty to OverhangMaxPenaltyScalar. | edge case | Unit |
| TC-0448 | NativeInitializeAnimationCachesOwner | Anim | NativeInitializeAnimation caches the owning character reference. | happy path | Unit |
| TC-0449 | NativeUpdateAnimationCopiesState | Anim | NativeUpdateAnimation copies current climbing state to anim blueprint properties | happy path | Unit |
| TC-0450 | NativeUpdateAnimationNullOwnerNoCrash | Anim | NativeUpdateAnimation does not crash when cached owner is null. | failure mode | Unit |
| TC-0451 | OnClimbingStateReplicatedRoutesToOnStateEnter | Multiplayer | OnClimbingStateReplicated calls OnStateEnter for the new state on proxy. | happy path | Component |
| TC-0452 | OnClimbingStateReplicatedRoutesToOnStateExit | Multiplayer | OnClimbingStateReplicated calls OnStateExit for the old state on proxy. | happy path | Component |
| TC-0453 | UpdateBracedWallIKValidTargets | IK | UpdateBracedWallIK produces valid hand/foot IK targets when wall is present. | happy path | Component |
| TC-0454 | UpdateBracedWallIKNoTargetsWhenNoWall | IK | UpdateBracedWallIK produces no IK targets when no wall contact found. | failure mode | Component |
| TC-0455 | WarpTargetLadderEnterTopPosition | WarpTarget | WarpTarget_LadderEnterTop registered at correct position on ladder top entry. | happy path | World |
| TC-0456 | WarpTargetLadderEnterTopRotation | WarpTarget | WarpTarget_LadderEnterTop rotation aligns character to face ladder. | happy path | World |
| TC-0457 | WarpTargetLadderExitBottomPosition | WarpTarget | WarpTarget_LadderExitBottom registered at correct position. | happy path | World |
| TC-0458 | WarpTargetLadderExitTopPosition | WarpTarget | WarpTarget_LadderExitTop registered at correct position. | happy path | World |
| TC-0459 | WarpTargetMantleLowPositionMatchesLedge | WarpTarget | WarpTarget_MantleLow position matches detection result LedgePosition. | happy path | World |
| TC-0460 | WarpTargetMantleHighPositionMatchesLedge | WarpTarget | WarpTarget_MantleHigh position matches detection result LedgePosition. | happy path | World |
| TC-0461 | WarpTargetLedgeGrabRotationMatchesNormal | WarpTarget | WarpTarget_LedgeGrab rotation aligns character to face wall (opposite of surface | happy path | World |
| TC-0462 | DebugDetectionTracesDrawnWhenEnabled | Debug | Detection traces (green/red/yellow) drawn when bDrawDebug=true. | happy path | World |
| TC-0463 | DebugIKTargetSpheresDrawn | Debug | IK target white spheres drawn when bDrawDebug=true and climbing. | happy path | World |
| TC-0464 | DebugAnchorCyanSphereDrawn | Debug | Anchor position drawn as cyan sphere when bDrawDebug=true. | happy path | World |
| TC-0465 | DebugCornerPredictiveTraceBlue | Debug | Corner predictive trace drawn in blue when bDrawDebug=true during shimmy. | happy path | World |
| TC-0466 | DebugStateTextOnScreen | Debug | Current and previous state printed on screen when bDrawDebug=true. | happy path | World |
| TC-0467 | DebugShimmyDirTextDuringShimmy | Debug | CommittedShimmyDir displayed as on-screen text during shimmy. | happy path | World |
| TC-0468 | DebugFreefallGrabWindowSphere | Debug | Freefall grab window drawn as shoulder-height sphere when bDrawDebug=true. | happy path | World |
| TC-0469 | HangIdleLeftSelectedOnNegativeLean | Anim | HangIdleLeft montage selected when lean direction is negative. | happy path | Unit |
| TC-0470 | HangIdleRightSelectedOnPositiveLean | Anim | HangIdleRight montage selected when lean direction is positive. | happy path | Unit |
| TC-0471 | DropDownSlotFromHanging | Anim | DropDown montage selected when PreviousState is Hanging. | happy path | Component |
| TC-0472 | LadderExitSideSlotFromOnLadder | Anim | LadderExitSide montage selected when PreviousState is OnLadder. | happy path | Component |
| TC-0473 | LadderFastAscendMontageOnSprint | Anim | LadderFastAscend montage selected when climbing up with Sprint held. | happy path | Component |
| TC-0474 | LadderFastDescendMontageOnCrouch | Anim | LadderFastDescend montage selected when climbing down with Crouch held. | happy path | Component |
| TC-0475 | ShimmyPlaybackRateScalesWithSpeed | Anim | Shimmy montage PlaybackRate scales between ShimmyPlaybackRateMin(0.4) and Max(1. | happy path | Component |
| TC-0476 | ClimbingMontageSlotDefaultFullBody | Anim | ClimbingMontageSlot defaults to FName("FullBody"). | happy path | Unit |
| TC-0477 | GrabFailMontagePlayedOnRejection | Anim | GrabFail montage plays after server rejection. | happy path | Component |
| TC-0478 | DebugCapsuleBoundsOrangeDrawn | Debug | Capsule override bounds drawn in orange when bDrawDebug=true during climbing. | happy path | World |
| TC-0479 | DebugAllDrawsCompiledOutInShipping | Debug | All debug draw code is inside #if !UE_BUILD_SHIPPING guards. | happy path | Unit |
| TC-0480 | EditorLacheArcOnlyWhenSelectedAndNotGameWorld | Debug | Editor Lache arc draws only when IsSelected()=true AND !IsGameWorld(). | edge case | Unit |
| TC-0481 | DebugDetectionTracesNotDrawnWhenDisabled | Debug | No detection traces drawn when bDrawDebug=false. | happy path | World |
| TC-0482 | WarpTargetLadderEnterBottomRotation | WarpTarget | WarpTarget_LadderEnterBottom rotation aligns character to face ladder. | happy path | World |
| TC-0483 | WarpTargetLedgeGrabAngledSurface | WarpTarget | WarpTarget_LedgeGrab rotation correctly projects onto angled surface normal. | edge case | World |
| TC-0484 | MantleLowVsHighThresholdBoundary | WarpTarget | Height exactly at MantleLowMaxHeight(100cm) selects MantleLow; 101cm selects Man | edge case | World |
| TC-0485 | LedgeHangIKHandTraceMissWeightFade | IK | When hand traces find no wall contact, IK weight decreases toward zero. | failure mode | Unit |
| TC-0486 | LadderIKRungTraceMissDisablesFoot | IK | When rung trace misses for one foot, that foot's IK is disabled for that frame. | failure mode | Unit |
| TC-0487 | CornerTransitionIKAllFourLimbsUpdate | IK | During corner transition all four FABRIK limb targets update each tick. | edge case | Component |
| TC-0488 | IKWeightBlendInOnStateEntry | IK | IK weight blends from 0 to 1 on climbing state entry. | edge case | Unit |
| TC-0489 | IKNullAnimInstanceNoCrash | IK | IK update functions do not crash when AnimInstance is null. | failure mode | Unit |
| TC-0490 | PerLimbMaxReachDistanceIndependent | IK | Each limb respects its own MaxReachDistance independently. | edge case | Unit |
| TC-0491 | IKFadeOutBlendTimeZeroImmediate | IK | IKFadeOutBlendTime=0 drops weight to 0 in one tick without crash. | edge case | Unit |
| TC-0492 | ShimmyPlaybackRateMinAtLowestSpeed | Shimmy | Playback rate clamps to ShimmyPlaybackRateMin(0.4) at lowest speed. | edge case | Unit |
| TC-0493 | ShimmyPlaybackRateMaxAtFullSpeed | Shimmy | Playback rate clamps to ShimmyPlaybackRateMax(1.2) at full speed. | edge case | Unit |
| TC-0494 | ShimmyPlaybackRateInterpolation | Shimmy | Rate interpolates linearly between min and max for mid-range speeds. | happy path | Unit |
| TC-0495 | ShimmyOverhangPenaltyApplied | Shimmy | Shimmy speed reduced by OverhangPenalty on overhanging surfaces. | edge case | Unit |
| TC-0496 | MaxContinuousShimmyDistanceTrigger | Shimmy | 300cm of lateral shimmy triggers ShimmyReposition. | edge case | Component |
| TC-0497 | ShimmyDistanceResetAfterReposition | Shimmy | ContinuousShimmyDistance resets to 0 after reposition. | edge case | Unit |
| TC-0498 | BracedShimmySpeedMatchesLedgeFormula | Shimmy | Braced shimmy uses same formula as ledge shimmy. | edge case | Unit |
| TC-0499 | TickLadderStateYAxisMovement | Ladder | Vertical input translated into world-space position delta each tick. | happy path | Component |
| TC-0500 | LadderTopExitTriggersTransition | Ladder | Reaching ladder top triggers LadderTransition. | edge case | Component |
| TC-0501 | LadderBottomExitTriggersTransition | Ladder | Reaching ladder bottom triggers LadderTransition. | edge case | Component |
| TC-0502 | LadderRungSnapGrid | Ladder | Position snaps to nearest LadderRungSpacing interval when stopping. | edge case | Unit |
| TC-0503 | LadderFastAscendSprintModifier | Ladder | Sprint applies LadderSprintModifier to base climb speed. | happy path | Unit |
| TC-0504 | LadderFastDescendCrouchModifier | Ladder | Crouch applies LadderCrouchModifier to descend speed. | happy path | Unit |
| TC-0505 | DetectionDuringFallingEveryTick | Detection | During MOVE_Falling, detection runs every tick. | edge case | Component |
| TC-0506 | DetectionSuppressedInCommittedStates | Detection | Detection scans do not run in committed states. | edge case | Unit |
| TC-0507 | DetectionFrequencyThreePhaseTransition | Detection | Frequency shifts correctly: ground(timer) -> climbing(per-tick) -> committed(non | edge case | Component |
| TC-0508 | ClimbableTagBypassesGeometry | Detection | Surface tagged Climbable accepted without geometric validation. | edge case | Component |
| TC-0509 | ClimbableOneWayApproachTolerance | Detection | OneWay surfaces accepted within tolerance; rejected outside. | edge case | Unit |
| TC-0510 | AllUPropertyDefaultsMatchSpec | Contracts | Every UPROPERTY with spec-defined default matches in CDO. | failure mode | Unit |
| TC-0511 | StateConfigsTMapHasAllEntries | Contracts | StateConfigs has exactly 17 entries (one per EClimbingState). | failure mode | Unit |
| TC-0512 | GetLifetimeReplicatedPropsComplete | Contracts | Every CPF_Net property appears in GetLifetimeReplicatedProps. | failure mode | Unit |
| TC-0513 | OnClimbingStateReplicatedRoutesToEnter | Multiplayer | Routes to OnStateEnter for new state on proxy. | happy path | Component |
| TC-0514 | OnClimbingStateReplicatedRoutesToExit | Multiplayer | Routes to OnStateExit for old state on proxy. | happy path | Component |
| TC-0515 | UpdateBracedWallIKValidTargets | IK | Produces valid hand/foot IK targets when wall present. | happy path | Component |
| TC-0516 | UpdateBracedWallIKNoTargetsNoWall | IK | No IK targets when no wall contact; weights fade to 0. | failure mode | Component |
| TC-0517 | ServerGrabNullHitComponentRejected | Multiplayer | Null HitComponent in net payload triggers rejection. | failure mode | Unit |
| TC-0518 | ServerGrabInvalidPayloadRejected | Multiplayer | bValid=false in net payload triggers rejection. | failure mode | Unit |
| TC-0519 | TransitionToSameStateNoOpRuntime | StateMachine | Transitioning to same state is a no-op at runtime (no entry/exit calls). | edge case | World |
| TC-0520 | GetMontageForSlotMAXEnumReturnsNull | Animation | MAX enum value returns null without crash. | failure mode | Unit |
| TC-0521 | AudioUnmappedSoundTypeNoCrash | Audio | Dispatching EClimbSoundType not in ClimbingSounds map does not crash. | failure mode | Unit |
| TC-0522 | CapsuleRestoreUncachedDimsNoCrash | Capsule | Capsule restore when original dims were never cached does not crash. | failure mode | World |
| TC-0523 | IKManagerZeroBudgetDisablesAll | IK | MaxSimultaneousIKCharacters=0 disables all IK. | edge case | World |
| TC-0524 | ShimmyDeadzoneZeroEveryInputUpdates | Shimmy | ShimmyDirectionDeadzone=0 means every non-zero input updates direction. | edge case | Unit |
| TC-0525 | LacheLaunchSpeedZeroVerticalDrop | Lache | LacheLaunchSpeed=0 produces vertical drop arc; no crash. | edge case | Unit |
| TC-0526 | LacheArcTraceStepsZeroNoCrash | Lache | LacheArcTraceSteps=0 produces no arc; no crash. | edge case | Unit |
| TC-0527 | RagdollRecoveryTimeZeroImmediateRecovery | Ragdoll | RagdollRecoveryTime=0 triggers immediate recovery. | edge case | Component |
| TC-0528 | CameraNudgeStrengthZeroNoDisplacement | Camera | CameraNudgeStrength=0 produces no camera displacement. | edge case | Component |
| TC-0529 | DetectionScanIntervalZeroPerTick | Detection | DetectionScanInterval=0 results in per-tick scanning. | edge case | Component |
| TC-0530 | LadderRungSpacingZeroNoCrash | Ladder | DefaultLadderRungSpacing=0 does not crash; no snap applied. | edge case | Unit |
| TC-0531 | GrabFailMontageNameCorrect | Anim | GrabFail montage slot maps to the GrabFail animation slot. | happy path | Unit |
| TC-0532 | ClimbingMontageSlotDefaultValue | Anim | ClimbingMontageSlot defaults to FName("FullBody"). | happy path | Unit |
| TC-0533 | ServerGrabNullComponentNoStateCorruption | Multiplayer | Null HitComponent leaves state completely unchanged (no intermediate corruption) | failure mode | Unit |
| TC-0534 | SameStateTransitionNoMontageRestart | StateMachine | Same-state transition does not restart playing montage. | edge case | World |
| TC-0535 | NegativeMaxIKCharactersClampedToZero | IK | Negative MaxSimultaneousIKCharacters clamped to zero. | edge case | Unit |
| TC-0543 | IntegrationFullLadderLifecycle | Integration | Full None->LadderTransition->OnLadder->Sprint->LadderTransition->None lifecycle. | happy path | Integration |
| TC-0544 | IntegrationFullBracedWallLifecycle | Integration | Full None->BracedWall->lip->Hanging->Shimmy->Drop->None lifecycle. | happy path | Integration |
| TC-0545 | IntegrationFullRagdollLifecycle | Integration | Full Hanging->GrabBreak->Ragdoll->RecoveryTimer->GetUp->None lifecycle. | happy path | Integration |
| TC-0546 | IntegrationMantleStepUpNoStateChange | Integration | CMC step-up below MaxStepHeight never enters climbing state. | happy path | Integration |
| TC-0547 | IntegrationBracedShimmyCornerTransition | Integration | 90-degree corner traversal during braced shimmy. | happy path | Integration |
| TC-0548 | IntegrationLacheAutoCinematicFullFlow | Integration | Camera locks during arc, unlocks after catch. | happy path | Integration |
| TC-0549 | IntegrationIdleVariationFullCycle | Integration | Two distinct variation montages play in sequence after delays. | happy path | Integration |
| TC-0550 | IntegrationFreefallReGrabFullFlow | Integration | Full fall->detect->grab->Hanging chain. | happy path | Integration |
| TC-0551 | NetworkServerGrabFullChain | Network | Server_AttemptGrab->confirm->OnRep->proxy IK update full chain. | happy path | Integration |
| TC-0552 | NetworkClientPredictionRollbackFullFlow | Network | Predict->reject->GrabFail->lerp back->None full flow. | failure mode | Integration |
| TC-0553 | NetworkTwoClientsSimultaneousGrabDrop | Network | Two clients: one grabs, one drops simultaneously; independent state. | happy path | Integration |
| TC-0554 | NetworkProxyShimmyMontageCorrect | Network | Proxy plays correct directional shimmy montage based on replicated direction. | happy path | Component |
| TC-0555 | NetworkServerDropDuringLacheCancelsArc | Network | Server_Drop in LacheInAir cancels arc; returns to None. | edge case | World |
| TC-0556 | NetworkAnchorReplicationFollowsServer | Network | Moving platform anchor replicates to client. | happy path | World |
| TC-0557 | CommittedStateMontageCompletionExits | StateMachine | All committed states exit correctly on montage completion. | happy path | World |
| TC-0558 | TickBracedWallNoLipNoInputStays | StateMachine | No lip + no input = stays BracedWall. | happy path | Unit |
| TC-0559 | TickBracedShimmyLipFoundTransitionsHanging | StateMachine | Lip found during braced shimmy tick transitions to Hanging. | edge case | Unit |
| TC-0560 | ShimmyAtExactMaxDistanceBoundary | Shimmy | Exactly MaxContinuousShimmyDistance triggers boundary behavior. | edge case | Unit |
| TC-0561 | LacheArcSingleTraceStep | Lache | LacheArcTraceSteps=1 completes without crash. | edge case | Unit |
| TC-0562 | DetectionSingleColumnSingleRow | Detection | 1x1 grid fires exactly one trace; no crash. | edge case | Unit |
| TC-0563 | AudioAllEightSoundTypesDispatched | Audio | All 8 EClimbSoundType values reach the dispatch handler. | happy path | Unit |
| TC-0564 | FreefallReGrabVelocityZeroedOnCatch | Freefall | Vertical velocity is zeroed after freefall grab. | edge case | World |
| TC-0565 | NetworkProxyOnRepFiresIKRefresh | Network | OnRep_ClimbingState triggers IK refresh on proxy. | happy path | Component |
| TC-0566 | IntegrationLadderSprintMidClimbNoEarlyExit | Integration | Sprint flag mid-ladder does not exit state. | edge case | Integration |
| TC-0567 | IntegrationRagdollRecoveryTimerUsesUPROPERTY | Integration | Timer respects RagdollRecoveryTime UPROPERTY, not hardcoded. | edge case | Integration |
| TC-0568 | EqualDistanceLedgeGrabWinsOverMantle | Priority | When ledge grab and mantle both valid at same distance, ledge grab wins. | edge case | Unit |
| TC-0569 | ClimbableOneWayCloserWinsOverNormalFarther | Priority | ClimbableOneWay surface at closer range wins over normal surface at farther rang | edge case | Unit |
| TC-0570 | LadderDetectionRunsSeparately | Priority | Ladder detection runs as separate pass from ledge/mantle. | edge case | Unit |
| TC-0571 | MultipleValidLedgesNearestSelected | Priority | When detection finds multiple valid ledges, nearest is selected. | happy path | Unit |
| TC-0572 | UnclimbableTagSkipsGeometricallyValid | Priority | Unclimbable-tagged surface skipped even if geometrically valid. | failure mode | Unit |
| TC-0573 | MantleVsLedgeGrabAtBoundaryHeight | Priority | At exact boundary height where both mantle and ledge grab are valid, ledge grab  | edge case | Unit |
| TC-0574 | PriorityOrderDeterministicAcrossFrames | Priority | Detection priority order is deterministic across 10 consecutive calls. | stress | Unit |
| TC-0575 | ClimbUpCrouchFullFlow | Integration | Full Hanging -> CrouchOnly clearance -> ClimbUpCrouch -> montage -> None. | happy path | Integration |
| TC-0576 | ClimbUpValidFromShimmyingState | Actions | ClimbUp is valid from Shimmying state (not just Hanging). | happy path | Component |
| TC-0577 | ClimbUpClearanceSweepUsesClimbingCapsule | Actions | Clearance sweep uses climbing capsule dims (48/24), not original. | edge case | Component |
| TC-0578 | ClimbUpNullMontageLogsWarningNoCrash | Actions | ClimbUp with null montage assigned logs warning, no crash. | failure mode | Unit |
| TC-0579 | WarpTargetClimbUpLandPositionMatchesLedge | WarpTarget | WarpTarget_ClimbUpLand position matches ledge top within 1cm. | happy path | Component |
| TC-0580 | WarpTargetClimbUpLandRotationMatchesNormal | WarpTarget | WarpTarget_ClimbUpLand rotation matches surface normal within 1 degree. | happy path | Component |
| TC-0581 | ServerClimbUpReRunsClearanceWithServerGeometry | Multiplayer | Server re-runs clearance with server-side geometry, not client data. | happy path | World |
| TC-0582 | ClimbUpCrouchMontageSelectionVerified | Actions | ClimbUpCrouch state plays the ClimbUpCrouch montage (not ClimbUp). | happy path | Component |
| TC-0583 | ClimbUpFromHangingFullClearance | Actions | Full clearance from Hanging selects ClimbingUp state (not ClimbingUpCrouch). | happy path | Component |
| TC-0584 | ClimbUpNoClearanceRemainsHanging | Actions | No clearance (ClearanceType=None) keeps character in current state. | failure mode | Component |
| TC-0585 | ClimbUpInterruptedBlendOutNoTransition | MontageCallback | bInterrupted=true does not trigger state transition. | edge case | Component |
| TC-0586 | ClimbUpFromShimmyingClearanceCrouch | Actions | ClimbUp from Shimmying with CrouchOnly clearance selects ClimbUpCrouch. | edge case | Component |
| TC-0587 | ClimbUpWarpTargetRegisteredBeforeMontage | Actions | WarpTarget_ClimbUpLand registered before ClimbUp montage begins playing. | happy path | Component |
| TC-0588 | CornerOutsideLeftMontageSelected | Actions | CornerOutsideLeft montage selected when bCurrentCornerIsInside=false and Committ | happy path | Unit |
| TC-0589 | CornerInsideRightMontageSelected | Actions | CornerInsideRight montage selected when bCurrentCornerIsInside=true and Committe | happy path | Unit |
| TC-0590 | CornerDotProductZeroDeterministic | Actions | Dot product exactly 0 (perpendicular normals) produces deterministic classificat | edge case | Unit |
| TC-0591 | CornerTransitionFromBracedShimmying | Actions | Corner transition valid from BracedShimmying state. | happy path | Component |
| TC-0592 | CornerNullMontageLogsWarningNoCrash | Actions | Null corner montage logs warning, no crash. | failure mode | Unit |
| TC-0593 | CornerPredictiveTraceDistanceRespected | Actions | Predictive trace distance/radius parameters are respected. | edge case | Component |
| TC-0594 | CornerTransitionResetsContinuousShimmyDistance | Actions | Corner transition resets ContinuousShimmyDistance to 0. | edge case | Unit |
| TC-0595 | CornerTransitionReplicatedCorrectly | Multiplayer | Corner transition state and bCurrentCornerIsInside replicated to proxy. | happy path | World |
| TC-0596 | BeginPlayNullMotionWarpingLogsWarning | Lifecycle | Null MotionWarpingComponent logs warning but continues initialization. | failure mode | World |
| TC-0597 | BeginPlayRegistersDetectionScanTimer | Lifecycle | BeginPlay registers detection scan timer at DetectionScanInterval. | happy path | World |
| TC-0598 | BeginPlayInitializesStateConfigsTMap | Lifecycle | BeginPlay initializes StateConfigs TMap with 17 entries. | happy path | World |
| TC-0599 | DestroyedDuringActiveMontageStopsSafely | Lifecycle | Destroyed during active montage stops montage safely, no crash. | failure mode | World |
| TC-0600 | EndPlayDuringLacheInAirClearsTarget | Lifecycle | EndPlay during LacheInAir clears LockedLacheTarget and transitions to DroppingDo | edge case | World |
| TC-0601 | EndPlayDuringRagdollRestoresPhysics | Lifecycle | EndPlay during Ragdoll restores physics (SetSimulatePhysics false). | edge case | World |
| TC-0602 | EndPlayRemovesFromActiveClimbingCharacters | Lifecycle | EndPlay removes character from ActiveClimbingCharacters (level transition path). | happy path | World |
| TC-0603 | DestroyedDuringCornerTransitionSafeExit | Lifecycle | Destroyed during CornerTransition committed state exits safely. | failure mode | World |
| TC-0604 | BeginPlayNullMotionWarpingCompletesOtherInit | Lifecycle | Null MotionWarpingComponent does not prevent other init (IK registration, timer, | failure mode | World |
| TC-0605 | CornerAngleExactly30FromBracedShimmy | Actions | Angle exactly 30° from BracedShimmying triggers corner transition. | edge case | Component |
| TC-0606 | EndPlayIdempotentDuringLacheInAir | Lifecycle | Double EndPlay during LacheInAir does not crash. | edge case | World |
| TC-0607 | CornerOutsideLeftNullMontageFallback | Actions | Null CornerOutsideLeft montage returns null from GetMontageForSlot, not wrong mo | failure mode | Unit |
| TC-0608 | PerfDetectionHeavyGeometry | Performance | Detection scan with 100+ actors in range completes under 5ms. | stress | Performance |
| TC-0609 | PerfNetworkRpcThroughput | Performance | 100 Server_AttemptGrab RPCs processed in < 100ms. | stress | Performance |
| TC-0610 | PerfMemoryStabilityRapidCycling | Performance | No allocation growth over 1000 state cycles. | stress | Performance |
| TC-0611 | PerfMontageThroughput | Performance | 1000 Montage_Play + Montage_Stop pairs complete in < 50ms. | stress | Performance |
| TC-0612 | PerfIKFourLimbsSingleCharacter | Performance | Single character 4-limb IK computation < 0.5ms. | stress | Performance |
| TC-0613 | PerfDetectionMaxGridSize | Performance | 10x10 grid scan completes under 2ms. | stress | Performance |
| TC-0614 | PerfStateTransitionFullCleanup | Performance | State transition with full cleanup (capsule+IMC+SetBase) < 0.1ms. | stress | Performance |
| TC-0615 | PerfAnchorFollowing100Ticks | Performance | 100 ticks with moving anchor < 1ms total. | stress | Performance |
| TC-0616 | PerfAudioDispatch100Sounds | Performance | 100 sound dispatches < 1ms total. | stress | Performance |
| TC-0617 | StressTenSimultaneousClimbers | Stress | 10 characters climbing simultaneously; all systems stable for 60 ticks. | stress | Integration |
| TC-0618 | StressRapidShimmyDirectionChanges | Stress | 100 rapid shimmy direction changes in 1s; no state corruption. | stress | Integration |
| TC-0619 | StressRapidLacheLaunchCancel | Stress | 10 Lache launch/cancel cycles in 2s; no arc leak. | stress | Integration |
| TC-0620 | PerfGetMontageForSlotOverride | Performance | 10000 GetMontageForSlot calls with override active < 5ms. | stress | Performance |
| TC-0621 | PerfResolveHitComponentFromNet | Performance | 100 ResolveHitComponentFromNet calls < 10ms. | stress | Performance |
| TC-0622 | PerfValidateOneWayApproach | Performance | 10000 ValidateOneWayApproach calls < 1ms. | stress | Performance |
| TC-0623 | PerfDetectionHeavyGeometryCorrectness | Performance | Detection under heavy geometry produces no false positives. | stress | Performance |
| TC-0624 | PerfMemoryBaselineStable | Performance | Memory baseline stable after 10-cycle warmup; no growth in subsequent 100 cycles | stress | Performance |
| TC-0625 | PerfMultiClimberIKBudgetTen | Performance | 10-character IK total < 5ms (only 4 active per budget). | stress | Performance |
| TC-0626 | StressLacheArcObjectCountPostCancel | Stress | After Lache cancel, arc-related object count returns to pre-launch baseline. | stress | Integration |
| TC-0627 | StressShimmyDirectionEnumIntegrity | Stress | CommittedShimmyDir is always -1, 0, or +1 after 100 rapid reversals. | stress | Integration |

---

## PLANNED — NOT YET IMPLEMENTED
<!-- Full entries. One entry per test. -->
<!-- Implementer reads this section to know what to build. -->
<!-- ALL 468 PLANNED TCs have been implemented. Section is empty. -->

---

## COVERAGE SUMMARY
<!-- Updated every run -->

| System | TCs Implemented | TCs Planned | Coverage Est. | Last Updated |
|--------|----------------|-------------|---------------|--------------|
| Types/Structs/Enums | 7 | 0 | ~100% | Sync |
| SurfaceData | 2 | 0 | ~100% | Sync |
| Traces | 5 | 0 | ~100% | Sync |
| Detection (Ledge/Mantle/Braced/Wall) | 19 | 0 | ~100% | Sync |
| Priority | 3 | 0 | ~100% | Sync |
| Movement Component | 18 | 0 | ~100% | Sync |
| State Machine / Montage Callbacks | 4 | 0 | ~100% | Sync |
| Actions (Shimmy) | 4 | 0 | ~100% | Sync |
| Actions (ClimbUp) | 5 | 0 | ~100% | Sync |
| Actions (Corner) | 1 | 0 | ~100% | Sync |
| Actions (BracedWall) | 1 | 0 | ~100% | Sync |
| Actions (Lache) | 5 | 0 | ~100% | Sync |
| Actions (Mantle) | 3 | 0 | ~100% | Sync |
| Ladder | 1 | 0 | ~100% | Sync |
| Ragdoll/Physics | 2 | 0 | ~100% | Sync |
| IK System | 3 | 0 | ~100% | Sync |
| Animation (AnimInstance/Set/Notifies) | 11 | 0 | ~100% | Sync |
| Audio | 3 | 0 | ~100% | Sync |
| Camera | 1 | 0 | ~100% | Sync |
| Multiplayer (RPCs/Replication) | 12 | 0 | ~100% | Sync |
| Input (IMC/Handlers) | 6 | 0 | ~100% | Sync |
| Debug Visualization | 0 | 0 | ~100% | Sync |
| Lifecycle (BeginPlay/EndPlay/Destroyed) | 0 | 0 | ~100% | Sync |
| Freefall / Coyote Time | 1 | 0 | ~100% | Sync |
| Integration | 10 | 0 | ~100% | Sync |
| Performance / Stress | 4 | 0 | ~100% | Sync |
| Contracts | 4 | 0 | ~100% | Sync |
| Arc | 3 | 0 | ~100% | Sync |
| Capsule Management | 0 | 0 | ~100% | Sync |
| Warp Targets | 0 | 0 | ~100% | Sync |

---
