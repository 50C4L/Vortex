# Refactoring Plan: Vortex — Full Project Review

## Executive Summary

The engine core is architecturally sound for its size, but three recurring problems limit its long-term maintainability: (1) backend APIs (SDL, Vulkan, Box2D) leak through public headers into layers that should not know about them; (2) `VortexGame` acts as a God Object, wiring every subsystem together and eliminating any seam for testing or re-use; (3) the ECS storage strategy is cache-hostile and will become a visible bottleneck as entity counts grow. The changes below address these problems in priority order without over-engineering.

---

## Current Architecture

```
main.cpp
└── VortexGame                     (owns ~15 systems, init, run loop)
    ├── SDL_Window (shared_ptr)
    ├── Renderer                   (Vulkan, leaks vk:: types publicly)
    ├── SceneRenderPass            (Vulkan render pass, leaks vk:: types)
    ├── ImGuiRenderPass            (Vulkan + ImGui)
    ├── InputController            (leaks SDL_Keycode publicly)
    ├── AudioMixer
    ├── ECSRegistry                (double-unordered_map + std::any storage)
    ├── [5× ECS engine systems]
    ├── SceneController            (owns AbstractScene* instances)
    └── PerformanceTracker         (depends directly on Renderer)

MainScene (AbstractScene)
    ├── receives 5 engine system refs by constructor
    ├── owns 5 game-specific systems
    └── calculates its own delta time

Game systems (BulletSystem, AsteroidGameplaySystem, WarpSystem)
    └── each holds RenderSystem& + PhysicsSystem&
```

Key coupling/leakage:
- `InputController.h` exposes `SDL_Keycode` — SDL pulled into every subscriber header
- `AbstractRenderPass.h` exposes `vk::CommandBuffer`, `vk::ImageView` — Vulkan in every pass header
- `Renderer.h` exposes `vk::Device`, `vk::Format`, `vk::UniqueSampler`, etc.
- `PhysicsSystem.cpp` calls `b2Body_*` C API directly (flagged with `@todo`)
- `HudRenderSystem` depends on `ImGuiRenderPass` — HUD tied to one backend
- `PerformanceTracker` depends on `graphics::Renderer`
- `AudioSourceComponent` / `AudioEventComponent` are outside `eage::ecs` namespace
- `ECSRegistry::components` field violates project naming convention (`m` prefix)
- `SceneGraphComponment` has a typo (missing 'e')
- Delta time calculated inside `MainScene`; `AudioSystem::Update( 0.f )` uses hardcoded zero
- `SceneGraphSystem::SetSceneRoot()` must be called manually after each `ChangeScene()`

---

## Problems Identified

1. **`VortexGame` is a God Object**
   - **Location**: `src/VortexGame.h`, `src/VortexGame.cpp`
   - **Impact**: All 15+ subsystems are owned and wired in one class. Impossible to swap subsystems, write unit tests, or re-use the engine core without dragging in the entire game configuration.

2. **`InputController` leaks SDL types in public API**
   - **Location**: `src/core/events/InputController.h`
   - **Impact**: Every file that includes `InputController.h` transitively includes `SDL2/SDL.h`. Subscribers that only want to react to events must pull in the full SDL windowing backend.

3. **`AbstractRenderPass` and `Renderer` leak Vulkan types**
   - **Location**: `src/core/graphics/AbstractRenderPass.h`, `src/core/graphics/Renderer.h`
   - **Impact**: Any code that creates or subclasses a render pass must include `<vulkan/vulkan.hpp>`. The `Renderer` public API exposes `vk::Device`, `vk::Format`, `vk::UniqueSampler`, `vk::DescriptorSetLayout`, and `VMAWrapper` — preventing any future backend swap and causing long compile chains.

4. **`PhysicsSystem` calls Box2D C API directly**
   - **Location**: `src/core/ecs/systems/PhysicsSystem.cpp`
   - **Impact**: Box2D's `b2Body_*` calls (acknowledged by an in-source `@todo`) appear directly in `PhysicsSystem`, bypassing `PhysicsEngine`. This breaks the abstraction boundary set up by `PhysicsEngine` and makes the physics backend non-swappable from `PhysicsSystem`.

5. **`HudRenderSystem` directly depends on `ImGuiRenderPass`**
   - **Location**: `src/core/ecs/systems/HudRenderSystem.h`
   - **Impact**: HUD rendering is coupled to a concrete ImGui/Vulkan class. Any HUD test or HUD implementation for a different backend must instantiate an `ImGuiRenderPass`.

6. **ECS storage is cache-hostile**
   - **Location**: `src/core/ecs/ECS.h`
   - **Impact**: Every component access requires (a) a `std::unordered_map<std::type_index, std::any>` lookup, (b) a `std::any_cast`, and (c) a second `std::unordered_map<Entity, T>` lookup. This is three pointer-chasing operations per access, with no spatial locality. Under any moderate entity count this will show up on a profiler.

7. **Delta time has no single authoritative source**
   - **Location**: `src/VortexGame.cpp` (`Run()`), `src/game/MainScene.cpp` (`Update()`), `src/core/ecs/systems/AudioSystem` (`Update( 0.f )`)
   - **Impact**: `MainScene` computes its own delta time independently; `AudioSystem` always receives `0.f`; `PhysicsSystem::Update()` has no time parameter at all. Game systems cannot interpolate or behave correctly at variable frame rates.

8. **`SceneController` and `SceneGraphSystem` have implicit ordering coupling**
   - **Location**: `src/VortexGame.cpp` (`Init()`), `src/SceneController.h`, `src/core/ecs/systems/SceneGraphSystem.h`
   - **Impact**: After `ChangeScene()`, the caller must manually call `SceneGraphSystem::SetSceneRoot()`. There is no enforcement. If a mid-game scene change ever happens (e.g. game-over screen), the scene graph root will silently remain stale.

9. **`PerformanceTracker` depends on `graphics::Renderer`**
   - **Location**: `src/core/profiling/PerformanceTracker.h`
   - **Impact**: The profiling module — which conceptually belongs to `core` — has a compile-time dependency on the Vulkan renderer. GPU frame time could be surfaced through a thin timing provider interface instead.

10. **Namespace and naming convention violations**
    - **Location**: `src/core/ecs/components/Audio.h`, `src/core/ecs/ECS.h`
    - **Impact**: `AudioSourceComponent` and `AudioEventComponent` are declared at global scope (not in `eage::ecs`). The `ECSRegistry` field `components` lacks the `m` prefix. The struct `SceneGraphComponment` has a typo. These are correctness and readability issues.

11. **Game systems hold `RenderSystem&` for one-time setup**
    - **Location**: `src/game/systems/BulletSystem.h`, `src/game/systems/AsteroidGameplaySystem.h`
    - **Impact**: `BulletSystem` and `AsteroidGameplaySystem` store a `RenderSystem&` permanently, but only need it during `PreparePool()`/`PrepareAsteroids()` to create GPU resources. The long-lived reference couples game logic to the render system throughout the frame.

---

## Proposed Changes

### 1. Introduce an `EngineContext` struct to break up `VortexGame`

- **Goal**: Goal 1 — decouple classes
- **Approach**: Extract a plain `struct EngineContext` that holds non-owning references (or raw pointers, since all objects are owned by `VortexGame` or `App`) to engine subsystems. Pass it to `SceneController` / `AbstractScene` constructors instead of individual system references. `VortexGame` retains ownership but only wires things up once. This removes the need for `MainScene`'s five-argument constructor and makes the boundary explicit.

```cpp
// src/EngineContext.h  (new file)
struct EngineContext
{
    eage::ecs::ECSRegistry&     registry;
    eage::ecs::RenderSystem&    render_system;
    eage::ecs::PhysicsSystem&   physics_system;
    eage::ecs::AudioSystem&     audio_system;
    events::InputController&    input;
};

// Before:
MainScene( events::InputController&, ECSRegistry&, AudioSystem&, RenderSystem&, PhysicsSystem& );

// After:
MainScene( const EngineContext& ctx );
```

- **Files affected**: `src/VortexGame.cpp`, `src/game/MainScene.h/.cpp`, `src/AbstractScene.h` (add `EngineContext` forward-decl), all future scene files.

---

### 2. Replace `SDL_Keycode` with a backend-agnostic `KeyCode` enum in `InputController`

- **Goal**: Goal 2 — abstract specific APIs
- **Approach**: Define a `KeyCode` enum in `src/core/events/KeyCode.h` covering the keys the project uses. In `InputController.cpp` (not the header) translate `SDL_Keycode` to `KeyCode`. The public constructor accepts `std::unordered_map<KeyCode, uint64_t>`. SDL is isolated to the `.cpp` translation unit.

```cpp
// Before (header leaks SDL):
InputController( std::unordered_map<SDL_Keycode, uint64_t>&& keys_to_event_ids );

// After (header is SDL-free):
InputController( std::unordered_map<KeyCode, uint64_t>&& keys_to_event_ids );
```

- **Files affected**: `src/core/events/InputController.h`, `src/core/events/InputController.cpp`, `src/VortexGame.cpp` (update key bindings to use `KeyCode`).

---

### 3. Hide Vulkan from `AbstractRenderPass` using an opaque command buffer handle

- **Goal**: Goal 2 — abstract specific APIs
- **Approach**: Introduce a thin `CommandBuffer` wrapper in `src/core/graphics/CommandBuffer.h` that holds the `vk::CommandBuffer` internally but exposes no Vulkan types in its header. `AbstractRenderPass::Execute()` takes `CommandBuffer&` instead of `vk::CommandBuffer&`. An `ExecutionContext` replacement (`FrameContext`) carries only opaque handles. The Vulkan cast is done in `Renderer.cpp` before dispatch.

  > **Note**: `SceneRenderPass` and `ImGuiRenderPass` are concrete implementations that already depend on Vulkan and sit inside `src/core/graphics/`. They may keep the `vk::CommandBuffer` cast in their own `.cpp` files. The gain is that the *interface* in `AbstractRenderPass.h` no longer forces Vulkan onto every file that subclasses it.

- **Files affected**: `src/core/graphics/AbstractRenderPass.h`, `src/core/graphics/Renderer.cpp`, `src/core/graphics/SceneRenderPass.cpp`, `src/core/graphics/ImGuiRenderPass.cpp`.

---

### 4. Move all Box2D calls out of `PhysicsSystem` and into `PhysicsEngine`

- **Goal**: Goal 2 — abstract specific APIs
- **Approach**: For every `b2Body_*` call currently in `PhysicsSystem.cpp`, add a corresponding method to `PhysicsEngine` (e.g. `ApplyForce( PhysicsBody&, glm::vec2 )`, `SetLinearVelocity( PhysicsBody&, glm::vec2 )`, etc.). `PhysicsSystem` then calls those methods. Box2D headers are removed from `PhysicsSystem.cpp`'s include list. This fulfils the existing `@todo` comment.

- **Files affected**: `src/core/physics/PhysicsEngine.h/.cpp`, `src/core/ecs/systems/PhysicsSystem.cpp`.

---

### 5. Introduce `AbstractHudRenderer` to decouple `HudRenderSystem` from ImGui

- **Goal**: Goal 2 — abstract specific APIs
- **Approach**: Define a small abstract class `AbstractHudRenderer` in `src/core/ecs/systems/AbstractHudRenderer.h`. It exposes the minimal surface the HUD system needs: `DrawText(...)`, `DrawProgressBar(...)`, etc. `ImGuiRenderPass` (or a thin adaptor wrapping it) inherits from `AbstractHudRenderer`. `HudRenderSystem` takes `AbstractHudRenderer&` instead of `ImGuiRenderPass&`.

```cpp
// New abstract class (no ImGui/Vulkan headers needed):
class AbstractHudRenderer
{
public:
    virtual ~AbstractHudRenderer() = default;
    virtual void DrawText( glm::vec2 pos, const std::string& text, glm::vec4 color, HudFontSize size ) = 0;
    virtual void DrawProgressBar( glm::vec2 pos, glm::vec2 size, float fraction,
                                  glm::vec4 fill, glm::vec4 bg ) = 0;
};
```

- **Files affected**: `src/core/ecs/systems/AbstractHudRenderer.h` (new), `src/core/ecs/systems/HudRenderSystem.h/.cpp`, `src/core/graphics/ImGuiRenderPass.h/.cpp` (inherit from `AbstractHudRenderer`), `src/VortexGame.cpp`.

---

### 6. Introduce `AbstractGPUTimingProvider` to decouple `PerformanceTracker` from `Renderer`

- **Goal**: Goal 1 — decouple classes
- **Approach**: Define a one-method abstract class `AbstractGPUTimingProvider { virtual float GetGPUFrameTime() const = 0; }` in `src/core/profiling/AbstractGPUTimingProvider.h`. `Renderer` inherits from it. `PerformanceTracker` takes `AbstractGPUTimingProvider&` instead of `graphics::Renderer&`. The profiling module no longer drags in graphics headers.

- **Files affected**: `src/core/profiling/AbstractGPUTimingProvider.h` (new), `src/core/profiling/PerformanceTracker.h/.cpp`, `src/core/graphics/Renderer.h`, `src/VortexGame.cpp`.

---

### 7. Centralise delta time and pass it through the system update chain

- **Goal**: Goal 3 — reduce boilerplate, correct behaviour
- **Approach**: In `VortexGame::Run()`, compute `float delta_time` once per frame using `std::chrono`. Pass it to every `Update( float dt )` call: `mSceneController->Update( dt )`, `mPhysicsSystem->Update( dt )`, `mAudioSystem->Update( dt )`. Remove the per-scene `mLastUpdateTime` member from `MainScene`; its `Update( float dt )` receives the time already computed. This also fixes `AudioSystem::Update( 0.f )`.

- **Files affected**: `src/VortexGame.cpp`, `src/AbstractScene.h` (change `Update()` signature to `Update( float dt )`), `src/SceneController.h/.cpp`, `src/game/MainScene.h/.cpp`, `src/core/ecs/systems/PhysicsSystem.h/.cpp`, game systems that use delta time.

---

### 8. Connect `SceneController` to `SceneGraphSystem` via an observer

- **Goal**: Goal 1 — decouple classes; prevent silent bugs
- **Approach**: Add a nested `Observer` class to `SceneController`. It exposes a single pure-virtual method `OnSceneChanged( uint64_t scene_root )` that fires whenever `ChangeScene()` activates a new scene. `SceneController` holds a list of registered observers and notifies all of them. `SceneGraphSystem` implements `SceneController::Observer` and calls `SetSceneRoot()` in response. `VortexGame` registers `mSceneGraphSystem` as an observer after both are constructed. Any future listener (e.g. a loading screen system) subscribes independently without touching `SceneController`.

```cpp
// SceneController:
class Observer
{
public:
    virtual void OnSceneChanged( uint64_t scene_root ) = 0;
};

void RegisterObserver( Observer* observer );
void UnregisterObserver( Observer* observer );

// SceneGraphSystem (inherits SceneController::Observer):
void OnSceneChanged( uint64_t scene_root ) override
{
    SetSceneRoot( scene_root );
}

// VortexGame::Init():
mSceneController->RegisterObserver( mSceneGraphSystem.get() );
```

- **Files affected**: `src/SceneController.h/.cpp`, `src/core/ecs/systems/SceneGraphSystem.h/.cpp`, `src/VortexGame.cpp`.

---

### 9. Switch ECS component storage to dense arrays

- **Goal**: Goal 4 — improve system efficiency
- **Approach**: Replace `std::unordered_map<Entity, T>` per component type with a `std::vector<T>` (dense array) plus a sparse index (`std::vector<uint32_t>` or a paged lookup). This is the standard "sparse-set" or "pool allocator" pattern used by EnTT and others. Entity IDs become indices into the sparse index, which stores the dense index. Iteration in systems (which is the hot path) becomes a straight `for` loop over a contiguous `vector<T>`, eliminating all hash-map overhead and putting components in cache-friendly order.

  A minimal implementation:
  ```cpp
  template<typename T>
  class ComponentPool
  {
      std::vector<T>        mDense;
      std::vector<uint32_t> mEntities;     // dense index -> entity
      std::vector<uint32_t> mSparse;       // entity    -> dense index (UINT32_MAX = absent)
  public:
      void  Add( Entity e, T&& c );
      bool  Has( Entity e ) const;
      T&    Get( Entity e );
      void  Remove( Entity e );
      // Iteration: range over mDense directly
  };
  ```

  The `ECSRegistry` replaces `std::unordered_map<std::type_index, std::any>` with `std::unordered_map<std::type_index, std::unique_ptr<AbstractComponentPool>>` where `AbstractComponentPool` is a type-erased abstract base class. The `std::any_cast` round-trip disappears.

  > **Migration note**: All callers that use `GetComponentMap<T>()` and iterate over the `std::unordered_map` will need updating to the new iteration API. This is the most impactful change and touches many files, so it should be done last.

- **Files affected**: `src/core/ecs/ECS.h`, all `PhysicsSystem.cpp`, `RenderSystem.cpp`, `AudioSystem.cpp`, `SceneGraphSystem.cpp`, and any game system that iterates components.

---

### 10. Fix namespace and naming conventions

- **Goal**: Goal 3 — readability / consistency
- **Approach**:
  1. Move `AudioSourceComponent` and `AudioEventComponent` into `namespace eage::ecs` in `Audio.h`.
  2. Rename `ECSRegistry::components` → `mComponents` in `ECS.h`.
  3. Rename `SceneGraphComponment` → `SceneGraphComponent` in `Basics.h`.
  4. Update all usages.

- **Files affected**: `src/core/ecs/components/Audio.h`, `src/core/ecs/ECS.h`, `src/core/ecs/components/Basics.h`, and all `.cpp` files that reference these types by name.

---

### 11. Pass `RenderSystem` only during construction of game systems (not stored)

- **Goal**: Goal 1 — decouple classes
- **Approach**: `BulletSystem::PreparePool()` and `AsteroidGameplaySystem::PrepareAsteroids()` are the only call sites that need `RenderSystem`. Extract a `BulletSystemConfig` / `AsteroidConfig` struct that is built externally (in `MainScene`) using `RenderSystem`, and pass the pre-built GPU resource IDs (`ResourceId`) into these systems. The systems then store only `ECSRegistry&` and `PhysicsSystem&`. This eliminates the `mRenderSystem` member entirely from both classes.

- **Files affected**: `src/game/systems/BulletSystem.h/.cpp`, `src/game/systems/AsteroidGameplaySystem.h/.cpp`, `src/game/MainScene.cpp`.

---

## Sequencing

Ordered from least to most disruptive (each step leaves the project compiling):

1. **Fix namespace/naming** (Change 10) — pure cosmetic, no logic change.
2. **`SceneController` observer** (Change 8) — small, self-contained, fixes a latent bug.
3. **Centralise delta time** (Change 7) — touches `Update()` signatures broadly but all changes are mechanical.
4. **`IGPUTimingProvider`** (Change 6) — isolated to profiling and renderer.
5. **`InputController` `KeyCode`** (Change 2) — isolated to events module + one `VortexGame.cpp` call site.
6. **Box2D isolation** (Change 4) — moves code between `.cpp` files only, no header changes for consumers.
7. **`IHudRenderer`** (Change 5) — introduces interface, single `VortexGame` wiring change.
8. **`EngineContext` struct** (Change 1) — reduces constructor parameter lists; depends on delta time being centralised (step 3).
9. **Game systems decouple from `RenderSystem`** (Change 11) — depends on `EngineContext` being established.
10. **`AbstractRenderPass` / Vulkan hiding** (Change 3) — moderate impact on graphics internals.
11. **ECS dense storage** (Change 9) — largest change; done last once all other refactors stabilise the API surface.

---

## Out-of-Scope Dependencies

- **`ManagedVulkanResources.h` leaks Vulkan `VmaAllocation` types**: `VortexGame.cpp` includes this header directly. A follow-up refactoring could wrap VMA allocation handles behind a `GpuBuffer` / `GpuImage` RAII type that hides VMA. Not addressed here because it requires changes deep inside the graphics module and is not blocking any of the items above.
- **Font loading in `ImGuiRenderPass`**: `mImGuiPass->LoadFont(...)` is called with `HudFontSize` enum values, coupling `ImGuiRenderPass` to an ECS type. If `IHudRenderer` (Change 5) is implemented, font configuration should move there too.
- **`ResourceManager` reference counting**: The `ResourceManager` template has `AddReference`/`RemoveReference` methods, but the ECS currently holds `ResourceId` values in components without calling these. Reference counting is effectively unused. A future cleanup should either enforce it or remove the mechanism.

---

## Risk & Notes

- **ECS dense storage (Change 9)** is the riskiest change. It modifies the lowest-level data structure that every system depends on. It must be done on its own branch with a full regression pass. The existing `GetComponentMap<T>()` API can be kept as a compatibility shim returning a view over the dense array, reducing the number of call-site changes needed in a first pass.
- **Delta time and physics**: `PhysicsSystem` likely uses Box2D's internal fixed-step mechanism. Adding a `dt` parameter to `PhysicsSystem::Update( float dt )` allows the system to accumulate time and step multiple times per frame if needed (standard fixed-timestep accumulator pattern). Do not simply pass `dt` to Box2D's `b2World_Step` without checking for spiral-of-death (clamp `dt` to a maximum before accumulation).
- **`SDL_Window` lifetime**: `mWindow` is a `std::shared_ptr<SDL_Window>` with a custom deleter. Nothing else shares ownership. This can be `std::unique_ptr` — using `shared_ptr` implies shared ownership that doesn't exist and carries an unnecessary atomic reference count on every access. This is a minor point but worth fixing when touching `VortexGame.h`.
- **`SceneController::mCurrentScene` is a raw observer pointer** into `mScenes`. Ensure `FreeAllScenes()` nulls `mCurrentScene` to avoid a dangling pointer. Currently this is safe because `FreeAllScenes()` is only called in `VortexGame::Run()` just before the destructor, but it is fragile.
