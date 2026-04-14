// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingCharacter.h"
// Part of AClimbingCharacter — see ClimbingCharacter.h
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

void AClimbingCharacter::PlayClimbingSound(EClimbSoundType SoundType)
{
	// Only play sounds on locally controlled characters
	if (!IsLocallyControlled())
	{
		return;
	}

	USoundBase* Sound = GetResolvedSound(SoundType);
	if (!Sound)
	{
		return;
	}

	// Play sound at character location using UGameplayStatics (no GAS)
	UGameplayStatics::SpawnSoundAtLocation(
		this,
		Sound,
		GetActorLocation(),
		FRotator::ZeroRotator,
		1.0f,  // Volume multiplier
		1.0f,  // Pitch multiplier
		0.0f,  // Start time
		nullptr,  // Attenuation settings (use sound asset settings)
		nullptr,  // Concurrency settings
		true  // Auto destroy
	);
}

USoundBase* AClimbingCharacter::GetResolvedSound(EClimbSoundType SoundType)
{
	// Check cache first
	if (TObjectPtr<USoundBase>* CachedSound = ResolvedSounds.Find(SoundType))
	{
		// Return cached sound (may be null if load failed)
		return CachedSound->Get();
	}

	// Look up soft pointer from map
	TSoftObjectPtr<USoundBase>* SoftPtr = ClimbingSounds.Find(SoundType);
	if (!SoftPtr || SoftPtr->IsNull())
	{
		// No entry for this sound type - log warning once and cache null
		UE_LOG(LogClimbing, Warning, TEXT("ClimbingSystem: Sound type '%s' not assigned in ClimbingSounds map on '%s'. Caching null - sound will not play."),
			*UEnum::GetValueAsString(SoundType), *GetName());
		ResolvedSounds.Add(SoundType, nullptr);
		return nullptr;
	}

	// Try to get already-loaded asset
	USoundBase* LoadedSound = SoftPtr->Get();
	if (LoadedSound)
	{
		ResolvedSounds.Add(SoundType, LoadedSound);
		return LoadedSound;
	}

	// Asset not loaded - attempt synchronous load (for immediate playback)
	// In production, consider async loading in BeginPlay for better performance
	LoadedSound = SoftPtr->LoadSynchronous();
	if (LoadedSound)
	{
		ResolvedSounds.Add(SoundType, LoadedSound);
		return LoadedSound;
	}

	// Load failed - log warning once and cache null to prevent repeated attempts
	UE_LOG(LogClimbing, Warning, TEXT("ClimbingSystem: Failed to load sound for type '%s' on '%s'. Path: %s. Caching null - sound will not play."),
		*UEnum::GetValueAsString(SoundType), *GetName(), *SoftPtr->ToString());
	ResolvedSounds.Add(SoundType, nullptr);
	return nullptr;
}
