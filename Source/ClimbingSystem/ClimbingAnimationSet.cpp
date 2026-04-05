// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingAnimationSet.h"

UClimbingAnimationSet::UClimbingAnimationSet()
{
	// All montage pointers default to nullptr via TObjectPtr
}

UAnimMontage* UClimbingAnimationSet::GetMontageForSlot(EClimbingAnimationSlot Slot) const
{
	switch (Slot)
	{
	// Ledge animations
	case EClimbingAnimationSlot::HangIdle:
		return HangIdle;
	case EClimbingAnimationSlot::HangIdleLeft:
		return HangIdleLeft;
	case EClimbingAnimationSlot::HangIdleRight:
		return HangIdleRight;
	case EClimbingAnimationSlot::ShimmyLeft:
		return ShimmyLeft;
	case EClimbingAnimationSlot::ShimmyRight:
		return ShimmyRight;
	case EClimbingAnimationSlot::CornerInsideLeft:
		return CornerInsideLeft;
	case EClimbingAnimationSlot::CornerInsideRight:
		return CornerInsideRight;
	case EClimbingAnimationSlot::CornerOutsideLeft:
		return CornerOutsideLeft;
	case EClimbingAnimationSlot::CornerOutsideRight:
		return CornerOutsideRight;
	case EClimbingAnimationSlot::ClimbUp:
		return ClimbUp;
	case EClimbingAnimationSlot::ClimbUpCrouch:
		return ClimbUpCrouch;
	case EClimbingAnimationSlot::DropDown:
		return DropDown;
	case EClimbingAnimationSlot::GrabLedge:
		return GrabLedge;
	case EClimbingAnimationSlot::GrabFail:
		return GrabFail;
	case EClimbingAnimationSlot::ShimmyReposition:
		return ShimmyReposition;

	// Lache animations
	case EClimbingAnimationSlot::LacheLaunch:
		return LacheLaunch;
	case EClimbingAnimationSlot::LacheFlight:
		return LacheFlight;
	case EClimbingAnimationSlot::LacheCatch:
		return LacheCatch;
	case EClimbingAnimationSlot::LacheMiss:
		return LacheMiss;

	// Mantle animations
	case EClimbingAnimationSlot::MantleLow:
		return MantleLow;
	case EClimbingAnimationSlot::MantleHigh:
		return MantleHigh;

	// Ragdoll animations
	case EClimbingAnimationSlot::RagdollGetUpFaceDown:
		return RagdollGetUpFaceDown;
	case EClimbingAnimationSlot::RagdollGetUpFaceUp:
		return RagdollGetUpFaceUp;

	// Braced wall animations
	case EClimbingAnimationSlot::BracedIdle:
		return BracedIdle;
	case EClimbingAnimationSlot::BracedShimmyLeft:
		return BracedShimmyLeft;
	case EClimbingAnimationSlot::BracedShimmyRight:
		return BracedShimmyRight;
	case EClimbingAnimationSlot::BracedToHang:
		return BracedToHang;

	// Ladder animations
	case EClimbingAnimationSlot::LadderIdle:
		return LadderIdle;
	case EClimbingAnimationSlot::LadderClimbUp:
		return LadderClimbUp;
	case EClimbingAnimationSlot::LadderClimbDown:
		return LadderClimbDown;
	case EClimbingAnimationSlot::LadderFastAscend:
		return LadderFastAscend;
	case EClimbingAnimationSlot::LadderFastDescend:
		return LadderFastDescend;
	case EClimbingAnimationSlot::LadderEnterBottom:
		return LadderEnterBottom;
	case EClimbingAnimationSlot::LadderEnterTop:
		return LadderEnterTop;
	case EClimbingAnimationSlot::LadderExitBottom:
		return LadderExitBottom;
	case EClimbingAnimationSlot::LadderExitTop:
		return LadderExitTop;
	case EClimbingAnimationSlot::LadderExitSide:
		return LadderExitSide;

	default:
		return nullptr;
	}
}

FPrimaryAssetId UClimbingAnimationSet::GetPrimaryAssetId() const
{
	// Use class name as asset type, object name as asset name
	return FPrimaryAssetId(TEXT("ClimbingAnimationSet"), GetFName());
}
