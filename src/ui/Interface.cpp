#include "ui/Interface.h"
#include "core/GameContext.h"
#include "gameplay/Gameplay.h"
#include "persistence/SaveSystem.h"
#include "raymath.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

// Menu interaction is retained here during the incremental source split.
bool DrawArcaneButton(Rectangle bounds, const char* label, const char* sublabel, Color primaryColor, bool disabled = false) {
    Vector2 m = GetMousePosition();
    bool hovered = !disabled && CheckCollisionPointRec(m, bounds);
    bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    Color bgCol = disabled ? Color{ 30, 25, 40, 255 } :
                  hovered  ? ColorAlpha(primaryColor, 0.35f) : Color{ 24, 18, 36, 230 };
    Color borderCol = disabled ? Color{ 60, 50, 80, 255 } :
                      hovered  ? GOLD : ColorAlpha(primaryColor, 0.7f);

    DrawRectangleRounded(bounds, 0.2f, 4, bgCol);
    DrawRectangleRoundedLines(bounds, 0.2f, 4, borderCol);

    int tw = MeasureText(label, 20);
    int ty = (sublabel != nullptr && strlen(sublabel) > 0) ? (int)(bounds.y + bounds.height * 0.22f) : (int)(bounds.y + bounds.height / 2 - 10);
    DrawText(label, (int)(bounds.x + bounds.width / 2 - tw / 2), ty, 20, disabled ? DARKGRAY : (hovered ? GOLD : WHITE));

    if (sublabel != nullptr && strlen(sublabel) > 0) {
        int sw = MeasureText(sublabel, 13);
        DrawText(sublabel, (int)(bounds.x + bounds.width / 2 - sw / 2), (int)(bounds.y + bounds.height * 0.62f), 13, disabled ? GRAY : (hovered ? CYAN : LIGHTGRAY));
    }

    if (clicked) {
        PlaySound(G.sndClick);
    }
    return clicked;
}

void DrawArcaneSlider(Rectangle bounds, float* value, float minVal, float maxVal, const char* label, const char* valueFmt, float displayScale = 1.0f) {
    Vector2 m = GetMousePosition();
    bool hovered = CheckCollisionPointRec(m, bounds);

    Rectangle track = { bounds.x, bounds.y + bounds.height * 0.55f, bounds.width, 10.0f };
    DrawRectangleRounded(track, 0.5f, 4, Color{ 20, 16, 32, 255 });
    DrawRectangleRoundedLines(track, 0.5f, 4, VIOLET);

    float norm = (*value - minVal) / (maxVal - minVal);
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;

    Rectangle fill = { track.x, track.y, track.width * norm, track.height };
    DrawRectangleRounded(fill, 0.5f, 4, CYAN);

    float handleX = track.x + track.width * norm;
    DrawCircle((int)handleX, (int)(track.y + track.height * 0.5f), 10.0f, hovered ? GOLD : WHITE);
    DrawCircleLines((int)handleX, (int)(track.y + track.height * 0.5f), 10.0f, DARKPURPLE);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && hovered) {
        float newNorm = (m.x - track.x) / track.width;
        if (newNorm < 0.0f) newNorm = 0.0f;
        if (newNorm > 1.0f) newNorm = 1.0f;
        *value = minVal + newNorm * (maxVal - minVal);
    }

    DrawText(label, (int)bounds.x, (int)bounds.y, 16, RAYWHITE);
    char buf[32];
    snprintf(buf, sizeof(buf), valueFmt, *value * displayScale);
    int vw = MeasureText(buf, 16);
    DrawText(buf, (int)(bounds.x + bounds.width - vw), (int)bounds.y, 16, GOLD);
}

void DrawArcaneHUD(int screenW, int screenH) {
    int cx = screenW / 2;
    int cy = screenH / 2;

    if (G.settings.crosshairStyle == 0) {
        DrawCircleLines(cx, cy, 6.0f, ColorAlpha(CYAN, 0.7f));
        DrawCircleLines(cx, cy, 2.0f, ColorAlpha(GOLD, 0.9f));
    } else if (G.settings.crosshairStyle == 1) {
        DrawLine(cx - 9, cy, cx - 3, cy, ColorAlpha(CYAN, 0.85f));
        DrawLine(cx + 4, cy, cx + 10, cy, ColorAlpha(CYAN, 0.85f));
        DrawLine(cx, cy - 9, cx, cy - 3, ColorAlpha(CYAN, 0.85f));
        DrawLine(cx, cy + 4, cx, cy + 10, ColorAlpha(CYAN, 0.85f));
    } else {
        DrawCircle(cx, cy, 3.0f, ColorAlpha(CYAN, 0.9f));
    }

    if (G.hitmarkerTimer > 0.0f) {
        float alpha = G.hitmarkerTimer / 0.15f;
        Color hmCol = ColorAlpha(GOLD, alpha);
        DrawLine(cx - 7, cy - 7, cx - 3, cy - 3, hmCol);
        DrawLine(cx + 3, cy + 3, cx + 7, cy + 7, hmCol);
        DrawLine(cx + 7, cy - 7, cx + 3, cy - 3, hmCol);
        DrawLine(cx - 7, cy + 7, cx - 3, cy + 3, hmCol);
    }

    if (G.hitFlash > 0.0f) {
        DrawRectangle(0, 0, screenW, screenH, ColorAlpha(RED, G.hitFlash * 0.5f));
    }

    // Top-Left: Player Vitals & XP
    int barX = 24;
    int barY = 24;
    int barW = 260;

    DrawRectangleRounded(Rectangle{ (float)barX - 10, (float)barY - 10, (float)barW + 20, 112 }, 0.15f, 6, ColorAlpha(Color{ 18, 14, 28, 255 }, 0.85f));
    DrawRectangleRoundedLines(Rectangle{ (float)barX - 10, (float)barY - 10, (float)barW + 20, 112 }, 0.15f, 6, ColorAlpha(GOLD, 0.6f));

    char lvlBuf[32];
    snprintf(lvlBuf, sizeof(lvlBuf), "LVL %d", G.playerLevel);
    DrawRectangleRounded(Rectangle{ (float)barX, (float)barY, 65, 24 }, 0.3f, 4, DARKPURPLE);
    DrawRectangleRoundedLines(Rectangle{ (float)barX, (float)barY, 65, 24 }, 0.3f, 4, MAGENTA);
    DrawText(lvlBuf, barX + 10, barY + 5, 16, GOLD);

    char scoreBuf[48];
    snprintf(scoreBuf, sizeof(scoreBuf), "SCORE: %d | KILLS: %d", G.score, G.kills);
    DrawText(scoreBuf, barX + 75, barY + 6, 13, RAYWHITE);

    float hpPercent = fmaxf(0.0f, G.playerHp / G.playerMaxHp);
    DrawRectangle(barX, barY + 32, barW, 16, Color{ 50, 15, 20, 255 });
    DrawRectangle(barX, barY + 32, (int)(barW * hpPercent), 16, Color{ 220, 35, 50, 255 });
    DrawRectangleLines(barX, barY + 32, barW, 16, ColorAlpha(RED, 0.6f));
    char hpBuf[32];
    snprintf(hpBuf, sizeof(hpBuf), "HP %d/%d", (int)G.playerHp, (int)G.playerMaxHp);
    DrawText(hpBuf, barX + 8, barY + 33, 13, WHITE);

    float manaPercent = fmaxf(0.0f, G.playerMana / G.playerMaxMana);
    DrawRectangle(barX, barY + 54, barW, 14, Color{ 10, 30, 60, 255 });
    DrawRectangle(barX, barY + 54, (int)(barW * manaPercent), 14, Color{ 0, 175, 240, 255 });
    DrawRectangleLines(barX, barY + 54, barW, 14, ColorAlpha(CYAN, 0.6f));
    char manaBuf[32];
    snprintf(manaBuf, sizeof(manaBuf), "MANA %d/%d", (int)G.playerMana, (int)G.playerMaxMana);
    DrawText(manaBuf, barX + 8, barY + 55, 12, WHITE);

    float xpPercent = fminf(1.0f, (float)G.playerXp / (float)G.playerNextXp);
    DrawRectangle(barX, barY + 74, barW, 8, Color{ 40, 35, 15, 255 });
    DrawRectangle(barX, barY + 74, (int)(barW * xpPercent), 8, GOLD);
    DrawRectangleLines(barX, barY + 74, barW, 8, ColorAlpha(GOLD, 0.5f));

    // Top-Center: Wave Announcement & Power Multiplier
    float powMult = powf(1.01f, (float)(G.wave - 1));
    if (G.waveIntroTimer > 0.0f) {
        float alpha = fminf(1.0f, G.waveIntroTimer);
        const char* wTitle = (G.wave % 5 == 0) ? "BOSS BATTLE: THE VOID ARCHON" : "WAVE INCOMING";
        int tw = MeasureText(wTitle, 28);
        DrawText(wTitle, screenW / 2 - tw / 2, 70, 28, ColorAlpha(GOLD, alpha));

        char wSub[48];
        snprintf(wSub, sizeof(wSub), "WAVE %d  |  ENEMY POWER: x%.2f", G.wave, powMult);
        int sw = MeasureText(wSub, 20);
        DrawText(wSub, screenW / 2 - sw / 2, 105, 20, ColorAlpha(RAYWHITE, alpha));
    } else {
        char wInfo[48];
        snprintf(wInfo, sizeof(wInfo), "WAVE %d (x%.2f)", G.wave, powMult);
        int tw = MeasureText(wInfo, 20);
        DrawRectangleRounded(Rectangle{ (float)(screenW / 2 - tw / 2 - 16), 18, (float)(tw + 32), 34 }, 0.3f, 4, ColorAlpha(DARKPURPLE, 0.85f));
        DrawRectangleRoundedLines(Rectangle{ (float)(screenW / 2 - tw / 2 - 16), 18, (float)(tw + 32), 34 }, 0.3f, 4, GOLD);
        DrawText(wInfo, screenW / 2 - tw / 2, 25, 20, GOLD);
    }

    if (G.bossActive) {
        for (auto& e : G.enemies) {
            if (e.active && e.type == ENEMY_BOSS) {
                int bbW = 440;
                int bbH = 20;
                int bbX = screenW / 2 - bbW / 2;
                int bbY = 62;
                float bHpP = fmaxf(0.0f, e.hp / e.maxHp);

                DrawRectangle(bbX, bbY, bbW, bbH, Color{ 35, 10, 25, 255 });
                DrawRectangle(bbX, bbY, (int)(bbW * bHpP), bbH, Color{ 190, 20, 70, 255 });
                DrawRectangleLines(bbX, bbY, bbW, bbH, GOLD);

                const char* bName = "VOID ARCHON (BOSS)";
                int nw = MeasureText(bName, 14);
                DrawText(bName, screenW / 2 - nw / 2, bbY + 3, 14, RAYWHITE);
                break;
            }
        }
    }

    if (G.combo > 1) {
        char cBuf[32];
        snprintf(cBuf, sizeof(cBuf), "COMBO x%d!", G.combo);
        DrawText(cBuf, screenW - 180, 110, 24, GOLD);
    }

    // Top-Right: Minimap Radar
    int radarR = 60;
    int radarCX = screenW - radarR - 24;
    int radarCY = radarR + 24;

    DrawCircle(radarCX, radarCY, radarR, ColorAlpha(Color{ 15, 12, 28, 255 }, 0.85f));
    DrawCircleLines(radarCX, radarCY, radarR, ColorAlpha(VIOLET, 0.7f));
    DrawCircleLines(radarCX, radarCY, radarR * 0.5f, ColorAlpha(VIOLET, 0.3f));
    DrawCircle(radarCX, radarCY, 3.5f, CYAN);

    float radarScale = (float)radarR / 36.0f;
    for (auto& e : G.enemies) {
        if (!e.active) continue;
        Vector3 rel = Vector3Subtract(e.pos, G.playerPos);
        int ex = radarCX + (int)(rel.x * radarScale);
        int ey = radarCY + (int)(rel.z * radarScale);
        int dx = ex - radarCX;
        int dy = ey - radarCY;
        if (dx * dx + dy * dy < radarR * radarR) {
            Color bCol = (e.type == ENEMY_BOSS) ? RED : ((e.type == ENEMY_GOLEM) ? ORANGE : ((e.type == ENEMY_PYRO) ? MAROON : PURPLE));
            DrawCircle(ex, ey, (e.type == ENEMY_BOSS) ? 4.5f : 2.5f, bCol);
        }
    }

    for (auto& g : G.gems) {
        if (!g.active) continue;
        Vector3 rel = Vector3Subtract(g.pos, G.playerPos);
        int gx = radarCX + (int)(rel.x * radarScale);
        int gy = radarCY + (int)(rel.z * radarScale);
        int dx = gx - radarCX;
        int dy = gy - radarCY;
        if (dx * dx + dy * dy < radarR * radarR) {
            DrawCircle(gx, gy, 1.5f, GOLD);
        }
    }

    // Bottom-Center: Hotbar with Cooldowns
    int slotSize = 64;
    int slotSpacing = 16;
    int totalSlots = 4;
    int hotbarW = totalSlots * slotSize + (totalSlots - 1) * slotSpacing;
    int hotbarX = screenW / 2 - hotbarW / 2;
    int hotbarY = screenH - slotSize - 24;

    struct SpellSlot {
        const char* key;
        const char* name;
        Color color;
        float cd;
        float maxCd;
    } slots[4] = {
        { "LMB", "Fireball", ORANGE, G.cdFireball, 0.28f },
        { "RMB/Q", "Frost Nova", SKYBLUE, G.cdFrost, 5.0f },
        { "E", "Thunder", PURPLE, G.cdThunder, 6.5f },
        { "SPACE", "Blink", MAGENTA, G.cdBlink, 2.8f }
    };

    for (int i = 0; i < 4; i++) {
        int sx = hotbarX + i * (slotSize + slotSpacing);
        int sy = hotbarY;

        DrawRectangleRounded(Rectangle{ (float)sx, (float)sy, (float)slotSize, (float)slotSize }, 0.2f, 4, ColorAlpha(Color{ 20, 16, 35, 255 }, 0.9f));
        DrawRectangleRoundedLines(Rectangle{ (float)sx, (float)sy, (float)slotSize, (float)slotSize }, 0.2f, 4, slots[i].color);

        if (i == 0) DrawCircle(sx + slotSize / 2, sy + slotSize / 2 - 4, 14, ORANGE);
        else if (i == 1) DrawPoly(Vector2{ (float)(sx + slotSize / 2), (float)(sy + slotSize / 2 - 4) }, 6, 15, 0, SKYBLUE);
        else if (i == 2) {
            int mx = sx + slotSize / 2;
            int my = sy + slotSize / 2 - 4;
            DrawLineEx(Vector2{ (float)mx - 4, (float)my - 12 }, Vector2{ (float)mx + 2, (float)my - 2 }, 3.0f, PURPLE);
            DrawLineEx(Vector2{ (float)mx + 2, (float)my - 2 }, Vector2{ (float)mx - 2, (float)my + 2 }, 3.0f, PURPLE);
            DrawLineEx(Vector2{ (float)mx - 2, (float)my + 2 }, Vector2{ (float)mx + 6, (float)my + 12 }, 3.0f, PURPLE);
        } else if (i == 3) {
            DrawRing(Vector2{ (float)(sx + slotSize / 2), (float)(sy + slotSize / 2 - 4) }, 7.0f, 13.0f, 0, 360, 16, MAGENTA);
        }

        if (slots[i].cd > 0.0f) {
            float ratio = slots[i].cd / slots[i].maxCd;
            DrawRectangle(sx, sy, slotSize, (int)(slotSize * ratio), ColorAlpha(BLACK, 0.72f));
            char cdText[16];
            snprintf(cdText, sizeof(cdText), "%.1f", slots[i].cd);
            int cdw = MeasureText(cdText, 14);
            DrawText(cdText, sx + slotSize / 2 - cdw / 2, sy + slotSize / 2 - 8, 14, WHITE);
        }

        int kw = MeasureText(slots[i].key, 11);
        DrawRectangleRounded(Rectangle{ (float)(sx + slotSize / 2 - kw / 2 - 4), (float)(sy + slotSize - 14), (float)(kw + 8), 14 }, 0.3f, 2, Color{ 35, 25, 55, 255 });
        DrawText(slots[i].key, sx + slotSize / 2 - kw / 2, sy + slotSize - 13, 11, RAYWHITE);
    }

    for (auto& dt : G.damageTexts) {
        if (!dt.active) continue;
        Vector2 sPos = GetWorldToScreen(dt.pos, G.camera);
        if (sPos.x > 0 && sPos.x < screenW && sPos.y > 0 && sPos.y < screenH) {
            float alpha = dt.life / dt.maxLife;
            DrawText(dt.text, (int)sPos.x - 10, (int)sPos.y, 18, ColorAlpha(dt.color, alpha));
        }
    }

    DrawText("ESC: Settings & Pause | TAB: Mouse Lock | WASD: Move | LMB: Fireball | RMB: Frost | E: Thunder | SPACE: Dash", 18, screenH - 22, 12, ColorAlpha(LIGHTGRAY, 0.75f));
}

// -----------------------------------------------------------------------------
// Menus: Title, Settings, Pause & Perk Select
// -----------------------------------------------------------------------------
void DrawTitleScreen(int screenW, int screenH) {
    DrawRectangle(0, 0, screenW, screenH, ColorAlpha(Color{ 10, 8, 18, 255 }, 0.9f));

    const char* title = "ARCANE REALM 3D";
    int tw = MeasureText(title, 48);
    DrawText(title, screenW / 2 - tw / 2, screenH / 2 - 200, 48, GOLD);

    const char* sub = "CHRONICLES OF THE MAGE: INFINITE SURVIVAL";
    int sw = MeasureText(sub, 20);
    DrawText(sub, screenW / 2 - sw / 2, screenH / 2 - 145, 20, CYAN);

    int btnW = 320;
    int btnH = 52;
    int btnX = screenW / 2 - btnW / 2;
    int startY = screenH / 2 - 80;
    int gap = 16;

    bool hasSave = HasSaveGame();

    char contSub[48] = "NO SAVED RUN FOUND";
    if (hasSave) {
        FILE* sf = fopen("savegame.dat", "rb");
        if (sf) {
            SaveData sd;
            if (fread(&sd, sizeof(SaveData), 1, sf) == 1) {
                snprintf(contSub, sizeof(contSub), "RESUME WAVE %d (LEVEL %d)", sd.wave, sd.level);
            }
            fclose(sf);
        }
    }
    if (DrawArcaneButton(Rectangle{ (float)btnX, (float)startY, (float)btnW, (float)btnH }, "CONTINUE GAME", contSub, GOLD, !hasSave)) {
        if (LoadGameFromFile()) {
            G.state = STATE_PLAYING;
            if (G.mouseCaptured) DisableCursor();
        }
    }

    if (DrawArcaneButton(Rectangle{ (float)btnX, (float)(startY + btnH + gap), (float)btnW, (float)btnH }, "NEW GAME", "START FROM WAVE 1 (FRESH RUN)", CYAN)) {
        ResetGame();
        SaveGameToFile();
        G.state = STATE_PLAYING;
        if (G.mouseCaptured) DisableCursor();
    }

    if (DrawArcaneButton(Rectangle{ (float)btnX, (float)(startY + (btnH + gap) * 2), (float)btnW, (float)btnH }, "SETTINGS", "AUDIO, DISPLAY, SENSITIVITY & CONTROLS", PURPLE)) {
        G.previousState = STATE_TITLE;
        G.escCooldown = 0.3f;
        G.state = STATE_SETTINGS;
    }

    if (DrawArcaneButton(Rectangle{ (float)btnX, (float)(startY + (btnH + gap) * 3), (float)btnW, (float)btnH }, "EXIT TO DESKTOP", "QUIT APPLICATION", RED)) {
        CloseWindow();
        exit(0);
    }

    DrawText("Native C++ Edition | Raylib 6.0 Engine", screenW / 2 - 120, screenH - 28, 12, ColorAlpha(LIGHTGRAY, 0.5f));
}

void DrawSettingsMenu(int screenW, int screenH) {
    float dt = GetFrameTime();
    if (G.escCooldown > 0.0f) G.escCooldown -= dt;

    DrawRectangle(0, 0, screenW, screenH, ColorAlpha(Color{ 12, 10, 24, 255 }, 0.92f));

    const char* title = "GAME SETTINGS";
    int tw = MeasureText(title, 36);
    DrawText(title, screenW / 2 - tw / 2, screenH / 2 - 200, 36, GOLD);

    int panelW = 500;
    int panelX = screenW / 2 - panelW / 2;
    int startY = screenH / 2 - 130;

    // 1. Master Volume
    float oldVol = G.settings.masterVolume;
    DrawArcaneSlider(Rectangle{ (float)panelX, (float)startY, (float)panelW, 40.0f }, &G.settings.masterVolume, 0.0f, 1.0f, "Master Volume:", "%.0f%%", 100.0f);
    if (fabsf(oldVol - G.settings.masterVolume) > 0.01f) {
        ApplyMasterVolume();
    }

    // 2. Mouse Sensitivity
    DrawArcaneSlider(Rectangle{ (float)panelX, (float)(startY + 60), (float)panelW, 40.0f }, &G.settings.mouseSensitivity, 0.4f, 2.5f, "Mouse Sensitivity:", "%.1fx");

    // 3. Camera FOV
    DrawArcaneSlider(Rectangle{ (float)panelX, (float)(startY + 120), (float)panelW, 40.0f }, &G.settings.cameraFov, 45.0f, 75.0f, "Camera Field of View (FOV):", "%.0f deg");

    // 4. Screen Shake Toggle
    char ssText[48];
    snprintf(ssText, sizeof(ssText), "Screen Shake: %s", G.settings.screenShake ? "ON" : "OFF");
    if (DrawArcaneButton(Rectangle{ (float)panelX, (float)(startY + 180), (float)(panelW / 2 - 8), 44.0f }, ssText, "", G.settings.screenShake ? CYAN : GRAY)) {
        G.settings.screenShake = !G.settings.screenShake;
    }

    // 5. Crosshair Style
    const char* chStyles[] = { "Arcane Ring", "Classic Cross", "Minimal Dot" };
    char chText[64];
    snprintf(chText, sizeof(chText), "Crosshair: %s", chStyles[G.settings.crosshairStyle]);
    if (DrawArcaneButton(Rectangle{ (float)(panelX + panelW / 2 + 8), (float)(startY + 180), (float)(panelW / 2 - 8), 44.0f }, chText, "", PURPLE)) {
        G.settings.crosshairStyle = (G.settings.crosshairStyle + 1) % 3;
    }

    // 6. Fullscreen
    char fsText[48];
    snprintf(fsText, sizeof(fsText), "Display Mode: %s", IsWindowFullscreen() ? "FULLSCREEN" : "WINDOWED");
    if (DrawArcaneButton(Rectangle{ (float)panelX, (float)(startY + 235), (float)panelW, 44.0f }, fsText, "Toggle Shortcut: F11", GOLD)) {
        ToggleFullscreen();
        G.settings.isFullscreen = IsWindowFullscreen();
    }

    // ESC Debounced Check
    bool escPressed = (IsKeyPressed(KEY_ESCAPE) && G.escCooldown <= 0.0f);

    // Navigation / Resume / Save Buttons
    if (G.previousState == STATE_PLAYING) {
        int bWidth = 236;
        if (DrawArcaneButton(Rectangle{ (float)panelX, (float)(startY + 295), (float)bWidth, 48.0f }, "RESUME GAME", "BACK TO BATTLE (ESC)", GREEN) || escPressed) {
            SaveSettingsToFile();
            G.escCooldown = 0.35f;
            G.state = STATE_PLAYING;
            if (G.mouseCaptured) DisableCursor();
        }

        if (DrawArcaneButton(Rectangle{ (float)(panelX + panelW - bWidth), (float)(startY + 295), (float)bWidth, 48.0f }, "SAVE & QUIT", "RETURN TO MAIN MENU", ORANGE)) {
            SaveSettingsToFile();
            SaveGameToFile();
            G.escCooldown = 0.35f;
            G.state = STATE_TITLE;
            EnableCursor();
        }
    } else {
        if (DrawArcaneButton(Rectangle{ (float)(screenW / 2 - 130), (float)(startY + 295), 260.0f, 48.0f }, "BACK TO MAIN MENU", "APPLY & RETURN", GREEN) || escPressed) {
            SaveSettingsToFile();
            G.escCooldown = 0.35f;
            G.state = STATE_TITLE;
            EnableCursor();
        }
    }
}

void DrawPerkSelectionModal(int screenW, int screenH) {
    DrawRectangle(0, 0, screenW, screenH, ColorAlpha(BLACK, 0.78f));

    const char* title = "LEVEL UP! SELECT ARCANE UPGRADE";
    int tw = MeasureText(title, 26);
    DrawText(title, screenW / 2 - tw / 2, screenH / 2 - 180, 26, GOLD);

    const char* sub = "Choose 1 upgrade to empower your battle mage (Press 1, 2, or 3):";
    int sw = MeasureText(sub, 16);
    DrawText(sub, screenW / 2 - sw / 2, screenH / 2 - 140, 16, RAYWHITE);

    int cardW = 250;
    int cardH = 220;
    int spacing = 24;
    int totalW = 3 * cardW + 2 * spacing;
    int startX = screenW / 2 - totalW / 2;
    int startY = screenH / 2 - 80;

    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < (int)G.currentPerkChoices.size(); i++) {
        const auto& perk = G.currentPerkChoices[i];
        Rectangle cardRect = { (float)(startX + i * (cardW + spacing)), (float)startY, (float)cardW, (float)cardH };

        bool hovered = CheckCollisionPointRec(mouse, cardRect);
        Color borderCol = hovered ? GOLD : VIOLET;
        Color bgCol = hovered ? ColorAlpha(Color{ 45, 30, 70, 255 }, 0.95f) : ColorAlpha(Color{ 25, 18, 42, 255 }, 0.9f);

        DrawRectangleRounded(cardRect, 0.12f, 6, bgCol);
        DrawRectangleRoundedLines(cardRect, 0.12f, 6, borderCol);

        char keyBuf[16];
        snprintf(keyBuf, sizeof(keyBuf), "[%d]", i + 1);
        DrawText(keyBuf, (int)cardRect.x + 14, (int)cardRect.y + 14, 20, GOLD);

        DrawText(perk.title, (int)cardRect.x + 14, (int)cardRect.y + 44, 18, RAYWHITE);
        DrawText(perk.subtitle, (int)cardRect.x + 14, (int)cardRect.y + 70, 13, CYAN);
        DrawText(perk.desc1, (int)cardRect.x + 14, (int)cardRect.y + 102, 13, LIGHTGRAY);
        DrawText(perk.desc2, (int)cardRect.x + 14, (int)cardRect.y + 122, 13, LIGHTGRAY);

        if ((hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(KEY_ONE + i)) {
            ApplyPerk(perk.id);
            break;
        }
    }
}

void DrawGameOverScreen(int screenW, int screenH) {
    DrawRectangle(0, 0, screenW, screenH, ColorAlpha(BLACK, 0.85f));

    const char* title = "YOU PERISHED!";
    int tw = MeasureText(title, 40);
    DrawText(title, screenW / 2 - tw / 2, screenH / 2 - 120, 40, RED);

    char stats[128];
    snprintf(stats, sizeof(stats), "Reached Wave: %d  |  Total Kills: %d  |  Final Score: %d", G.wave, G.kills, G.score);
    int sw = MeasureText(stats, 18);
    DrawText(stats, screenW / 2 - sw / 2, screenH / 2 - 50, 18, RAYWHITE);

    int btnW = 260;
    int btnH = 48;
    if (DrawArcaneButton(Rectangle{ (float)(screenW / 2 - btnW / 2), (float)(screenH / 2 + 10), (float)btnW, (float)btnH }, "RETRY (PRESS R)", "RESTART FROM WAVE 1", GOLD) || IsKeyPressed(KEY_R)) {
        ResetGame();
        SaveGameToFile();
        G.state = STATE_PLAYING;
        if (G.mouseCaptured) DisableCursor();
    }

    if (DrawArcaneButton(Rectangle{ (float)(screenW / 2 - btnW / 2), (float)(screenH / 2 + 70), (float)btnW, (float)btnH }, "MAIN MENU", "RETURN TO TITLE SCREEN", PURPLE)) {
        G.state = STATE_TITLE;
        EnableCursor();
    }
}


} // namespace

void DrawInterface(int screenW, int screenH) {
    // --- 2D UI & Menus ---
    if (G.state == STATE_PLAYING) {
        DrawArcaneHUD(screenW, screenH);
    } else if (G.state == STATE_SETTINGS) {
        DrawSettingsMenu(screenW, screenH);
    } else if (G.state == STATE_PERK_SELECT) {
        DrawArcaneHUD(screenW, screenH);
        DrawPerkSelectionModal(screenW, screenH);
    } else if (G.state == STATE_TITLE) {
        DrawTitleScreen(screenW, screenH);
    } else if (G.state == STATE_GAMEOVER) {
        DrawGameOverScreen(screenW, screenH);
    }

}
