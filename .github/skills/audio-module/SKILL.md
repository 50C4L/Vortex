---
name: audio-module
description: "USE WHEN: writing, reading, or modifying code in src/core/audio/. Covers AudioMixer (miniaudio engine wrapper) and SoundInstance (movable sound handle). Includes RAII cleanup, looping vs one-shot patterns, and DOs/DON'Ts."
---

# Audio Module

## Purpose

Thin RAII wrapper over [miniaudio](https://miniaud.io/). `AudioMixer` owns the engine and creates `SoundInstance` objects from file paths. The ECS `AudioSystem` manages `SoundInstance` pools and drives playback via `AudioEventComponent`.

## Key Classes

| Class | Responsibility |
|---|---|
| `AudioMixer` | Owns `ma_engine`; creates `SoundInstance`; RAII cleanup via unique_ptr custom deleters |
| `SoundInstance` | Wraps `ma_sound`; movable, not copyable; supports play / stop / restart |

## API

```cpp
// Mixer lifetime is managed by AudioSystem — rarely construct directly
AudioMixer mixer;

// Create a sound (loaded asynchronously by miniaudio)
SoundInstance sfx = mixer.CreateSound( "resources/sounds/laser.wav", /*looping=*/false );
SoundInstance music = mixer.CreateSound( "resources/sounds/bgm.ogg", /*looping=*/true );

// Playback control
sfx.Play();
sfx.Stop();
sfx.Restart();   // seek to 0 + Play — used for overlapping one-shot SFX
```

## Key Patterns

- **RAII:** Both `ma_engine` and `ma_sound` are held in `unique_ptr` with custom deleters that call miniaudio teardown. Never manually uninit.
- **Move semantics:** `SoundInstance` is movable for storage in `std::vector` / `ResourceManager`. Copy is deleted.
- **SoundPool (in AudioSystem):** For SFX that can overlap (e.g., bullet fire), `AudioSystem` keeps a pool of `SoundInstance` objects and round-robins `Restart()` calls instead of creating new instances each time.
- **Async loading:** Miniaudio loads audio data on a background thread. `Play()` called immediately after `CreateSound()` may produce silence for a frame — this is expected.

## DOs

- Use `Restart()` for one-shot SFX that can overlap — cheaper than creating a new `SoundInstance` each time
- Store `SoundInstance` by value in pools (it's movable) — don't heap-allocate unnecessarily
- Control audio from game code via `AudioEventComponent` queue + `AudioSystem`, not by calling `SoundInstance` methods directly

## DON'Ts

- Don't copy `SoundInstance` — it's deleted; move it instead
- Don't call `ma_engine_uninit` / `ma_sound_uninit` manually — RAII handles cleanup
- Don't construct `AudioMixer` more than once — `ma_engine` is a singleton-like resource
