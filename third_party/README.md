# Third-party dependencies

## Raylib 6.0: bundled Windows MinGW distribution

- Upstream: <https://github.com/raysan5/raylib>
- Website: <https://www.raylib.com/>
- Local directory: `raylib-6.0_win64_mingw-w64/`
- License: zlib/libpng; see [the bundled LICENSE](raylib-6.0_win64_mingw-w64/LICENSE)
- Contents: Raylib headers, static/import libraries and a DLL for 64-bit MinGW with the UCRT runtime.

Use a matching **MSYS2 UCRT64** compiler. A MinGW compiler targeting the older
MSVCRT runtime cannot link this static library; configure CMake with
`-DARCANE_USE_VENDORED_RAYLIB=OFF` to build Raylib for that compiler instead.

This is the distribution previously stored at the repository root, relocated
without changing its upstream contents. It is intended for the Windows MinGW
build, not Linux, macOS, MSVC or 32-bit builds. Preserve its license and upstream
notices when redistributing it. See the bundled README for upstream details.

CMake can instead use an installed Raylib 6.0 package or fetch a pinned upstream
revision. The exact source revision is recorded in the root `CMakeLists.txt`.
Downloaded source and build outputs belong under the ignored build directory.

The project itself is covered by the root [MIT license](../LICENSE); dependency
licenses remain separate.
