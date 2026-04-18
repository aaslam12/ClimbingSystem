// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

// Test-only access for Input_Grab and cached detection state.
#define protected public
#include "Character/ClimbingCharacter.h"
#undef protected

#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "Animation/AnimMontage.h"
#include "InputActionValue.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Helpers/SharedTestHelpers.h"
#include "Misc/AutomationTest.h"

namespace
{
struct FSpawnedObstacle
{
	AActor* Actor = nullptr;
	UBoxComponent* Box = nullptr;
};

static FSpawnedObstacle SpawnObstacle(
	UWorld* World,
	const FVector& Location,
	const FVector& Extent,
	const TArray<FName>& Tags = TArray<FName>())
{
	FSpawnedObstacle Result;
	if (!World)
	{
		return Result;
	}

	Result.Actor = World->SpawnActor<AActor>(AActor::StaticClass(), Location, FRotator::ZeroRotator);
	if (!Result.Actor)
	{
		return Result;
	}

	Result.Box = NewObject<UBoxComponent>(Result.Actor);
	if (!Result.Box)
	{
		Result.Actor->Destroy();
		Result.Actor = nullptr;
		return Result;
	}

	Result.Box->SetBoxExtent(Extent);
	Result.Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Result.Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	for (const FName& Tag : Tags)
	{
		Result.Box->ComponentTags.Add(Tag);
	}

	Result.Actor->SetRootComponent(Result.Box);
	Result.Actor->AddInstanceComponent(Result.Box);
	Result.Box->RegisterComponent();
	Result.Actor->SetActorLocation(Location);

	return Result;
}
}

// WHAT: Verifies Input_Grab transitions to Mantling when a climbable obstacle is in mantle height range.
// WHY: Mantle fallback is a key recovery path when normal ledge detection misses.
// EDGE CASES: Runtime obstacle with Climbable tag and valid mantle height.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeMantleFallbackValidRangeTest,
	"ClimbingSystem.Character.Runtime.MantleDetectionFallback.InValidRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeMantleFallbackValidRangeTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>(AClimbingCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	TestNotNull(TEXT("Mantle runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Mantle runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	// Top of this obstacle is z=60; with default capsule this falls in mantle range.
	const FSpawnedObstacle Obstacle = SpawnObstacle(
		Helper.World,
		FVector(70.0f, 0.0f, 0.0f),
		FVector(20.0f, 120.0f, 60.0f),
		{ FName("Climbable") });
	TestNotNull(TEXT("Mantle runtime: obstacle actor should spawn"), Obstacle.Actor);
	TestNotNull(TEXT("Mantle runtime: obstacle component should spawn"), Obstacle.Box);
	if (!Obstacle.Box)
	{
		if (Obstacle.Actor) { Obstacle.Actor->Destroy(); }
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Character->CurrentDetectionResult.Reset();
	Character->MantleLow = NewObject<UAnimMontage>(Character);
	const FInputActionValue Pressed(true);
	Character->Input_Grab(Pressed);

	TestEqual(TEXT("Mantle runtime: valid climbable obstacle in mantle range should transition to Mantling"),
		Movement->CurrentClimbingState, EClimbingState::Mantling);

	Obstacle.Actor->Destroy();
	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies Input_Grab does not mantle an explicitly Unclimbable obstacle.
// WHY: Level-design safety tags must reliably block mantle/grab transitions.
// EDGE CASES: Obstacle hit exists but tag gate rejects traversal.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeMantleFallbackRejectsUnclimbableTest,
	"ClimbingSystem.Character.Runtime.MantleDetectionFallback.RejectsUnclimbable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeMantleFallbackRejectsUnclimbableTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>(AClimbingCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	TestNotNull(TEXT("Mantle runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Mantle runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	const FSpawnedObstacle Obstacle = SpawnObstacle(
		Helper.World,
		FVector(70.0f, 0.0f, 0.0f),
		FVector(20.0f, 120.0f, 60.0f),
		{ FName("Unclimbable") });
	TestNotNull(TEXT("Mantle runtime: obstacle actor should spawn"), Obstacle.Actor);
	TestNotNull(TEXT("Mantle runtime: obstacle component should spawn"), Obstacle.Box);
	if (!Obstacle.Box)
	{
		if (Obstacle.Actor) { Obstacle.Actor->Destroy(); }
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Character->CurrentDetectionResult.Reset();
	const FInputActionValue Pressed(true);
	Character->Input_Grab(Pressed);

	TestEqual(TEXT("Mantle runtime: Unclimbable obstacle should not change state from None"),
		Movement->CurrentClimbingState, EClimbingState::None);

	Obstacle.Actor->Destroy();
	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies Input_Grab does not enter Mantling when obstacle height exceeds mantle max.
// WHY: High ledges should not use mantle animation path.
// EDGE CASES: Over-height climbable obstacle should avoid Mantling.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeMantleFallbackTooHighTest,
	"ClimbingSystem.Character.Runtime.MantleDetectionFallback.TooHighNotMantling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeMantleFallbackTooHighTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>(AClimbingCharacter::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	TestNotNull(TEXT("Mantle runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Mantle runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	// Top at z=240 puts obstacle above MantleHighMaxHeight for default character feet.
	const FSpawnedObstacle Obstacle = SpawnObstacle(
		Helper.World,
		FVector(70.0f, 0.0f, 120.0f),
		FVector(20.0f, 120.0f, 120.0f),
		{ FName("Climbable") });
	TestNotNull(TEXT("Mantle runtime: obstacle actor should spawn"), Obstacle.Actor);
	TestNotNull(TEXT("Mantle runtime: obstacle component should spawn"), Obstacle.Box);
	if (!Obstacle.Box)
	{
		if (Obstacle.Actor) { Obstacle.Actor->Destroy(); }
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Character->CurrentDetectionResult.Reset();
	const FInputActionValue Pressed(true);
	Character->Input_Grab(Pressed);

	TestTrue(TEXT("Mantle runtime: over-height obstacle should not transition to Mantling"),
		Movement->CurrentClimbingState != EClimbingState::Mantling);

	Obstacle.Actor->Destroy();
	Character->Destroy();
	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
