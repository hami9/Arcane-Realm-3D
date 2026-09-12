# 🔮 Arcane Realm 3D: Chronicles of the Mage

[![C++17](https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=for-the-badge&logo=c%2B%2B)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Raylib](https://img.shields.io/badge/Engine-Raylib%206.0-white?style=for-the-badge&logo=raylib&logoColor=black)](https://www.raylib.com)
[![Platform](https://img.shields.io/badge/Platform-Windows%20(x64)-0078D6?style=for-the-badge&logo=windows)](https://github.com/hami9/Arcane-Realm-3D/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](LICENSE)

A fast-paced, native **3D magical survival action game** built from scratch in **modern C++ (C++17)** with **Raylib 6.0**. Battle relentless waves of dark creatures, unleash devastating arcane spells, upgrade your wizard with rogue-lite perks, and synthesize all audio procedurally in real-time.

> 📖 **زبان فارسی**: جهت مطالعه مستندات و راهنمای بازی به زبان فارسی، به [README_FA.md](README_FA.md) مراجعه کنید.

---

## 🎮 Play the Game Instantly

No compiler or dependencies needed to play! Grab the latest pre-compiled standalone executable from the Releases page:

👉 **[⬇️ Download Arcane Realm 3D (Latest Release)](https://github.com/hami9/Arcane-Realm-3D/releases/latest)**

Simply download `ArcaneRealm.exe`, double-click, and start slinging spells!

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
- **Auto-Save**: Progress is serialized to binary `savegame.dat` with magic header verification (`0x4152434E`) upon clearing waves or pausing.
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

## 🛠️ Building from Source

### Prerequisites
- **Compiler**: GCC with C++17 support (MinGW-w64 on Windows) or Clang/MSVC
- **Library**: [Raylib 6.0](https://www.raylib.com/) *(Pre-packaged 64-bit MinGW binaries are included in the repository for turnkey compilation on Windows!)*

### Option 1: Quick Build Script (Windows)
Double-click `build.bat` or run it from PowerShell/CMD:
```bat
.\build.bat
```
The script automatically detects `g++` from your `PATH` or standard installation directories, links against Raylib and OpenGL, applies `-O2` optimizations, and launches `ArcaneRealm.exe`.

### Option 2: CMake (Cross-Platform)
```bash
# Generate build files
cmake -B build -S .

# Build the game
cmake --build build --config Release
```

---

## 🏛️ Technical Highlights

- **Pure Native C++17**: Clean implementation without heavy engine runtime overhead.
- **Procedural 3D Geometry**: Character robes, wizard staff, shields, enemy models, and spell effects are rendered procedurally using Raylib's 3D primitives and `rlgl` matrix pipelines.
- **State Machine Architecture**: Clean separation between `Title`, `Playing`, `Paused`, `Settings`, `PerkSelection`, and `GameOver` states.
- **Zero Third-Party Assets**: Entirely self-contained codebase without dependencies on external image or audio files.

---

## 📜 License

This project is licensed under the [MIT License](LICENSE) - see the LICENSE file for details.

Developed with passion by [hami9](https://github.com/hami9) 🧙‍♂️
