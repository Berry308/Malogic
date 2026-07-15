# AGENTS.md

## Project Overview

Malogic is an Unreal Engine 5.6.1 game project(Based on UE Lyra).

- Project file: `Malogic.uproject`
- Main runtime module: `Source/Malogic`
- Primary target files: `Source/Malogic.Target.cs`, `Source/MalogicEditor.Target.cs`
- Important systems already present:
  - Gameplay Ability System
  - Gameplay Tags
  - Enhanced Input
  - Modular Gameplay / ModularGameplayActors
  - UnLua plugins and Lua staging support

When working in this repository, prefer the existing project patterns over introducing new architecture.

## Repository Boundaries

Do edit:

- `Source/Malogic/**`
- `Config/*.ini` when the task explicitly requires project settings, input, tags, packaging, or asset manager changes
- Project-owned plugin code only when the task is clearly about that plugin

Do not edit unless explicitly requested:

- `Binaries/**`
- `Intermediate/**`
- `DerivedDataCache/**`
- `Saved/**`
- `.vs/**`
- generated solution/project files such as `Malogic.sln`
- third-party plugin internals, especially `Plugins/UnLua/**`, unless the request is specifically about modifying that plugin

Do not commit or rely on generated artifacts from the Unreal Editor, Visual Studio, or Unreal Build Tool.

## Unreal Engine Rules

- Target Unreal Engine version is 5.6.1.
- Local Unreal Engine path: `E:\Unreal Engines\UE_5.6`.
- Keep `DefaultBuildSettings = BuildSettingsVersion.V5`.
- Keep `IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6`.
- Use Unreal types and containers (`FString`, `FName`, `TArray`, `TMap`, `TObjectPtr`, etc.) in UE-facing code.
- Use Unreal reflection macros correctly. Any reflected type must include its generated header last.
- Prefer forward declarations in headers and concrete includes in `.cpp` files.
- Avoid adding module dependencies unless the code actually needs them.
- If a new dependency is required, update `Source/Malogic/Malogic.Build.cs` deliberately and explain why.

## C++ Style

- Follow the existing Unreal naming conventions:
  - `A` for actors
  - `U` for UObject classes
  - `F` for structs
  - `I` for interfaces
  - `E` for enums
  - `b` prefix for bools
- Keep public headers small and stable.
- Prefer `const` correctness for parameters and member functions.
- Prefer `TObjectPtr<>` for reflected UObject member fields.
- Use `UPROPERTY` for UObject references that must participate in GC, replication, serialization, or Blueprint/editor exposure.
- Do not use raw owning pointers for UObjects.
- Avoid broad refactors while implementing a focused feature or fix.
- Keep comments useful and concise. Do not add comments that only restate the code.

## Logging

- Use the existing log categories from `Source/Malogic/MalogicLogChannels.h`.
- Prefer `UE_LOG(LogMalogic, ...)` or a more specific existing category such as `LogMRAbilitySystem`.
- Add new log categories only when the feature is large enough to justify one.
- Include enough context in warnings/errors to diagnose the object, tag, ability, or actor involved.

## Gameplay Ability System

This project already has a custom ability system layer under `Source/Malogic/AbilitySystem`.

- Prefer extending existing classes such as:
  - `UMalogicAbilitySystemComponent`
  - `UMalogicGameplayAbility`
  - `UMalogicAttributeSet`
  - existing attribute sets under `AbilitySystem/Attributes`
- Do not bypass GAS state with parallel ad hoc state unless there is a clear reason.
- Gameplay effects, gameplay tags, ability activation groups, and input tags should remain the primary mechanisms for ability behavior.
- Preserve network prediction and replication behavior when touching abilities, input, effects, or attributes.
- Be careful with client/server authority. Server-authoritative gameplay state should not be trusted from clients.

## Gameplay Tags

- Native gameplay tags live in `Source/Malogic/MalogicGameplayTags.h/.cpp`.
- Add C++ tags with `UE_DECLARE_GAMEPLAY_TAG_EXTERN` and matching definitions.
- Keep tag names hierarchical and descriptive.
- Reuse existing tag groups where possible:
  - `Ability.*`
  - `InputTag.*`
  - `InitState.*`
  - `GameplayEvent.*`
  - `SetByCaller.*`
  - `Status.*`
  - `Movement.Mode.*`
- Do not hard-code gameplay tag strings throughout the code when a native tag is appropriate.

## Input

- Enhanced Input is enabled.
- Prefer the existing input layer under `Source/Malogic/Input`.
- Input-driven abilities should use input gameplay tags rather than tightly coupling input actions directly to ability classes.

## Modular Gameplay

- Modular Gameplay and ModularGameplayActors are part of this project.
- Prefer extension/component-based gameplay initialization where existing code already follows that model.
- Be cautious when changing lifecycle code for pawn, controller, player state, or ability initialization; these changes often affect spawn, possession, replication, and seamless travel.

## UnLua

- UnLua is present as a plugin and Lua scripts are staged through packaging settings.
- Treat `Plugins/UnLua/**` and `Plugins/UnLuaExtensions/**` as third-party/plugin code unless the task explicitly targets them.
- If adding Lua-facing C++ APIs, keep reflection metadata intentional and avoid exposing internal-only engine state unnecessarily.
- If changing packaging or script staging, check `Config/DefaultGame.ini`.

## Assets And Content

- Do not move, rename, or delete `.uasset` or `.umap` files unless explicitly requested.
- Asset references are fragile; prefer editor-aware workflows for asset renames and moves.
- If code expects a content path, keep it configurable where practical rather than hard-coding temporary asset paths.

## Config Files

- Keep `.ini` changes narrow.
- Preserve existing Unreal array syntax such as `+Entry=`, `-Entry=`, and repeated keys.
- Do not reorder large config sections unless the task requires it.
- When changing packaging, input, asset manager, collision, or gameplay tag config, mention the affected file in the final response.

## Build And Verification

Use the lightest verification that gives confidence for the change.

Common checks:

```powershell
dotnet --info
```

```powershell
& "E:\Unreal Engines\UE_5.6\Engine\Build\BatchFiles\Build.bat" MalogicEditor Win64 Development -Project="E:\Unreal Projects\Malogic\Malogic.uproject" -WaitMutex
```

If Unreal is installed elsewhere, locate the matching UE 5.6 installation before running the build.

For C++ changes:

- Build `MalogicEditor Win64 Development`.
- If the task affects runtime packaging, also consider a game target or cook/package validation.
- If the task affects GAS behavior, include a brief note about what runtime path still needs editor/playtest validation.

## Git And Generated Files

- Before editing, check the relevant files and avoid overwriting user changes.
- Do not revert unrelated changes.
- Do not stage generated directories.
- Keep commits focused if the user asks for commits.
- Generated folders such as `Binaries`, `Intermediate`, `Saved`, and `DerivedDataCache` should remain ignored.

## Agent Behavior

- Read relevant existing code before changing it.
- Prefer small, targeted edits.
- Explain assumptions when they affect gameplay behavior, networking, assets, or editor configuration.
- If a requested change may break Blueprint assets, serialized data, asset references, or network compatibility, call that out before making broad changes.
- When unsure whether behavior belongs in C++, Blueprint, Gameplay Ability assets, or Lua, inspect the surrounding implementation first and choose the path that matches the project.
- Final responses should summarize changed files and verification performed.
