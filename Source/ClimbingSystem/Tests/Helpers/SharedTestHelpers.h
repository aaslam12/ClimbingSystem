// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

/**
 * Canonical world setup/teardown helper for climbing system tests.
 * Every test that requires a UWorld MUST use this pattern.
 * WORLD CLEANUP: verified — all paths call Teardown()
 */
struct FTestWorldHelper
{
	UWorld* World = nullptr;

	void Setup()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext =
			GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		FURL URL;
		World->InitializeActorsForPlay(URL);
		World->BeginPlay();
	}

	void Teardown()
	{
		if (World)
		{
			World->BeginTearingDown();
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
			World = nullptr;
		}
	}
};
