// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingCharacter.h"
// Part of AClimbingCharacter — see ClimbingCharacter.h

void AClimbingCharacter::ValidateAnimationSlots()
{
	// Iterate every EClimbingAnimationSlot value and call GetMontageForSlot
	// For each null result, log warning
	for (uint8 SlotIndex = 0; SlotIndex < static_cast<uint8>(EClimbingAnimationSlot::MAX); ++SlotIndex)
	{
		const EClimbingAnimationSlot Slot = static_cast<EClimbingAnimationSlot>(SlotIndex);
		if (!GetMontageForSlot(Slot))
		{
			UE_LOG(LogClimbing, Warning, TEXT("ClimbingSystem: Slot '%s' unassigned on '%s'. The action requiring this animation will fail silently at runtime."),
				*UEnum::GetValueAsString(Slot), *GetName());
		}
	}
}
