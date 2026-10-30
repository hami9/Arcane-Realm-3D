# 🔮 Arcane Realm 3D: Chronicles of the Mage

[![C++17](https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=for-the-badge&logo=c%2B%2B)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Raylib](https://img.shields.io/badge/Engine-Raylib%206.0-white?style=for-the-badge&logo=raylib&logoColor=black)](https://www.raylib.com)
[![Build](https://github.com/hami9/Arcane-Realm-3D/actions/workflows/build.yml/badge.svg)](https://github.com/hami9/Arcane-Realm-3D/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](LICENSE)

A fast-paced, native **3D magical survival action game** built from scratch in **modern C++ (C++17)** with **Raylib 6.0**. Battle relentless waves of dark creatures, unleash devastating arcane spells, upgrade your wizard with rogue-lite perks, and synthesize all audio procedurally in real-time.

> 📖 **زبان فارسی**: جهت مطالعه مستندات و راهنمای بازی به زبان فارسی، به [README_FA.md](README_FA.md) مراجعه کنید.

---

## 🎮 Play the Game Instantly

Check the [Releases page](https://github.com/hami9/Arcane-Realm-3D/releases) for available Windows builds. If a release provides `ArcaneRealm.exe`, download it and launch it from a writable folder. To build the current source, follow the guide below.

---

## ✨ Key Features

### 🧙‍♂️ Arcane Spellcasting & Mobility
- **Arcane Fireball (LMB)**: Rapid-fire concentrated fireballs dealing direct impact and explosive area-of-effect damage.
- **Frost Nova (RMB / Q)**: Emits a freezing ring of runic energy, halting incoming hordes and leaving them vulnerable.
- **Thunderstrike (E)**: Summons high-voltage lightning bolts from the storm clouds to instantly disintegrate threats.
- **Blink Dash (Space / Left Shift)**: Phase through enemies with invulnerability frames and glowing particle trails.

### 👾 8 Unique Enemy Types & Custom AI
Every enemy features dedicated AI behaviors, custom 3D geometric models, animations, and attack patterns:

| Enemy | Type / Behavior |
| :--- | :--- |
| **Shadow Wisp** | Agile airborne orb that strafes and casts ranged purple void bolts. |
| **Rock Golem** | Massive, heavily-armored bruiser delivering lethal ground-pound shockwaves. |
| **Void Mage** | Teleporting dark sorcerer channeling seeking void stars. |
| **Hellfire Imp** | Fast winged menace raining persistent fire projectiles. |
| **Frost Wraith** | Ethereal specter immune to freeze effects, casting slowing frost spikes. |
| **Storm Elemental**| Electrified orb with orbiting energy conductors and rapid charging dashes. |
| **Necromancer** | Masked reaper that summons swarms of minor souls into the arena. |
| **Void Archon (Boss)** | Colossal multi-phase boss appearing every 5 waves with rotating blade rings and 5-way energy barrages. |

### 🌊 Infinite Wave Progression & Rogue-Lite Perks
- **Dynamic Wave Scaling**: Waves progress indefinitely with continuous $1.01^{(\text{Wave} - 1)}$ enemy health and damage scaling, displayed directly on your HUD.
- **Mana Crystals & Level Ups**: Vanquished foes drop glowing mana crystals. Leveling up triggers a rogue-lite perk selection (Multishot, Blast Radius, Longer Freeze, Chain Lightning, Mana Regen, Move Speed).
- **Boss Waves**: Face the Void Archon every 5 waves (5, 10, 15, 20...) for massive XP and mana rewards.

### 🎵 Real-Time Procedural Audio Synthesis
- **Zero external sound files** (`.wav` or `.mp3`) are shipped with the game.
- Every audio effect (fireballs, explosions, frost freeze, thunderclaps, blink teleports, gem chimes, level-up fanfare, and hit markers) is **mathematically synthesized in real-time** into raw PCM wave buffers using sine oscillators, exponential decay envelopes, and white-noise modulation.

### 💾 Binary Save/Load & Settings System
- **Auto-Save**: Progress is serialized to binary `savegame.dat` with magic header verification (`0x4152434E`) when waves start, bosses are defeated, or you choose **Save & Quit** from the pause/settings menu. Continuing restores player progress and restarts the saved wave.
- **Persistent Settings (`settings.dat`)**:
  - Master Volume Slider (0% – 100%)
  - Mouse Sensitivity (0.4x – 2.5x)
  - Camera Field of View (45° cinematic to 75° wide)
  - Screen Shake toggle
  - Custom Crosshair Styles (Arcane Ring, Classic Cross, Minimal Dot)
  - Fullscreen toggle (`F11`) with anti-debounce ESC menu logic.

---

## 🕹️ Controls

| Action | Key / Input |
| :--- | :--- |
| **Move Wizard** | `W` (Forward), `S` (Backward), `A` (Left), `D` (Right) |
| **Aim & Look** | Mouse Movement (360° Free Orbit) |
| **Capture / Free Mouse** | `TAB` |
| **Camera Zoom** | Mouse Scroll Wheel |
| **Cast Fireball** | Left Mouse Button (`LMB`) |
| **Cast Frost Nova** | Right Mouse Button (`RMB`) or `Q` |
| **Cast Thunderstrike** | `E` |
| **Blink Dash** | `SPACE` or `Left Shift` |
| **Pause & Settings Menu** | `ESC` |
| **Toggle Fullscreen** | `F11` |
| **Restart (Game Over)** | `R` |

---

## Building from Source

### Prerequisites

- A C++17 compiler: GCC/MinGW-w64, Clang or MSVC.
- **CMake 3.25+** and **Ninja** for the preset workflow.
- Raylib 6.0 and a desktop graphics environment. CMake uses the compatible bundled
  64-bit MinGW UCRT library on Windows, an installed Raylib 6.0 package, or a pinned
  upstream source revision. The first source download requires network access.

On Debian/Ubuntu, install the compiler and Raylib's native build dependencies:

```bash
sudo apt-get update
sudo apt-get install build-essential cmake ninja-build pkg-config \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
    libgl1-mesa-dev libasound2-dev
```

Check `cmake --version`; older distributions may need a newer CMake package.
On macOS, install Xcode Command Line Tools (`xcode-select --install`), then
install CMake and Ninja, for example with `brew install cmake ninja`.
On Windows, use the **MSYS2 UCRT64** toolchain for the bundled Raylib library.
Install its tools from the UCRT64 shell:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

For MSVC, use a Developer Command Prompt. With MSVC or a MinGW toolchain using another C runtime, configure
with `-DARCANE_USE_VENDORED_RAYLIB=OFF` so Raylib is built with that toolchain
(or use a compatible installed package). The bundled library requires UCRT.

### CMake presets

Run from the repository root:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./build/debug/bin/ArcaneRealm
```

Use `release` in place of `debug` for an optimized build. On Windows, run
`build\debug\bin\ArcaneRealm.exe` after building. Presets use Ninja; for another
generator or IDE, configure directly:

```bash
cmake -S . -B build/custom -DCMAKE_BUILD_TYPE=Release
cmake --build build/custom --config Release
```

To require a locally installed Raylib and prevent downloads, configure with
`-DARCANE_USE_VENDORED_RAYLIB=OFF -DARCANE_FETCH_RAYLIB=OFF`.
CMake reports which dependency source it selects. See
[third-party details](third_party/README.md) for licensing and platform limits.

### Quick Windows MinGW UCRT build

With a compatible 64-bit UCRT MinGW compiler, the bundled library also supports
a direct build without CMake:

```bat
.\build.bat
```

The script finds `g++` in standard MinGW UCRT locations or `PATH`, compiles the source
modules, writes `build\windows\ArcaneRealm.exe`, and launches the game. Use
`build.bat --no-run` to compile only. It can be invoked from another working
directory; the script resolves paths from its own location.

### Saves and troubleshooting

- Run from a consistent, writable working directory: `savegame.dat` and
  `settings.dat` are stored there. Keep backups before moving saves or modifying
  persistence. The legacy binary format is not guaranteed portable across
  compiler ABIs or architectures.
- On Linux, missing X11/OpenGL headers during configuration usually mean a native
  development package from the prerequisite list is missing. Running the game
  also requires a working display and graphics driver.
- If a dependency download fails, check network access or install Raylib 6.0
  locally and disable fetching as shown above.
- Build outputs and local save files are ignored by Git. CI checks compilation
  and save/settings regressions; interactive play and visual quality still
  need a desktop check.

## Project layout and development

```text
src/
  main.cpp              Application lifecycle and frame loop
  core/                 Shared game types and context
  audio/                Procedural sound synthesis
  gameplay/             Spells, enemies, waves and simulation
  persistence/          Saved runs and settings
  rendering/            World geometry and effects
  ui/                   HUD and menus
docs/                   Architecture and graphics roadmap
third_party/            Bundled Windows Raylib and upstream notices
tests/                  Save/settings regression checks
.github/                Build workflow and contribution templates
```

The source split preserves the procedural game and shared `GameContext`; further
subsystem isolation can be introduced incrementally. The graphics roadmap is a
plan for future work, not a claim that shaders or new lighting are implemented.

- [Architecture and persistence contract](docs/ARCHITECTURE.md)
- [برنامهٔ ارتقای گرافیک — Graphics roadmap (Persian)](docs/GRAPHICS_PLAN_FA.md)
- [Contributing and validation guide](CONTRIBUTING.md)

---

## 📜 License

This project is licensed under the [MIT License](LICENSE) - see the LICENSE file for details.

Developed with passion by [hami9](https://github.com/hami9) 🧙‍♂️
