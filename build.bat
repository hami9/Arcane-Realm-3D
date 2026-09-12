@echo off
title Arcane Realm 3D - Builder
echo =========================================================
echo    Compiling Arcane Realm 3D (Native C++17 / Raylib 6.0)
echo =========================================================

:: 1. Detect G++ compiler (prefer C:\mingw64 to avoid path-with-spaces issue on Windows)
set CXX=
if exist "C:\mingw64\bin\g++.exe" (
    set CXX=C:\mingw64\bin\g++.exe
    echo [OK] Using MinGW at C:\mingw64\bin\g++.exe
) else if exist "C:\msys64\ucrt64\bin\g++.exe" (
    set CXX=C:\msys64\ucrt64\bin\g++.exe
    echo [OK] Using MinGW UCRT at C:\msys64\ucrt64\bin\g++.exe
) else if exist "C:\msys64\mingw64\bin\g++.exe" (
    set CXX=C:\msys64\mingw64\bin\g++.exe
    echo [OK] Using MinGW64 at C:\msys64\mingw64\bin\g++.exe
) else (
    where g++ >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        set CXX=g++
        echo [OK] Using g++ from system PATH.
    ) else (
        echo [ERROR] No g++ compiler found in standard directories or PATH!
        echo Please install MinGW-W64 or add g++ to your PATH.
        pause
        exit /b 1
    )
)

set INCLUDE_DIR=.\raylib-6.0_win64_mingw-w64\include
set LIB_DIR=.\raylib-6.0_win64_mingw-w64\lib

echo [1/2] Compiling main.cpp with -O2 optimization and static linking...
"%CXX%" -O2 -std=c++17 main.cpp -I"%INCLUDE_DIR%" -L"%LIB_DIR%" -lraylib -lopengl32 -lgdi32 -lwinmm -static -o ArcaneRealm.exe

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Compilation failed!
    pause
    exit /b 1
)

echo [2/2] Build successful: ArcaneRealm.exe is ready!
echo =========================================================
echo Launching Arcane Realm 3D...
start "" ArcaneRealm.exe
