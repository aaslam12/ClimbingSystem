// Copyright Epic Games, Inc. All Rights Reserved.

#include "ClimbingCharacter.h"
// Part of AClimbingCharacter — see ClimbingCharacter.h
#include "Movement/ClimbingMovementComponent.h"
#include "Animation/ClimbingAnimInstance.h"
#include "Animation/AnimTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "TimerManager.h"

void AClimbingCharacter::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
	bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);

	// Only process grab break during active climbing states
	if (!ClimbingMovement || ClimbingMovement->CurrentClimbingState == EClimbingState::None ||
		ClimbingMovement->CurrentClimbingState == EClimbingState::Ragdoll)
	{
		return;
	}

	// Check if grab break is enabled
	if (FMath::IsNearlyZero(GrabBreakImpulseThreshold))
	{
		return;
	}

	// Calculate impulse magnitude (Newtons)
	const float ImpulseMagnitude = NormalImpulse.Size();

	// Check if impulse exceeds break threshold
	if (ImpulseMagnitude >= GrabBreakImpulseThreshold)
	{
		// Calculate launch velocity from impulse
		const FVector LaunchVelocity = NormalImpulse * GrabBreakLaunchScale;

		UE_LOG(LogClimbing, Log, TEXT("ClimbingSystem: Grab broken by impact (%.1f N >= %.1f N threshold). Launching with velocity: %s"),
			ImpulseMagnitude, GrabBreakImpulseThreshold, *LaunchVelocity.ToString());

		TriggerGrabBreak(LaunchVelocity);
	}
}

void AClimbingCharacter::TriggerGrabBreak(const FVector& LaunchVelocity)
{
	if (!ClimbingMovement)
	{
		return;
	}

	// Stop current montage immediately
	StopAnimMontage(nullptr);

	// Store launch velocity for application after transitioning to ragdoll
	const FVector StoredLaunchVelocity = LaunchVelocity;

	// Transition to ragdoll state
	FClimbingDetectionResult EmptyResult;
	TransitionToState(EClimbingState::Ragdoll, EmptyResult);

	// Apply launch velocity to mesh (physics is enabled in OnStateEnter for Ragdoll)
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		// Add impulse to pelvis bone for more natural ragdoll launch
		MeshComp->AddImpulse(StoredLaunchVelocity, PelvisBoneName, true);
	}
}

// ============================================================================
// Input Handlers
// ============================================================================

void AClimbingCharacter::RecoverFromRagdoll()
{
	if (!ClimbingMovement || ClimbingMovement->CurrentClimbingState != EClimbingState::Ragdoll)
	{
		return;
	}

	// Determine face up or face down using pelvis up-vector dot world up
	// Do NOT use pelvis Z rotation - ambiguous on skeletons with non-trivial reference pose
	bool bFaceUp = false;
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		const FQuat PelvisQuat = MeshComp->GetBoneQuaternion(PelvisBoneName, EBoneSpaces::WorldSpace);
		const FVector PelvisUp = PelvisQuat.GetUpVector();
		bFaceUp = FVector::DotProduct(PelvisUp, FVector::UpVector) > 0.0f;
	}

	// Disable physics
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		// Get pelvis location before disabling physics for positioning
		const FVector PelvisLocation = MeshComp->GetBoneLocation(PelvisBoneName);

		MeshComp->SetSimulatePhysics(false);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		// Position character at pelvis location
		SetActorLocation(PelvisLocation + FVector::UpVector * OriginalCapsuleHalfHeight);
	}

	// Select and play get-up animation
	const EClimbingAnimationSlot GetUpSlot = bFaceUp ?
		EClimbingAnimationSlot::RagdollGetUpFaceUp :
		EClimbingAnimationSlot::RagdollGetUpFaceDown;

	if (UAnimMontage* GetUpMontage = GetMontageForSlot(GetUpSlot))
	{
		PlayAnimMontage(GetUpMontage);
	}

	// Update animation instance
	if (UClimbingAnimInstance* AnimInstance = CachedAnimInstance.Get())
	{
		AnimInstance->bRagdollFaceUp = bFaceUp;
	}

	// Transition to None state
	FClimbingDetectionResult EmptyResult;
	TransitionToState(EClimbingState::None, EmptyResult);
}
