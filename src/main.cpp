#include "core/GameContext.h"
#include "audio/ProceduralAudio.h"
#include "gameplay/Gameplay.h"
#include "persistence/SaveSystem.h"
#include "rendering/WorldRenderer.h"
#include "ui/Interface.h"

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Arcane Realm 3D: Chronicles of the Mage");
    SetExitKey(KEY_NULL); // Prevent Raylib from closing window on ESC!
    SetTargetFPS(60);

    InitializeGameAudio();

    LoadSettingsFromFile();
    InitPerks();
    ResetGame();

    InitializeWorld();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        int curW = GetScreenWidth();
        int curH = GetScreenHeight();

        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
            G.settings.isFullscreen = IsWindowFullscreen();
        }

        if (G.state == STATE_PLAYING) {
            UpdateGameplay(dt);
        }

        BeginDrawing();
        ClearBackground(Color{ 10, 8, 18, 255 });

        DrawWorld();
        DrawInterface(curW, curH);

        EndDrawing();
    }

    ShutdownGameAudio();
    CloseWindow();
    return 0;
}
