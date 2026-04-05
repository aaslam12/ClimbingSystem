# Repository Guidelines

## Project Structure & Module Organization
`ClimbingSystem.uproject` targets Unreal Engine 5.7 and defines one runtime module: `Source/ClimbingSystem`. Core gameplay code lives in paired `.h`/`.cpp` files such as `ClimbingCharacter`, `ClimbingMovementComponent`, `ClimbingAnimInstance`, and `ClimbingSurfaceData`. Project settings are in `Config/Default*.ini`. Assets live under `Content/`, primarily `Content/ClimbingSystem`, `Content/Input`, and the sample `Content/ThirdPerson` content. Do not commit generated folders such as `Binaries/`, `Intermediate/`, or transient `Saved/` output beyond intentional config changes.

## Build, Test, and Development Commands
Open the editor with:
```bash
UnrealEditor ClimbingSystem.uproject
```
Build the editor target from a shell with:
```bash
UnrealBuildTool ClimbingSystemEditor Win64 Development -Project="$PWD/ClimbingSystem.uproject"
```
Build a game target with:
```bash
UnrealBuildTool ClimbingSystem Win64 Development -Project="$PWD/ClimbingSystem.uproject"
```
Regenerate project files after adding modules or targets:
```bash
UnrealBuildTool -ProjectFiles -Project="$PWD/ClimbingSystem.uproject"
```
If your local setup uses Unreal Automation Tool wrappers instead, use the equivalent engine-provided build command for the same targets.

## Coding Style & Naming Conventions
Match the existing Unreal C++ style: tabs for indentation, braces on new lines, and `PascalCase` for types, methods, and `UPROPERTY` fields. Prefix Unreal types conventionally (`AClimbingCharacter`, `UClimbingMovementComponent`, `EClimbingState`). Keep filenames aligned with class names. Favor clear category names like `Climbing|Input` and concise comments only where behavior is non-obvious. Blueprints here should stay data- or wiring-focused; core climbing logic belongs in C++.

## Testing Guidelines
There are currently no `AutomationSpec` or `IMPLEMENT_SIMPLE_AUTOMATION_TEST` tests in the repository. For new features, add automation coverage where practical and keep test names tied to the class or feature under test. At minimum, validate in editor play sessions that climbing, input mapping, animation notifies, and replication-sensitive flows still work.

## Commit & Pull Request Guidelines
Recent commits follow a scoped pattern like `[ClimbingSystem] Milestone 16: BeginPlay validation and EndPlay/Destroyed cleanup`. Keep that format: module tag, milestone or feature label, then a short imperative summary. PRs should state gameplay impact, list touched systems, note config or asset dependencies, and include screenshots or short clips for visible movement, camera, animation, or debug-visualization changes.
