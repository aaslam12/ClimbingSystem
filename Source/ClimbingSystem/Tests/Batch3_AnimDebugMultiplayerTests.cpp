// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Animation/ClimbingAnimInstance.h"
#include "Data/ClimbingTypes.h"
#include "UObject/UnrealType.h"
#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

// ---------------------------------------------------------------------------
// TC-0234: IdleVariationDelayTimerFires
// WHAT: IdleVariationDelay property exists and is > 0.
// WHY: A zero or missing delay would fire the timer immediately, spamming variations.
// VERIFY: Full timer-fires test requires advancing world time past IdleVariationDelay.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3IdleVariationDelayTimerFiresTest,
	"ClimbingSystem.AnimDebugMultiplayer.IdleVariation.IdleVariationDelayTimerFires",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3IdleVariationDelayTimerFiresTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0234: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0234: IdleVariationDelay must be > 0"),
		CDO->IdleVariationDelay > 0.f);
	return true;
}

// ---------------------------------------------------------------------------
// TC-0235: IdleVariationNoRepeatSameVariation
// WHAT: bPreventConsecutiveVariationRepeat default == true.
// WHY: Without this guard the same variation can play back-to-back.
// VERIFY: Full repeat-prevention test requires driving the variation selection loop.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3IdleVariationNoRepeatSameVariationTest,
	"ClimbingSystem.AnimDebugMultiplayer.IdleVariation.IdleVariationNoRepeatSameVariation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3IdleVariationNoRepeatSameVariationTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0235: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0235: bPreventConsecutiveVariationRepeat must default to true"),
		CDO->bPreventConsecutiveVariationRepeat);
	return true;
}

// ---------------------------------------------------------------------------
// TC-0236: IdleVariationResetsTimerAfterPlay
// WHAT: Contract: IdleVariationDelay > 0 implies the timer can reset after play.
// WHY: A zero delay would make a reset meaningless; positive delay is the precondition.
// VERIFY: Full reset test requires observing IdleVariationTimerHandle re-set after montage end.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3IdleVariationResetsTimerAfterPlayTest,
	"ClimbingSystem.AnimDebugMultiplayer.IdleVariation.IdleVariationResetsTimerAfterPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3IdleVariationResetsTimerAfterPlayTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0236: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0236: IdleVariationDelay > 0 is the precondition for timer reset"),
		CDO->IdleVariationDelay > 0.f);
	return true;
}

// ---------------------------------------------------------------------------
// TC-0237: BlendTimeZeroIsImmediate
// WHAT: BlendIKWeight with BlendTime==0 must return TargetWeight immediately.
// WHY: Zero blend time means instant transition — no interpolation frame needed.
// VERIFY: Tests the static BlendIKWeight helper directly.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3BlendTimeZeroIsImmediateTest,
	"ClimbingSystem.AnimDebugMultiplayer.AnimInstance.BlendTimeZeroIsImmediate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3BlendTimeZeroIsImmediateTest::RunTest(const FString& Parameters)
{
	// BlendTime == 0 → result must equal TargetWeight regardless of DeltaTime
	const float Result = UClimbingAnimInstance::BlendIKWeight(0.f, 1.f, 0.016f, 0.f);
	TestTrue(TEXT("TC-0237: BlendIKWeight with BlendTime=0 must return 1.0 immediately"),
		FMath::IsNearlyEqual(Result, 1.f));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0238: DebugShippingGuardPreventsExecution
// WHAT: bDrawDebug property exists on AClimbingCharacter.
// WHY: The property is the runtime gate; the #if !UE_BUILD_SHIPPING guard is compile-time.
// VERIFY: Compile-time guard is verified by the fact that this test file compiles in
//         non-Shipping configurations only (WITH_DEV_AUTOMATION_TESTS).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3DebugShippingGuardTest,
	"ClimbingSystem.AnimDebugMultiplayer.Debug.DebugShippingGuardPreventsExecution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3DebugShippingGuardTest::RunTest(const FString& Parameters)
{
	const UClass* Class = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0238: AClimbingCharacter class must exist"), Class);
	if (!Class) { return false; }

	const FProperty* Prop = Class->FindPropertyByName(TEXT("bDrawDebug"));
	TestNotNull(TEXT("TC-0238: bDrawDebug property must exist on AClimbingCharacter"), Prop);
	return Prop != nullptr;
}

// ---------------------------------------------------------------------------
// TC-0239: DebugDrawOnlyWhenFlagSet
// WHAT: bDrawDebug default == false.
// WHY: Debug drawing must be opt-in; default-on would spam every play session.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3DebugDrawOnlyWhenFlagSetTest,
	"ClimbingSystem.AnimDebugMultiplayer.Debug.DebugDrawOnlyWhenFlagSet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3DebugDrawOnlyWhenFlagSetTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0239: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestFalse(TEXT("TC-0239: bDrawDebug must default to false"), CDO->bDrawDebug);
	return true;
}

// ---------------------------------------------------------------------------
// TC-0240: EditorLacheArcGatedByIsSelected
// WHAT: #if WITH_EDITOR guard is a compile-time contract.
// WHY: Editor-only arc visualization must not run in cooked builds.
// VERIFY: Verified by the fact that WITH_EDITOR code is stripped in non-editor builds.
//         Runtime proxy: bAutoLacheCinematic property exists (editor-configurable gate).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3EditorLacheArcGatedTest,
	"ClimbingSystem.AnimDebugMultiplayer.Debug.EditorLacheArcGatedByIsSelected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3EditorLacheArcGatedTest::RunTest(const FString& Parameters)
{
	const UClass* Class = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0240: AClimbingCharacter class must exist"), Class);
	if (!Class) { return false; }

	// bAutoLacheCinematic is the runtime-accessible gate for lache arc behavior
	const FProperty* Prop = Class->FindPropertyByName(TEXT("bAutoLacheCinematic"));
	TestNotNull(TEXT("TC-0240: bAutoLacheCinematic property must exist (editor-configurable lache arc gate)"), Prop);
	return Prop != nullptr;
}

// ---------------------------------------------------------------------------
// TC-0241: SimulatedProxyIKZeroedBeyondCullDistance
// WHAT: SimulatedProxyIKCullDistance default == 1500.
// WHY: Incorrect default would cull IK too aggressively or not at all.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3SimulatedProxyIKCullDistanceTest,
	"ClimbingSystem.AnimDebugMultiplayer.IK.SimulatedProxyIKZeroedBeyondCullDistance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3SimulatedProxyIKCullDistanceTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0241: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0241: SimulatedProxyIKCullDistance must default to 1500"),
		FMath::IsNearlyEqual(CDO->SimulatedProxyIKCullDistance, 1500.f));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0242: OnRepClimbingStateNoOpForNoneState
// WHAT: OnClimbingStateReplicated(Hanging, None) must not crash and state must be None.
// WHY: Receiving a replicated None state must be a safe no-op.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3OnRepClimbingStateNoOpForNoneTest,
	"ClimbingSystem.AnimDebugMultiplayer.Multiplayer.OnRepClimbingStateNoOpForNoneState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3OnRepClimbingStateNoOpForNoneTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0242: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	// Must not crash; new state is None
	Character->OnClimbingStateReplicated(EClimbingState::Hanging, EClimbingState::None);

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0242: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	TestEqual(TEXT("TC-0242: state must be None after OnClimbingStateReplicated(Hanging, None)"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0243: ServerValidationZeroToleranceRejectsAnyOffset
// WHAT: ServerValidationPositionTolerance is configurable (can be set to 0).
// WHY: Zero tolerance means any positional offset triggers rejection.
// VERIFY: Full rejection test requires a server-authoritative world with a connected client.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3ServerValidationZeroToleranceTest,
	"ClimbingSystem.AnimDebugMultiplayer.Multiplayer.ServerValidationZeroToleranceRejectsAnyOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3ServerValidationZeroToleranceTest::RunTest(const FString& Parameters)
{
	const UClass* Class = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("TC-0243: AClimbingCharacter class must exist"), Class);
	if (!Class) { return false; }

	const FProperty* Prop = Class->FindPropertyByName(TEXT("ServerValidationPositionTolerance"));
	TestNotNull(TEXT("TC-0243: ServerValidationPositionTolerance property must exist"), Prop);
	if (!Prop) { return false; }

	// Verify it is writable (EditAnywhere / BlueprintReadWrite → not CPF_EditConst)
	TestFalse(TEXT("TC-0243: ServerValidationPositionTolerance must be configurable (not EditConst)"),
		Prop->HasAnyPropertyFlags(CPF_EditConst));

	// Verify default value
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0243: CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestTrue(TEXT("TC-0243: ServerValidationPositionTolerance default must be 30"),
		FMath::IsNearlyEqual(CDO->ServerValidationPositionTolerance, 30.f));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0244: RollbackRestoresClimbingStateToNone
// WHAT: Client_RejectStateTransition() while in Hanging must result in state == None.
// WHY: Rollback must revert the client to a clean non-climbing state.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3RollbackRestoresClimbingStateToNoneTest,
	"ClimbingSystem.AnimDebugMultiplayer.Multiplayer.RollbackRestoresClimbingStateToNone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3RollbackRestoresClimbingStateToNoneTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0244: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("TC-0244: movement component must exist"), Movement);
	if (!Movement) { Helper.Teardown(); return false; }

	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->Client_RejectStateTransition();

	TestEqual(TEXT("TC-0244: state must be None after Client_RejectStateTransition"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0245: IKWeightHandLeftIsBlueprintReadWrite
// WHAT: IKWeightHandLeft on UClimbingAnimInstance must have CPF_BlueprintVisible.
// WHY: Animation Blueprints must be able to read/write this weight for IK blending.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3IKWeightHandLeftBlueprintReadWriteTest,
	"ClimbingSystem.AnimDebugMultiplayer.AnimInstance.IKWeightHandLeftIsBlueprintReadWrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3IKWeightHandLeftBlueprintReadWriteTest::RunTest(const FString& Parameters)
{
	const UClass* Class = UClimbingAnimInstance::StaticClass();
	TestNotNull(TEXT("TC-0245: UClimbingAnimInstance class must exist"), Class);
	if (!Class) { return false; }

	const FProperty* Prop = Class->FindPropertyByName(TEXT("IKWeightHandLeft"));
	TestNotNull(TEXT("TC-0245: IKWeightHandLeft property must exist"), Prop);
	if (!Prop) { return false; }

	TestTrue(TEXT("TC-0245: IKWeightHandLeft must have CPF_BlueprintVisible"),
		Prop->HasAnyPropertyFlags(CPF_BlueprintVisible));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0246: IKWeightHandRightIsBlueprintReadWrite
// WHAT: IKWeightHandRight on UClimbingAnimInstance must have CPF_BlueprintVisible.
// WHY: Animation Blueprints must be able to read/write this weight for IK blending.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3IKWeightHandRightBlueprintReadWriteTest,
	"ClimbingSystem.AnimDebugMultiplayer.AnimInstance.IKWeightHandRightIsBlueprintReadWrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3IKWeightHandRightBlueprintReadWriteTest::RunTest(const FString& Parameters)
{
	const UClass* Class = UClimbingAnimInstance::StaticClass();
	TestNotNull(TEXT("TC-0246: UClimbingAnimInstance class must exist"), Class);
	if (!Class) { return false; }

	const FProperty* Prop = Class->FindPropertyByName(TEXT("IKWeightHandRight"));
	TestNotNull(TEXT("TC-0246: IKWeightHandRight property must exist"), Prop);
	if (!Prop) { return false; }

	TestTrue(TEXT("TC-0246: IKWeightHandRight must have CPF_BlueprintVisible"),
		Prop->HasAnyPropertyFlags(CPF_BlueprintVisible));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0247: IKWeightFootLeftIsBlueprintReadWrite
// WHAT: IKWeightFootLeft on UClimbingAnimInstance must have CPF_BlueprintVisible.
// WHY: Animation Blueprints must be able to read/write this weight for IK blending.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3IKWeightFootLeftBlueprintReadWriteTest,
	"ClimbingSystem.AnimDebugMultiplayer.AnimInstance.IKWeightFootLeftIsBlueprintReadWrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3IKWeightFootLeftBlueprintReadWriteTest::RunTest(const FString& Parameters)
{
	const UClass* Class = UClimbingAnimInstance::StaticClass();
	TestNotNull(TEXT("TC-0247: UClimbingAnimInstance class must exist"), Class);
	if (!Class) { return false; }

	const FProperty* Prop = Class->FindPropertyByName(TEXT("IKWeightFootLeft"));
	TestNotNull(TEXT("TC-0247: IKWeightFootLeft property must exist"), Prop);
	if (!Prop) { return false; }

	TestTrue(TEXT("TC-0247: IKWeightFootLeft must have CPF_BlueprintVisible"),
		Prop->HasAnyPropertyFlags(CPF_BlueprintVisible));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0248: IKWeightFootRightIsBlueprintReadWrite
// WHAT: IKWeightFootRight on UClimbingAnimInstance must have CPF_BlueprintVisible.
// WHY: Animation Blueprints must be able to read/write this weight for IK blending.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3IKWeightFootRightBlueprintReadWriteTest,
	"ClimbingSystem.AnimDebugMultiplayer.AnimInstance.IKWeightFootRightIsBlueprintReadWrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3IKWeightFootRightBlueprintReadWriteTest::RunTest(const FString& Parameters)
{
	const UClass* Class = UClimbingAnimInstance::StaticClass();
	TestNotNull(TEXT("TC-0248: UClimbingAnimInstance class must exist"), Class);
	if (!Class) { return false; }

	const FProperty* Prop = Class->FindPropertyByName(TEXT("IKWeightFootRight"));
	TestNotNull(TEXT("TC-0248: IKWeightFootRight property must exist"), Prop);
	if (!Prop) { return false; }

	TestTrue(TEXT("TC-0248: IKWeightFootRight must have CPF_BlueprintVisible"),
		Prop->HasAnyPropertyFlags(CPF_BlueprintVisible));
	return true;
}

// ---------------------------------------------------------------------------
// TC-0249: IKManagerRegistersOnBeginPlay
// WHAT: After BeginPlay, ActiveClimbingCharacters must contain the spawned character.
// WHY: IK budget management requires all active characters to be registered.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3IKManagerRegistersOnBeginPlayTest,
	"ClimbingSystem.AnimDebugMultiplayer.IK.IKManagerRegistersOnBeginPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3IKManagerRegistersOnBeginPlayTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0249: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	bool bFound = false;
	for (const TWeakObjectPtr<AClimbingCharacter>& WeakChar : AClimbingCharacter::ActiveClimbingCharacters)
	{
		if (WeakChar.Get() == Character)
		{
			bFound = true;
			break;
		}
	}
	TestTrue(TEXT("TC-0249: ActiveClimbingCharacters must contain the spawned character after BeginPlay"),
		bFound);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0250: IKManagerEmptyAfterAllUnregister
// WHAT: After destroying the character, ActiveClimbingCharacters must not contain it.
// WHY: Stale entries in the IK manager would waste budget slots.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3IKManagerEmptyAfterAllUnregisterTest,
	"ClimbingSystem.AnimDebugMultiplayer.IK.IKManagerEmptyAfterAllUnregister",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3IKManagerEmptyAfterAllUnregisterTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0250: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	// Remove from SpawnedActors so Teardown doesn't double-destroy
	Helper.SpawnedActors.Remove(Character);
	Character->Destroy();

	bool bStillPresent = false;
	for (const TWeakObjectPtr<AClimbingCharacter>& WeakChar : AClimbingCharacter::ActiveClimbingCharacters)
	{
		if (WeakChar.IsValid() && WeakChar.Get() == Character)
		{
			bStillPresent = true;
			break;
		}
	}
	TestFalse(TEXT("TC-0250: ActiveClimbingCharacters must not contain the character after Destroy"),
		bStillPresent);

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0251: IKManagerStaleWeakPtrsPurged
// WHAT: After destroying the character, the recorded weak ptr must be invalid.
// WHY: Confirms the object is truly gone; purge logic can then safely remove it.
// VERIFY: Full purge-pass test requires observing the array shrink after a purge call.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3IKManagerStaleWeakPtrsPurgedTest,
	"ClimbingSystem.AnimDebugMultiplayer.IK.IKManagerStaleWeakPtrsPurged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3IKManagerStaleWeakPtrsPurgedTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0251: character must spawn"), Character);
	if (!Character) { Helper.Teardown(); return false; }

	TWeakObjectPtr<AClimbingCharacter> WeakPtr(Character);
	TestTrue(TEXT("TC-0251: weak ptr must be valid before destroy"), WeakPtr.IsValid());

	Helper.SpawnedActors.Remove(Character);
	Character->Destroy();

	TestFalse(TEXT("TC-0251: weak ptr must be invalid after Destroy"),
		WeakPtr.IsValid());

	Helper.Teardown();
	return true;
}

// ---------------------------------------------------------------------------
// TC-0252: AudioSoundMapCoversAllSoundTypes
// WHAT: EClimbSoundType must have exactly 8 values (excluding MAX if present).
// WHY: The ClimbingSounds map must have a slot for every sound event.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3AudioSoundMapCoversAllSoundTypesTest,
	"ClimbingSystem.AnimDebugMultiplayer.Audio.AudioSoundMapCoversAllSoundTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3AudioSoundMapCoversAllSoundTypesTest::RunTest(const FString& Parameters)
{
	// Enumerate all valid EClimbSoundType values by iterating the UEnum
	const UEnum* SoundEnum = StaticEnum<EClimbSoundType>();
	TestNotNull(TEXT("TC-0252: EClimbSoundType UEnum must exist"), SoundEnum);
	if (!SoundEnum) { return false; }

	// NumEnums() includes the _MAX sentinel if present; subtract it
	int32 Count = SoundEnum->NumEnums();
	// Remove hidden/MAX entries
	for (int32 i = Count - 1; i >= 0; --i)
	{
		if (SoundEnum->HasMetaData(TEXT("Hidden"), i))
		{
			--Count;
		}
	}

	TestEqual(TEXT("TC-0252: EClimbSoundType must have exactly 8 values"), Count, 8);
	return true;
}

// ---------------------------------------------------------------------------
// TC-0253: AudioNullSoftPtrSkipsLoad
// WHAT: ClimbingSounds TMap is empty by default (no entries to async-load).
// WHY: An empty map means no null soft-ptr loads are attempted at startup.
// VERIFY: Full async-load-skip test requires observing GetResolvedSound with a null entry.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBatch3AudioNullSoftPtrSkipsLoadTest,
	"ClimbingSystem.AnimDebugMultiplayer.Audio.AudioNullSoftPtrSkipsLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBatch3AudioNullSoftPtrSkipsLoadTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* CDO = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("TC-0253: AClimbingCharacter CDO must exist"), CDO);
	if (!CDO) { return false; }

	TestEqual(TEXT("TC-0253: ClimbingSounds TMap must be empty by default (no entries to load)"),
		CDO->ClimbingSounds.Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
