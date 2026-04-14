// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingCharacterDefaultsTest,
	"ClimbingSystem.Character.Contracts.DefaultValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingCharacterDefaultsTest::RunTest(const FString& Parameters)
{
	const AClimbingCharacter* Defaults = GetDefault<AClimbingCharacter>();
	TestNotNull(TEXT("Character defaults should exist"), Defaults);
	if (!Defaults)
	{
		return false;
	}

	TestEqual(TEXT("Climbing IMC priority default"), Defaults->ClimbingIMCPriority, 1);
	TestTrue(TEXT("Detection scan interval default"), FMath::IsNearlyEqual(Defaults->DetectionScanInterval, 0.05f));
	TestTrue(TEXT("Falling grab scan interval default"), FMath::IsNearlyEqual(Defaults->FallingGrabCheckInterval, 0.05f));
	TestTrue(TEXT("Shimmy direction deadzone default"), FMath::IsNearlyEqual(Defaults->ShimmyDirectionDeadzone, 0.1f));
	TestTrue(TEXT("Default ladder rung spacing"), FMath::IsNearlyEqual(Defaults->DefaultLadderRungSpacing, 30.0f));
	TestTrue(TEXT("Climbing camera probe radius"), FMath::IsNearlyEqual(Defaults->ClimbingCameraProbeRadius, 12.0f));
	TestTrue(TEXT("Prediction rollback blendout"), FMath::IsNearlyEqual(Defaults->PredictionRollbackBlendOut, 0.2f));
	TestTrue(TEXT("Coyote time should be enabled by default"), Defaults->bEnableCoyoteTime);
	TestTrue(TEXT("Falling grab should be enabled by default"), Defaults->bEnableFallingGrab);
	TestEqual(TEXT("Pelvis bone default should be pelvis"), Defaults->PelvisBoneName, FName(TEXT("pelvis")));
	TestEqual(TEXT("Ragdoll camera socket default should be pelvis"), Defaults->RagdollCameraTargetSocket, FName(TEXT("pelvis")));
	TestEqual(TEXT("Montage slot default should be FullBody"), Defaults->ClimbingMontageSlot, FName(TEXT("FullBody")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingCharacterReplicationContractTest,
	"ClimbingSystem.Character.Contracts.ReplicationFlags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingCharacterReplicationContractTest::RunTest(const FString& Parameters)
{
	const UClass* CharacterClass = AClimbingCharacter::StaticClass();
	TestNotNull(TEXT("Character class should exist"), CharacterClass);
	if (!CharacterClass)
	{
		return false;
	}

	const FProperty* ShimmyDirProp = CharacterClass->FindPropertyByName(TEXT("CommittedShimmyDir"));
	const FProperty* CornerProp = CharacterClass->FindPropertyByName(TEXT("bCurrentCornerIsInside"));
	TestNotNull(TEXT("CommittedShimmyDir property should exist"), ShimmyDirProp);
	TestNotNull(TEXT("bCurrentCornerIsInside property should exist"), CornerProp);
	if (!ShimmyDirProp || !CornerProp)
	{
		return false;
	}

	TestTrue(TEXT("CommittedShimmyDir should be replicated"), ShimmyDirProp->HasAnyPropertyFlags(CPF_Net));
	TestTrue(TEXT("bCurrentCornerIsInside should be replicated"), CornerProp->HasAnyPropertyFlags(CPF_Net));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingCharacterClimbUpDetectionSelectionPrefersFreshTest,
	"ClimbingSystem.Character.Contracts.ClimbUpDetectionSelection.PrefersFresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingCharacterClimbUpDetectionSelectionPrefersFreshTest::RunTest(const FString& Parameters)
{
	FClimbingDetectionResult FreshDetection;
	FreshDetection.bValid = true;
	FreshDetection.LedgePosition = FVector(10.0f, 20.0f, 30.0f);
	FreshDetection.ClearanceType = EClimbClearanceType::Full;

	FClimbingDetectionResult CachedDetection;
	CachedDetection.bValid = true;
	CachedDetection.LedgePosition = FVector(100.0f, 0.0f, 0.0f);
	CachedDetection.ClearanceType = EClimbClearanceType::CrouchOnly;

	const FClimbingDetectionResult SelectedDetection = AClimbingCharacter::SelectClimbUpDetectionResult(
		FreshDetection, CachedDetection);

	TestTrue(TEXT("Selected detection should be valid"), SelectedDetection.bValid);
	TestEqual(TEXT("Fresh detection should be preferred"), SelectedDetection.LedgePosition, FreshDetection.LedgePosition);
	TestEqual(TEXT("Fresh clearance should be preserved"), SelectedDetection.ClearanceType, FreshDetection.ClearanceType);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingCharacterClimbUpDetectionSelectionFallsBackToCachedTest,
	"ClimbingSystem.Character.Contracts.ClimbUpDetectionSelection.FallsBackToCached",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingCharacterClimbUpDetectionSelectionFallsBackToCachedTest::RunTest(const FString& Parameters)
{
	FClimbingDetectionResult FreshDetection;
	FreshDetection.bValid = false;

	FClimbingDetectionResult CachedDetection;
	CachedDetection.bValid = true;
	CachedDetection.LedgePosition = FVector(-40.0f, 5.0f, 80.0f);
	CachedDetection.ClearanceType = EClimbClearanceType::CrouchOnly;

	const FClimbingDetectionResult SelectedDetection = AClimbingCharacter::SelectClimbUpDetectionResult(
		FreshDetection, CachedDetection);

	TestTrue(TEXT("Cached detection should be used when fresh detection is invalid"), SelectedDetection.bValid);
	TestEqual(TEXT("Fallback should keep cached ledge position"), SelectedDetection.LedgePosition, CachedDetection.LedgePosition);
	TestEqual(TEXT("Fallback should keep cached clearance"), SelectedDetection.ClearanceType, CachedDetection.ClearanceType);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
