# Project Instructions for Copilot

This is a modern C++20 project. Using a custom Vulkan render engine as backend. The game layer uses ECS architecture.

Guidelines:
- Prefer RAII over manual memory management
- Avoid raw pointers unless required
- Follow the project C++ coding standard (see `.github/skills/cpp-coding-standard/SKILL.md`):
  - Classes and structs: PascalCase
  - Member functions: PascalCase
  - Free functions: snake_case
  - Local variables: snake_case
  - Class member variables: `m` prefix + PascalCase (e.g. `mRegistry`)
  - Struct public fields: snake_case (no prefix)
  - Spaces inside parentheses: always (e.g. `Foo( arg )`)
  - Brace style: Allman (opening brace on its own line)
  - Out-of-line function definitions: return type on its own line above `Class::Method()`
  - Indentation: hard tabs

Architecture:
- Under directory `src/`:
  - Immediate files: game entry point and main loop
  - `core/`: Core engine code (e.g. windowing, input, logging)
    - Each folder under `core/` is a module
    - codes in `core/` should be platform-agnostic and reusable across different projects, and should not depend on game-specific code
  - `game/`: Game layer, including ECS systems and components, and game logic

Build system:
- Uses CMake

Module Skills (load before working in these areas):
- `src/core/graphics/` → `.github/skills/graphics-module/SKILL.md`
