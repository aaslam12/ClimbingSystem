// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNotify_ClimbSoundCue.h"
#include "ClimbingCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"

UAnimNotify_ClimbSoundCue::UAnimNotify_ClimbSoundCue()
	: SoundType(EClimbSoundType::HandGrab)
	, BoneName(NAME_None)
	, VolumeMultiplier(1.0f)
	, PitchMultiplier(1.0f)
{
}

void UAnimNotify_ClimbSoundCue::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	// Get the climbing character
	AClimbingCharacter* ClimbingChar = Cast<AClimbingCharacter>(MeshComp->GetOwner());
	if (!ClimbingChar)
	{
		return;
	}

	// Only play sounds for locally controlled characters
	if (!ClimbingChar->IsLocallyControlled())
	{
		return;
	}

	// Get the resolved sound from the character's cache
	USoundBase* Sound = ClimbingChar->GetResolvedSound(SoundType);
	if (!Sound)
	{
		// Sound not available or failed to load - already logged in GetResolvedSound
		return;
	}

	// Determine sound location
	FVector SoundLocation = ClimbingChar->GetActorLocation();
	if (!BoneName.IsNone() && MeshComp->DoesSocketExist(BoneName))
	{
		SoundLocation = MeshComp->GetSocketLocation(BoneName);
	}

	// Play the sound at location using UGameplayStatics (no GAS)
	UGameplayStatics::SpawnSoundAtLocation(
		ClimbingChar,
		Sound,
		SoundLocation,
		FRotator::ZeroRotator,
		VolumeMultiplier,
		PitchMultiplier,
		0.0f,  // Start time
		nullptr,  // Attenuation settings (use sound asset settings)
		nullptr,  // Concurrency settings
		true  // Auto destroy
	);
}

FString UAnimNotify_ClimbSoundCue::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("Climb Sound: %s"), *UEnum::GetValueAsString(SoundType));
}
