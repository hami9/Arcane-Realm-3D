cmake_minimum_required(VERSION 3.16)
project(ArcaneRealm3D
    VERSION 1.0.0
    DESCRIPTION "A native 3D magical survival action game built with C++ and Raylib"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Location of vendored Raylib 6.0 for MinGW
set(VENDORED_RAYLIB "${CMAKE_CURRENT_SOURCE_DIR}/raylib-6.0_win64_mingw-w64")

if(EXISTS "${VENDORED_RAYLIB}/include/raylib.h" AND WIN32 AND MINGW)
    message(STATUS "Using local vendored Raylib at: ${VENDORED_RAYLIB}")
    include_directories("${VENDORED_RAYLIB}/include")
    link_directories("${VENDORED_RAYLIB}/lib")
    set(RAYLIB_LIB "raylib")
else()
    # If not on MinGW Windows or vendored folder missing, fetch Raylib automatically
    include(FetchContent)
    message(STATUS "Fetching Raylib automatically via FetchContent...")
    FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(raylib)
    set(RAYLIB_LIB "raylib")
endif()

add_executable(ArcaneRealm main.cpp)

if(TARGET raylib)
    target_link_libraries(ArcaneRealm PRIVATE raylib)
else()
    target_link_libraries(ArcaneRealm PRIVATE ${RAYLIB_LIB} opengl32 gdi32 winmm)
    if(MINGW)
        target_link_options(ArcaneRealm PRIVATE -static)
    endif()
endif()

# Compiler optimization
if(MSVC)
    target_compile_options(ArcaneRealm PRIVATE /O2 /W3)
else()
    target_compile_options(ArcaneRealm PRIVATE -O2 -Wall -Wextra)
endif()
