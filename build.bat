@echo off
setlocal
title Arcane Realm 3D - Builder
pushd "%~dp0"
if errorlevel 1 exit /b 1

:: Keep the bundled 64-bit MinGW build available without requiring CMake.
if defined ARCANE_CXX goto compiler_found
if exist "C:\msys64\ucrt64\bin\g++.exe" (
    set "ARCANE_CXX=C:\msys64\ucrt64\bin\g++.exe"
    goto compiler_found
)
where g++ >nul 2>&1
if not errorlevel 1 (
    set "ARCANE_CXX=g++"
    goto compiler_found
)
if exist "C:\mingw64\bin\g++.exe" (
    set "ARCANE_CXX=C:\mingw64\bin\g++.exe"
    goto compiler_found
)
if exist "C:\msys64\mingw64\bin\g++.exe" (
    set "ARCANE_CXX=C:\msys64\mingw64\bin\g++.exe"
    goto compiler_found
)
echo [ERROR] Install MSYS2 UCRT64 GCC or add a 64-bit UCRT g++ to PATH.
goto failed

:compiler_found
"%ARCANE_CXX%" -dumpmachine | findstr /b /c:"x86_64" >nul
if errorlevel 1 (
    echo [ERROR] The bundled Raylib library requires a 64-bit MinGW-w64 compiler.
    goto failed
)
"%ARCANE_CXX%" -dM -E -x c++ -include stdlib.h NUL | findstr /b /c:"#define _UCRT " >nul
if errorlevel 1 (
    echo [ERROR] The bundled Raylib requires a UCRT compiler, such as MSYS2 UCRT64 GCC.
    echo Use CMake with -DARCANE_USE_VENDORED_RAYLIB=OFF for other compilers.
    goto failed
)
set "ARCANE_RAYLIB=third_party\raylib-6.0_win64_mingw-w64"
if not exist "%ARCANE_RAYLIB%\lib\libraylib.a" (
    echo [ERROR] The bundled Raylib library is missing. Use CMake to download Raylib.
    goto failed
)
if not exist "build\windows" mkdir "build\windows"
if errorlevel 1 goto failed

echo Building Arcane Realm 3D with %ARCANE_CXX%...
"%ARCANE_CXX%" -O2 -std=c++17 -Wall -Wextra -Isrc -I"%ARCANE_RAYLIB%\include" ^
    src\main.cpp src\core\GameContext.cpp src\audio\ProceduralAudio.cpp ^
    src\gameplay\Gameplay.cpp src\gameplay\EnemySystem.cpp src\persistence\SaveSystem.cpp ^
    src\rendering\WorldRenderer.cpp src\ui\Interface.cpp ^
    -L"%ARCANE_RAYLIB%\lib" -lraylib -lopengl32 -lgdi32 -lwinmm -static ^
    -o "build\windows\ArcaneRealm.exe"
if errorlevel 1 goto failed

echo [OK] Built build\windows\ArcaneRealm.exe
if /i "%~1"=="--no-run" goto done
start "" "build\windows\ArcaneRealm.exe"

:done
popd
exit /b 0

:failed
echo [ERROR] Build failed.
popd
exit /b 1
