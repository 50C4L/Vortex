---
name: ecs-module
description: "USE WHEN: writing, reading, or modifying code in src/core/ecs/. Covers ECSRegistry, ResourceManager, components (Transform, Render, Physics, Audio, HUD, SceneGraph), and systems (RenderSystem, PhysicsSystem, AudioSystem, SceneGraphSystem, HudRenderSystem). Includes ownership patterns, event queueing, and DOs/DON'Ts."
---

# ECS Module

## Purpose

Provides the Entity-Component-System architecture for the game layer. `ECSRegistry` is the central store. Systems operate on components each frame. Resource-heavy objects (meshes, materials, physics bodies, sounds) are held in `ResourceManager` pools and referenced by opaque `ResourceId` handles.

## Key Classes

| Class | Responsibility |
|---|---|
| `ECSRegistry` | Type-erased component store; template API for compile-time safety; uses `std::type_index` → `std::any` internally |
| `ResourceManager<PtrType>` | Generic ref-counted resource pool; stores any smart-pointer type; returns opaque `ResourceId` |
| `TransformComponent` | Position / rotation / scale with dirty-flag caching; world matrix updated by `SceneGraphSystem` |
| `SceneGraphComponent` | Parent entity ID + children list; drives hierarchical transform propagation |
| `RenderComponent` | Holds `ResourceId`s for mesh, material, descriptor, uniform buffer; UV rect for sprite control; visibility flag |
| `PhysicsComponent` | Holds body `ResourceId`; event queue for forces/impulses/velocity/position/rotation/sleep |
| `BoxColliderComponent` / `CircleColliderComponent` | Collision shape + `CollisionFilter` (category/mask/group bits) |
| `AudioSourceComponent` | Named sound source map (`string` → `ResourceId` for `SoundPool`) |
| `AudioEventComponent` | Queue of Play/Stop/Pause/Resume events keyed by source name |
| `HudTransformComponent` | Normalized screen position (0–1) + pixel offset + anchor enum |
| `HudTextComponent` | Text string + font size for ImGui HUD rendering |
| `HudBarComponent` | Progress bar value/max for HUD rendering |
| `RenderSystem` | Creates Vulkan mesh/material/image resources; attaches `RenderComponent`; syncs camera state |
| `PhysicsSystem` | Wraps `PhysicsEngine`; creates Box2D bodies; processes physics event queue; dispatches collision events via `Observer` |
| `AudioSystem` | Loads sound pools; processes `AudioEventComponent` queue each frame |
| `SceneGraphSystem` | Traverses parent-child hierarchy; propagates world matrices top-down |
| `HudRenderSystem` | Renders HUD components via ImGui; applies DPI scale factor |

## ECSRegistry API

```cpp
Entity entity = registry.CreateEntity();
registry.AddComponent<TransformComponent>( entity, TransformComponent{ ... } );

if( registry.HasComponent<RenderComponent>( entity ) )
{
    auto& render = registry.GetComponent<RenderComponent>( entity );
}

// Iterate all entities with a given component type
for( auto& [entity, transform] : registry.GetComponentMap<TransformComponent>() )
{
    // ...
}
```

## ResourceManager API

```cpp
ResourceId id = manager.Store( std::make_unique<MyResource>( ... ) );
MyResource* ptr = manager.Get( id );  // nullptr if invalid
manager.AddReference( id );
manager.RemoveReference( id );        // frees if ref count hits zero
bool live = manager.Exists( id );
```

## Physics Event Queue Pattern

Game logic queues physics commands on the component; `PhysicsSystem::Update()` drains the queue:

```cpp
auto& phys = registry.GetComponent<PhysicsComponent>( entity );
phys.QueueImpulse( { 0.f, 100.f, 0.f } );
phys.QueueSetVelocity( { 0.f, 0.f, 0.f } );
// Processed next PhysicsSystem::Update()
```

## Audio Event Queue Pattern

```cpp
auto& audio_event = registry.GetComponent<AudioEventComponent>( entity );
audio_event.events.push_back( { "thruster", AudioEvent::Type::Play } );
// Processed next AudioSystem::Update()
```

## Collision Callbacks (PhysicsSystem::Observer)

```cpp
class MyListener : public PhysicsSystem::Observer
{
    void OnCollideBegin( Entity a, Entity b ) override { /* hit logic */ }
    void OnSensorEnter( Entity sensor, Entity other ) override { /* trigger zone */ }
};

physics_system.RegisterObserver( &my_listener );
```

## Key Patterns

- **Event queueing:** Components accumulate commands during logic phase; systems drain them during `Update()`. Prevents mid-frame state corruption.
- **ResourceId indirection:** ECS never stores raw graphics/physics/audio pointers. Use `ResourceManager::Get()` to resolve, check for `nullptr`.
- **PIMPL in RenderSystem:** Public header has no Vulkan includes. All Vulkan types live in the `.cpp` impl struct.
- **Dirty-flag transforms:** Only recompute local matrix when `TransformComponent` is modified. `SceneGraphSystem` propagates world matrices.
- **Unit conversion:** `PhysicsSystem` converts pixels ↔ meters internally. Game code always works in pixel units.

## DOs

- Queue physics commands via `PhysicsComponent::Queue*()` — never modify Box2D bodies directly
- Use `registry.GetComponentMap<T>()` to iterate all entities with a given component
- Check `ResourceManager::Exists( id )` before calling `Get()` in non-critical paths
- Register `PhysicsSystem::Observer` for collision responses; don't poll physics state per frame
- Use `RenderSystem::AttachSprite()` / `AttachRenderable()` helpers for common setup

## DON'Ts

- Don't store raw pointers to components across frames — component maps can reallocate
- Don't call `PhysicsEngine` methods directly from game code — go through `PhysicsSystem`
- Don't add engine-specific (graphics/audio/physics) includes to component headers — keep them POD-friendly
- Don't manually manage `ResourceId` ref counts unless you explicitly call `AddReference()`
