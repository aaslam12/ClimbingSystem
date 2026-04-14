// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingSurfaceData.h"

UClimbingSurfaceData::UClimbingSurfaceData()
{
	// Default values set in header via UPROPERTY initializers
}

FPrimaryAssetId UClimbingSurfaceData::GetPrimaryAssetId() const
{
	// Use class name as asset type, object name as asset name
	return FPrimaryAssetId(TEXT("ClimbingSurfaceData"), GetFName());
}
