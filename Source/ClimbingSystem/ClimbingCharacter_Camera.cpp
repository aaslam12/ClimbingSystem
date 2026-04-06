// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingCharacter.h"
// Part of AClimbingCharacter — see ClimbingCharacter.h
#include "ClimbingMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

void AClimbingCharacter::LockCameraToFrame(FVector Location, FRotator Rotation, float BlendTime)
{
	// Affects local player camera only - spectator systems out of scope
	if (!IsLocallyControlled())
	{
		return;
	}

	if (!CameraBoom || !FollowCamera)
	{
		return;
	}

	// Store original spring arm state if not already locked
	if (!bCameraLocked)
	{
		OriginalTargetArmLength = CameraBoom->TargetArmLength;
		OriginalSpringArmRotation = CameraBoom->GetRelativeRotation();
	}

	bCameraLocked = true;
	LockedCameraLocation = Location;
	LockedCameraRotation = Rotation;
	CameraLockBlendTime = BlendTime;
	CameraLockBlendTimeTotal = BlendTime;
	CameraReleaseBlendTime = 0.0f;

	// Disable camera lag during lock for precise control
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->bEnableCameraRotationLag = false;
}

void AClimbingCharacter::ReleaseCameraLock(float BlendTime)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	if (!bCameraLocked)
	{
		return;
	}

	CameraReleaseBlendTime = BlendTime;
	CameraReleaseBlendTimeTotal = BlendTime;

	// Note: bCameraLocked is cleared when blend completes in UpdateCameraLock
}

// ============================================================================
// Camera Helpers
// ============================================================================

void AClimbingCharacter::UpdateCameraNudge(float DeltaTime)
{
	// Only run on local player
	if (!IsLocallyControlled())
	{
		return;
	}

	// Don't nudge during camera lock
	if (bCameraLocked)
	{
		return;
	}

	// Only nudge during active climbing
	if (!ClimbingMovement || ClimbingMovement->CurrentClimbingState == EClimbingState::None)
	{
		return;
	}

	// Only nudge if we have valid detection result
	if (!CurrentDetectionResult.bValid)
	{
		return;
	}

	// Check if nudge strength is enabled
	if (FMath::IsNearlyZero(CameraNudgeStrength))
	{
		return;
	}

	if (!CameraBoom || !GetController())
	{
		return;
	}

	// Get current camera facing direction
	const FRotator ControlRotation = GetControlRotation();
	const FVector CameraForward = FRotationMatrix(ControlRotation).GetUnitAxis(EAxis::X);

	// Calculate ideal facing direction (toward wall = negative surface normal)
	const FVector IdealForward = -CurrentDetectionResult.SurfaceNormal;

	// Calculate angle between camera forward and ideal forward
	const float DotProduct = FVector::DotProduct(CameraForward, IdealForward);
	const float AngleFromWall = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(DotProduct, -1.0f, 1.0f)));

	// Only nudge if angle exceeds activation threshold
	if (AngleFromWall < CameraNudgeActivationAngle)
	{
		return;
	}

	// Calculate rotation nudge
	const FRotator IdealRotation = IdealForward.Rotation();
	const FRotator CurrentRotation = ControlRotation;

	// Interpolate toward ideal rotation
	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, IdealRotation, DeltaTime, CameraNudgeBlendSpeed * CameraNudgeStrength);

	// Apply rotation to controller (only yaw for horizontal nudge)
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// Only adjust yaw, preserve pitch
		FRotator AdjustedRotation = CurrentRotation;
		AdjustedRotation.Yaw = NewRotation.Yaw;
		PC->SetControlRotation(AdjustedRotation);
	}
}

void AClimbingCharacter::UpdateCameraLock(float DeltaTime)
{
	// Only run on local player
	if (!IsLocallyControlled())
	{
		return;
	}

	if (!CameraBoom || !FollowCamera)
	{
		return;
	}

	// Handle camera release blend
	if (CameraReleaseBlendTime > 0.0f)
	{
		CameraReleaseBlendTime -= DeltaTime;

		if (CameraReleaseBlendTime <= 0.0f)
		{
			// Blend complete - fully release camera
			bCameraLocked = false;
			CameraReleaseBlendTime = 0.0f;

			// Restore original spring arm settings
			CameraBoom->TargetArmLength = OriginalTargetArmLength;
			CameraBoom->bEnableCameraLag = true;
			CameraBoom->bEnableCameraRotationLag = true;
		}
		else if (CameraReleaseBlendTimeTotal > 0.0f)
		{
			// Interpolate back to normal following
			const float Alpha = 1.0f - (CameraReleaseBlendTime / CameraReleaseBlendTimeTotal);
			CameraBoom->TargetArmLength = FMath::Lerp(0.0f, OriginalTargetArmLength, Alpha);
		}
		return;
	}

	// Handle camera lock blend
	if (!bCameraLocked)
	{
		return;
	}

	if (CameraLockBlendTime > 0.0f)
	{
		CameraLockBlendTime -= DeltaTime;

		if (CameraLockBlendTimeTotal > 0.0f)
		{
			const float Alpha = 1.0f - (CameraLockBlendTime / CameraLockBlendTimeTotal);

			// Interpolate spring arm to create locked camera effect
			// Reduce arm length to zero and control position directly
			CameraBoom->TargetArmLength = FMath::Lerp(OriginalTargetArmLength, 0.0f, Alpha);

			// Set camera world position/rotation via spring arm control rotation
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				const FRotator CurrentRotation = PC->GetControlRotation();
				const FRotator InterpolatedRotation = FMath::RInterpConstantTo(CurrentRotation, LockedCameraRotation, DeltaTime, 180.0f);
				PC->SetControlRotation(InterpolatedRotation);
			}
		}
	}
	else
	{
		// Lock complete - maintain locked position
		CameraBoom->TargetArmLength = 0.0f;

		// Keep camera at locked rotation
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PC->SetControlRotation(LockedCameraRotation);
		}
	}
}
