cmake_minimum_required(VERSION 3.25)
project(ArcaneRealm3D
    VERSION 1.1.0
    DESCRIPTION "A native 3D magical survival action game built with C++ and Raylib"
    LANGUAGES C CXX
)

option(ARCANE_USE_VENDORED_RAYLIB "Try the bundled Raylib on compatible 64-bit MinGW" ON)
option(ARCANE_FETCH_RAYLIB "Download pinned Raylib if no compatible library is available" ON)

set(ARCANE_VENDORED_RAYLIB "${CMAKE_CURRENT_SOURCE_DIR}/third_party/raylib-6.0_win64_mingw-w64")
set(ARCANE_CAN_USE_VENDORED_RAYLIB OFF)

if(ARCANE_USE_VENDORED_RAYLIB AND WIN32 AND MINGW AND CMAKE_SIZEOF_VOID_P EQUAL 8
        AND EXISTS "${ARCANE_VENDORED_RAYLIB}/include/raylib.h"
        AND EXISTS "${ARCANE_VENDORED_RAYLIB}/lib/libraylib.a")
    # The prebuilt library uses UCRT. Probe the actual linker contract so other
    # MinGW runtimes fall back to a library built with their own compiler.
    include(CheckCSourceCompiles)
    include(CMakePushCheckState)
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_INCLUDES "${ARCANE_VENDORED_RAYLIB}/include")
    set(CMAKE_REQUIRED_LIBRARIES
        "${ARCANE_VENDORED_RAYLIB}/lib/libraylib.a" opengl32 gdi32 winmm)
    check_c_source_compiles("#include <raylib.h>
        int main(void) { InitWindow(1, 1, \"link check\"); CloseWindow(); return 0; }"
        ARCANE_VENDORED_RAYLIB_LINKS)
    cmake_pop_check_state()
    set(ARCANE_CAN_USE_VENDORED_RAYLIB "${ARCANE_VENDORED_RAYLIB_LINKS}")
    if(NOT ARCANE_CAN_USE_VENDORED_RAYLIB)
        message(STATUS "Bundled Raylib is incompatible with this toolchain; trying installed or source-built Raylib")
    endif()
endif()

if(ARCANE_CAN_USE_VENDORED_RAYLIB)
    message(STATUS "Using compatible bundled Raylib 6.0 (64-bit MinGW/UCRT)")
    add_library(raylib STATIC IMPORTED)
    set_target_properties(raylib PROPERTIES
        IMPORTED_LOCATION "${ARCANE_VENDORED_RAYLIB}/lib/libraylib.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ARCANE_VENDORED_RAYLIB}/include"
        INTERFACE_LINK_LIBRARIES "opengl32;gdi32;winmm"
    )
else()
    find_package(raylib 6.0 EXACT QUIET)
    if(NOT raylib_FOUND)
        if(NOT ARCANE_FETCH_RAYLIB)
            message(FATAL_ERROR "Raylib 6.0 was not found. Install it or enable ARCANE_FETCH_RAYLIB.")
        endif()
        include(FetchContent)
        set(BUILD_EXAMPLES OFF CACHE BOOL "Build Raylib examples" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
        # Raylib 6.0 release, pinned by commit and archive checksum.
        FetchContent_Declare(raylib
            URL https://codeload.github.com/raysan5/raylib/tar.gz/dbc56a87da87d973a9c5baa4e7438a9d20121d28
            URL_HASH SHA256=81b06ce7c19cf3b634b0271c23c361ba6ad8bf45fb8b036abbfeb4260ec1e126
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(raylib)
    endif()
endif()

# Keep upstream header diagnostics separate from project warnings.
get_target_property(ARCANE_RAYLIB_INCLUDES raylib INTERFACE_INCLUDE_DIRECTORIES)
if(ARCANE_RAYLIB_INCLUDES)
    set_property(TARGET raylib APPEND PROPERTY
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${ARCANE_RAYLIB_INCLUDES}")
endif()

add_executable(ArcaneRealm
    src/main.cpp
    src/core/GameContext.cpp
    src/audio/ProceduralAudio.cpp
    src/gameplay/Gameplay.cpp
    src/gameplay/EnemySystem.cpp
    src/persistence/SaveSystem.cpp
    src/rendering/WorldRenderer.cpp
    src/ui/Interface.cpp
)
target_compile_features(ArcaneRealm PRIVATE cxx_std_17)
set_target_properties(ArcaneRealm PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
target_include_directories(ArcaneRealm PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(ArcaneRealm PRIVATE raylib)

if(MSVC)
    target_compile_options(ArcaneRealm PRIVATE /W4)
else()
    target_compile_options(ArcaneRealm PRIVATE -Wall -Wextra)
endif()
if(MINGW)
    target_link_options(ArcaneRealm PRIVATE -static)
endif()

install(TARGETS ArcaneRealm RUNTIME DESTINATION bin)

include(CTest)
if(BUILD_TESTING)
    add_executable(ArcaneSaveTests
        tests/SaveSystemTests.cpp
        src/core/GameContext.cpp
        src/gameplay/Gameplay.cpp
        src/gameplay/EnemySystem.cpp
        src/persistence/SaveSystem.cpp
    )
    target_compile_features(ArcaneSaveTests PRIVATE cxx_std_17)
    set_target_properties(ArcaneSaveTests PROPERTIES CXX_EXTENSIONS OFF)
    target_include_directories(ArcaneSaveTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(ArcaneSaveTests PRIVATE raylib)
    add_test(NAME save-system COMMAND ArcaneSaveTests)
    set_tests_properties(save-system PROPERTIES TIMEOUT 15)

    add_executable(ArcaneCombatTests
        tests/CombatSystemTests.cpp
        src/core/GameContext.cpp
        src/gameplay/Gameplay.cpp
        src/gameplay/EnemySystem.cpp
        src/persistence/SaveSystem.cpp
    )
    target_compile_features(ArcaneCombatTests PRIVATE cxx_std_17)
    set_target_properties(ArcaneCombatTests PROPERTIES CXX_EXTENSIONS OFF)
    target_include_directories(ArcaneCombatTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(ArcaneCombatTests PRIVATE raylib)
    add_test(NAME combat-system COMMAND ArcaneCombatTests)
    set_tests_properties(combat-system PROPERTIES TIMEOUT 15)
endif()
