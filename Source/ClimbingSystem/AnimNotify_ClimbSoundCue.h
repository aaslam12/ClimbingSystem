// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ClimbingTypes.h"
#include "AnimNotify_ClimbSoundCue.generated.h"

class AClimbingCharacter;

/**
 * Animation notify that plays a climbing sound via the character's audio system.
 * Uses EClimbSoundType lookup to play sounds from the ClimbingSounds map.
 * 
 * Usage:
 * - Add to climbing montages at appropriate keyframes (hand contact, foot plant, etc.)
 * - Set SoundType to the desired EClimbSoundType value
 * - Sound assets are resolved via the character's GetResolvedSound() with caching
 * 
 * Audio is played via UGameplayStatics::SpawnSoundAtLocation (no GAS dependency).
 */
UCLASS()
class CLIMBINGSYSTEM_API UAnimNotify_ClimbSoundCue : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_ClimbSoundCue();

	// UAnimNotify interface
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
	// End UAnimNotify interface

	/** The type of climbing sound to play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Audio",
		meta = (ToolTip = "The climbing sound type to play. Maps to ClimbingSounds on the ClimbingCharacter."))
	EClimbSoundType SoundType;

	/** Optional bone name to use as sound location. If empty, uses character location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Audio",
		meta = (ToolTip = "Optional bone name to use as sound origin. Leave empty to use character location. For hand sounds, use 'hand_l' or 'hand_r'."))
	FName BoneName;

	/** Volume multiplier for this specific notify. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Audio",
		meta = (ToolTip = "Volume multiplier for this sound instance. 1.0 = normal volume.",
		ClampMin = "0.0", ClampMax = "2.0"))
	float VolumeMultiplier;

	/** Pitch multiplier for this specific notify. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Climbing|Audio",
		meta = (ToolTip = "Pitch multiplier for this sound instance. 1.0 = normal pitch.",
		ClampMin = "0.5", ClampMax = "2.0"))
	float PitchMultiplier;
};
