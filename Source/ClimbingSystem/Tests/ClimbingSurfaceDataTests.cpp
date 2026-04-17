// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Data/ClimbingSurfaceData.h"
#include "Misc/AutomationTest.h"

// WHAT: Validates UClimbingSurfaceData default property values match PROMPT.md spec
// WHY: Incorrect defaults silently break surface behavior for untagged surfaces
// EDGE CASES: Default construction only — no world required

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingSurfaceDataDefaultsTest,
	"ClimbingSystem.SurfaceData.Defaults.AllFieldsMatchSpec",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingSurfaceDataDefaultsTest::RunTest(const FString& Parameters)
{
	UClimbingSurfaceData* SurfaceData = NewObject<UClimbingSurfaceData>();
	TestNotNull(TEXT("SurfaceData should be created"), SurfaceData);
	if (!SurfaceData) { return false; }

	TestEqual(TEXT("SurfaceData: default SurfaceTier should be Climbable"),
		SurfaceData->SurfaceTier, EClimbSurfaceTier::Climbable);
	TestTrue(TEXT("SurfaceData: default ClimbSpeedMultiplier should be 1.0"),
		FMath::IsNearlyEqual(SurfaceData->ClimbSpeedMultiplier, 1.0f));
	TestTrue(TEXT("SurfaceData: default bAllowLache should be true"),
		SurfaceData->bAllowLache);
	TestTrue(TEXT("SurfaceData: default LadderRungSpacing should be 30.0"),
		FMath::IsNearlyEqual(SurfaceData->LadderRungSpacing, 30.0f));
	TestTrue(TEXT("SurfaceData: default SoundVolumeMultiplier should be 1.0"),
		FMath::IsNearlyEqual(SurfaceData->SoundVolumeMultiplier, 1.0f));
	TestTrue(TEXT("SurfaceData: default ApproachAngleTolerance should be 30.0"),
		FMath::IsNearlyEqual(SurfaceData->ApproachAngleTolerance, 30.0f));

	return true;
}

// WHAT: Validates GetPrimaryAssetId returns a valid asset identifier
// WHY: Asset manager relies on this for async loading of surface data
// EDGE CASES: Ensures type string is correct

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingSurfaceDataAssetIdTest,
	"ClimbingSystem.SurfaceData.AssetId.ReturnsValidId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingSurfaceDataAssetIdTest::RunTest(const FString& Parameters)
{
	UClimbingSurfaceData* SurfaceData = NewObject<UClimbingSurfaceData>();
	TestNotNull(TEXT("SurfaceData should be created"), SurfaceData);
	if (!SurfaceData) { return false; }

	FPrimaryAssetId AssetId = SurfaceData->GetPrimaryAssetId();
	TestTrue(TEXT("SurfaceData: PrimaryAssetId should be valid"),
		AssetId.IsValid());
	TestEqual(TEXT("SurfaceData: PrimaryAssetType should be ClimbingSurfaceData"),
		AssetId.PrimaryAssetType.GetName(), FName(TEXT("ClimbingSurfaceData")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
