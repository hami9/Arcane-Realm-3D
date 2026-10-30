# Contributing

Thanks for helping improve Arcane Realm. English and Persian issue descriptions
are welcome.

## Development setup

Follow [Building from Source](README.md#building-from-source), then read the
[source map and architecture](docs/ARCHITECTURE.md). The
[graphics roadmap](docs/GRAPHICS_PLAN_FA.md) describes proposed visual work.

Keep gameplay changes separate from broad formatting or file moves so reviewers
can assess their effect. Use C++17, four-space indentation and the repository's
editor settings. Place new code in the module that owns the responsibility;
avoid adding substantial logic to `src/main.cpp`.

## Before opening a pull request

- Build the configuration you changed and record the exact command and platform.
  Run `ctest --preset debug` (or `release`) after building to check combat,
  progression and save/settings regressions.
- Run the game when a display is available. Check new game, movement, spells,
  pause/resume, settings, save/continue and normal exit as relevant to the change.
- For visual changes, include screenshots or a short clip at the same camera,
  resolution and scene before and after. Report hardware and quality settings for
  performance claims.
- Keep generated build files, executables and personal saves out of Git.
- Explain behavior changes and any limitations that still need checking. A
  successful compilation does not establish that interactive gameplay is correct.

Build CI is a portability check. It does not replace a manual gameplay check on
the affected platform. Add focused automated checks when they exercise a real
contract or regression; avoid tests that merely duplicate implementation details.

## Save compatibility

The current save format uses native binary structures. Do not reorder or change
serialized fields casually. A format change needs a version/migration decision
and a check using an existing save. Use a separate working directory for test
runs, or back up `savegame.dat` and `settings.dat` first.

## Dependencies and assets

Keep Raylib source downloads pinned to a reviewed revision. Do not edit the
bundled upstream library as part of ordinary gameplay work. Its distribution
details and license are listed in [third_party/README.md](third_party/README.md).

For any future external font, texture, model, shader or audio asset, record the
source URL, author, license and modifications beside the asset. Include required
license/attribution files and confirm redistribution is permitted. The current
game generates its own geometry and audio.

## Reporting problems

Include the OS, compiler/CMake version, GPU/driver when relevant, reproduction
steps, expected behavior and actual behavior. Attach build errors as text.
Mention the starting wave and settings for gameplay or rendering bugs. Remove
personal paths or other private information from logs before sharing them.
