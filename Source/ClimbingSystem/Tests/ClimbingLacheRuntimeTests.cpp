// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

// Test-only access for Input_Lache, TickLacheInAirState and Lache internals.
#define protected public
#include "Character/ClimbingCharacter.h"
#undef protected

#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "InputActionValue.h"
#include "Movement/ClimbingMovementComponent.h"
#include "Helpers/SharedTestHelpers.h"
#include "Misc/AutomationTest.h"

namespace
{
struct FSpawnedLacheTarget
{
	AActor* Actor = nullptr;
	UBoxComponent* Box = nullptr;
};

static FSpawnedLacheTarget SpawnClimbableLacheTarget(UWorld* World)
{
	FSpawnedLacheTarget Result;
	if (!World)
	{
		return Result;
	}

	Result.Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector(140.0f, 0.0f, 20.0f), FRotator::ZeroRotator);
	if (!Result.Actor)
	{
		return Result;
	}

	Result.Box = NewObject<UBoxComponent>(Result.Actor);
	if (!Result.Box)
	{
		Result.Actor->Destroy();
		Result.Actor = nullptr;
		return Result;
	}

	Result.Box->SetBoxExtent(FVector(40.0f, 140.0f, 80.0f));
	Result.Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Result.Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Result.Box->ComponentTags.Add(FName("Climbable"));
	Result.Actor->SetRootComponent(Result.Box);
	Result.Actor->AddInstanceComponent(Result.Box);
	Result.Box->RegisterComponent();
	Result.Actor->SetActorLocation(FVector(140.0f, 0.0f, 20.0f));

	return Result;
}
}

// WHAT: Verifies Input_Lache does not transition when no valid target is found.
// WHY: Invalid lache attempts should fail safely without state corruption.
// EDGE CASES: Empty world with no climbable intercepts.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeLacheNoTargetStaysHangingTest,
	"ClimbingSystem.Actions.Lache.Runtime.NoTargetStaysHanging",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeLacheNoTargetStaysHangingTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Lache runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Lache runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->LockedLacheTarget.Reset();
	Character->Input_Lache(FInputActionValue(true));

	TestEqual(TEXT("Lache runtime: no-target lache attempt should keep state in Hanging"),
		Movement->CurrentClimbingState, EClimbingState::Hanging);
	TestFalse(TEXT("Lache runtime: no-target lache attempt should keep LockedLacheTarget invalid"),
		Character->LockedLacheTarget.bValid);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies Input_Lache transitions to Lache when a climbable runtime target is intercepted.
// WHY: Core launch path requires valid arc target acquisition.
// EDGE CASES: Real-world sweep + ledge detection at impact location.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeLacheValidTargetTransitionsToLacheTest,
	"ClimbingSystem.Actions.Lache.Runtime.ValidTargetTransitionsToLache",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeLacheValidTargetTransitionsToLacheTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Lache runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Lache runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	const FSpawnedLacheTarget Target = SpawnClimbableLacheTarget(Helper.World);
	TestNotNull(TEXT("Lache runtime: climbable lache target actor should spawn"), Target.Actor);
	TestNotNull(TEXT("Lache runtime: climbable lache target component should spawn"), Target.Box);
	if (!Target.Box)
	{
		if (Target.Actor) { Target.Actor->Destroy(); }
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->Input_Lache(FInputActionValue(true));

	TestEqual(TEXT("Lache runtime: valid arc target should transition to Lache"),
		Movement->CurrentClimbingState, EClimbingState::Lache);
	TestTrue(TEXT("Lache runtime: valid arc target should lock a valid lache target"),
		Character->LockedLacheTarget.bValid);

	Target.Actor->Destroy();
	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies Input_Lache applies rightward blend to launch direction when horizontal input is positive.
// WHY: Directional lache is a primary gameplay requirement for lateral jumps.
// EDGE CASES: Blend path only executes when horizontal climb input is non-zero.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeLacheDirectionalInputBlendsLaunchDirectionTest,
	"ClimbingSystem.Actions.Lache.Runtime.DirectionalInputBlendsLaunchDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeLacheDirectionalInputBlendsLaunchDirectionTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Lache runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Lache runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	const FSpawnedLacheTarget Target = SpawnClimbableLacheTarget(Helper.World);
	TestNotNull(TEXT("Lache runtime: climbable lache target actor should spawn"), Target.Actor);
	TestNotNull(TEXT("Lache runtime: climbable lache target component should spawn"), Target.Box);
	if (!Target.Box)
	{
		if (Target.Actor) { Target.Actor->Destroy(); }
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::Hanging);
	Character->Input_ClimbMove(FInputActionValue(FVector2D(1.0f, 0.0f)));
	Character->Input_Lache(FInputActionValue(true));

	TestEqual(TEXT("Lache runtime: successful lache attempt should enter Lache state"),
		Movement->CurrentClimbingState, EClimbingState::Lache);
	TestTrue(TEXT("Lache runtime: positive horizontal input should introduce a positive rightward launch component"),
		Character->LacheLaunchDirection.Y > 0.0f);

	Target.Actor->Destroy();
	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies TickLacheInAirState transitions to LacheMiss when no valid locked target exists.
// WHY: Prevents in-air lache state from stalling when target data is invalidated.
// EDGE CASES: LockedLacheTarget invalid early-exit path.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeLacheInAirInvalidTargetMissesTest,
	"ClimbingSystem.Actions.Lache.Runtime.InAirInvalidTargetMisses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeLacheInAirInvalidTargetMissesTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Lache runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Lache runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::LacheInAir);
	Character->LockedLacheTarget.Reset();
	Character->TickLacheInAirState(0.016f);

	TestEqual(TEXT("Lache runtime: invalid locked target should transition LacheInAir to LacheMiss"),
		Movement->CurrentClimbingState, EClimbingState::LacheMiss);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

// WHAT: Verifies TickLacheInAirState transitions to LacheCatch when expected position is within catch radius.
// WHY: Catch transition is the success condition for lache traversal.
// EDGE CASES: Zero-delta tick boundary where expected position equals launch origin.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FClimbingRuntimeLacheInAirCatchesWithinRadiusTest,
	"ClimbingSystem.Actions.Lache.Runtime.InAirCatchesWithinRadius",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClimbingRuntimeLacheInAirCatchesWithinRadiusTest::RunTest(const FString& Parameters)
{
	FTestWorldHelper Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.World->SpawnActor<AClimbingCharacter>();
	TestNotNull(TEXT("Lache runtime: character should spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	UClimbingMovementComponent* Movement = Character->FindComponentByClass<UClimbingMovementComponent>();
	TestNotNull(TEXT("Lache runtime: movement component should exist"), Movement);
	if (!Movement)
	{
		Character->Destroy();
		Helper.Teardown();
		return false;
	}

	Movement->SetClimbingState(EClimbingState::LacheInAir);
	Character->LacheLaunchPosition = Character->GetActorLocation();
	Character->LacheLaunchDirection = FVector::ForwardVector;
	Character->LacheLaunchSpeed = 1200.0f;
	Character->LacheFlightTime = 0.0f;
	Character->LockedLacheTarget = FClimbingDetectionResult();
	Character->LockedLacheTarget.LedgePosition = Character->GetActorLocation();
	Character->LockedLacheTarget.bValid = true;

	Character->TickLacheInAirState(0.0f);

	TestEqual(TEXT("Lache runtime: target at expected position should transition LacheInAir to LacheCatch"),
		Movement->CurrentClimbingState, EClimbingState::LacheCatch);

	Character->Destroy();
	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
