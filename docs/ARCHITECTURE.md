# Architecture

Arcane Realm is a C++17 desktop game using Raylib 6.0. Its world geometry and
audio are generated in code; no external game assets are needed to run it.

## Source map

| Location | Responsibility |
| --- | --- |
| `src/main.cpp` | Window/audio lifecycle and frame loop |
| `src/core/GameContext.h` | Shared game types and the declaration of `G` |
| `src/core/GameContext.cpp` | The single shared game context |
| `src/audio/ProceduralAudio.*` | PCM generation and procedural sound functions |
| `src/gameplay/Gameplay.*` | Movement, spells, waves, perks and effect updates |
| `src/gameplay/EnemySystem.*` | Enemy AI, attack windups, deaths and deferred summons |
| `src/persistence/SaveSystem.*` | Settings and saved-run file I/O |
| `src/rendering/WorldRenderer.*` | Arena, wizard, enemies and world effect drawing |
| `src/ui/Interface.*` | HUD, buttons, sliders and menus |
| `third_party/` | Bundled Windows MinGW Raylib distribution and its license |
| `tests/SaveSystemTests.cpp` | Save/settings regression checks without a game window |
| `tests/CombatSystemTests.cpp` | Combat, attack timing and progression regression checks |
| `docs/GRAPHICS_PLAN_FA.md` | Proposed visual direction and staged graphics work |

## Frame flow

1. Initialize the window, audio, sounds, settings, perks, gameplay and decoration.
2. Process global input such as fullscreen switching.
3. Update gameplay while the state is `STATE_PLAYING`.
4. Draw the 3D world with the game camera.
5. Draw the HUD or the current menu, then present the frame.
6. Release sounds and close the audio device/window on normal shutdown.

The game uses a variable timestep capped at 50 ms per update. The frame loop
targets 60 FPS. Neither setting is a measured performance guarantee.

## State and module boundaries

The first restructuring keeps one `GameContext G`, accessed through an `extern`
declaration. This preserves the existing data model while giving audio, rendering,
UI, gameplay and persistence separate compilation units. It is an intermediate
architecture: modules still share mutable state, and menus still process some
input while drawing.

`Gameplay` owns simulation changes. `WorldRenderer` reads that state to draw the
scene. `Interface` reads HUD data and applies menu actions through the gameplay
and persistence APIs. `SaveSystem` translates between the shared state and files.
`ProceduralAudio` produces Raylib sound resources. Keep new responsibilities in
the appropriate module and expose only functions that another module needs.

Existing enum values include title, playing, paused, settings, perk selection and
game over. The current pause interaction opens the settings screen; a dedicated
paused-screen renderer is not part of this restructuring.

## Combat and progression contracts

- Fireball explosions explicitly request Shatter; lightning explosions do not.
  A living frozen target takes 1.5 times the usual fire explosion damage and loses
  its freeze. Frost Wraiths cannot be frozen.
- Each Frost Nova records the enemy indices it has hit. Indices stay stable
  during a wave; starting a wave clears old novas before reusing those indices.
- Golem windups store their ground origin. Renderer and simulation share the
  same windup/radius constants; freezing or killing the golem cancels its attack.
- Enemy deaths resolve before movement and attacks. Summons commit after the
  enemy loop, so vector growth cannot invalidate an enemy being simulated.
- Perk selection pauses simulation. Additional earned levels open subsequent
  choices after the current selection; capped Triad is removed from the pool.
- Enemy attack previews and per-nova hit tracking are runtime state. The legacy
  `SaveData` and `GameSettings` field layouts remain unchanged.

## Persistence contract

`savegame.dat` and `settings.dat` are resolved relative to the process working
directory. Launch from the same directory to use the same files. Back them up
before changing save-related code or moving between build locations.

The legacy format writes native C++ structures directly. Its integer magic
identifier is `0x4152434E`; it does not provide a versioned, compiler-independent
serialization format. Struct order and field types therefore matter, and saves
are not guaranteed to transfer between architectures or compiler ABIs. A future
format change should add explicit versioning, validation and migration rather
than silently changing the layout.

Saved runs contain wave, player progression, resources, score and perk values.
They do not serialize every enemy, projectile or particle, so continuing a run
restarts the saved wave rather than reconstructing an exact frame.

## Build and dependencies

CMake lists translation units explicitly and uses target-scoped settings. It
selects compatible bundled Windows MinGW UCRT Raylib, an installed Raylib 6.0 package,
or a pinned upstream 6.0 source checkout. See [the build guide](../README.md#building-from-source)
for options and platform prerequisites. Preserve the upstream license when moving
or distributing the bundled library.

## Next architecture steps

- Pass explicit context references into subsystem APIs when changing those APIs.
- Separate menu input handling from drawing before adding more complex UI.
- Introduce owned renderer resources and cleanup before adding render targets,
  meshes, textures or shaders; follow the [graphics plan](GRAPHICS_PLAN_FA.md).
- Add a versioned save format as a separate, migration-aware change.

These are follow-up directions, not features implemented by the folder split.
