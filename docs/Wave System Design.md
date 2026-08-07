# Wave System - Architecture Options and Risks

Design exploration for a JSON-driven wave system: each wave spawns a variable number of
enemies, and waves can be authored and tuned without recompiling.

This document is analysis and a recommended plan. No source files are changed by it.

---

## Executive Summary

The engine already has everything needed to drive waves from data, but one engine fact
dominates every design decision: **`ECSRegistry` cannot destroy entities, and
`PhysicsSystem` cannot destroy bodies.** Entities and Box2D bodies created at runtime live
until the process exits. Every existing spawner (`AsteroidGameplaySystem`, `BulletSystem`,
`EffectSystem`) therefore pre-creates a fixed pool and recycles it, and a wave system must
do the same or it will leak monotonically across a run.

The recommended shape is three pieces with clean responsibilities: a `WaveLibrary` that
parses and validates JSON into typed structs, an `EnemySystem` (a generalisation of today's
`AsteroidGameplaySystem`) that owns archetype-keyed entity pools and enemy behaviour, and a
`WaveSystem` that owns wave timing, state, and completion. Splitting "what an enemy is"
from "when and how many spawn" is what makes pool sizing computable at load time, which is
the linchpin of the whole design.

The biggest risks are not in the runtime, they are in the data path: silent JSON parse
failures, wave assets that are not in the scene manifest, and the fact that `resources/` is
currently gitignored so balance data would live outside version control.

---

## Current Architecture

### What a wave system has to sit on

| Piece | Where | Relevance |
|---|---|---|
| `ECSRegistry` | `src/core/ecs/ECS.h` | Entity + component store. `CreateEntity()` is `mNextEntity++`; **no `DestroyEntity`** |
| `PhysicsSystem` | `src/core/ecs/systems/PhysicsSystem.h` | Lazily creates a Box2D body when `physics.body_id == INVALID_ID`; **no removal path** |
| `AsteroidGameplaySystem` | `src/game/systems/` | The current enemy system: pre-creates 100 asteroids, recycles via `mAvailableAsteroids` / `mAllAsteroids` |
| `BulletSystem` | `src/game/systems/` | `PreparePool( config, count, root ) -> BulletPoolId`, then `Fire( pool_id, ... )`. The closest existing model for a multi-type spawner |
| `EffectSystem` | `src/core/ecs/systems/` | Same pooling shape in core: `EffectConfig { pool_size }`, `Create()` returns an id, `Apply()` plays one |
| `SceneResourceLoader` | `src/core/assets/` | Manifest dispatch. Section key -> `Delegate`, path-as-id lookup tables |
| `utility/JsonParser.h` | `src/core/utility/` | rapidjson wrappers: `parse_json_document`, `get_json_int/string/array/object` |
| `MainScene` | `src/game/` | Wires and ticks every gameplay system in `Update( dt )` |
| `StatusPanel` + `UIDataModel` | `src/game/ui/`, `src/core/ui/` | `Declare()` / `Set()` named values consumed by `hud.rml` |
| `InputController::Observer`, `PhysicsSystem::Observer` | `src/core/events/`, `src/core/ecs/systems/` | The only cross-system notification idioms. There is no general event bus |

### What is hardcoded today

`AsteroidGameplaySystem::PrepareAsteroids` bakes in exactly the values a wave designer will
want to change: texture path, `32.f x 32.f` sprite, collider radius `16.f`, `HealthComponent{ 1.f, 1.f, 0.f }`,
`RewardComponent{ 2 }`, `max_linear_velocity = 150.0f`, and in `SpawnAsteroid`, speed
`50 + rand() % 100` and angular speed `(rand() % 20) - 10`. Contact damage of `50.f` is
hardcoded in `OnCollideBegin`. This set is precisely the enemy archetype schema.

`MainScene::CreateEnemyEntities()` currently does the whole thing in three lines:
prepare 100, set the death effect, spawn 10. There is no wave concept, no timer, and no
completion condition.

The design target from `docs/Main Gameplay Loop.md` is: *"Wave ends in 30 seconds or all
enemies dead"*, with a between-waves shop and a level-up selection.

---

## Hard Constraints

These are engine facts, verified in the code, that eliminate options. They are listed first
because most of the design follows from them.

### 1. Entities are permanent

`ECSRegistry` has `CreateEntity()`, `AddComponent`, `RemoveComponent<T>`, and nothing else.
There is no `DestroyEntity`, and because component types are type-erased into
`unordered_map<type_index, ...>` with no per-entity type list, a generic teardown cannot be
written without extending the registry.

**Consequence:** any design that calls `CreateEntity()` when a wave starts leaks. Twenty
waves of forty enemies is 800 permanent entities.

### 2. Physics bodies are permanent

`PhysicsSystem::Update` creates a body when it sees `body_id == INVALID_ID` and stores it in
`mBodyManager` with a refcount of 1. Nothing ever calls `RemoveReference` for entity bodies.
`b2DestroyBody` runs only in `~PhysicsBody`, which is reached only when the whole
`PhysicsSystem` is destroyed.

**Consequence:** the leak in (1) is not just memory, it is Box2D world population.

### 3. Per-frame cost scales with pool size, not live enemy count

`PhysicsSystem::Update` iterates every `PhysicsComponent` each frame and drains its event
queue regardless of `enabled` or sleep state. `SceneGraphSystem`, `RenderSystem`, and
`AsteroidGameplaySystem::Update` similarly walk their full sets. A pool sized for the
hardest wave is paid for on wave one.

**Consequence:** pools should be sized to the computed worst case, not padded "to be safe".

### 4. Pool warm-up performs immediate GPU work

`RenderSystem::CreateMaterial`, `CreateSpriteMesh`, and `CreateTexture` go through
`Renderer::ImmediateSubmit`. Both `PrepareAsteroids` and `BulletSystem::PreparePool` carry
an explicit "performs immediate GPU operations" warning.

**Consequence:** every archetype any wave references must be prepared before gameplay
starts, or during an intermission. Never at the instant a wave begins.

### 5. JSON helpers fail soft

`get_json_int` logs `"Expected json int."` and returns `0`. `get_json_string` returns `""`.
The caller cannot distinguish "absent", "wrong type", and "legitimately zero", and the log
line names neither the file, the key, nor the surrounding object.

**Consequence:** for a system whose entire purpose is designer iteration without a compile,
this is the single highest-friction risk. It needs a validation layer, not just parsing.

### 6. `resources/` is gitignored

`.gitignore` contains a bare `resources` entry. `resources/scenes/main_scene.json` is
already untracked today; wave definitions placed alongside it would be too.

**Consequence:** game balance would have no diff, no review, and no rollback.

### 7. `rand()` is global and never seeded

There is no `srand` call anywhere in `src/`. Every run therefore produces an identical
`rand()` sequence, shared by the asteroid, bullet, and effect systems.

**Consequence:** determinism today is accidental. Adding `rand()` calls in a wave system
reshuffles the stream for every existing consumer.

---

## Architecture Choices

### Choice 1: Where the system lives

The project rule is that `src/core/` must be platform-agnostic and free of game-specific
code. A wave system needs `HealthComponent`, `RewardComponent`, `WarpComponent`, and enemy
archetype semantics, all of which are `src/game/`.

**Recommendation: `src/game/systems/`.** Do not put it in `core/ecs/systems/`.

A generic "pooled entity spawner keyed by archetype" could theoretically be lifted into
core later, but it cannot be expressed without game components today, and `EffectSystem`
already demonstrates the core-side pooling shape if that is ever wanted. Lifting it now is
speculative generality.

### Choice 2: How JSON gets loaded

**Option A - a `"waves"` section delegate on `SceneResourceLoader`.** This matches the
established pattern and everything loads on scene enter. But `Delegate::Load` fills a
`ResourceTable` of `string -> uint32_t`; wave definitions are rich nested structs, not
opaque ids. The delegate would have to stash parsed data in a side object and return
meaningless ids, which is fighting the interface. It also forces gameplay content
registration into `VortexGame::Init`, coupling the shell to wave data.

**Option B - a standalone game-layer `WaveLibrary`, loaded explicitly by `MainScene::OnEnter`
after `LoadManifest`.** Typed structs, no abuse of the resource-table interface, shell stays
free of gameplay data, and a second call site for hot-reload is trivial to add. The cost is
one extra data-loading entry point in the scene.

**Recommendation: Option B for the wave definitions, and keep the manifest for assets.**
The two are complementary: `main_scene.json` continues to list every texture, clip, and
sound that any archetype uses (per the assets-module rule that all file-backed scene assets
are declared in the manifest), and the wave files reference those assets by their exact
manifest path, reusing the existing "path is the ID" convention.

### Choice 3: Data model shape

The naive schema is `{ "wave": 1, "enemies": { "asteroid": 10 } }`. It works for a first
wave and boxes you in immediately. The recommended model has two levels:

**Enemy archetype - the "what".** Id, sprite path, sprite size, collider radius, max health,
XP reward, contact damage, speed range, max velocity, death effect. This is exactly the set
of literals currently hardcoded in `PrepareAsteroids`. Archetypes determine pool
requirements.

**Spawn group - the "when, where, how many".** Archetype id, count, start delay, interval
between individual spawns (`0` = all at once), spawn pattern.

**Wave** - name, duration limit, intermission length, and an ordered list of spawn groups.

Separating archetypes from waves is the key decision, for two reasons:

1. **Pool sizing becomes computable.** Required pool per archetype is
   `max over all waves ( sum of counts of that archetype within the wave )`. That can only
   be computed by scanning every wave, and only if archetypes are shared identities rather
   than inlined per wave.
2. **Designers reuse archetypes across dozens of waves** without copy-paste drift in health
   and reward values.

Include a `"version"` integer at every file root from day one. It is nearly free and it is
the only thing that makes a later schema migration painless.

### Choice 4: One file or one file per wave

The stated requirement is one JSON per wave, and that is the better authoring experience -
smaller diffs, fewer merge conflicts when two people tune different waves. The counter-
pressure is that pool sizing and difficulty-curve review need to see all waves at once.

**Recommendation: an index file plus per-wave files.** `resources/waves/index.json` holds
the shared archetype table and an ordered list of wave file paths;
`resources/waves/wave_01.json` and friends hold one wave each. The loader reads the index
and then every referenced wave up front, so pool sizing still works, and designers still get
one file per wave.

### Choice 5: Runtime spawn execution

Generalise today's pooling into an archetype-keyed `EnemySystem` whose API deliberately
mirrors `BulletSystem`, so it reads as native to this codebase:

```cpp
EnemyPoolId PreparePool( eage::ecs::RenderSystem& render_system, const EnemyArchetype& archetype,
						 int count, uint64_t root_entity );
bool Spawn( EnemyPoolId pool_id, glm::vec2 position, glm::vec2 velocity );
```

`AsteroidGameplaySystem` becomes this class: it keeps its `PhysicsSystem::Observer` role and
its death -> XP -> death-effect -> despawn logic, but the hardcoded literals move into
`EnemyArchetype` and the single pool becomes a map of pools.

`WaveSystem` then owns only timing and state, and calls `EnemySystem::Spawn`. The split is
"when and how many" versus "what an enemy is and what it does".

At scene enter, `MainScene` asks `WaveLibrary::ComputePoolRequirements()` and calls
`PreparePool` once per archetype. This converts a spawn failure from a silent mid-wave
no-op into a load-time diagnostic.

### Choice 6: Wave lifecycle and completion detection

A four-state machine is enough: `IDLE`, `RUNNING`, `INTERMISSION`, `FINISHED`. Spawn groups
release on their own timers while `RUNNING`; the wave ends when the live enemy count reaches
zero or `duration_sec` expires, whichever comes first.

**The timeout case forces a design decision the schema must answer:** if the timer expires
with enemies still alive, are survivors despawned or do they bleed into the next wave?
Recommendation is to despawn survivors, because it keeps pool accounting deterministic and
matches the plain reading of "wave ends in 30 seconds". Whichever is chosen, it has to be
explicit, because carry-over invalidates the worst-case pool calculation.

For live-count tracking there are two options. An `EnemySystem::Observer` callback mirroring
`PhysicsSystem::Observer` is the idiomatic choice, but `EnemySystem` despawns from inside
its own `Update` loop, so firing observers there repeats the reentrancy hazard the events
rule warns about. A `GetLiveCount()` getter polled once per frame by `WaveSystem` is simpler
and has no reentrancy risk.

**Recommendation: polling for v1**, with the observer added later if the shop and HUD banner
also need the notification.

Tick order in `MainScene::Update` matters: put `mWaveSystem->Update( dt )` *after*
`mEnemySystem->Update()` so that deaths processed this frame are reflected in the count the
wave system reads.

### Choice 7: HUD integration

Extend `StatusPanel` with `wave_number`, `wave_time_remaining`, and `enemies_remaining` via
the existing `UIDataModel::Declare` / `Set` pattern, plus the matching bindings in
`hud.rml`. No new mechanism needed.

---

## Proposed Schema

`resources/waves/index.json`:

```json
{
  "version": 1,
  "seed": 20260807,
  "enemy_archetypes": [
    {
      "id": "asteroid_large",
      "texture": "./resources/textures/asteroid/Asteroid L.png",
      "sprite_width": 32.0,
      "sprite_height": 32.0,
      "collider_radius": 16.0,
      "max_health": 1.0,
      "xp_reward": 2,
      "contact_damage": 50.0,
      "speed_min": 50.0,
      "speed_max": 150.0,
      "max_linear_velocity": 150.0,
      "angular_speed_max": 10.0,
      "death_effect": "./resources/textures/effects/anim_explosion1/animation.json"
    }
  ],
  "waves": [
    "./resources/waves/wave_01.json",
    "./resources/waves/wave_02.json"
  ]
}
```

`resources/waves/wave_01.json`:

```json
{
  "version": 1,
  "name": "First Contact",
  "duration_sec": 30.0,
  "intermission_sec": 5.0,
  "on_timeout": "DESPAWN_SURVIVORS",
  "spawn_groups": [
    { "archetype": "asteroid_large", "count": 6, "start_delay_sec": 0.0,  "interval_sec": 0.5,  "pattern": "RANDOM_EDGE" },
    { "archetype": "asteroid_large", "count": 4, "start_delay_sec": 10.0, "interval_sec": 0.25, "pattern": "RANDOM_EDGE" }
  ]
}
```

Every archetype asset path above must also appear in `resources/scenes/main_scene.json`.
See Risk 4 for how to stop those two files drifting apart.

---

## Proposed Types

Following the project coding standard: PascalCase types and member functions, `snake_case`
struct fields, `m` + PascalCase members, spaces inside parentheses, Allman braces, hard tabs.

```cpp
enum class SpawnPattern
{
	RANDOM_EDGE,
	EDGE_LEFT,
	EDGE_RIGHT,
	EDGE_TOP,
	EDGE_BOTTOM,
	RING
};

struct EnemyArchetype
{
	std::string id;
	std::string texture_path;
	std::string death_effect_path;
	float sprite_width = 32.f;
	float sprite_height = 32.f;
	float collider_radius = 16.f;
	float max_health = 1.f;
	float contact_damage = 50.f;
	float speed_min = 50.f;
	float speed_max = 150.f;
	float max_linear_velocity = 150.f;
	float angular_speed_max = 10.f;
	int xp_reward = 0;
};

struct SpawnGroup
{
	std::string archetype_id;
	int count = 0;
	float start_delay_sec = 0.f;
	float interval_sec = 0.f;
	SpawnPattern pattern = SpawnPattern::RANDOM_EDGE;
};

struct WaveDefinition
{
	std::string name;
	float duration_sec = 30.f;
	float intermission_sec = 5.f;
	bool despawn_survivors_on_timeout = true;
	std::vector<SpawnGroup> spawn_groups;
};
```

```cpp
///
/// WaveLibrary: parses and validates the wave index and every referenced wave file.
/// Pure data - no ECS, no rendering. Safe to reload and diff.
///
class WaveLibrary
{
public:
	bool Load( const std::string& index_path );

	const std::vector<EnemyArchetype>& GetArchetypes() const;
	const std::vector<WaveDefinition>& GetWaves() const;

	/// Worst-case concurrent count per archetype across all waves. Drives PreparePool sizing.
	std::unordered_map<std::string, int> ComputePoolRequirements() const;

private:
	std::vector<EnemyArchetype> mArchetypes;
	std::vector<WaveDefinition> mWaves;
	uint32_t mSeed = 0;
};
```

```cpp
///
/// WaveSystem: owns wave sequencing, spawn-group timers, and completion.
/// Knows nothing about rendering or physics - it only asks EnemySystem to spawn.
///
class WaveSystem
{
public:
	WaveSystem( eage::ecs::ECSRegistry& registry, EnemySystem& enemy_system );

	void SetLibrary( const WaveLibrary& library );
	void StartWave( int wave_index );
	void Update( float dt );

	int GetCurrentWaveIndex() const;
	float GetTimeRemaining() const;
	int GetEnemiesRemaining() const;

private:
	enum class State
	{
		IDLE,
		RUNNING,
		INTERMISSION,
		FINISHED
	};

	eage::ecs::ECSRegistry& mRegistry;
	EnemySystem& mEnemySystem;
	std::mt19937 mRng;
	State mState = State::IDLE;
};
```

Out-of-line definitions put the return type on its own line, matching the rest of the
codebase:

```cpp
void
WaveSystem::Update( float dt )
{
	...
}
```

---

## Risks

Ordered by expected cost.

### 1. Unbounded entity and physics-body growth (severity: high)

Covered in Hard Constraints 1 and 2. Nothing in the engine frees an entity or a Box2D body.

*Mitigation:* pool everything, size pools from `ComputePoolRequirements()` at scene enter,
and treat "no `CreateEntity` after scene enter" as an invariant of the gameplay layer. The
real fix - adding `ECSRegistry::DestroyEntity` plus a physics body release path - is a much
larger change (it touches every component pool, the physics `ResourceManager`, and
`SceneGraphComponent` child-list pruning) and should be a separate effort. See Out-of-Scope.

### 2. Frame cost is driven by pool size, not live enemies (severity: medium-high)

`PhysicsSystem::Update` walks every `PhysicsComponent` every frame whether asleep or not.
A pool sized for wave 30 is paid for on wave 1, and the current `unordered_set` iteration in
`AsteroidGameplaySystem::Update` compounds it with scattered hash lookups.

*Mitigation:* size pools to the computed maximum rather than padding; and while
generalising, switch the enemy update to a dense walk of
`registry.GetComponentMap<HealthComponent>()` filtered to enemies, which is the pattern the
ECS module rule already recommends. If late-game archetypes ever need to be prepared lazily,
do it during an intermission, never mid-wave (Hard Constraint 4).

### 3. Silent JSON failures (severity: high, and the one that will actually cost time)

`get_json_int` returns `0` for a missing key, a misspelled key, and a genuine zero alike,
logging only `"Expected json int."` with no file, key, or wave context. A wave file with
`"cont": 10` instead of `"count": 10` produces an empty wave and a useless log line. This is
the direct enemy of the stated goal of tuning without recompiling.

*Mitigation:* add a validation pass inside `WaveLibrary::Load` that checks required fields
are present and correctly typed, checks numeric ranges (`count > 0`, `duration_sec > 0`),
and checks that every `archetype_id` in every spawn group resolves against the archetype
table. Report failures as `file:wave:group:field`. On validation failure, refuse to start
rather than silently starting an empty wave. This layer is worth more than any other single
piece of the system.

### 4. Wave assets that are not in the scene manifest (severity: medium-high)

`SceneResourceLoader::GetTexture` logs `"not in catalog"` and returns `0` for an unknown
path. Zero is a *valid* bindless index, so the result is the wrong sprite rather than an
error. Wave JSON and `main_scene.json` would have to be kept in sync by hand.

*Mitigation, two options:*
- *Cheap:* validate at load that every archetype asset path resolves, treating `0` as
  missing. Imperfect because it conflates with a legitimate index 0.
- *Better:* have `WaveLibrary` collect archetype asset paths and feed them into the manifest
  load, making the wave file the single source of truth for enemy assets. This removes the
  class of bug entirely. Watch the ordering constraint: the manifest load must still happen
  before `UIView::LoadDocument`, because RmlUi needs fonts at parse time.

### 5. Hot-reload is only half free (severity: medium)

Changing counts, timings, and which archetype a group uses is pure data consumed at wave
start, so reload is genuinely cheap. Changing pool sizes or introducing a new archetype or
asset is not: those need GPU uploads and pool warm-up (Hard Constraint 4).

*Mitigation:* be explicit about scope. Ship reload-at-scene-enter first, which already
delivers most of the iteration benefit at zero risk. If in-session reload is added later,
apply it only at a wave boundary, and reject reloads that reference an unprepared archetype
or exceed a prepared pool, with a clear log. A reload that works 90% of the time and
occasionally corrupts pool state is worse than none.

### 6. Balance data outside version control (severity: medium)

`.gitignore` ignores `resources` wholesale. Wave definitions are the game's balance, not a
build artifact; they need diff, review, and rollback.

*Mitigation:* un-ignore the design-data subtree (a `!resources/waves/` negation, noting that
git will not descend into an ignored directory, so the parent needs to be re-included too)
or move design data to a tracked path. `resources/scenes/main_scene.json` has the same
problem today and should be fixed alongside.

### 7. Determinism (severity: low-medium, but easy to get right now and painful later)

`rand()` is never seeded, so every run is identical - convenient for balance testing, but
accidental. The asteroid, bullet, and effect systems all draw from the same global stream,
so adding wave-system `rand()` calls perturbs existing behaviour, and any future `srand`
changes every wave layout at once.

*Mitigation:* give `WaveSystem` its own `std::mt19937` seeded from the `"seed"` field in the
index file. Wave layouts then stay reproducible independently of anything else that calls
`rand()`.

### 8. Spawn-position policy against the play field (severity: low-medium)

Current spawn logic picks a point outside `PLAY_FIELD_*` and aims at a random interior
point. Two things to pin down before making patterns data-driven:

- The existing `case 2` ("Top") uses `mScreenTopLeft.y + SPAWN_AREA_PADDING`, which places
  the spawn *inside* the field, while the other three cases place it outside. Note also that
  `PLAY_FIELD_TOP` is `+360` and `PLAY_FIELD_BOTTOM` is `-360`, so "top-left" has the larger
  y - an easy sign error to replicate.
- Enemies carry `WarpComponent` and the play field is a warp sensor, so spawning outside the
  boundary depends on the warp system not teleporting them immediately on entry. Worth
  verifying explicitly once spawn positions come from data.

Consolidate spawn-position maths into one function driven by `SpawnPattern` rather than
copying the current switch.

### 9. Wave-clear detection conflating despawn reasons (severity: low)

Death is currently the only despawn path, so "live count reaches zero" works. A future
archetype that flees or times out would break that equivalence silently.

*Mitigation:* keep an explicit alive-this-wave counter decremented on despawn for *any*
reason, separate from the kill counter used for scoring and XP.

### 10. No pause concept for the between-waves shop (severity: low now, blocking later)

`docs/Main Gameplay Loop.md` puts a level-up selection and a shop between waves. Today
`MainScene::Update` ticks every gameplay system unconditionally, and `VortexGame::Run` ticks
physics, animation, and audio unconditionally afterwards. There is no way to freeze
gameplay while UI runs.

*Mitigation:* an `INTERMISSION` state that simply stops spawning is sufficient for v1. A
real pause or timescale is a separate change to the shell loop, and the wave state machine
should be shaped so it slots in without restructuring.

### 11. Contact damage is not archetype-driven (severity: low)

`AsteroidGameplaySystem::OnCollideBegin` applies a hardcoded `50.f` to the player without
regard to which enemy hit. Data-driving the rest of the archetype while leaving this behind
is a half-finished abstraction.

*Mitigation:* add a small `ContactDamageComponent` populated from the archetype at pool
preparation, and read it in the collision handler.

### 12. Endless scaling (severity: design, not technical)

A purely enumerated wave list means wave 50 needs 50 files. Decide early whether the
authored list is the whole game or a hand-tuned prefix followed by a procedural rule (for
example a top-level `"endless": { "base_wave": "...", "count_multiplier": 1.15 }` applied
after the authored waves are exhausted). It affects the schema, which is why the `"version"`
field matters.

---

## Sequencing

Each phase is independently shippable and leaves the game working.

1. **De-risk the data path.** Move wave data under version control and build the validating
   parse with contextual error messages. Doing this before any runtime work means every
   later phase gets clear diagnostics for free.
2. **Generalise `AsteroidGameplaySystem` into `EnemySystem`** with archetype-keyed pools and
   a `PreparePool` / `Spawn` API mirroring `BulletSystem`. Keep a single hardcoded archetype
   so this phase is a pure refactor with no behaviour change and is easy to verify.
3. **Add `WaveLibrary` and the schema.** Drive `PreparePool` from `ComputePoolRequirements()`
   at scene enter; the archetype fields replace the hardcoded literals from phase 2.
4. **Add `WaveSystem`** - state machine, spawn-group timers, completion detection - and wire
   it into `MainScene::Update` after the enemy system tick.
5. **HUD.** Wave number, time remaining, and enemies remaining through `StatusPanel` and
   `hud.rml`.
6. **Optional: in-session hot-reload** at wave boundaries, guarded by the archetype and pool
   checks from Risk 5.

Phases 1 and 2 are independent and can proceed in parallel.

---

## Out-of-Scope Dependencies

These are needed eventually but are not part of this system.

- **`ECSRegistry::DestroyEntity` and a physics body release path.** The real fix for the
  pooling ceiling. Touches every component pool, the physics `ResourceManager`, and
  `SceneGraphComponent` child lists. Until it exists, pool sizes are a hard cap on how large
  a wave can be.
- **Pause / timescale in the shell loop.** Required before the shop and level-up UI can
  meaningfully interrupt gameplay (Risk 10).
- **Shop, souls currency, and permanent upgrades** from the gameplay loop doc. The wave
  system should expose wave-completion state cleanly so these can hook in, and nothing more.
- **Boss and scripted waves.** These need per-group behaviour, not just counts, and would
  extend the schema with a behaviour or script reference. The archetype/spawn-group split
  leaves room for it.
- **Enemy AI beyond straight-line drift.** Everything above assumes the current "pick a
  direction and coast" behaviour. Seeking or formation movement is an `EnemySystem` concern,
  orthogonal to wave sequencing.

---

## Notes for the Implementing Agent

- Keep `WaveLibrary` free of ECS, rendering, and physics includes. It should be pure data in
  and typed structs out, which also makes it the one part of this system that is trivially
  testable.
- `WaveSystem` should not touch `RenderSystem` or `PhysicsSystem` directly. Everything goes
  through `EnemySystem::Spawn`. If `WaveSystem` needs a graphics include, the split is wrong.
- Follow the existing `PreparePool` / `Fire` naming shape from `BulletSystem` so the new API
  reads as native.
- Preserve the "performs immediate GPU operations" doc comment on any new `Prepare*` method.
  It is load-bearing information.
- ASCII only in `.cpp` and `.h` per the coding standard - no en-dashes or curly quotes in
  comments.
