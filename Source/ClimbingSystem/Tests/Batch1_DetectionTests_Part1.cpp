// Copyright Epic Games, Inc. All Rights Reserved.
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ClimbingCharacter.h"
#include "Data/ClimbingTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Helpers/ClimbingTestHelpers.h"
#include "Misc/AutomationTest.h"

// TC-0153
// WHAT: Surface normal from ledge detection points toward the character (dot > 0).
// WHY: Ensures the wall normal is oriented correctly for hang positioning and IK.
// EDGE CASES: Normal must be in 2D plane — uses GetSafeNormal2D.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeDetectionSurfaceNormalPointsTowardCharacterTest,
	"ClimbingSystem.Detection.LedgeGrab.SurfaceNormal_PointsTowardCharacter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeDetectionSurfaceNormalPointsTowardCharacterTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0153: character must spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	Helper.SpawnBoxAt(FVector(150.f, 0.f, 100.f), FVector(50.f, 100.f, 100.f));

	const FClimbingDetectionResult Result = Character->PerformLedgeDetection();
	TestTrue(TEXT("TC-0153: detection must be valid to test normal direction"), Result.bValid);
	if (!Result.bValid)
	{
		Helper.Teardown();
		return false;
	}

	const FVector CharLoc = Character->GetActorLocation();
	const FVector ToChar2D = (CharLoc - Result.LedgePosition).GetSafeNormal2D();
	const float Dot = FVector::DotProduct(Result.SurfaceNormal, ToChar2D);
	TestTrue(TEXT("TC-0153: SurfaceNormal dot (CharLoc - LedgePos).GetSafeNormal2D() must be > 0"), Dot > 0.f);

	Helper.Teardown();
	return true;
}

// TC-0155
// WHAT: PerformLedgeDetectionAtLocation returns valid for a reachable ledge.
// WHY: AtLocation variant must work for Lache targeting and predictive arc detection.
// EDGE CASES: Ledge box placed at (150,0,100); query at (150,0,0).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeDetectionAtLocationReturnsValidForReachableLedgeTest,
	"ClimbingSystem.Detection.AtLocation.ReturnsValid_ForReachableLedge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeDetectionAtLocationReturnsValidForReachableLedgeTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0155: character must spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	Helper.SpawnBoxAt(FVector(150.f, 0.f, 100.f), FVector(50.f, 100.f, 100.f));

	const FClimbingDetectionResult Result = Character->PerformLedgeDetectionAtLocation(FVector(150.f, 0.f, 0.f));
	TestTrue(TEXT("TC-0155: PerformLedgeDetectionAtLocation must return bValid=true for reachable ledge"), Result.bValid);

	Helper.Teardown();
	return true;
}

// TC-0156
// WHAT: PerformLedgeDetectionAtLocation returns invalid when no geometry exists.
// WHY: Must not produce false positives in empty space.
// EDGE CASES: Query at (1000,1000,1000) with no spawned geometry.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeDetectionAtLocationReturnsInvalidWhenNoGeometryTest,
	"ClimbingSystem.Detection.AtLocation.ReturnsInvalid_WhenNoGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeDetectionAtLocationReturnsInvalidWhenNoGeometryTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0156: character must spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	const FClimbingDetectionResult Result = Character->PerformLedgeDetectionAtLocation(FVector(1000.f, 1000.f, 1000.f));
	TestFalse(TEXT("TC-0156: PerformLedgeDetectionAtLocation must return bValid=false when no geometry exists"), Result.bValid);

	Helper.Teardown();
	return true;
}

// TC-0158
// WHAT: ResolveHitComponentFromNet uses ConfirmationTraceRadius to accept/reject hits.
// WHY: Radius boundary must be respected — 15cm wall succeeds, 20cm wall fails with radius=16.
// EDGE CASES: Two sub-tests in one TC; ConfirmationTraceRadius set to 16.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResolveHitComponentFromNetUsesConfirmationRadiusTest,
	"ClimbingSystem.Detection.ConfirmationTrace.UsesConfiguredRadius",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResolveHitComponentFromNetUsesConfirmationRadiusTest::RunTest(const FString& Parameters)
{
	// Sub-test A: wall at 15cm offset — should resolve (within radius=16)
	{
		FClimbingTestWorld Helper;
		Helper.Setup();

		AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
		TestNotNull(TEXT("TC-0158A: character must spawn"), Character);
		if (!Character)
		{
			Helper.Teardown();
			return false;
		}

		Character->ConfirmationTraceRadius = 16.f;

		// LedgePosition at origin; wall face at X=15 (box center at X=65, half-extent X=50)
		const FVector LedgePos(0.f, 0.f, 90.f);
		Helper.SpawnBoxAt(FVector(65.f, 0.f, 90.f), FVector(50.f, 120.f, 120.f));

		FClimbingDetectionResultNet NetResult;
		NetResult.bValid = true;
		NetResult.LedgePosition = LedgePos;
		NetResult.SurfaceNormal = FVector(-1.f, 0.f, 0.f); // pointing away from wall (toward character)
		NetResult.SurfaceTier = EClimbSurfaceTier::Climbable;
		NetResult.ClearanceType = EClimbClearanceType::Full;

		UPrimitiveComponent* Resolved = Character->ResolveHitComponentFromNet(NetResult);
		TestNotNull(TEXT("TC-0158A: wall at 15cm offset should resolve (within ConfirmationTraceRadius=16)"), Resolved);

		Helper.Teardown();
	}

	// Sub-test B: wall at 20cm offset — should fail (beyond radius=16)
	{
		FClimbingTestWorld Helper;
		Helper.Setup();

		AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
		TestNotNull(TEXT("TC-0158B: character must spawn"), Character);
		if (!Character)
		{
			Helper.Teardown();
			return false;
		}

		Character->ConfirmationTraceRadius = 16.f;

		// LedgePosition at origin; wall face at X=20 (box center at X=70, half-extent X=50)
		const FVector LedgePos(0.f, 0.f, 90.f);
		Helper.SpawnBoxAt(FVector(70.f, 0.f, 90.f), FVector(50.f, 120.f, 120.f));

		FClimbingDetectionResultNet NetResult;
		NetResult.bValid = true;
		NetResult.LedgePosition = LedgePos;
		NetResult.SurfaceNormal = FVector(-1.f, 0.f, 0.f);
		NetResult.SurfaceTier = EClimbSurfaceTier::Climbable;
		NetResult.ClearanceType = EClimbClearanceType::Full;

		UPrimitiveComponent* Resolved = Character->ResolveHitComponentFromNet(NetResult);
		TestNull(TEXT("TC-0158B: wall at 20cm offset should not resolve (beyond ConfirmationTraceRadius=16)"), Resolved);

		Helper.Teardown();
	}

	return true;
}

// TC-0159
// WHAT: ResolveHitComponentFromNet returns nullptr and does not crash when SurfaceNormal is zero.
// WHY: Zero normal is degenerate input; must be rejected gracefully without crash.
// EDGE CASES: bValid=true but SurfaceNormal=ZeroVector.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResolveHitComponentFromNetRejectsZeroNormalTest,
	"ClimbingSystem.Detection.ConfirmationTrace.RejectsZeroNormal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FResolveHitComponentFromNetRejectsZeroNormalTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0159: character must spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	FClimbingDetectionResultNet NetResult;
	NetResult.bValid = true;
	NetResult.LedgePosition = FVector(100.f, 0.f, 90.f);
	NetResult.SurfaceNormal = FVector::ZeroVector;
	NetResult.SurfaceTier = EClimbSurfaceTier::Climbable;
	NetResult.ClearanceType = EClimbClearanceType::Full;

	UPrimitiveComponent* Resolved = Character->ResolveHitComponentFromNet(NetResult);
	TestNull(TEXT("TC-0159: zero SurfaceNormal must return nullptr without crash"), Resolved);

	Helper.Teardown();
	return true;
}

// TC-0166
// WHAT: HitComponent is valid on a successful PerformLedgeDetection.
// WHY: HitComponent is required for IK anchoring and surface data lookup.
// EDGE CASES: Checks both bValid and HitComponent.IsValid().
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeDetectionHitComponentIsValidOnSuccessTest,
	"ClimbingSystem.Detection.LedgeGrab.HitComponent_ValidOnSuccess",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeDetectionHitComponentIsValidOnSuccessTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0166: character must spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	Helper.SpawnBoxAt(FVector(150.f, 0.f, 100.f), FVector(50.f, 100.f, 100.f));

	const FClimbingDetectionResult Result = Character->PerformLedgeDetection();
	TestTrue(TEXT("TC-0166: detection must be valid"), Result.bValid);
	TestTrue(TEXT("TC-0166: HitComponent must be valid on successful detection"), Result.HitComponent.IsValid());

	Helper.Teardown();
	return true;
}

// TC-0154
// WHAT: PerformLedgeDetection finds a ledge along the falling arc when movement mode is Falling.
// WHY: Arc-based detection must use actual velocity to predict trajectory during freefall.
// EDGE CASES: SetMovementMode may not be directly accessible — uses CharacterMovement cast.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLedgeDetectionArcUsesActualVelocityWhenFallingTest,
	"ClimbingSystem.Detection.Arc.UsesActualVelocity_WhenFalling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLedgeDetectionArcUsesActualVelocityWhenFallingTest::RunTest(const FString& Parameters)
{
	FClimbingTestWorld Helper;
	Helper.Setup();

	AClimbingCharacter* Character = Helper.SpawnCharacterAt(FVector::ZeroVector);
	TestNotNull(TEXT("TC-0154: character must spawn"), Character);
	if (!Character)
	{
		Helper.Teardown();
		return false;
	}

	// Set movement mode to Falling and apply a forward+downward velocity
	UCharacterMovementComponent* CMC = Character->GetCharacterMovement();
	TestNotNull(TEXT("TC-0154: CharacterMovementComponent must be present"), CMC);
	if (!CMC)
	{
		Helper.Teardown();
		return false;
	}

	// VERIFY: SetMovementMode is available on UCharacterMovementComponent
	CMC->SetMovementMode(MOVE_Falling);
	CMC->Velocity = FVector(200.f, 0.f, -400.f);

	// Spawn a ledge along the predicted arc trajectory: forward ~200cm, down ~400cm over ~1s
	// Place box at (200, 0, -100) with generous extents to intercept the arc
	Helper.SpawnBoxAt(FVector(200.f, 0.f, -100.f), FVector(50.f, 100.f, 100.f));

	const FClimbingDetectionResult Result = Character->PerformLedgeDetection();
	TestTrue(TEXT("TC-0154: PerformLedgeDetection must find ledge along falling arc trajectory"), Result.bValid);

	Helper.Teardown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
