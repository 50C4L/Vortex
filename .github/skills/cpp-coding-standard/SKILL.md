---
name: cpp-coding-standard
description: "USE WHEN: writing, reviewing, or editing C++ code in this workspace. Enforces the project's naming conventions (classes, functions, free functions, stack variables, member variables), spaces inside parentheses, brace style, return type placement, and other style rules derived from the existing Vortex codebase."
applyTo: "**/*.cpp, **/*.h"
---

# C++ Coding Standard — Vortex Project

Always follow these rules when generating or editing C++ code in this workspace.

| Element | Convention | Example |
|---|---|---|
| Class / Struct name | PascalCase | `AudioSystem`, `SoundConfig` |
| Member function | PascalCase | `LoadSound()`, `Update()` |
| Free function | snake_case | `check_validation_layer_support()` |
| Local variable | snake_case | `found_layer`, `player_entity` |
| Class member variable | `m` + PascalCase | `mRegistry`, `mNextSoundId` |
| Struct public field | snake_case | `pool_size`, `body_id` |
| Constant | ALL_CAPS | `MAX_FRAMES_IN_FLIGHT` |
| Enum value | ALL_CAPS | `STATIC`, `DYNAMIC` |
| Spaces in parens | Always | `SDL_Init( SDL_INIT_VIDEO )` |
| Braces | Allman | opening brace on its own line |
| Return type (out-of-line) | Separate line | `void\nVortexGame::Run()` |
| Indentation | Hard tabs | one tab per level |
