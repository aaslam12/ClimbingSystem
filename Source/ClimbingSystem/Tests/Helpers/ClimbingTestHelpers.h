// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/CollisionProfile.h"
#include "Character/ClimbingCharacter.h"
#include "Movement/ClimbingMovementComponent.h"

/**
 * Shared test infrastructure for climbing detection tests.
 * Provides world setup, actor spawning, and cleanup.
 */
struct FClimbingTestWorld
{
	UWorld* World = nullptr;
	TArray<AActor*> SpawnedActors;

	void Setup()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
		Ctx.SetCurrentWorld(World);
		FURL URL;
		World->InitializeActorsForPlay(URL);
		World->BeginPlay();
	}

	void Teardown()
	{
		for (AActor* A : SpawnedActors)
		{
			if (A && A->IsValidLowLevel()) { A->Destroy(); }
		}
		SpawnedActors.Empty();
		if (World)
		{
			World->BeginTearingDown();
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
			World = nullptr;
		}
	}

	AClimbingCharacter* SpawnCharacterAt(const FVector& Location)
	{
		if (!World) { return nullptr; }
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AClimbingCharacter* C = World->SpawnActor<AClimbingCharacter>(AClimbingCharacter::StaticClass(), FTransform(FRotator::ZeroRotator, Location), Params);
		if (C) { SpawnedActors.Add(C); }
		return C;
	}

	/** Spawns a static box with BlockAll collision. Returns the actor and box component. */
	AActor* SpawnBoxAt(const FVector& Location, const FVector& HalfExtents, const FName& Tag = NAME_None)
	{
		if (!World) { return nullptr; }
		AActor* A = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(FRotator::ZeroRotator, Location));
		if (!A) { return nullptr; }
		UBoxComponent* Box = NewObject<UBoxComponent>(A);
		Box->SetBoxExtent(HalfExtents);
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		if (!Tag.IsNone()) { Box->ComponentTags.Add(Tag); }
		A->SetRootComponent(Box);
		Box->RegisterComponent();
		A->SetActorLocation(Location);
		SpawnedActors.Add(A);
		return A;
	}

	/** Spawns a thin box rotated to form a slope at the given angle (degrees from horizontal). */
	AActor* SpawnSlopeAt(const FVector& Location, float AngleDegrees, const FVector& HalfExtents = FVector(50.f, 50.f, 5.f))
	{
		if (!World) { return nullptr; }
		FRotator Rot(AngleDegrees, 0.f, 0.f);
		AActor* A = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform(Rot, Location));
		if (!A) { return nullptr; }
		UBoxComponent* Box = NewObject<UBoxComponent>(A);
		Box->SetBoxExtent(HalfExtents);
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		A->SetRootComponent(Box);
		Box->RegisterComponent();
		SpawnedActors.Add(A);
		return A;
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
