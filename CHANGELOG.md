# Changelog

## 1.1.0 — 2026-10-30

### Combat and progression

- Frost followed by a fireball explosion triggers Shatter: 50% bonus explosion
  damage and consumed freeze. Each Frost Nova hits each enemy once, including
  the Frost Wraith's reduced-damage hit.
- Blink works at zero mana. Fleetfoot reduces its base 2.8-second cooldown toward
  a 1.2-second minimum, in addition to increasing movement speed.
- Golem slams show a fixed 3.2m ground warning for 0.7 seconds. The golem stops
  during windup; freezing interrupts the attack.
- Triad stacks through 3, 5 and 7 fireballs, then leaves the perk selection pool.
  Absolute Zero adds 2 seconds of freeze and 30% base frost damage per choice.
  Mana Wellspring adds 100% base mana regeneration and 40 maximum mana per choice.
- Multiple earned levels present their perk choices sequentially.
- Dead frozen enemies now resolve kills, score and drops before AI runs.
  Necromancer summons are deferred and limited to 48 living enemies; the limit
  affects summoning, not the wave director.
- Enemy hits grant 0.35 seconds of protection against overlapping damage.

### Systems and distribution

- Split application, audio, gameplay, enemy AI, persistence, rendering and UI into
  source modules. Move unchanged bundled dependencies under `third_party/`.
- Pin Raylib 6.0, add CMake debug/release presets and Linux/Windows CI, and check
  bundled Windows library compatibility before selecting it.
- Preserve saved runs on startup and Continue, restore defaults for incomplete
  settings, and correct the volume percentage display and title camera.
- Add gameplay/save regression checks, bilingual setup guides, architecture
  documentation and a Persian graphics roadmap.
- Package Windows x64 and Linux x86_64 builds with licenses, runtime notes and
  SHA-256 checksums.

### Saves

The native binary save/settings layouts remain unchanged. Existing saves from
the same platform/compiler ABI retain player progression and perks; Continue
restarts the saved wave. Back up `savegame.dat` and `settings.dat` before upgrading.
New perk formulas apply to loaded perk values. Pending extra levels are offered
after resuming play. Cross-platform save portability is not guaranteed.

## 1.0.0

- Initial native C++17/Raylib release with eight enemy types, four spells,
  procedural geometry/audio, waves, perks and saved runs.
