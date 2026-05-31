---
name: animation-module
description: "USE WHEN: writing, reading, or modifying code in src/core/animation/. Covers AnimatedSprite (frame-based sprite animation controller). Explains how it drives RenderComponent UV rects, time accumulation, looping, and DOs/DON'Ts."
---

# Animation Module

## Purpose

Frame-based sprite animation. `AnimatedSprite` reads an `AnimationClip` and writes UV rects directly into the entity's `RenderComponent` each frame. The full atlas texture is uploaded once; the UV rect selects the current frame sub-region.

## Key Class

| Class | Responsibility |
|---|---|
| `AnimatedSprite` | Drives animation for one entity; accumulates delta time; advances frame index; writes `RenderComponent::uv_rect` |

## API

```cpp
// Construction — binds to an entity (must already have RenderComponent)
AnimatedSprite sprite( clip, entity, registry );

// Playback control
sprite.Play();
sprite.Pause();
sprite.SetLoop( true );      // default: loops
sprite.ShowFrame( 2 );       // jump to frame index (also pauses)

// Per-frame update (call from game system Update)
sprite.Update( delta_time_sec );
```

## How It Works

1. `Update()` accumulates `delta_time_sec` into `mElapsed`.
2. When `mElapsed >= frame.duration_sec`, advance to the next frame (reset elapsed).
3. Write the new frame's `uv_min` / `uv_max` into `registry.GetComponent<RenderComponent>( entity ).uv_rect`.
4. At end-of-clip: loop back to frame 0 if `mLoop == true`, else hold last frame and set `mPlaying = false`.

## Key Patterns

- **Requires `RenderComponent`:** The entity must have a `RenderComponent` before constructing `AnimatedSprite`. Missing component → undefined behaviour.
- **UV rect as viewport:** The mesh UVs are expected to span `(0,0)-(1,1)`. The `uv_rect` in `RenderComponent` acts as a per-frame sub-region multiplier in the shader.
- **Clip composition:** `AnimationClip` pre-resolves UV rects from its atlas at load time, so `Update()` is just a timer + array lookup — no JSON or atlas parsing at runtime.
- **One `AnimatedSprite` per entity:** Each instance tracks its own timer state. For entities with multiple sprite states, swap out the `AnimationClip` or create multiple `AnimatedSprite` instances and activate one at a time.

## DOs

- Call `Update( delta_time )` every frame for playing sprites — missing frames causes visible hitches
- Pair `AnimatedSprite` with `RenderSystem::AttachSprite()` which sets up the `RenderComponent` correctly
- Use `ShowFrame( 0 )` + `Pause()` to display a static frame from the atlas without running animation

## DON'Ts

- Don't manually write `RenderComponent::uv_rect` while an `AnimatedSprite` is playing — it will be overwritten next `Update()`
- Don't construct `AnimatedSprite` before the entity has a `RenderComponent`
- Don't share one `AnimatedSprite` instance across multiple entities — timer state is per-instance
