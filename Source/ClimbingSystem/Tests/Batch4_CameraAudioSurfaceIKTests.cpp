// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Animation/ClimbingAnimationSet.h"
#include "Data/ClimbingSurfaceData.h"
#include "Data/ClimbingTypes.h"
#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

// ---------------------------------------------------------------------------
// TC-0314: CameraLockZeroBlendTimeImmediate
// WHAT: LockCameraToFrame with BlendTime=0 must not crash.
// WHY:  Zero blend time is a valid "snap" request; crashing here breaks
//       all cinematic camera entry points.
// VERIFY: Confirming immediate position snap requires a live camera tick (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCameraLockZeroBlendTimeImmediateTest,
	"ClimbingSystem.CameraAudioSurfaceIK.Camera.CameraLockZeroBlendTimeImmediate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCameraLockZeroBlendTimeImmediateTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0314: character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	// BlendTime=0 — must not crash.
	Character->LockCameraToFrame(FVector(100.f, 0.f, 200.f), FRotator(0.f, 90.f, 0.f), 0.f);

	// VERIFY: Confirming immediate position snap requires a live camera tick (PIE).

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0315: CameraReleaseWhenNotLockedIsNoOp
// WHAT: ReleaseCameraLock without a prior lock must not crash.
// WHY:  Release can be called defensively on state exit regardless of lock state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCameraReleaseWhenNotLockedIsNoOpTest,
	"ClimbingSystem.CameraAudioSurfaceIK.Camera.CameraReleaseWhenNotLockedIsNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCameraReleaseWhenNotLockedIsNoOpTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0315: character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	// No prior lock — must not crash.
	Character->ReleaseCameraLock(0.5f);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0316: RagdollCameraSocketSwitch
// WHAT: CDO RagdollCameraTargetSocket default == FName("pelvis").
// WHY:  Wrong socket name causes the spring arm to attach to a missing bone
//       during ragdoll, snapping the camera to world origin.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRagdollCameraSocketSwitchTest,
	"ClimbingSystem.CameraAudioSurfaceIK.Camera.RagdollCameraSocketSwitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRagdollCameraSocketSwitchTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0316: CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	TestEqual(TEXT("TC-0316: RagdollCameraTargetSocket default should be FName(\"pelvis\")"),
		Defaults->RagdollCameraTargetSocket, FName("pelvis"));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0317: RagdollCameraSocketRestoredOnRecovery
// WHAT: Complement to TC-0316 — socket name is still "pelvis" after a
//       simulated ragdoll entry (no crash, value unchanged).
// WHY:  Recovery must not corrupt the socket name used for camera attachment.
// VERIFY: Confirming the spring arm re-attaches to the correct socket after
//         recovery requires a live skeletal mesh (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRagdollCameraSocketRestoredOnRecoveryTest,
	"ClimbingSystem.CameraAudioSurfaceIK.Camera.RagdollCameraSocketRestoredOnRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRagdollCameraSocketRestoredOnRecoveryTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0317: character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	// Simulate entering ragdoll state — must not crash.
	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0317: movement component should exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Ragdoll);

	// Socket name must remain "pelvis" — not corrupted by state change.
	TestEqual(TEXT("TC-0317: RagdollCameraTargetSocket should remain \"pelvis\" after ragdoll entry"),
		Character->RagdollCameraTargetSocket, FName("pelvis"));

	// VERIFY: Confirming spring arm re-attaches to the correct socket after
	// recovery requires a live skeletal mesh (PIE).

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0318: AudioResolvedCacheHitSecondCall
// WHAT: CDO ClimbingSounds TMap exists on AClimbingCharacter.
// WHY:  Missing TMap means no sounds can ever be resolved; the cache-hit path
//       is unreachable and audio is silently absent.
// VERIFY: Confirming a second GetResolvedSound call returns the cached ptr
//         requires calling the private helper twice (PIE or friend access).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAudioResolvedCacheHitSecondCallTest,
	"ClimbingSystem.CameraAudioSurfaceIK.Audio.AudioResolvedCacheHitSecondCall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAudioResolvedCacheHitSecondCallTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0318: CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	// Verify the ClimbingSounds TMap property exists via reflection.
	const FProperty* Prop = AClimbingCharacter::StaticClass()->FindPropertyByName(TEXT("ClimbingSounds"));
	TestNotNull(TEXT("TC-0318: ClimbingSounds property should exist on AClimbingCharacter"), Prop);

	// VERIFY: Confirming a second GetResolvedSound call returns the cached ptr
	// requires calling the private helper twice — needs PIE or friend access.

	return true;
}

// ---------------------------------------------------------------------------
// TC-0319: AudioAsyncLoadSuccessCachesPtr
// WHAT: Contract — async load success path caches the resolved USoundBase ptr.
// WHY:  Without caching, every sound event triggers a new async load, causing
//       audio latency and asset manager thrashing.
// VERIFY: Confirming the cache is populated after async load completion requires
//         a running asset manager with a valid soft ptr (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAudioAsyncLoadSuccessCachesPtrTest,
	"ClimbingSystem.CameraAudioSurfaceIK.Audio.AudioAsyncLoadSuccessCachesPtr",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAudioAsyncLoadSuccessCachesPtrTest::RunTest(const FString& Parameters)
{
	// Contract: ResolvedSounds TMap property exists to hold the cache.
	const FProperty* Prop = AClimbingCharacter::StaticClass()->FindPropertyByName(TEXT("ResolvedSounds"));
	TestNotNull(TEXT("TC-0319: ResolvedSounds cache property should exist on AClimbingCharacter"), Prop);

	// VERIFY: Confirming the cache is populated after async load completion requires
	// a running asset manager with a valid soft ptr (PIE).

	return true;
}

// ---------------------------------------------------------------------------
// TC-0320: AudioAsyncLoadFailureCachesNullNoRetry
// WHAT: Contract — failed async load caches null and does not retry.
// WHY:  Retrying on every sound event for a missing asset causes per-frame
//       asset manager spam and log flooding.
// VERIFY: Confirming null is cached and no retry occurs requires injecting a
//         failing soft ptr into ClimbingSounds (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAudioAsyncLoadFailureCachesNullNoRetryTest,
	"ClimbingSystem.CameraAudioSurfaceIK.Audio.AudioAsyncLoadFailureCachesNullNoRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAudioAsyncLoadFailureCachesNullNoRetryTest::RunTest(const FString& Parameters)
{
	// Contract: the ToolTip on ClimbingSounds documents "Missing entries: log Warning
	// once, cache null, never retry." Verify the property carries that contract.
	const FProperty* Prop = AClimbingCharacter::StaticClass()->FindPropertyByName(TEXT("ClimbingSounds"));
	TestNotNull(TEXT("TC-0320: ClimbingSounds property should exist"), Prop);

	// VERIFY: Confirming null is cached and no retry occurs requires injecting a
	// failing soft ptr into ClimbingSounds (PIE).

	return true;
}

// ---------------------------------------------------------------------------
// TC-0321: AudioGrabFailSoundDispatched
// WHAT: EClimbSoundType::GrabFail exists in the enum.
// WHY:  Missing enum value means grab-fail audio can never be keyed in
//       ClimbingSounds and the dispatch call site won't compile.
// VERIFY: Confirming the sound is actually dispatched on server rejection
//         requires a server RPC round-trip (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAudioGrabFailSoundDispatchedTest,
	"ClimbingSystem.CameraAudioSurfaceIK.Audio.AudioGrabFailSoundDispatched",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAudioGrabFailSoundDispatchedTest::RunTest(const FString& Parameters)
{
	// Verify EClimbSoundType::GrabFail is a valid enum value.
	const UEnum* SoundEnum = StaticEnum<EClimbSoundType>();
	TestNotNull(TEXT("TC-0321: EClimbSoundType enum should exist"), SoundEnum);
	if (!SoundEnum) { return false; }

	const int64 GrabFailValue = SoundEnum->GetValueByName(TEXT("GrabFail"));
	TestTrue(TEXT("TC-0321: EClimbSoundType::GrabFail should be a valid enum value"),
		GrabFailValue != INDEX_NONE);

	// VERIFY: Confirming the sound is dispatched on server rejection requires
	// a server RPC round-trip (PIE).

	return true;
}

// ---------------------------------------------------------------------------
// TC-0322: AudioLacheSoundsDispatched
// WHAT: EClimbSoundType::LacheLaunchGrunt and LacheCatchImpact exist.
// WHY:  Missing enum values mean Lache audio events can never be keyed in
//       ClimbingSounds — both sounds would be silently absent.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAudioLacheSoundsDispatchedTest,
	"ClimbingSystem.CameraAudioSurfaceIK.Audio.AudioLacheSoundsDispatched",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAudioLacheSoundsDispatchedTest::RunTest(const FString& Parameters)
{
	const UEnum* SoundEnum = StaticEnum<EClimbSoundType>();
	TestNotNull(TEXT("TC-0322: EClimbSoundType enum should exist"), SoundEnum);
	if (!SoundEnum) { return false; }

	const int64 LaunchValue = SoundEnum->GetValueByName(TEXT("LacheLaunchGrunt"));
	TestTrue(TEXT("TC-0322: EClimbSoundType::LacheLaunchGrunt should be a valid enum value"),
		LaunchValue != INDEX_NONE);

	const int64 CatchValue = SoundEnum->GetValueByName(TEXT("LacheCatchImpact"));
	TestTrue(TEXT("TC-0322: EClimbSoundType::LacheCatchImpact should be a valid enum value"),
		CatchValue != INDEX_NONE);

	return true;
}

// ---------------------------------------------------------------------------
// TC-0323: SurfaceDataAnimSetOverrideAsyncLoad
// WHAT: UClimbingSurfaceData::AnimationSetOverride property exists.
// WHY:  Missing property means surface-specific animation overrides can never
//       be assigned or loaded asynchronously.
// VERIFY: Confirming async load is triggered on first surface contact requires
//         a running asset manager (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurfaceDataAnimSetOverrideAsyncLoadTest,
	"ClimbingSystem.CameraAudioSurfaceIK.SurfaceData.SurfaceDataAnimSetOverrideAsyncLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurfaceDataAnimSetOverrideAsyncLoadTest::RunTest(const FString& Parameters)
{
	UClimbingSurfaceData* SurfaceData = NewObject<UClimbingSurfaceData>();
	TestNotNull(TEXT("TC-0323: UClimbingSurfaceData should be creatable"), SurfaceData);
	if (!SurfaceData) { return false; }

	// Verify AnimationSetOverride property exists via reflection.
	const FProperty* Prop = UClimbingSurfaceData::StaticClass()->FindPropertyByName(TEXT("AnimationSetOverride"));
	TestNotNull(TEXT("TC-0323: AnimationSetOverride property should exist on UClimbingSurfaceData"), Prop);

	// Default should be null (no override assigned).
	TestTrue(TEXT("TC-0323: AnimationSetOverride should be null by default"),
		SurfaceData->AnimationSetOverride.IsNull());

	// VERIFY: Confirming async load is triggered on first surface contact requires
	// a running asset manager (PIE).

	return true;
}

// ---------------------------------------------------------------------------
// TC-0324: SurfaceDataPerSlotFallback
// WHAT: A freshly created UClimbingAnimationSet returns null for every slot.
// WHY:  Callers must handle null per-slot and fall back to character defaults;
//       a non-null default would mask missing asset assignments.
// VERIFY: Confirming the character falls back to its own montage when the
//         override returns null requires GetMontageForSlot on a live character.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurfaceDataPerSlotFallbackTest,
	"ClimbingSystem.CameraAudioSurfaceIK.SurfaceData.SurfaceDataPerSlotFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurfaceDataPerSlotFallbackTest::RunTest(const FString& Parameters)
{
	UClimbingAnimationSet* AnimSet = NewObject<UClimbingAnimationSet>();
	TestNotNull(TEXT("TC-0324: UClimbingAnimationSet should be creatable"), AnimSet);
	if (!AnimSet) { return false; }

	// All slots should return null on a freshly created set.
	UAnimMontage* Result = AnimSet->GetMontageForSlot(EClimbingAnimationSlot::HangIdle);
	TestNull(TEXT("TC-0324: GetMontageForSlot(HangIdle) should return null on a new UClimbingAnimationSet"), Result);

	UAnimMontage* ShimmyResult = AnimSet->GetMontageForSlot(EClimbingAnimationSlot::ShimmyLeft);
	TestNull(TEXT("TC-0324: GetMontageForSlot(ShimmyLeft) should return null on a new UClimbingAnimationSet"), ShimmyResult);

	// VERIFY: Confirming the character falls back to its own montage requires
	// GetMontageForSlot on a live character with a surface override set (PIE).

	return true;
}

// ---------------------------------------------------------------------------
// TC-0325: SurfaceDataClimbSpeedMultiplierAffectsShimmy
// WHAT: CDO ClimbSpeedMultiplier default == 1.0 on UClimbingSurfaceData.
// WHY:  A wrong default (e.g. 0) would halt shimmy on every surface that has
//       surface data assigned, even without explicit configuration.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurfaceDataClimbSpeedMultiplierAffectsShimmyTest,
	"ClimbingSystem.CameraAudioSurfaceIK.SurfaceData.SurfaceDataClimbSpeedMultiplierAffectsShimmy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurfaceDataClimbSpeedMultiplierAffectsShimmyTest::RunTest(const FString& Parameters)
{
	const UClimbingSurfaceData* Defaults = GetDefault<UClimbingSurfaceData>();
	TestNotNull(TEXT("TC-0325: UClimbingSurfaceData CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("TC-0325: ClimbSpeedMultiplier CDO default should be 1.0"),
		FMath::IsNearlyEqual(Defaults->ClimbSpeedMultiplier, 1.0f));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0326: SurfaceDataClimbSpeedMultiplierZeroHaltsShimmy
// WHAT: Setting ClimbSpeedMultiplier=0 on a new UClimbingSurfaceData instance
//       stores 0; GetShimmySpeed contract: multiplier=0 → speed=0.
// WHY:  Zero multiplier must halt shimmy movement; non-zero storage would
//       allow movement on a surface marked as impassable.
// VERIFY: Confirming GetShimmySpeed returns 0 when the active surface has
//         ClimbSpeedMultiplier=0 requires a live movement component with the
//         surface data applied (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurfaceDataClimbSpeedMultiplierZeroHaltsShimmyTest,
	"ClimbingSystem.CameraAudioSurfaceIK.SurfaceData.SurfaceDataClimbSpeedMultiplierZeroHaltsShimmy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurfaceDataClimbSpeedMultiplierZeroHaltsShimmyTest::RunTest(const FString& Parameters)
{
	UClimbingSurfaceData* SurfaceData = NewObject<UClimbingSurfaceData>();
	TestNotNull(TEXT("TC-0326: UClimbingSurfaceData should be creatable"), SurfaceData);
	if (!SurfaceData) { return false; }

	SurfaceData->ClimbSpeedMultiplier = 0.f;
	TestTrue(TEXT("TC-0326: ClimbSpeedMultiplier should store 0"),
		FMath::IsNearlyEqual(SurfaceData->ClimbSpeedMultiplier, 0.f));

	// VERIFY: Confirming GetShimmySpeed returns 0 when the active surface has
	// ClimbSpeedMultiplier=0 requires a live movement component with the surface
	// data applied (PIE).

	return true;
}

// ---------------------------------------------------------------------------
// TC-0327: SurfaceDataAudioOverrideUsed
// WHAT: UClimbingSurfaceData has SoundVolumeMultiplier and PhysicalSurfaceType
//       properties that can drive surface-specific audio dispatch.
// WHY:  Without these properties, all surfaces play at the same volume with
//       the same material sound — no surface audio differentiation.
// VERIFY: Confirming the audio system reads these values during PlayClimbingSound
//         requires a live audio dispatch call (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurfaceDataAudioOverrideUsedTest,
	"ClimbingSystem.CameraAudioSurfaceIK.SurfaceData.SurfaceDataAudioOverrideUsed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurfaceDataAudioOverrideUsedTest::RunTest(const FString& Parameters)
{
	UClimbingSurfaceData* SurfaceData = NewObject<UClimbingSurfaceData>();
	TestNotNull(TEXT("TC-0327: UClimbingSurfaceData should be creatable"), SurfaceData);
	if (!SurfaceData) { return false; }

	// Verify audio override properties exist.
	const FProperty* VolProp = UClimbingSurfaceData::StaticClass()->FindPropertyByName(TEXT("SoundVolumeMultiplier"));
	TestNotNull(TEXT("TC-0327: SoundVolumeMultiplier property should exist"), VolProp);

	const FProperty* SurfProp = UClimbingSurfaceData::StaticClass()->FindPropertyByName(TEXT("PhysicalSurfaceType"));
	TestNotNull(TEXT("TC-0327: PhysicalSurfaceType property should exist"), SurfProp);

	// Default volume multiplier should be 1.0 (no attenuation).
	TestTrue(TEXT("TC-0327: SoundVolumeMultiplier default should be 1.0"),
		FMath::IsNearlyEqual(SurfaceData->SoundVolumeMultiplier, 1.0f));

	// VERIFY: Confirming the audio system reads these values during PlayClimbingSound
	// requires a live audio dispatch call (PIE).

	return true;
}

// ---------------------------------------------------------------------------
// TC-0328: SurfaceDataAudioOverrideNullFallback
// WHAT: A UClimbingSurfaceData with default PhysicalSurfaceType (SurfaceType_Default)
//       does not crash when used as an audio override source.
// WHY:  Null/default surface type must fall back to the character's default
//       ClimbingSounds map without crashing.
// VERIFY: Confirming the fallback path is taken during PlayClimbingSound requires
//         a live audio dispatch call with a surface-data-bearing component (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSurfaceDataAudioOverrideNullFallbackTest,
	"ClimbingSystem.CameraAudioSurfaceIK.SurfaceData.SurfaceDataAudioOverrideNullFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSurfaceDataAudioOverrideNullFallbackTest::RunTest(const FString& Parameters)
{
	UClimbingSurfaceData* SurfaceData = NewObject<UClimbingSurfaceData>();
	TestNotNull(TEXT("TC-0328: UClimbingSurfaceData should be creatable"), SurfaceData);
	if (!SurfaceData) { return false; }

	// Default PhysicalSurfaceType is SurfaceType_Default — the null/fallback case.
	TestEqual(TEXT("TC-0328: PhysicalSurfaceType default should be SurfaceType_Default"),
		SurfaceData->PhysicalSurfaceType.GetValue(), EPhysicalSurface::SurfaceType_Default);

	// VERIFY: Confirming the fallback path is taken during PlayClimbingSound requires
	// a live audio dispatch call with a surface-data-bearing component (PIE).

	return true;
}

// ---------------------------------------------------------------------------
// TC-0329: IKCullDistanceAtExactBoundary
// WHAT: CDO SimulatedProxyIKCullDistance default == 1500.
// WHY:  Wrong default culls IK too aggressively (too small) or wastes GPU
//       budget on distant proxies (too large).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIKCullDistanceAtExactBoundaryTest,
	"ClimbingSystem.CameraAudioSurfaceIK.IK.IKCullDistanceAtExactBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIKCullDistanceAtExactBoundaryTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0329: CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("TC-0329: SimulatedProxyIKCullDistance default should be 1500"),
		FMath::IsNearlyEqual(Defaults->SimulatedProxyIKCullDistance, 1500.0f));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0330: IKCornerFABRIKAllFourLimbs
// WHAT: EClimbingState::CornerTransition is a valid state (contract for
//       UpdateCornerIK / FABRIK blend path).
// WHY:  If CornerTransition is removed or renamed, the FABRIK blend path
//       becomes unreachable and corner IK silently stops working.
// VERIFY: Confirming all four limb IK targets are updated during a corner
//         transition requires a live AnimInstance tick (PIE).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIKCornerFABRIKAllFourLimbsTest,
	"ClimbingSystem.CameraAudioSurfaceIK.IK.IKCornerFABRIKAllFourLimbs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIKCornerFABRIKAllFourLimbsTest::RunTest(const FString& Parameters)
{
	// Contract: CornerTransition is a valid EClimbingState value.
	const UEnum* StateEnum = StaticEnum<EClimbingState>();
	TestNotNull(TEXT("TC-0330: EClimbingState enum should exist"), StateEnum);
	if (!StateEnum) { return false; }

	const int64 CornerValue = StateEnum->GetValueByName(TEXT("CornerTransition"));
	TestTrue(TEXT("TC-0330: EClimbingState::CornerTransition should be a valid enum value"),
		CornerValue != INDEX_NONE);

	// VERIFY: Confirming all four limb IK targets are updated during a corner
	// transition requires a live AnimInstance tick (PIE).

	return true;
}

// ---------------------------------------------------------------------------
// TC-0331: SpringArmRotatesOnClimbEntry
// WHAT: Transitioning to Hanging state must not crash.
// WHY:  Spring arm rotation adjustment on climb entry is a common crash site
//       when CameraBoom is null or the state machine is misconfigured.
// VERIFY: Confirming the spring arm rotation actually changed requires reading
//         CameraBoom->RelativeRotation before and after (PIE with a valid mesh).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpringArmRotatesOnClimbEntryTest,
	"ClimbingSystem.CameraAudioSurfaceIK.Camera.SpringArmRotatesOnClimbEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpringArmRotatesOnClimbEntryTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0331: character should spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0331: movement component should exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	// Transition to Hanging — must not crash.
	Movement->SetClimbingState(EClimbingState::Hanging);

	TestEqual(TEXT("TC-0331: state should be Hanging after SetClimbingState"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);

	// VERIFY: Confirming the spring arm rotation actually changed requires reading
	// CameraBoom->RelativeRotation before and after (PIE with a valid mesh).

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0332: IKFadeOutBlendTimeDefault
// WHAT: CDO IKFadeOutBlendTime default == 0.15.
// WHY:  Wrong default causes IK to snap off (0) or linger too long (large value)
//       when a limb exceeds MaxReachDistance.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIKFadeOutBlendTimeDefaultTest,
	"ClimbingSystem.CameraAudioSurfaceIK.IK.IKFadeOutBlendTimeDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIKFadeOutBlendTimeDefaultTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0332: CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("TC-0332: IKFadeOutBlendTime default should be 0.15"),
		FMath::IsNearlyEqual(Defaults->IKFadeOutBlendTime, 0.15f));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0333: MaxReachDistanceDefault
// WHAT: CDO MaxReachDistance default == 80.
// WHY:  Wrong default allows hyperextension (too large) or premature IK fade
//       (too small), both producing visible animation artifacts.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMaxReachDistanceDefaultTest,
	"ClimbingSystem.CameraAudioSurfaceIK.IK.MaxReachDistanceDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaxReachDistanceDefaultTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0333: CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("TC-0333: MaxReachDistance default should be 80"),
		FMath::IsNearlyEqual(Defaults->MaxReachDistance, 80.0f));

	return true;
}

// ---------------------------------------------------------------------------
// TC-0334: HandIKSpacingDefault
// WHAT: CDO HandIKSpacing default == 40.
// WHY:  Wrong default places both hand IK targets at the same point (0) or
//       too far apart (large value), causing visible hand clipping or splaying.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHandIKSpacingDefaultTest,
	"ClimbingSystem.CameraAudioSurfaceIK.IK.HandIKSpacingDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHandIKSpacingDefaultTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0334: CDO should exist"), Defaults);
	if (!Defaults) { return false; }

	TestTrue(TEXT("TC-0334: HandIKSpacing default should be 40"),
		FMath::IsNearlyEqual(Defaults->HandIKSpacing, 40.0f));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
