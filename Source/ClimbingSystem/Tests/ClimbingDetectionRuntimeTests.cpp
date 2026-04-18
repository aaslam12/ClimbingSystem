// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "Helpers/SharedTestHelpers.h"
#include "Misc/AutomationTest.h"

namespace
{
struct FSpawnedBox
{
	AActor* Actor = nullptr;
	UBoxComponent* Box = nullptr;
};

static FSpawnedBox SpawnBlockingBox(
	UWorld* World,
	const FVector& Location,
	const FVector& Extent,
	const TArray<FName>& Tags = TArray<FName>())
{
	FSpawnedBox Spawned;
	if (!World)
	{
		return Spawned;
	}

	Spawned.Actor = World->SpawnActor<AActor>(AActor::StaticClass(), Location, FRotator::ZeroRotator);
	if (!Spawned.Actor)
	{
		return Spawned;
	}

	Spawned.Box = NewObject<UBoxComponent>(Spawned.Actor);
	if (!Spawned.Box)
	{
		Spawned.Actor->Destroy();
		Spawned.Actor = nullptr;
		return Spawned;
	}

	Spawned.Box->SetBoxExtent(Extent);
	Spawned.Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Spawned.Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	for (const FName& Tag : Tags)
	{
		Spawned.Box->ComponentTags.Add(Tag);
	}

	Spawned.Actor->SetRootComponent(Spawned.Box);
	Spawned.Actor->AddInstanceComponent(Spawned.Box);
	Spawned.Box->RegisterComponent();
	Spawned.Actor->SetActorLocation(Location);

	return Spawned;
}
}

// WHAT: Verifies PerformLadderDetection succeeds only for LadderOnly-tagged surfaces in front of the character.
// WHY: Ladder entry is a critical gameplay path and should not regress to generic ledge behavior.
// EDGE CASES: Real world trace with a dynamically spawned collision component.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingDetectionRuntimeLadderOnlyTagTest,
	"ClimbingSystem.Detection.Runtime.LadderOnlyTagDetected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingDetectionRuntimeLadderOnlyTagTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>(AClimbingCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	TestNotNull(TEXT("Detection runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	const FSpawnedBox LadderWall = SpawnBlockingBox(
		Helper.World,
		FVector(80.0f, 0.0f, 90.0f),
		FVector(12.0f, 120.0f, 120.0f),
		{ FName("LadderOnly") });
	TestNotNull(TEXT("Detection runtime: ladder wall actor should spawn"), LadderWall.Actor);
	TestNotNull(TEXT("Detection runtime: ladder wall component should spawn"), LadderWall.Box);
	if (!LadderWall.Box)
	{
		if (Character) { Character->Destroy(); }
		if (LadderWall.Actor) { LadderWall.Actor->Destroy(); }
		Helper.Teardown();
		return false;
	}

	const FClimbingDetectionResult Detection = Character->PerformLadderDetection();
	TestTrue(TEXT("Detection runtime: LadderOnly-tagged wall should produce a valid ladder detection result"), Detection.bValid);
	TestEqual(TEXT("Detection runtime: ladder detection should classify surface tier as LadderOnly"),
		Detection.SurfaceTier, EClimbSurfaceTier::LadderOnly);
	TestTrue(TEXT("Detection runtime: detected component should match ladder component"),
		Detection.HitComponent.Get() == LadderWall.Box);

	LadderWall.Actor->Destroy();
	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies PerformLadderDetection rejects surfaces without LadderOnly tag.
// WHY: Prevents non-ladder walls from entering ladder state.
// EDGE CASES: Trace hit exists but tag gate should block detection.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingDetectionRuntimeLadderRejectsNonTaggedWallTest,
	"ClimbingSystem.Detection.Runtime.LadderRejectsNonLadderSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingDetectionRuntimeLadderRejectsNonTaggedWallTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>(AClimbingCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	TestNotNull(TEXT("Detection runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	const FSpawnedBox PlainWall = SpawnBlockingBox(
		Helper.World,
		FVector(80.0f, 0.0f, 90.0f),
		FVector(12.0f, 120.0f, 120.0f));
	TestNotNull(TEXT("Detection runtime: plain wall actor should spawn"), PlainWall.Actor);
	TestNotNull(TEXT("Detection runtime: plain wall component should spawn"), PlainWall.Box);
	if (!PlainWall.Box)
	{
		if (Character) { Character->Destroy(); }
		if (PlainWall.Actor) { PlainWall.Actor->Destroy(); }
		Helper.Teardown();
		return false;
	}

	const FClimbingDetectionResult Detection = Character->PerformLadderDetection();
	TestFalse(TEXT("Detection runtime: non-ladder wall should not produce a valid ladder detection result"), Detection.bValid);

	PlainWall.Actor->Destroy();
	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies PerformLedgeDetection rejects Unclimbable-tagged walls even when trace hits.
// WHY: Unclimbable tags are the primary data-driven safety gate for level design.
// EDGE CASES: Forward hit succeeds but should early-return invalid.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingDetectionRuntimeUnclimbableTagTest,
	"ClimbingSystem.Detection.Runtime.LedgeRejectsUnclimbableTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingDetectionRuntimeUnclimbableTagTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>(AClimbingCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	TestNotNull(TEXT("Detection runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	const FSpawnedBox UnclimbableWall = SpawnBlockingBox(
		Helper.World,
		FVector(95.0f, 0.0f, 90.0f),
		FVector(15.0f, 120.0f, 120.0f),
		{ FName("Unclimbable") });
	TestNotNull(TEXT("Detection runtime: unclimbable wall actor should spawn"), UnclimbableWall.Actor);
	TestNotNull(TEXT("Detection runtime: unclimbable wall component should spawn"), UnclimbableWall.Box);
	if (!UnclimbableWall.Box)
	{
		if (Character) { Character->Destroy(); }
		if (UnclimbableWall.Actor) { UnclimbableWall.Actor->Destroy(); }
		Helper.Teardown();
		return false;
	}

	const FClimbingDetectionResult Detection = Character->PerformLedgeDetection();
	TestFalse(TEXT("Detection runtime: Unclimbable-tagged wall should always produce invalid ledge detection"), Detection.bValid);

	UnclimbableWall.Actor->Destroy();
	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies PerformBracedWallDetection rejects LadderOnly surfaces.
// WHY: Ladder-only geometry should never enter braced-wall traversal state.
// EDGE CASES: Trace hit exists and is vertical, but tag gate must reject.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingDetectionRuntimeBracedRejectsLadderTest,
	"ClimbingSystem.Detection.Runtime.BracedRejectsLadderOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingDetectionRuntimeBracedRejectsLadderTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>(AClimbingCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	TestNotNull(TEXT("Detection runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	const FSpawnedBox LadderWall = SpawnBlockingBox(
		Helper.World,
		FVector(80.0f, 0.0f, 90.0f),
		FVector(12.0f, 120.0f, 120.0f),
		{ FName("LadderOnly") });
	TestNotNull(TEXT("Detection runtime: ladder wall actor should spawn"), LadderWall.Actor);
	TestNotNull(TEXT("Detection runtime: ladder wall component should spawn"), LadderWall.Box);
	if (!LadderWall.Box)
	{
		if (Character) { Character->Destroy(); }
		if (LadderWall.Actor) { LadderWall.Actor->Destroy(); }
		Helper.Teardown();
		return false;
	}

	const FClimbingDetectionResult Detection = Character->PerformBracedWallDetection();
	TestFalse(TEXT("Detection runtime: LadderOnly-tagged wall should be rejected by braced wall detection"), Detection.bValid);

	LadderWall.Actor->Destroy();
	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies ResolveHitComponentFromNet resolves a blocking component via confirmation trace.
// WHY: Simulated proxy IK and replicated climbing rely on component resolution from net-safe payloads.
// EDGE CASES: Real object-type sweep against world geometry in test world.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMultiplayerRuntimeConfirmationTraceResolvesComponentTest,
	"ClimbingSystem.Multiplayer.ConfirmationTrace.ResolvesHitComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMultiplayerRuntimeConfirmationTraceResolvesComponentTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>(AClimbingCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	TestNotNull(TEXT("Multiplayer runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	const FSpawnedBox ClimbWall = SpawnBlockingBox(
		Helper.World,
		FVector(100.0f, 0.0f, 90.0f),
		FVector(12.0f, 120.0f, 120.0f),
		{ FName("Climbable") });
	TestNotNull(TEXT("Multiplayer runtime: climb wall actor should spawn"), ClimbWall.Actor);
	TestNotNull(TEXT("Multiplayer runtime: climb wall component should spawn"), ClimbWall.Box);
	if (!ClimbWall.Box)
	{
		if (Character) { Character->Destroy(); }
		if (ClimbWall.Actor) { ClimbWall.Actor->Destroy(); }
		Helper.Teardown();
		return false;
	}

	FClimbingDetectionResultNet NetResult;
	NetResult.bValid = true;
	NetResult.LedgePosition = FVector(88.0f, 0.0f, 90.0f);
	NetResult.SurfaceNormal = FVector(-1.0f, 0.0f, 0.0f);
	NetResult.SurfaceTier = EClimbSurfaceTier::Climbable;
	NetResult.ClearanceType = EClimbClearanceType::Full;

	UPrimitiveComponent* Resolved = Character->ResolveHitComponentFromNet(NetResult);
	TestNotNull(TEXT("Multiplayer runtime: confirmation trace should resolve the hit component"), Resolved);
	TestTrue(TEXT("Multiplayer runtime: resolved component should match spawned climb wall"),
		Resolved == ClimbWall.Box);

	ClimbWall.Actor->Destroy();
	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies ResolveHitComponentFromNet returns null for invalid net payloads.
// WHY: Guards against false-positive IK anchors on malformed replicated state.
// EDGE CASES: bValid=false short-circuit path.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingMultiplayerRuntimeConfirmationTraceInvalidResultTest,
	"ClimbingSystem.Multiplayer.ConfirmationTrace.InvalidPayloadReturnsNull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingMultiplayerRuntimeConfirmationTraceInvalidResultTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>(AClimbingCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	TestNotNull(TEXT("Multiplayer runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	FClimbingDetectionResultNet InvalidNetResult;
	InvalidNetResult.bValid = false;
	InvalidNetResult.LedgePosition = FVector(100.0f, 0.0f, 100.0f);
	InvalidNetResult.SurfaceNormal = FVector(-1.0f, 0.0f, 0.0f);

	UPrimitiveComponent* Resolved = Character->ResolveHitComponentFromNet(InvalidNetResult);
	TestNull(TEXT("Multiplayer runtime: invalid net payload should return null component"), Resolved);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
