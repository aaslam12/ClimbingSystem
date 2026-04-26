# Final Prompt: Generate Comprehensive Test Suite for UE5 Project

```
Before writing a single line of test code, you MUST complete the following
reading phase in full. Do not skip or skim any file.

═══════════════════════════════════════════════════════════════
## PHASE 0: STATE YOUR ASSUMPTIONS (Output this before anything else)
═══════════════════════════════════════════════════════════════

Before reading any files, output this block — then fill it in after reading:

## My Assumptions (fill after Phase 1 reading)
- UE5 version: [inferred from .uproject or Build.cs files]
- Project name: [inferred from .uproject]
- Test module name: [what I will name it and why]
- Primary game type: [what the project prompt/documentation says this project is]
- Systems found in source vs. described in project documentation but missing
  from source: [list any gaps]
- APIs I am uncertain about: [list here — do NOT silently guess]
- Blueprint vs C++ split: [estimate of how much logic is in BP vs C++]

Do not proceed to Phase 1 until this block is written, even if fields
must be left as [UNKNOWN — will fill after reading].

═══════════════════════════════════════════════════════════════
## PHASE 1: MANDATORY READING (Complete ALL before writing any tests)
═══════════════════════════════════════════════════════════════

Read every file listed below in order. Do not skip any.

### Step 1.1 — Read the original project prompt and all documentation
Read every file that describes what this project is, its goals, its systems,
and its intended behavior. This includes:
- Any PROMPT.md, README.md, DESIGN.md, or equivalent top-level description
- Any design documents, GDD files, or feature specification documents
- Any architecture or system description files

This documentation is your north star. After reading, write a 2-sentence
summary:
  "This project is: ..."
  "The core systems are: ..."
If you cannot write those 2 sentences, stop and say so — do not proceed.

### Step 1.2 — Read all .github/ files
Read every file in the .github/ directory:
- .github/copilot-instructions.md (if present)
- .github/instructions/ (all files, if present)
- .github/prompts/ (all files, if present)
- .github/workflows/ (all .yml files — note what CI already runs)

After reading, list:
- Coding conventions enforced by instruction files
- What the CI pipeline already validates
- Any test-related workflows already defined

### Step 1.3 — Read all root-level documentation
Read every .md, .txt, CONTRIBUTING, and CONVENTIONS file in the root
directory. Note any architectural decisions, naming rules, or system
descriptions that will affect test design.

### Step 1.4 — Scan for ALL existing tests
Search the entire repository for:
- Files matching: *Test*.cpp, *Test*.h, *Spec.cpp, *Tests.cpp
- Directories named: Tests/, Testing/, Specs/
- Any Python or shell scripts that run any form of validation

Output a complete list:
## Existing Tests Found
- [filepath]: [brief description of what it tests]
- [filepath]: [brief description of what it tests]
## NO existing tests will be recreated. This list is final.

### Step 1.5 — Scan Source/ for all systems
List every .cpp and .h file found in Source/. Group by subsystem/folder.
This becomes your test coverage map in Phase 2.

═══════════════════════════════════════════════════════════════
## PHASE 2: GAP ANALYSIS REPORT (Output this before any test code)
═══════════════════════════════════════════════════════════════

Produce the gap analysis first. No test code until this is complete.

For every system found in Step 1.5 and every system described in the
project documentation, output one row:

## Gap Analysis

| System | Source File(s) | In Project Docs | Test Status | Priority |
|--------|---------------|-----------------|-------------|----------|
| [name] | [path]        | ✅ / ❌          | ✅ Tested / ❌ Untested / ⚠️ Partial / 🔵 Blueprint-only | P1/P2/P3 |

Priority definitions:
- P1: Critical gameplay path (as described in project documentation)
- P2: Core systems (supporting the critical path)
- P3: Secondary systems, polish, edge features

Blueprint-only systems:
- Mark as 🔵 BLUEPRINT — do NOT write automated tests for these
- Instead write: // MANUAL QA REQUIRED: [describe what a tester should verify]
- Flag any C++ interfaces that Blueprint calls into — those ARE testable

If the project has more than 20 systems:
- Generate complete tests for the top 10 by priority
- For the remainder, output test stubs with TODO comments
- State clearly at the end:
  "Deferred systems: [list] — reason: token/scope limit"

═══════════════════════════════════════════════════════════════
## PHASE 3: TEST SUITE GENERATION
═══════════════════════════════════════════════════════════════

Now generate tests. Produce exactly ONE file at a time.
After each file output:
  --- FILE COMPLETE: [filename] ---
  NEXT: [next filename]

If the project has more than 5 test files, pause after each file
and wait for "continue" before producing the next.

---

### CRITICAL: No Duplicate Work

Before writing any test, cross-reference:
1. The existing tests list from Step 1.4
2. Every test already written in the current session

If a behavior is already covered — even partially — do NOT write another
test for it. Write a comment instead:
  // COVERAGE NOTE: [behavior] is already tested in [filename]

---

### CRITICAL: API Accuracy Requirement

UE5's automation API has specific macros and method signatures.
If you are not 100% certain a macro or method exists exactly as written:
1. Write a comment: // VERIFY: confirm this API exists in UE5.[version]
2. List it in your Assumptions block as "uncertain API"
3. Never silently invent a method name that sounds plausible

The following are CONFIRMED valid in UE5.3+. Use only these unless
you find others confirmed in Epic's documentation:

  Macros:
  - IMPLEMENT_SIMPLE_AUTOMATION_TEST(TClass, PrettyName, TFlags)
  - IMPLEMENT_COMPLEX_AUTOMATION_TEST(TClass, PrettyName, TFlags)
  - DEFINE_LATENT_AUTOMATION_COMMAND(CommandName)
  - DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(CommandName, ParamType, ParamName)
  - ADD_LATENT_AUTOMATION_COMMAND(CommandInstance)

  Assertion methods (called as this->TestX(...) inside RunTest()):
  - TestEqual(What, Actual, Expected)
  - TestNotEqual(What, Actual, Expected)
  - TestTrue(What, bValue)
  - TestFalse(What, bValue)
  - TestNotNull(What, Pointer)
  - TestNull(What, Pointer)
  - TestValid(What, WeakPtr)
  - AddError(Message) — for custom failure messages

  World creation (confirmed pattern — see template below):
  - UWorld::CreateWorld(EWorldType::Game, false)
  - GEngine->CreateNewWorldContext(EWorldType::Game)
  - World->InitializeActorsForPlay(FURL())
  - World->BeginPlay()
  - GEngine->DestroyWorldContext(World)
  - World->DestroyWorld(false)

  Object creation:
  - NewObject<T>() — for UObjects not requiring a world
  - World->SpawnActor<T>() — for Actors requiring a world

  Do NOT invent:
  - TestWorld::CreateTestActor() ← does not exist
  - FAutomationTestWorld ← does not exist
  - UTestHelper::Spawn() ← does not exist
  - Any method not in the confirmed list above without a // VERIFY comment

---

### Canonical World Setup Template

Every test that requires a UWorld MUST use this pattern exactly.
Do not invent alternatives.

```cpp
// SharedTestHelpers.h — include this in any world-dependent test

#pragma once
#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

struct FTestWorldHelper
{
    UWorld* World = nullptr;

    void Setup()
    {
        World = UWorld::CreateWorld(EWorldType::Game, false);
        FWorldContext& WorldContext =
            GEngine->CreateNewWorldContext(EWorldType::Game);
        WorldContext.SetCurrentWorld(World);
        FURL URL;
        World->InitializeActorsForPlay(URL);
        World->BeginPlay();
    }

    void Teardown()
    {
        if (World)
        {
            World->BeginTearingDown();
            GEngine->DestroyWorldContext(World);
            World->DestroyWorld(false);
            World = nullptr;
        }
    }
};
```

Every test using FTestWorldHelper MUST call Teardown() before returning,
including early returns on test failure.
Pattern:
```cpp
bool FMyWorldTest::RunTest(const FString& Parameters)
{
    FTestWorldHelper Helper;
    Helper.Setup();

    // ... test code ...

    Helper.Teardown(); // MUST be called before every return path
    return true;
}
```

---

### Memory and Cleanup Requirements

Every test must clean up what it creates:

| Created With          | Must Be Cleaned Up With                              |
|-----------------------|------------------------------------------------------|
| UWorld::CreateWorld() | World->DestroyWorld() + GEngine->DestroyWorldContext() |
| World->SpawnActor<>() | Actor->Destroy() OR world teardown handles it        |
| NewObject<>()         | Obj->ConditionalBeginDestroy() if not in world       |
| Static state used     | // STATIC-SAFE: [explain why this is safe]           |

Every world-dependent test file must have a comment at the top:
// WORLD CLEANUP: verified — all paths call Helper.Teardown()

---

### Test File Structure

Mirror the source directory structure exactly:

```
Source/
  [ProjectName]/
    Tests/
      Helpers/
        SharedTestHelpers.h        ← FTestWorldHelper + common utilities
      Systems/
        [SystemName]Tests.cpp      ← mirrors Source/[ProjectName]/Systems/
      Components/
        [ComponentName]Tests.cpp   ← mirrors Source/[ProjectName]/Components/
      GameModes/
        [GameMode]Tests.cpp
      AI/
        [AISystem]Tests.cpp
      UI/
        [Widget]Tests.cpp
      Integration/
        [Feature]IntegrationTests.cpp
      Performance/
        [System]PerfTests.cpp
```

---

### Test Naming — Registry Name Format

The string passed to IMPLEMENT_SIMPLE_AUTOMATION_TEST is the name that
appears in Session Frontend and CI output. Format it precisely:

```
"[ProjectName].[System].[Subsystem].[WhatIsTested]"
```

Examples:
```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FInventoryStackLimitTest,
    "MyGame.Inventory.Stacking.AddBeyondLimitClampsToMax",
    EAutomationTestFlags::ApplicationContextMask |
    EAutomationTestFlags::ProductFilter
)
```

Rules:
- No spaces in any segment
- Use PascalCase within each segment
- The class name (first arg) must be unique across the entire project
- The registry name (second arg) must be unique across the entire project
- Names must reflect the specific behavior under test, not just the system

---

### Required Test Categories

Generate tests for every untested system from the gap analysis.
Work through categories in priority order (P1 → P2 → P3).
Do NOT generate a test in a lower priority category if work in a higher
priority category remains.

#### CATEGORY 1: Unit Tests — Pure Logic (No World required)

For every system/utility class identified:
- All pure functions and static methods
- Math and formula correctness (especially formulas mentioned in project docs)
- State machine transitions
- Data validation and parsing
- Configuration and default values
- Game balance calculations specific to the project's design

Template:
```cpp
// WHAT: [one sentence — what behavior is being tested]
// WHY: [why this matters to the game — reference project documentation]
// EDGE CASES: [what edge cases this covers]

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    F[System][Behavior]Test,
    "[ProjectName].[System].[Subsystem].[Behavior]",
    EAutomationTestFlags::ApplicationContextMask |
    EAutomationTestFlags::ProductFilter
)

bool F[System][Behavior]Test::RunTest(const FString& Parameters)
{
    // Arrange
    // ...

    // Act
    // ...

    // Assert — every TestEqual must have a meaningful first argument
    TestEqual(
        TEXT("[System]: [what this value represents] should be [expected]"),
        ActualValue,
        ExpectedValue
    );

    return true;
}
```

#### CATEGORY 2: Component Tests

For every UActorComponent subclass:
- Default values on construction (use NewObject<>(), no world needed)
- Behavior when all dependencies are valid
- Graceful degradation when dependencies are null or invalid
- All public methods
- Delegate/event firing conditions

#### CATEGORY 3: Gameplay Tests — Requires World

Use FTestWorldHelper for all of these.
- Actor spawning and initialization
- Player controller and pawn setup
- Game mode rule enforcement
- Win/lose condition logic
- Respawn and checkpoint behavior
- Any core loop mechanic described in project documentation

#### CATEGORY 4: Networking Tests
(only if multiplayer is described in project documentation)

Note: True replication tests require a multi-process setup.
For single-process tests:
- Authority checks (HasAuthority() gating)
- RPC function signatures exist and are callable
- Mark as // NETWORK: single-process approximation — full replication
  requires PIE multi-player session

#### CATEGORY 5: Performance Tests

Use only these confirmed timing methods:

```cpp
// WHAT: Measures [hot path] performance under [N] iterations
// THRESHOLD: [value]ms — justify: [from project spec / industry baseline /
//            measured baseline]

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    F[System]PerformanceTest,
    "[ProjectName].Performance.[System].[WhatIsMeasured]",
    EAutomationTestFlags::ApplicationContextMask |
    EAutomationTestFlags::PerformanceFilter
)

bool F[System]PerformanceTest::RunTest(const FString& Parameters)
{
    const int32 Iterations = 1000;
    // THRESHOLD: justify this value with a comment
    const double MaxMedianMs = 1.0;

    TArray<double> Timings;
    Timings.Reserve(Iterations);

    for (int32 i = 0; i < Iterations; ++i)
    {
        double Start = FPlatformTime::Seconds();
        // ... hot path code ...
        double End = FPlatformTime::Seconds();
        Timings.Add((End - Start) * 1000.0); // convert to ms
    }

    Timings.Sort();
    double MedianMs = Timings[Iterations / 2];

    TestTrue(
        FString::Printf(TEXT("[System]: median time %.4fms should be < %.1fms"),
            MedianMs, MaxMedianMs),
        MedianMs < MaxMedianMs
    );

    return true;
}
```

All thresholds must have a comment:
// THRESHOLD: [value] — source: [project spec / measured baseline /
//            industry standard]
Never invent a threshold without justification.

#### CATEGORY 6: Integration Tests — Full Feature Flows

For each major user-facing feature described in the project documentation,
write one end-to-end test that exercises the complete flow.
Do NOT write an integration test for a flow already covered by a unit test
or component test in the same session.

Structure each integration test as:
```
// FLOW: [describe the sequence in plain English]
// Step 1: [setup]
// Step 2: [action]
// Step 3: [verify intermediate state]
// Step 4: [action]
// Step 5: [verify final state]
```

These will use FTestWorldHelper and live in:
  Tests/Integration/[FeatureName]IntegrationTests.cpp

---

### Assertion Quality Standard

Every assertion must be self-explanatory when it fails in CI output:

```cpp
// ✅ GOOD — tells you exactly what failed and what was expected
TestEqual(
    TEXT("Inventory: adding 10 items to a stack of 95 (max 100) should clamp to 100"),
    Inventory->GetItemCount(ItemID),
    100
);

TestTrue(
    TEXT("AI: entering chase state should set movement speed to SprintSpeed (600)"),
    FMath::IsNearlyEqual(AIController->GetMoveSpeed(), 600.0f, 0.1f)
);

// ❌ BAD — useless when it fails
TestTrue(TEXT("health check"), bResult);
TestEqual(TEXT("test"), Value, 0);
```

---

### Build File Requirements

#### [ProjectName]Tests.Build.cs

Provide complete Build.cs content:
```csharp
using UnrealBuildTool;

public class [ProjectName]Tests : ModuleRules
{
    public [ProjectName]Tests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "[ProjectName]"  // the module being tested
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "AutomationController",
            // VERIFY: confirm module name in UE5.[version]
        });

        // Only include test module in non-shipping builds
        if (Target.Configuration != UnrealTargetConfiguration.Shipping)
        {
            PrivateDependencyModuleNames.Add("AutomationTest");
            // VERIFY: confirm AutomationTest is a separate module in UE5.[version]
            // Alternative: tests may be included in the main module under
            // #if WITH_AUTOMATION_TESTS
        }
    }
}
```

Note any VERIFY comments and resolve them against the actual UE5 version
found in the project files.

---

═══════════════════════════════════════════════════════════════
## PHASE 4: OUTPUT VALIDATION
═══════════════════════════════════════════════════════════════

Before considering the output complete, verify each item below.
Do NOT self-report. Demonstrate each check with evidence from your output.

| Check | How to Demonstrate |
|-------|--------------------|
| Gap analysis covers every .cpp in Source/ | Quote the total file count from your scan |
| No existing test is recreated | List the existing tests found and confirm none appear in new output |
| No behavior tested in one file is re-tested in another | Quote 2 cases where you added a COVERAGE NOTE instead of a duplicate test |
| Every test file includes all required headers | Show the #include list for the most complex test file |
| No invented API is used without // VERIFY | Quote 3 test assertions from your output and confirm each uses a confirmed API |
| Test names follow [Project].[System].[Subsystem].[Behavior] format | Quote 3 test registry names from your output |
| World-dependent tests all call Teardown() | Quote the teardown call from one world test |
| All performance thresholds have // THRESHOLD comments | Quote one threshold with its justification |
| Build.cs is provided | State the filename and the modules listed |
| Deferred systems are listed (if >20 systems) | List them or confirm count was ≤20 |

Output this table filled in. Any row you cannot fill in means that item
is missing from your output — go back and add it before finishing.

═══════════════════════════════════════════════════════════════
## EXECUTION ORDER SUMMARY
═══════════════════════════════════════════════════════════════

1.  Output Phase 0 Assumptions block (partially filled)
2.  Read all project documentation → complete the 2-sentence project summary
3.  Read .github/ files → list conventions and note any existing CI
4.  Read root docs → note architectural rules
5.  Scan for existing tests → output complete list
6.  Scan Source/ → output complete file list grouped by subsystem
7.  Fill in Phase 0 Assumptions block fully
8.  Output Gap Analysis table
9.  Output SharedTestHelpers.h
      --- FILE COMPLETE: SharedTestHelpers.h ---
      NEXT: [ProjectName]Tests.Build.cs
10. Output Build.cs
      --- FILE COMPLETE: [ProjectName]Tests.Build.cs ---
      NEXT: [first test file by priority]
11. Output each test file one at a time, in priority order (P1 → P2 → P3),
    pausing for "continue" if more than 5 files
12. Output Phase 4 validation table, filled in with evidence

HARD RULES — enforced at every step:
- Zero duplicate tests: if a behavior is covered, write COVERAGE NOTE
- Zero invented APIs without // VERIFY
- Zero world tests without Teardown() on every return path
- Zero performance thresholds without // THRESHOLD justification
- Zero test names that are vague or non-descriptive
- Tests reflect the actual project described in the documentation —
  do not generate generic placeholder tests
```

