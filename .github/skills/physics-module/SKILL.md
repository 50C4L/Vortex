---
name: physics-module
description: "USE WHEN: writing, reading, or modifying code in src/core/physics/. Covers PhysicsEngine (Box2D wrapper), PhysicsBody (opaque handle), PhysicsEventListener (collision callback interface), and CollisionFilter. Includes unit conversion, collision filtering, and DOs/DON'Ts."
---

# Physics Module

## Purpose

Thin C++ wrapper over [Box2D v3](https://box2d.org/). `PhysicsEngine` manages the world, creates bodies and colliders, steps the simulation, and fires collision/sensor events via `PhysicsEventListener`. The ECS `PhysicsSystem` sits above this module and bridges it to the component system.

## Key Classes

| Class | Responsibility |
|---|---|
| `PhysicsEngine` | Owns `b2WorldId`; creates bodies and colliders; steps simulation; fires events |
| `PhysicsBody` | Opaque wrapper around `b2BodyId`; non-owning handle (world owns the body) |
| `PhysicsEngine::CollisionFilter` | Category bits / mask bits / group index for layered collision filtering |
| `PhysicsEngine::PhysicsBodyTransform` | `position` (vec3) + `rotation` (quat) — output of `GetBodyTransform()` |
| `PhysicsEventListener` | Abstract interface for collision/sensor callbacks; implemented by `PhysicsSystem` |

## API

```cpp
// Initialization (done by PhysicsSystem)
engine.CreateWorld( { 0.f, -9.8f } );  // gravity in m/s²

// Body creation
b2BodyDef body_def = b2DefaultBodyDef();
body_def.type = b2_dynamicBody;
auto body = engine.CreateBody( body_def );   // returns unique_ptr<PhysicsBody>

// Attach colliders
PhysicsEngine::CollisionFilter filter{ category, mask, group };
engine.AddCircleColliderToBody( *body, radius_meters, filter );
engine.AddBoxColliderToBody( *body, half_width, half_height, filter );

// Per-frame step (called by PhysicsSystem::Update)
engine.Update();  // steps simulation + fires events

// Query state
auto t = engine.GetBodyTransform( *body );  // t.position, t.rotation
engine.UpdateBodyTransform( *body, position_pixels, rotation_quat );

// Events
engine.SetEventListener( &my_listener );
engine.ClearEventListener();

// User data (store entity ID for collision lookup)
engine.GetUserData( *body );  // returns void*
```

## Collision Filtering

Box2D standard 3-layer filter:

| Field | Meaning |
|---|---|
| `category` | Which group this body belongs to (bitmask, one bit per group) |
| `mask` | Which categories this body collides with |
| `group` | Positive = always collide with same group; negative = never collide; 0 = use category/mask |

Example:
```cpp
// Ship (category 0x0001) collides with enemies (0x0002) and bullets (0x0004)
CollisionFilter ship_filter{ 0x0001, 0x0002 | 0x0004, 0 };
// Enemy bullet (category 0x0004) collides with ship only
CollisionFilter enemy_bullet_filter{ 0x0004, 0x0001, 0 };
```

## PhysicsEventListener Interface

```cpp
class MyListener : public PhysicsEventListener
{
    void OnCollideBegin( PhysicsBody* a, PhysicsBody* b ) override {}
    void OnCollideEnd( PhysicsBody* a, PhysicsBody* b ) override {}
    void OnSensorEnter( PhysicsBody* sensor, PhysicsBody* other ) override {}
    void OnSensorExit( PhysicsBody* sensor, PhysicsBody* other ) override {}
};
```

Resolve entity from body via `engine.GetUserData( *body )` (cast to `Entity`).

## Key Patterns

- **Unit conversion:** Box2D works in meters; game code works in pixels. `PhysicsSystem` handles conversion — pass pixel values to `PhysicsSystem` APIs.
- **Non-owning handles:** `PhysicsBody` stores a `b2BodyId` (value type). The world owns the actual body. Destroying `PhysicsBody` does not destroy the Box2D body — use `PhysicsEngine` to destroy bodies explicitly.
- **User data for entity lookup:** Store the `Entity` (cast to `void*`) on bodies at creation time; retrieve it in event callbacks to identify which entity was hit.
- **Event dispatch during Update:** Events fire synchronously inside `engine.Update()`. Do not create or destroy bodies inside an event callback.

## DOs

- Set user data on all bodies at creation time — it's the only way to identify entities in collision callbacks
- Use sensor colliders (trigger zones) for area-of-effect detection instead of full collision shapes
- Let `PhysicsSystem::Update()` drive `engine.Update()` — don't call it elsewhere

## DON'Ts

- Don't create or destroy bodies inside a `PhysicsEventListener` callback — deferred to next frame
- Don't call Box2D APIs (`b2Body_*`) directly from game code — go through `PhysicsSystem` / `PhysicsComponent` queue
- Don't store `PhysicsBody*` raw pointers across frames without verifying the body is still alive
