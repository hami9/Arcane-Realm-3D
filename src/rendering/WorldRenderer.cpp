#include "rendering/WorldRenderer.h"
#include "core/GameContext.h"
#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdlib>

namespace {

void DrawRuneRing(Vector3 center, float radius, Color color, float rotation) {
    int segments = 32;
    for (int i = 0; i < segments; i++) {
        float a1 = rotation + (float)i / (float)segments * 2.0f * PI;
        float a2 = rotation + (float)(i + 1) / (float)segments * 2.0f * PI;
        Vector3 p1 = { center.x + cosf(a1) * radius, center.y + 0.03f, center.z + sinf(a1) * radius };
        Vector3 p2 = { center.x + cosf(a2) * radius, center.y + 0.03f, center.z + sinf(a2) * radius };
        DrawLine3D(p1, p2, color);
        if (i % 4 == 0) {
            Vector3 pInner = { center.x + cosf(a1) * (radius * 0.72f), center.y + 0.03f, center.z + sinf(a1) * (radius * 0.72f) };
            DrawLine3D(p1, pInner, color);
        }
    }
}

void DrawCrystal(Vector3 pos, Color col, float size, float rot) {
    Vector3 top = { pos.x, pos.y + size * 1.6f, pos.z };
    Vector3 bot = { pos.x, pos.y - size * 1.6f, pos.z };
    int sides = 6;
    Vector3 pts[6];
    for (int i = 0; i < sides; i++) {
        float a = rot + (float)i / (float)sides * 2.0f * PI;
        pts[i] = { pos.x + cosf(a) * size, pos.y, pos.z + sinf(a) * size };
    }
    for (int i = 0; i < sides; i++) {
        int next = (i + 1) % sides;
        DrawTriangle3D(top, pts[next], pts[i], ColorAlpha(col, 0.8f));
        DrawTriangle3D(bot, pts[i], pts[next], ColorAlpha(col, 0.7f));
        DrawLine3D(top, pts[i], WHITE);
        DrawLine3D(bot, pts[i], WHITE);
        DrawLine3D(pts[i], pts[next], WHITE);
    }
}

void DrawWizardCharacter(Vector3 pos, float yaw, float animTime, bool isMoving) {
    rlPushMatrix();
    rlTranslatef(pos.x, pos.y, pos.z);
    rlRotatef(yaw * RAD2DEG, 0, 1, 0);

    float bob = isMoving ? sinf(animTime * 12.0f) * 0.1f : sinf(animTime * 3.0f) * 0.04f;

    // Outer Cape
    float capeSway = isMoving ? sinf(animTime * 12.0f) * 0.25f + 0.35f : 0.12f;
    Vector3 cTop = { 0, 1.4f + bob, 0.2f };
    Vector3 cB1 = { -0.45f, 0.2f + bob, 0.5f + capeSway };
    Vector3 cB2 = {  0.45f, 0.2f + bob, 0.5f + capeSway };
    DrawTriangle3D(cTop, cB1, cB2, Color{ 25, 15, 45, 255 });
    DrawTriangle3D(cTop, cB2, cB1, Color{ 45, 20, 75, 255 });

    // Robe Upper Torso
    DrawCylinder(Vector3{ 0, 0.8f + bob, 0 }, 0.45f, 0.7f, 1.2f, 10, DARKPURPLE);
    DrawCylinderWires(Vector3{ 0, 0.8f + bob, 0 }, 0.46f, 0.71f, 1.2f, 10, VIOLET);

    // Lower Robe Skirt (Extending down to ground level)
    DrawCylinder(Vector3{ 0, 0.08f, 0 }, 0.52f, 0.44f, 0.75f + bob, 10, DARKPURPLE);
    DrawCylinderWires(Vector3{ 0, 0.08f, 0 }, 0.53f, 0.45f, 0.75f + bob, 10, VIOLET);

    // Boots standing on the arena surface
    DrawCube(Vector3{ -0.18f, 0.08f, -0.05f }, 0.20f, 0.16f, 0.38f, Color{ 30, 20, 42, 255 });
    DrawCube(Vector3{  0.18f, 0.08f, -0.05f }, 0.20f, 0.16f, 0.38f, Color{ 30, 20, 42, 255 });

    // Torso / Belt
    DrawCylinder(Vector3{ 0, 1.35f + bob, 0 }, 0.38f, 0.42f, 0.4f, 8, PURPLE);
    DrawCylinderWires(Vector3{ 0, 1.35f + bob, 0 }, 0.39f, 0.43f, 0.4f, 8, GOLD);

    // Shoulders
    DrawSphere(Vector3{ -0.42f, 1.4f + bob, 0 }, 0.18f, DARKBLUE);
    DrawSphereWires(Vector3{ -0.42f, 1.4f + bob, 0 }, 0.19f, 6, 6, GOLD);
    DrawSphere(Vector3{  0.42f, 1.4f + bob, 0 }, 0.18f, DARKBLUE);
    DrawSphereWires(Vector3{  0.42f, 1.4f + bob, 0 }, 0.19f, 6, 6, GOLD);

    // Head
    DrawSphere(Vector3{ 0, 1.7f + bob, 0 }, 0.28f, BEIGE);

    // Glowing Eyes
    DrawSphere(Vector3{ -0.09f, 1.72f + bob, -0.22f }, 0.05f, CYAN);
    DrawSphere(Vector3{  0.09f, 1.72f + bob, -0.22f }, 0.05f, CYAN);

    // Wizard Hat Brim
    DrawCylinder(Vector3{ 0, 1.82f + bob, 0 }, 0.64f, 0.64f, 0.06f, 14, DARKBLUE);
    DrawCylinderWires(Vector3{ 0, 1.82f + bob, 0 }, 0.65f, 0.65f, 0.06f, 14, GOLD);

    // Golden Star on Hat
    DrawSphere(Vector3{ 0, 1.95f + bob, -0.32f }, 0.08f, GOLD);

    // Wizard Hat Cone
    DrawCylinder(Vector3{ 0, 2.3f + bob, -0.08f }, 0.05f, 0.42f, 0.9f, 10, DARKBLUE);
    DrawCylinderWires(Vector3{ 0, 2.3f + bob, -0.08f }, 0.06f, 0.43f, 0.9f, 10, SKYBLUE);

    // Magic Staff in right hand
    float staffSway = isMoving ? sinf(animTime * 12.0f) * 0.15f : 0.0f;
    Vector3 staffBase = { 0.6f, 0.9f + bob + staffSway, -0.2f };
    DrawCylinder(staffBase, 0.04f, 0.04f, 1.6f, 6, BROWN);
    DrawCylinderWires(staffBase, 0.045f, 0.045f, 1.6f, 6, GOLD);

    // Glowing Crystal Orb atop Staff
    Vector3 staffTop = { staffBase.x, staffBase.y + 0.9f, staffBase.z };
    Color staffGemCol = (G.cdFireball <= 0.05f) ? ORANGE : ColorAlpha(ORANGE, 0.4f);
    DrawSphere(staffTop, 0.16f + sinf(animTime * 8.0f) * 0.03f, staffGemCol);
    DrawSphereWires(staffTop, 0.18f, 6, 6, WHITE);

    // Off-hand floating rune orb (Left Hand)
    float orbBob = sinf(animTime * 5.0f) * 0.08f;
    Vector3 offHandOrb = { -0.55f, 1.15f + bob + orbBob, -0.2f };
    DrawSphere(offHandOrb, 0.11f, CYAN);
    DrawSphereWires(offHandOrb, 0.13f, 6, 6, WHITE);

    if (G.invulnerableTimer > 0.0f) {
        DrawCircle3D(pos, 1.25f, Vector3{ 1, 0, 0 }, 90.0f, MAGENTA);
    }

    rlPopMatrix();
}

void DrawEnemyModel(const Enemy& e) {
    rlPushMatrix();
    rlTranslatef(e.pos.x, e.pos.y, e.pos.z);

    Color tint = (e.hurtTimer > 0.0f) ? RED : ((e.frozenTimer > 0.0f) ? SKYBLUE : WHITE);

    if (e.type == ENEMY_WISP) {
        float bob = sinf(e.animTimer * 6.0f) * 0.18f;
        DrawSphere(Vector3{ 0, bob, 0 }, 0.55f, ColorTint(DARKPURPLE, tint));
        DrawSphereWires(Vector3{ 0, bob, 0 }, 0.62f, 6, 6, ColorTint(PURPLE, tint));
        DrawSphere(Vector3{ 0, bob, 0 }, 0.28f, ColorTint(MAGENTA, tint));
        DrawSphere(Vector3{ -0.18f, bob + 0.1f, -0.4f }, 0.1f, RED);
        DrawSphere(Vector3{  0.18f, bob + 0.1f, -0.4f }, 0.1f, RED);
        for (int w = 1; w <= 3; w++) {
            float tailOffset = (float)w * 0.28f;
            float twBob = sinf(e.animTimer * 8.0f + w) * 0.12f;
            DrawSphere(Vector3{ 0, bob - tailOffset * 0.5f + twBob, tailOffset }, 0.22f / w, PURPLE);
        }
    } else if (e.type == ENEMY_GOLEM) {
        DrawCube(Vector3{ 0, 1.2f, 0 }, 1.5f, 1.8f, 1.2f, ColorTint(DARKGRAY, tint));
        DrawCubeWires(Vector3{ 0, 1.2f, 0 }, 1.52f, 1.82f, 1.22f, ColorTint(GRAY, tint));
        DrawCube(Vector3{ -0.4f, 2.1f, 0.4f }, 0.3f, 0.7f, 0.3f, RED);
        DrawCube(Vector3{  0.4f, 2.0f, 0.4f }, 0.3f, 0.6f, 0.3f, ORANGE);
        DrawCube(Vector3{ 0, 1.2f, -0.55f }, 0.5f, 0.5f, 0.2f, ColorTint(RED, tint));
        DrawSphere(Vector3{ -1.1f, 0.8f, 0 }, 0.45f, ColorTint(GRAY, tint));
        DrawSphere(Vector3{  1.1f, 0.8f, 0 }, 0.45f, ColorTint(GRAY, tint));
    } else if (e.type == ENEMY_MAGE) {
        DrawCylinder(Vector3{ 0, 1.0f, 0 }, 0.3f, 0.6f, 1.6f, 8, ColorTint(BLACK, tint));
        DrawSphere(Vector3{ 0, 1.9f, 0 }, 0.35f, ColorTint(PURPLE, tint));
        DrawSphere(Vector3{ 0, 1.9f, -0.28f }, 0.12f, ColorTint(VIOLET, tint));
    } else if (e.type == ENEMY_PYRO) {
        // Hellfire Imp: Crimson body, fiery wings, curved black horns
        float bob = sinf(e.animTimer * 8.0f) * 0.15f;
        DrawSphere(Vector3{ 0, bob, 0 }, 0.5f, ColorTint(MAROON, tint));
        DrawSphereWires(Vector3{ 0, bob, 0 }, 0.52f, 6, 6, ColorTint(ORANGE, tint));
        DrawSphere(Vector3{ 0, bob, -0.2f }, 0.2f, GOLD);
        // Horns
        DrawCylinderWires(Vector3{ -0.22f, bob + 0.45f, -0.1f }, 0.04f, 0.08f, 0.4f, 4, BLACK);
        DrawCylinderWires(Vector3{  0.22f, bob + 0.45f, -0.1f }, 0.04f, 0.08f, 0.4f, 4, BLACK);
        // Small wings
        float wingFlap = sinf(e.animTimer * 16.0f) * 0.3f;
        Vector3 wCenter = { 0, bob + 0.2f, 0.25f };
        DrawTriangle3D(wCenter, { -0.8f, bob + 0.5f + wingFlap, 0.35f }, { -0.6f, bob - 0.2f, 0.3f }, ORANGE);
        DrawTriangle3D(wCenter, {  0.6f, bob - 0.2f, 0.3f }, {  0.8f, bob + 0.5f + wingFlap, 0.35f }, ORANGE);
    } else if (e.type == ENEMY_FROST_WRAITH) {
        // Frost Wraith: Pale translucent ice ghost with frozen spires
        float bob = sinf(e.animTimer * 4.0f) * 0.2f;
        DrawCylinder(Vector3{ 0, bob + 0.8f, 0 }, 0.1f, 0.55f, 1.4f, 8, ColorAlpha(ColorTint(SKYBLUE, tint), 0.75f));
        DrawSphere(Vector3{ 0, bob + 1.6f, 0 }, 0.32f, ColorTint(WHITE, tint));
        DrawSphere(Vector3{ -0.1f, bob + 1.65f, -0.25f }, 0.06f, BLUE);
        DrawSphere(Vector3{  0.1f, bob + 1.65f, -0.25f }, 0.06f, BLUE);
        // Ice crystal crown
        DrawCylinderWires(Vector3{ 0, bob + 1.9f, 0 }, 0.35f, 0.35f, 0.15f, 6, WHITE);
    } else if (e.type == ENEMY_STORM_ELEMENT) {
        // Storm Elemental: crackling lightning orb with 4 spinning spikes
        float bob = sinf(e.animTimer * 10.0f) * 0.1f;
        DrawSphere(Vector3{ 0, bob, 0 }, 0.45f, ColorTint(YELLOW, tint));
        DrawSphereWires(Vector3{ 0, bob, 0 }, 0.52f, 8, 8, ColorTint(VIOLET, tint));
        // 4 electric spikes spinning
        for (int s = 0; s < 4; s++) {
            float a = e.animTimer * 6.0f + (float)s / 4.0f * 2.0f * PI;
            Vector3 spPos = { cosf(a) * 0.95f, bob + sinf(a * 2.0f) * 0.25f, sinf(a) * 0.95f };
            DrawCube(spPos, 0.18f, 0.18f, 0.18f, GOLD);
            DrawLine3D(Vector3{ 0, bob, 0 }, spPos, WHITE);
        }
    } else if (e.type == ENEMY_NECROMANCER) {
        // Necromancer: dark robed figure with bone horned mask & scythe
        DrawCylinder(Vector3{ 0, 1.0f, 0 }, 0.25f, 0.65f, 1.7f, 8, Color{ 20, 16, 26, 255 });
        // Pale skull mask
        DrawSphere(Vector3{ 0, 1.95f, -0.15f }, 0.3f, ColorTint(LIGHTGRAY, tint));
        DrawSphere(Vector3{ -0.09f, 1.98f, -0.38f }, 0.06f, LIME);
        DrawSphere(Vector3{  0.09f, 1.98f, -0.38f }, 0.06f, LIME);
        // Bone scythe in hand
        DrawCylinder(Vector3{ 0.7f, 1.1f, -0.1f }, 0.035f, 0.035f, 2.0f, 6, GRAY);
        DrawCube(Vector3{ 0.7f, 2.0f, -0.3f }, 0.05f, 0.1f, 0.55f, WHITE);
    } else if (e.type == ENEMY_BOSS) {
        float pulse = sinf(e.animTimer * 4.0f) * 0.15f;
        DrawSphere(Vector3{ 0, 0, 0 }, 1.9f + pulse, ColorTint(BLACK, tint));
        DrawSphereWires(Vector3{ 0, 0, 0 }, 2.1f + pulse, 12, 12, ColorTint(PURPLE, tint));
        DrawSphere(Vector3{ 0, 0, -1.3f }, 0.65f, ColorTint(RED, tint));
        DrawSphere(Vector3{ 0, 0, -1.6f }, 0.28f, GOLD);

        int spikes = 8;
        for (int i = 0; i < spikes; i++) {
            float a1 = e.animTimer * 2.0f + (float)i / (float)spikes * 2.0f * PI;
            Vector3 sp1 = { cosf(a1) * 3.2f, sinf(a1 * 2.0f) * 0.7f, sinf(a1) * 3.2f };
            DrawCube(sp1, 0.45f, 0.45f, 0.45f, ColorTint(MAGENTA, tint));

            float a2 = -e.animTimer * 1.5f + (float)i / (float)spikes * 2.0f * PI;
            Vector3 sp2 = { cosf(a2) * 2.6f, cosf(a2 * 2.0f) * 0.5f, sinf(a2) * 2.6f };
            DrawCube(sp2, 0.35f, 0.35f, 0.35f, ColorTint(PURPLE, tint));
        }
    }

    if (e.frozenTimer > 0.0f) {
        DrawCubeWires(Vector3{ 0, 0, 0 }, e.radius * 2.2f, e.radius * 2.2f, e.radius * 2.2f, SKYBLUE);
    }

    rlPopMatrix();
}

void DrawArenaEnvironment() {
    float arenaRadius = 35.0f;
    float t = (float)GetTime();

    for (int i = 0; i < 6; i++) {
        DrawSphere(G.nebulas[i].pos, G.nebulas[i].radius, ColorAlpha(G.nebulas[i].color, 0.04f));
    }

    for (int i = 0; i < 16; i++) {
        auto& d = G.debris[i];
        float curAngle = d.angle + t * d.orbitSpeed;
        Vector3 dPos = {
            cosf(curAngle) * d.orbitRadius,
            d.height + sinf(t * 1.5f + i) * 0.8f,
            sinf(curAngle) * d.orbitRadius
        };
        rlPushMatrix();
        rlTranslatef(dPos.x, dPos.y, dPos.z);
        rlRotatef(t * d.rotSpeed * RAD2DEG, d.rotAxis.x, d.rotAxis.y, d.rotAxis.z);
        DrawCube(Vector3{0, 0, 0}, d.size, d.size * 1.2f, d.size * 0.8f, Color{ 45, 38, 62, 255 });
        DrawCubeWires(Vector3{0, 0, 0}, d.size * 1.02f, d.size * 1.22f, d.size * 0.82f, Color{ 90, 75, 125, 255 });
        rlPopMatrix();
    }

    // Top floor surface is precisely at y = 0.0f
    DrawCircle3D(Vector3{ 0, 0.0f, 0 }, arenaRadius, Vector3{ 1, 0, 0 }, 90.0f, Color{ 30, 26, 44, 255 });

    // Island rim only extends downward from y = 0.0f to y = -2.0f
    DrawCylinderEx(Vector3{ 0, -2.0f, 0 }, Vector3{ 0, 0.0f, 0 }, arenaRadius + 1.2f, arenaRadius, 32, Color{ 24, 18, 34, 255 });
    DrawCylinderWiresEx(Vector3{ 0, -2.0f, 0 }, Vector3{ 0, 0.0f, 0 }, arenaRadius + 1.2f, arenaRadius, 32, Color{ 70, 50, 100, 255 });

    // Tapered floating rock underside descending down to y = -9.0f
    DrawCylinderEx(Vector3{ 0, -9.0f, 0 }, Vector3{ 0, -2.0f, 0 }, 4.0f, arenaRadius + 1.2f, 16, Color{ 14, 10, 22, 255 });
    DrawCylinderWiresEx(Vector3{ 0, -9.0f, 0 }, Vector3{ 0, -2.0f, 0 }, 4.0f, arenaRadius + 1.2f, 16, Color{ 45, 32, 60, 255 });

    // Rune Rings painted at y = 0.02f on top of the floor
    DrawRuneRing(Vector3{ 0, 0.02f, 0 }, 6.0f, ColorAlpha(CYAN, 0.85f), t * 0.4f);
    DrawRuneRing(Vector3{ 0, 0.02f, 0 }, 14.0f, ColorAlpha(VIOLET, 0.75f), -t * 0.25f);
    DrawRuneRing(Vector3{ 0, 0.02f, 0 }, 25.0f, ColorAlpha(MAGENTA, 0.65f), t * 0.15f);
    DrawRuneRing(Vector3{ 0, 0.02f, 0 }, arenaRadius - 1.5f, ColorAlpha(GOLD, 0.8f), -t * 0.1f);

    int pillars = 8;
    for (int i = 0; i < pillars; i++) {
        float a = (float)i / (float)pillars * 2.0f * PI;
        Vector3 pPos = { cosf(a) * (arenaRadius - 3.5f), 3.0f, sinf(a) * (arenaRadius - 3.5f) };
        DrawCube(pPos, 1.4f, 6.0f, 1.4f, Color{ 45, 40, 60, 255 });
        DrawCubeWires(pPos, 1.42f, 6.02f, 1.42f, Color{ 90, 80, 120, 255 });

        float pulse = 0.5f + 0.5f * sinf(t * 3.0f + (float)i);
        Color rCol = (i % 2 == 0) ? ColorAlpha(CYAN, pulse) : ColorAlpha(MAGENTA, pulse);
        DrawCube(Vector3{ pPos.x, 3.2f, pPos.z }, 1.45f, 1.2f, 1.45f, rCol);

        Vector3 topPos = { pPos.x, pPos.y + 4.2f + sinf(t * 2.0f + i) * 0.3f, pPos.z };
        DrawCrystal(topPos, rCol, 0.6f, t * 1.5f + i);
    }
}

} // namespace

void InitializeWorld() {
    G.crystals[0] = GameContext::Crystal(Vector3{  18.0f, 6.0f,  18.0f }, PURPLE, 1.2f, 1.8f, 0.8f);
    G.crystals[1] = GameContext::Crystal(Vector3{ -18.0f, 7.0f,  18.0f }, CYAN,   1.5f, 2.2f, 1.0f);
    G.crystals[2] = GameContext::Crystal(Vector3{  18.0f, 6.5f, -18.0f }, GOLD,   1.0f, 1.5f, 0.7f);
    G.crystals[3] = GameContext::Crystal(Vector3{ -18.0f, 8.0f, -18.0f }, MAGENTA,1.4f, 2.0f, 0.9f);
    G.crystals[4] = GameContext::Crystal(Vector3{   0.0f, 10.0f,  0.0f }, SKYBLUE,0.8f, 1.2f, 0.6f);

    for (int i = 0; i < 16; i++) {
        G.debris[i].orbitRadius = 40.0f + fmodf((float)i * 3.7f, 18.0f);
        G.debris[i].orbitSpeed = 0.08f + ((float)rand() / (float)RAND_MAX) * 0.12f;
        G.debris[i].angle = (float)i / 16.0f * 2.0f * PI;
        G.debris[i].height = -4.0f + fmodf((float)i * 2.5f, 12.0f);
        G.debris[i].size = 0.6f + ((float)rand() / (float)RAND_MAX) * 0.9f;
        G.debris[i].rotSpeed = 0.5f + ((float)rand() / (float)RAND_MAX) * 1.5f;
        G.debris[i].rotAxis = Vector3Normalize({ (float)rand(), (float)rand(), (float)rand() });
    }

    Color nebCols[6] = { Color{ 70, 20, 110, 255 }, Color{ 20, 50, 120, 255 }, Color{ 90, 20, 70, 255 },
                         Color{ 15, 80, 100, 255 }, Color{ 80, 40, 120, 255 }, Color{ 30, 20, 60, 255 } };
    for (int i = 0; i < 6; i++) {
        float a = (float)i / 6.0f * 2.0f * PI;
        // Deep space cosmic backdrop, never overlapping the arena floor or players
        G.nebulas[i].pos = Vector3{ cosf(a) * 95.0f, -55.0f - fmodf((float)i * 3.0f, 10.0f), sinf(a) * 95.0f };
        G.nebulas[i].radius = 36.0f;
        G.nebulas[i].color = nebCols[i];
    }

}

void DrawWorld() {
    // --- 3D Scene ---
    BeginMode3D(G.camera);

    DrawArenaEnvironment();

    float curTime = (float)GetTime();
    for (int i = 0; i < 5; i++) {
        Vector3 cPos = G.crystals[i].pos;
        cPos.y += sinf(curTime * G.crystals[i].bobSpeed) * G.crystals[i].bobHeight;
        DrawCrystal(cPos, G.crystals[i].color, 1.4f, curTime * G.crystals[i].rotSpeed);
    }

    for (auto& fn : G.frostNovas) {
        if (!fn.active) continue;
        DrawCircle3D(fn.center, fn.radius, Vector3{ 1, 0, 0 }, 90.0f, ColorAlpha(SKYBLUE, 0.45f));
        DrawRuneRing(fn.center, fn.radius, SKYBLUE, curTime * 2.0f);
    }

    for (auto& lb : G.lightnings) {
        if (!lb.active) continue;
        Vector3 prev = lb.start;
        int segments = 8;
        for (int s = 1; s <= segments; s++) {
            float frac = (float)s / (float)segments;
            Vector3 cur = Vector3Lerp(lb.start, lb.end, frac);
            if (s < segments) {
                cur.x += ((float)rand() / (float)RAND_MAX - 0.5f) * 1.5f;
                cur.z += ((float)rand() / (float)RAND_MAX - 0.5f) * 1.5f;
            }
            DrawLine3D(prev, cur, WHITE);
            DrawLine3D(Vector3{ prev.x + 0.05f, prev.y, prev.z }, Vector3{ cur.x + 0.05f, cur.y, cur.z }, PURPLE);
            prev = cur;
        }
    }

    for (auto& g : G.gems) {
        if (!g.active) continue;
        float bob = sinf(curTime * 5.0f + g.pos.x) * 0.15f;
        Vector3 gp = { g.pos.x, g.pos.y + bob, g.pos.z };
        DrawSphere(gp, 0.28f, GOLD);
        DrawSphereWires(gp, 0.32f, 6, 6, YELLOW);
    }

    for (auto& p : G.projectiles) {
        if (!p.active) continue;
        DrawSphere(p.pos, p.radius, p.color);
        DrawSphereWires(p.pos, p.radius * 1.25f, 6, 6, WHITE);
    }

    for (int s = 0; s < 3; s++) {
        float sAngle = G.shieldAngle + (float)s / 3.0f * 2.0f * PI;
        Vector3 sPos = {
            G.playerPos.x + cosf(sAngle) * 2.2f,
            G.playerPos.y + 1.2f + sinf(curTime * 4.0f + s) * 0.2f,
            G.playerPos.z + sinf(sAngle) * 2.2f
        };
        DrawSphere(sPos, 0.3f, CYAN);
        DrawSphereWires(sPos, 0.35f, 6, 6, WHITE);
    }

    for (auto& e : G.enemies) {
        if (!e.active) continue;
        DrawEnemyModel(e);
    }

    DrawWizardCharacter(G.playerPos, G.playerYaw, curTime, Vector3Length(G.playerVel) > 0.1f);

    for (auto& p : G.particles) {
        if (!p.active) continue;
        float alpha = p.life / p.maxLife;
        DrawSphere(p.pos, p.size * alpha, ColorAlpha(p.color, alpha));
    }

    EndMode3D();

}
