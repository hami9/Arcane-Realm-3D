#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <algorithm>

// =============================================================================
// ARCANE REALM 3D: CHRONICLES OF THE MAGE
// A rich, native 3D magical survival action game written in C++ with Raylib
// Features: Infinite Waves (1.01x power scaling), 8 Unique Enemy Types,
// Debounced Settings/Pause Panel (ESC), Save/Load System, Visual Overhaul
// =============================================================================

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#ifndef RAD2DEG
#define RAD2DEG (180.0f / PI)
#endif

#ifndef DEG2RAD
#define DEG2RAD (PI / 180.0f)
#endif

#ifndef CYAN
#define CYAN (Color){ 0, 225, 255, 255 }
#endif

// -----------------------------------------------------------------------------
// Procedural Sound Generator
// -----------------------------------------------------------------------------
Sound GenSound(int sampleRate, float duration, float (*generator)(float t, float p), float param = 0.0f) {
    int totalSamples = (int)(sampleRate * duration);
    short* data = (short*)malloc(totalSamples * sizeof(short));
    for (int i = 0; i < totalSamples; i++) {
        float t = (float)i / (float)sampleRate;
        float sample = generator(t, param);
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        data[i] = (short)(sample * 32000.0f);
    }
    Wave wave = { 0 };
    wave.frameCount = totalSamples;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return snd;
}

float WaveFireball(float t, float p) {
    float freq = 200.0f + 700.0f * (1.0f - t / 0.22f);
    float env = expf(-t * 9.0f);
    float s = sinf(2.0f * PI * freq * t) * 0.6f + sinf(2.0f * PI * (freq * 1.5f) * t) * 0.3f;
    float noise = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 0.25f;
    return (s + noise) * env;
}

float WaveExplosion(float t, float p) {
    float env = expf(-t * 5.0f);
    float noise = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f);
    float boom = sinf(2.0f * PI * (80.0f * expf(-t * 6.0f)) * t);
    return (boom * 0.65f + noise * 0.45f) * env;
}

float WaveFrost(float t, float p) {
    float env = expf(-t * 6.0f);
    float s1 = sinf(2.0f * PI * 1320.0f * t);
    float s2 = sinf(2.0f * PI * 1760.0f * t);
    float s3 = sinf(2.0f * PI * 2640.0f * t);
    return (s1 + s2 * 0.7f + s3 * 0.4f) * 0.4f * env;
}

float WaveThunder(float t, float p) {
    float env = expf(-t * 4.0f);
    float crack = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f);
    float rumble = sinf(2.0f * PI * (65.0f * expf(-t * 2.0f)) * t) * 0.8f;
    if (t < 0.05f) return (crack * 0.9f) * env;
    return (rumble + crack * 0.3f) * env;
}

float WaveBlink(float t, float p) {
    float freq = 900.0f - 600.0f * (t / 0.18f);
    float env = sinf(PI * (t / 0.18f));
    return sinf(2.0f * PI * freq * t) * env * 0.7f;
}

float WaveGem(float t, float p) {
    float freq = (t < 0.06f) ? 1046.5f : 1567.98f;
    float env = expf(-fmodf(t, 0.06f) * 20.0f);
    return sinf(2.0f * PI * freq * t) * env * 0.5f;
}

float WaveLevelUp(float t, float p) {
    float freqs[4] = { 523.25f, 659.25f, 783.99f, 1046.50f };
    int step = (int)(t / 0.12f);
    if (step > 3) step = 3;
    float env = expf(-fmodf(t, 0.12f) * 8.0f);
    return sinf(2.0f * PI * freqs[step] * t) * env * 0.6f;
}

float WaveHurt(float t, float p) {
    float env = expf(-t * 12.0f);
    float s = sinf(2.0f * PI * (140.0f - t * 400.0f) * t);
    return s * env * 0.8f;
}

float WaveButtonClick(float t, float p) {
    float env = expf(-t * 22.0f);
    return sinf(2.0f * PI * 880.0f * t) * env * 0.4f;
}

// -----------------------------------------------------------------------------
// Structures & Enums
// -----------------------------------------------------------------------------
enum GameState {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_SETTINGS,
    STATE_PERK_SELECT,
    STATE_GAMEOVER
};

enum EnemyType {
    ENEMY_WISP,            // Fast flying shadow ball, fires ranged void bolt
    ENEMY_GOLEM,           // Heavy brute, ground slam shockwave
    ENEMY_MAGE,            // Teleporting dark sorcerer, fires dark star
    ENEMY_PYRO,            // Hellfire Imp: fast, winged, fires rapid flames
    ENEMY_FROST_WRAITH,    // Frost Wraith: pale icy specter, immune to freeze, fires ice shards
    ENEMY_STORM_ELEMENT,   // Storm Elemental: electric orb with lightning sparks, fast charge dash
    ENEMY_NECROMANCER,     // Necromancer: skull mask, summons mini wisps
    ENEMY_BOSS             // Void Archon: multi-phase boss
};

struct Particle {
    Vector3 pos;
    Vector3 vel;
    Color color;
    float size;
    float life;
    float maxLife;
    bool active;
};

struct DamageText {
    Vector3 pos;
    char text[16];
    Color color;
    float life;
    float maxLife;
    bool active;
};

struct Projectile {
    Vector3 pos;
    Vector3 vel;
    float radius;
    float damage;
    int type; // 0: Fireball, 1: Enemy Void/Flame/Frost bolt
    float life;
    bool active;
    Color color;
};

struct FrostNova {
    Vector3 center;
    float radius;
    float maxRadius;
    float speed;
    float damage;
    float duration;
    bool active;
};

struct LightningBolt {
    Vector3 start;
    Vector3 end;
    float timer;
    bool active;
};

struct ManaGem {
    Vector3 pos;
    float value;
    float life;
    bool active;
};

struct Enemy {
    Vector3 pos;
    Vector3 vel;
    EnemyType type;
    float hp;
    float maxHp;
    float speed;
    float radius;
    float attackCooldown;
    float frozenTimer;
    float hurtTimer;
    float animTimer;
    float attackDamage;
    bool active;
};

struct Perk {
    const char* title;
    const char* subtitle;
    const char* desc1;
    const char* desc2;
    int id;
};

// -----------------------------------------------------------------------------
// Settings & Save Data
// -----------------------------------------------------------------------------
struct GameSettings {
    float masterVolume;     // 0.0f to 1.0f
    float mouseSensitivity; // 0.5f to 2.5f
    float cameraFov;        // 45.0f to 75.0f
    bool screenShake;       // true/false
    int crosshairStyle;     // 0: Arcane Ring, 1: Classic Cross, 2: Dot
    bool isFullscreen;      // true/false
};

struct SaveData {
    int magic;              // 0x4152434E ("ARCN")
    int wave;
    int level;
    int xp;
    int nextXp;
    int score;
    int kills;
    float hp;
    float maxHp;
    float mana;
    float maxMana;
    int perkFireMultishot;
    float perkExplosionRadius;
    float perkFrostDuration;
    int perkChainCount;
    float perkSpeedMult;
    float perkManaRegenMult;
};

// -----------------------------------------------------------------------------
// Global Context
// -----------------------------------------------------------------------------
struct GameContext {
    GameState state;
    GameState previousState;
    GameSettings settings;
    float escCooldown;

    Camera3D camera;
    float cameraDistance;
    float cameraYaw;
    float cameraPitch;
    bool mouseCaptured;

    // Player Wizard
    Vector3 playerPos;
    Vector3 playerVel;
    float playerYaw;
    float playerHp;
    float playerMaxHp;
    float playerMana;
    float playerMaxMana;
    int playerLevel;
    int playerXp;
    int playerNextXp;
    int score;
    int kills;
    int combo;
    float comboTimer;
    float screenShake;
    float hitFlash;
    float invulnerableTimer;
    float hitmarkerTimer;

    // Player Perks
    int perkFireMultishot;
    float perkExplosionRadius;
    float perkFrostDuration;
    int perkChainCount;
    float perkSpeedMult;
    float perkManaRegenMult;

    // Cooldowns
    float cdFireball;
    float cdFrost;
    float cdThunder;
    float cdBlink;

    // Shield
    float shieldAngle;

    // Waves (Infinite Progression)
    int wave;
    int enemiesToSpawn;
    float spawnTimer;
    float waveIntroTimer;
    bool bossActive;

    // Objects
    std::vector<Particle> particles;
    std::vector<DamageText> damageTexts;
    std::vector<Projectile> projectiles;
    std::vector<Enemy> enemies;
    std::vector<ManaGem> gems;
    std::vector<FrostNova> frostNovas;
    std::vector<LightningBolt> lightnings;

    // Perks
    std::vector<Perk> availablePerks;
    std::vector<Perk> currentPerkChoices;

    // Sounds
    Sound sndFireball;
    Sound sndExplosion;
    Sound sndFrost;
    Sound sndThunder;
    Sound sndBlink;
    Sound sndGem;
    Sound sndLevelUp;
    Sound sndHurt;
    Sound sndClick;

    // Decorative floating crystals
    struct Crystal {
        Vector3 pos;
        Color color;
        float rotSpeed;
        float bobSpeed;
        float bobHeight;
        Crystal() : pos{0,0,0}, color{255,255,255,255}, rotSpeed(1.0f), bobSpeed(1.0f), bobHeight(1.0f) {}
        Crystal(Vector3 p, Color c, float rs, float bs, float bh) : pos(p), color(c), rotSpeed(rs), bobSpeed(bs), bobHeight(bh) {}
    } crystals[5];

    // Floating Debris / Space Rocks
    struct Debris {
        Vector3 pos;
        Vector3 rotAxis;
        float rotSpeed;
        float angle;
        float orbitRadius;
        float orbitSpeed;
        float height;
        float size;
    } debris[16];

    // Cosmic Nebula Clouds
    struct NebulaCloud {
        Vector3 pos;
        Color color;
        float radius;
        float rotSpeed;
    } nebulas[6];
};

static GameContext G;

// -----------------------------------------------------------------------------
// Settings & Save Management Functions
// -----------------------------------------------------------------------------
void ApplyMasterVolume() {
    SetMasterVolume(G.settings.masterVolume);
}

void SaveSettingsToFile() {
    FILE* f = fopen("settings.dat", "wb");
    if (f) {
        fwrite(&G.settings, sizeof(GameSettings), 1, f);
        fclose(f);
    }
}

void LoadSettingsFromFile() {
    G.settings.masterVolume = 0.8f;
    G.settings.mouseSensitivity = 1.0f;
    G.settings.cameraFov = 54.0f;
    G.settings.screenShake = true;
    G.settings.crosshairStyle = 0;
    G.settings.isFullscreen = false;

    FILE* f = fopen("settings.dat", "rb");
    if (f) {
        fread(&G.settings, sizeof(GameSettings), 1, f);
        fclose(f);
    }
    ApplyMasterVolume();
}

bool HasSaveGame() {
    FILE* f = fopen("savegame.dat", "rb");
    if (!f) return false;
    SaveData data;
    size_t r = fread(&data, sizeof(SaveData), 1, f);
    fclose(f);
    return (r == 1 && data.magic == 0x4152434E && data.hp > 0.0f);
}

void SaveGameToFile() {
    SaveData data;
    data.magic = 0x4152434E;
    data.wave = G.wave;
    data.level = G.playerLevel;
    data.xp = G.playerXp;
    data.nextXp = G.playerNextXp;
    data.score = G.score;
    data.kills = G.kills;
    data.hp = G.playerHp;
    data.maxHp = G.playerMaxHp;
    data.mana = G.playerMana;
    data.maxMana = G.playerMaxMana;
    data.perkFireMultishot = G.perkFireMultishot;
    data.perkExplosionRadius = G.perkExplosionRadius;
    data.perkFrostDuration = G.perkFrostDuration;
    data.perkChainCount = G.perkChainCount;
    data.perkSpeedMult = G.perkSpeedMult;
    data.perkManaRegenMult = G.perkManaRegenMult;

    FILE* f = fopen("savegame.dat", "wb");
    if (f) {
        fwrite(&data, sizeof(SaveData), 1, f);
        fclose(f);
    }
}

// -----------------------------------------------------------------------------
// Particles & Effects
// -----------------------------------------------------------------------------
void AddParticle(Vector3 pos, Vector3 vel, Color color, float size, float life) {
    for (auto& p : G.particles) {
        if (!p.active) {
            p.pos = pos;
            p.vel = vel;
            p.color = color;
            p.size = size;
            p.life = life;
            p.maxLife = life;
            p.active = true;
            return;
        }
    }
    Particle p = { pos, vel, color, size, life, life, true };
    G.particles.push_back(p);
}

void AddDamageText(Vector3 pos, int damage, Color color, bool crit = false) {
    for (auto& dt : G.damageTexts) {
        if (!dt.active) {
            dt.pos = pos;
            if (crit) snprintf(dt.text, sizeof(dt.text), "!%d!", damage);
            else snprintf(dt.text, sizeof(dt.text), "%d", damage);
            dt.color = color;
            dt.life = 0.85f;
            dt.maxLife = 0.85f;
            dt.active = true;
            return;
        }
    }
    DamageText dt;
    dt.pos = pos;
    if (crit) snprintf(dt.text, sizeof(dt.text), "!%d!", damage);
    else snprintf(dt.text, sizeof(dt.text), "%d", damage);
    dt.color = color;
    dt.life = 0.85f;
    dt.maxLife = 0.85f;
    dt.active = true;
    G.damageTexts.push_back(dt);
}

void TriggerExplosion(Vector3 pos, float radius, float damage, Color coreColor) {
    PlaySound(G.sndExplosion);
    if (G.settings.screenShake) G.screenShake = 0.45f;

    for (int i = 0; i < 50; i++) {
        float angle = (float)rand() / (float)RAND_MAX * 2.0f * PI;
        float pitch = ((float)rand() / (float)RAND_MAX - 0.5f) * PI;
        float speed = 3.5f + (float)rand() / (float)RAND_MAX * 10.0f;
        Vector3 vel = {
            cosf(pitch) * cosf(angle) * speed,
            sinf(pitch) * speed + 2.5f,
            cosf(pitch) * sinf(angle) * speed
        };
        Color c = (i % 3 == 0) ? GOLD : ((i % 3 == 1) ? ORANGE : coreColor);
        AddParticle(pos, vel, c, 0.4f + (float)rand() / (float)RAND_MAX * 0.5f, 0.6f + (float)rand() / (float)RAND_MAX * 0.4f);
    }

    for (auto& e : G.enemies) {
        if (!e.active) continue;
        float dist = Vector3Distance(pos, e.pos);
        if (dist <= radius) {
            float falloff = 1.0f - (dist / radius) * 0.5f;
            float dmg = damage * falloff;
            e.hp -= dmg;
            e.hurtTimer = 0.2f;
            G.hitmarkerTimer = 0.15f;
            AddDamageText(Vector3Add(e.pos, {0, 1.8f, 0}), (int)dmg, ORANGE, (dmg > 50));

            Vector3 push = Vector3Normalize(Vector3Subtract(e.pos, pos));
            e.pos = Vector3Add(e.pos, Vector3Scale(push, 2.8f * falloff));
        }
    }
}

// -----------------------------------------------------------------------------
// Spells
// -----------------------------------------------------------------------------
void CastFireball(Vector3 from, Vector3 dir) {
    if (G.playerMana < 12.0f) return;
    G.playerMana -= 12.0f;
    PlaySound(G.sndFireball);
    G.cdFireball = 0.28f;

    int count = G.perkFireMultishot;
    float spreadStep = 0.12f;
    float startOffset = -(count - 1) * 0.5f * spreadStep;

    for (int i = 0; i < count; i++) {
        float offset = startOffset + i * spreadStep;
        Vector3 pDir = {
            dir.x * cosf(offset) - dir.z * sinf(offset),
            dir.y,
            dir.x * sinf(offset) + dir.z * cosf(offset)
        };
        pDir = Vector3Normalize(pDir);

        Projectile p;
        p.pos = from;
        p.vel = Vector3Scale(pDir, 34.0f);
        p.radius = 0.45f;
        p.damage = 45.0f;
        p.type = 0;
        p.life = 2.5f;
        p.active = true;
        p.color = ORANGE;
        G.projectiles.push_back(p);
    }
}

void CastFrostNova() {
    if (G.playerMana < 30.0f) return;
    G.playerMana -= 30.0f;
    PlaySound(G.sndFrost);
    G.cdFrost = 5.0f;

    FrostNova fn;
    fn.center = G.playerPos;
    fn.radius = 1.0f;
    fn.maxRadius = 14.5f;
    fn.speed = 22.0f;
    fn.damage = 35.0f;
    fn.duration = 3.5f + G.perkFrostDuration;
    fn.active = true;
    G.frostNovas.push_back(fn);

    for (int i = 0; i < 35; i++) {
        float angle = (float)rand() / (float)RAND_MAX * 2.0f * PI;
        float speed = 5.0f + (float)rand() / (float)RAND_MAX * 7.0f;
        Vector3 vel = { cosf(angle) * speed, 0.5f + (float)rand() / (float)RAND_MAX * 3.0f, sinf(angle) * speed };
        AddParticle(G.playerPos, vel, SKYBLUE, 0.35f, 0.8f);
    }
}

void CastThunderstrike() {
    if (G.playerMana < 40.0f) return;
    G.playerMana -= 40.0f;
    PlaySound(G.sndThunder);
    G.cdThunder = 6.5f;
    if (G.settings.screenShake) G.screenShake = 0.6f;

    Enemy* target = nullptr;
    float closestDist = 999.0f;
    for (auto& e : G.enemies) {
        if (!e.active) continue;
        float d = Vector3Distance(G.playerPos, e.pos);
        if (d < closestDist && d < 35.0f) {
            closestDist = d;
            target = &e;
        }
    }

    Vector3 strikePos = G.playerPos;
    if (target) strikePos = target->pos;
    else {
        Vector3 fwd = { -sinf(G.playerYaw), 0.0f, -cosf(G.playerYaw) };
        strikePos = Vector3Add(G.playerPos, Vector3Scale(fwd, 12.0f));
    }

    Vector3 skyPos = { strikePos.x, strikePos.y + 30.0f, strikePos.z };
    LightningBolt lb = { skyPos, strikePos, 0.25f, true };
    G.lightnings.push_back(lb);

    TriggerExplosion(strikePos, 7.5f, 95.0f, PURPLE);

    if (target) {
        std::vector<Enemy*> chained;
        chained.push_back(target);
        Vector3 lastPos = strikePos;

        for (int c = 0; c < G.perkChainCount; c++) {
            Enemy* nextTarget = nullptr;
            float bestDist = 17.0f;
            for (auto& candidate : G.enemies) {
                if (!candidate.active) continue;
                if (std::find(chained.begin(), chained.end(), &candidate) != chained.end()) continue;
                float d = Vector3Distance(lastPos, candidate.pos);
                if (d < bestDist) {
                    bestDist = d;
                    nextTarget = &candidate;
                }
            }
            if (nextTarget) {
                LightningBolt chainBolt = { lastPos, nextTarget->pos, 0.2f, true };
                G.lightnings.push_back(chainBolt);
                nextTarget->hp -= 65.0f;
                nextTarget->hurtTimer = 0.2f;
                G.hitmarkerTimer = 0.15f;
                AddDamageText(Vector3Add(nextTarget->pos, {0, 1.8f, 0}), 65, VIOLET, true);
                chained.push_back(nextTarget);
                lastPos = nextTarget->pos;
            } else {
                break;
            }
        }
    }
}

void CastBlink() {
    if (G.playerMana < 18.0f) return;
    G.playerMana -= 18.0f;
    PlaySound(G.sndBlink);
    G.cdBlink = 2.8f;
    G.invulnerableTimer = 0.45f;

    for (int i = 0; i < 28; i++) {
        Vector3 rnd = {
            ((float)rand() / (float)RAND_MAX - 0.5f) * 1.6f,
            ((float)rand() / (float)RAND_MAX) * 2.2f,
            ((float)rand() / (float)RAND_MAX - 0.5f) * 1.6f
        };
        AddParticle(Vector3Add(G.playerPos, rnd), {0, 0.5f, 0}, PURPLE, 0.35f, 0.6f);
    }

    Vector3 moveDir = { -sinf(G.playerYaw), 0.0f, -cosf(G.playerYaw) };
    if (Vector3Length(G.playerVel) > 0.1f) {
        moveDir = Vector3Normalize(G.playerVel);
    }
    Vector3 newPos = Vector3Add(G.playerPos, Vector3Scale(moveDir, 11.5f));

    float arenaRadius = 33.0f;
    float distFromCenter = sqrtf(newPos.x * newPos.x + newPos.z * newPos.z);
    if (distFromCenter > arenaRadius) {
        newPos.x = (newPos.x / distFromCenter) * arenaRadius;
        newPos.z = (newPos.z / distFromCenter) * arenaRadius;
    }
    G.playerPos = newPos;

    for (int i = 0; i < 28; i++) {
        Vector3 rnd = {
            ((float)rand() / (float)RAND_MAX - 0.5f) * 1.6f,
            ((float)rand() / (float)RAND_MAX) * 2.2f,
            ((float)rand() / (float)RAND_MAX - 0.5f) * 1.6f
        };
        AddParticle(Vector3Add(G.playerPos, rnd), {0, 1.2f, 0}, MAGENTA, 0.35f, 0.6f);
    }
}

// -----------------------------------------------------------------------------
// Perks & Level Up
// -----------------------------------------------------------------------------
void InitPerks() {
    G.availablePerks = {
        { "Arcane Triad", "TRIPLE FIREBALL", "Fires 3 fireballs at once", "with wide area blast damage.", 1 },
        { "Supernova Blast", "EXPLOSIVE FORCE", "Increases blast radius +50%", "and strengthens knockback.", 2 },
        { "Absolute Zero", "PERMAFROST", "+2s freeze ring duration", "and boosts ice damage +30%.", 3 },
        { "Chain Shock", "CHAIN LIGHTNING", "Lightning arcs across 4", "additional nearby enemies.", 4 },
        { "Fleetfoot Sorcerer", "ETHEREAL STRIDE", "+30% movement speed bonus", "and lower dash cooldown.", 5 },
        { "Mana Wellspring", "INFINITE FOUNT", "2x faster mana recharge", "and grants +40 max mana.", 6 }
    };
}

void TriggerLevelUp() {
    PlaySound(G.sndLevelUp);
    G.playerLevel++;
    G.playerXp -= G.playerNextXp;
    G.playerNextXp = (int)(G.playerNextXp * 1.5f);
    G.playerHp = G.playerMaxHp;
    G.playerMana = G.playerMaxMana;

    std::vector<Perk> pool = G.availablePerks;
    G.currentPerkChoices.clear();
    for (int i = 0; i < 3 && !pool.empty(); i++) {
        int idx = rand() % pool.size();
        G.currentPerkChoices.push_back(pool[idx]);
        pool.erase(pool.begin() + idx);
    }
    G.state = STATE_PERK_SELECT;
    EnableCursor();
}

void ApplyPerk(int perkId) {
    if (perkId == 1) G.perkFireMultishot = 3;
    else if (perkId == 2) G.perkExplosionRadius += 0.6f;
    else if (perkId == 3) G.perkFrostDuration += 2.0f;
    else if (perkId == 4) G.perkChainCount += 3;
    else if (perkId == 5) G.perkSpeedMult += 0.3f;
    else if (perkId == 6) { G.perkManaRegenMult += 1.2f; G.playerMaxMana += 40.0f; }

    SaveGameToFile();
    G.state = STATE_PLAYING;
    if (G.mouseCaptured) DisableCursor();
}

// -----------------------------------------------------------------------------
// Spawning & Infinite Waves (1.01x per Wave Power Scaling)
// -----------------------------------------------------------------------------
void SpawnEnemy(EnemyType type) {
    float angle = (float)rand() / (float)RAND_MAX * 2.0f * PI;
    float dist = 28.0f + (float)rand() / (float)RAND_MAX * 4.0f;
    Vector3 pos = { cosf(angle) * dist, 0.0f, sinf(angle) * dist };

    // 1.01x scale factor for each wave past Wave 1
    float powScale = powf(1.01f, (float)(G.wave - 1));

    Enemy e;
    e.pos = pos;
    e.vel = { 0, 0, 0 };
    e.type = type;
    e.frozenTimer = 0.0f;
    e.hurtTimer = 0.0f;
    e.animTimer = 0.0f;
    e.active = true;

    if (type == ENEMY_WISP) {
        e.hp = e.maxHp = (32.0f + G.wave * 4.0f) * powScale;
        e.speed = 6.2f;
        e.radius = 0.8f;
        e.attackCooldown = 2.0f;
        e.attackDamage = (15.0f + G.wave * 1.5f) * powScale;
        e.pos.y = 1.8f;
    } else if (type == ENEMY_GOLEM) {
        e.hp = e.maxHp = (120.0f + G.wave * 18.0f) * powScale;
        e.speed = 3.3f;
        e.radius = 1.4f;
        e.attackCooldown = 1.5f;
        e.attackDamage = (28.0f + G.wave * 2.5f) * powScale;
    } else if (type == ENEMY_MAGE) {
        e.hp = e.maxHp = (65.0f + G.wave * 8.0f) * powScale;
        e.speed = 4.6f;
        e.radius = 0.9f;
        e.attackCooldown = 3.0f;
        e.attackDamage = (22.0f + G.wave * 2.0f) * powScale;
        e.pos.y = 1.2f;
    } else if (type == ENEMY_PYRO) {
        // Fast flying red Hellfire Imp
        e.hp = e.maxHp = (45.0f + G.wave * 6.0f) * powScale;
        e.speed = 7.5f;
        e.radius = 0.75f;
        e.attackCooldown = 1.8f;
        e.attackDamage = (16.0f + G.wave * 1.8f) * powScale;
        e.pos.y = 1.4f;
    } else if (type == ENEMY_FROST_WRAITH) {
        // Ghostly ice specter (immune to cold)
        e.hp = e.maxHp = (85.0f + G.wave * 10.0f) * powScale;
        e.speed = 5.0f;
        e.radius = 0.95f;
        e.attackCooldown = 2.6f;
        e.attackDamage = (20.0f + G.wave * 2.0f) * powScale;
        e.pos.y = 1.8f;
    } else if (type == ENEMY_STORM_ELEMENT) {
        // High-speed electric orb charging directly
        e.hp = e.maxHp = (70.0f + G.wave * 9.0f) * powScale;
        e.speed = 8.8f;
        e.radius = 0.85f;
        e.attackCooldown = 2.2f;
        e.attackDamage = (25.0f + G.wave * 2.2f) * powScale;
        e.pos.y = 1.1f;
    } else if (type == ENEMY_NECROMANCER) {
        // Skull summoner that spawns wisps
        e.hp = e.maxHp = (110.0f + G.wave * 14.0f) * powScale;
        e.speed = 3.6f;
        e.radius = 1.1f;
        e.attackCooldown = 4.0f;
        e.attackDamage = (18.0f + G.wave * 1.5f) * powScale;
        e.pos.y = 1.2f;
    } else if (type == ENEMY_BOSS) {
        // Colossal Void Archon Boss
        e.hp = e.maxHp = (750.0f + G.wave * 120.0f) * powScale;
        e.speed = 3.0f;
        e.radius = 2.6f;
        e.attackCooldown = 2.5f;
        e.attackDamage = (35.0f + G.wave * 3.0f) * powScale;
        e.pos.y = 2.5f;
        G.bossActive = true;
    }

    G.enemies.push_back(e);

    for (int i = 0; i < 20; i++) {
        Vector3 vel = {
            ((float)rand() / (float)RAND_MAX - 0.5f) * 4.0f,
            1.0f + (float)rand() / (float)RAND_MAX * 3.0f,
            ((float)rand() / (float)RAND_MAX - 0.5f) * 4.0f
        };
        AddParticle(pos, vel, PURPLE, 0.4f, 0.7f);
    }
}

void StartWave(int w) {
    G.wave = w;
    G.waveIntroTimer = 2.8f;
    G.spawnTimer = 0.4f;

    // Clean up corpses and active projectiles from previous wave
    G.enemies.clear();
    G.projectiles.clear();

    if (w % 5 == 0) {
        // Boss Wave every 5 waves!
        G.enemiesToSpawn = 8 + (w / 5) * 3;
        SpawnEnemy(ENEMY_BOSS);
    } else {
        G.enemiesToSpawn = 8 + w * 3;
        G.bossActive = false;
    }
    SaveGameToFile();
}

// -----------------------------------------------------------------------------
// Reset & Load System
// -----------------------------------------------------------------------------
void ResetGame() {
    G.state = STATE_TITLE;
    G.previousState = STATE_TITLE;
    G.escCooldown = 0.0f;
    G.cameraDistance = 9.0f;
    G.cameraYaw = 0.0f;
    G.cameraPitch = 0.35f;
    G.mouseCaptured = true;

    G.playerPos = { 0.0f, 0.0f, 0.0f };
    G.playerVel = { 0.0f, 0.0f, 0.0f };
    G.playerYaw = 0.0f;
    G.playerHp = G.playerMaxHp = 100.0f;
    G.playerMana = G.playerMaxMana = 100.0f;
    G.playerLevel = 1;
    G.playerXp = 0;
    G.playerNextXp = 60;
    G.score = 0;
    G.kills = 0;
    G.combo = 0;
    G.comboTimer = 0.0f;
    G.screenShake = 0.0f;
    G.hitFlash = 0.0f;
    G.invulnerableTimer = 0.0f;
    G.hitmarkerTimer = 0.0f;

    G.perkFireMultishot = 1;
    G.perkExplosionRadius = 1.0f;
    G.perkFrostDuration = 0.0f;
    G.perkChainCount = 1;
    G.perkSpeedMult = 1.0f;
    G.perkManaRegenMult = 1.0f;

    G.cdFireball = 0.0f;
    G.cdFrost = 0.0f;
    G.cdThunder = 0.0f;
    G.cdBlink = 0.0f;
    G.shieldAngle = 0.0f;

    G.particles.clear();
    G.damageTexts.clear();
    G.projectiles.clear();
    G.enemies.clear();
    G.gems.clear();
    G.frostNovas.clear();
    G.lightnings.clear();

    StartWave(1);
}

bool LoadGameFromFile() {
    FILE* f = fopen("savegame.dat", "rb");
    if (!f) return false;
    SaveData data;
    size_t r = fread(&data, sizeof(SaveData), 1, f);
    fclose(f);
    if (r != 1 || data.magic != 0x4152434E || data.hp <= 0.0f) return false;

    ResetGame();
    G.wave = data.wave;
    G.playerLevel = data.level;
    G.playerXp = data.xp;
    G.playerNextXp = data.nextXp;
    G.score = data.score;
    G.kills = data.kills;
    G.playerHp = data.hp;
    G.playerMaxHp = data.maxHp;
    G.playerMana = data.mana;
    G.playerMaxMana = data.maxMana;
    G.perkFireMultishot = data.perkFireMultishot;
    G.perkExplosionRadius = data.perkExplosionRadius;
    G.perkFrostDuration = data.perkFrostDuration;
    G.perkChainCount = data.perkChainCount;
    G.perkSpeedMult = data.perkSpeedMult;
    G.perkManaRegenMult = data.perkManaRegenMult;

    StartWave(G.wave);

    G.state = STATE_PLAYING;
    if (G.mouseCaptured) DisableCursor();
    return true;
}

// -----------------------------------------------------------------------------
// 3D Rendering Elements
// -----------------------------------------------------------------------------
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
    DrawTriangle3D(cTop, cB1, cB2, (Color){ 25, 15, 45, 255 });
    DrawTriangle3D(cTop, cB2, cB1, (Color){ 45, 20, 75, 255 });

    // Robe Upper Torso
    DrawCylinder((Vector3){ 0, 0.8f + bob, 0 }, 0.45f, 0.7f, 1.2f, 10, DARKPURPLE);
    DrawCylinderWires((Vector3){ 0, 0.8f + bob, 0 }, 0.46f, 0.71f, 1.2f, 10, VIOLET);

    // Lower Robe Skirt (Extending down to ground level)
    DrawCylinder((Vector3){ 0, 0.08f, 0 }, 0.52f, 0.44f, 0.75f + bob, 10, DARKPURPLE);
    DrawCylinderWires((Vector3){ 0, 0.08f, 0 }, 0.53f, 0.45f, 0.75f + bob, 10, VIOLET);

    // Boots standing on the arena surface
    DrawCube((Vector3){ -0.18f, 0.08f, -0.05f }, 0.20f, 0.16f, 0.38f, (Color){ 30, 20, 42, 255 });
    DrawCube((Vector3){  0.18f, 0.08f, -0.05f }, 0.20f, 0.16f, 0.38f, (Color){ 30, 20, 42, 255 });

    // Torso / Belt
    DrawCylinder((Vector3){ 0, 1.35f + bob, 0 }, 0.38f, 0.42f, 0.4f, 8, PURPLE);
    DrawCylinderWires((Vector3){ 0, 1.35f + bob, 0 }, 0.39f, 0.43f, 0.4f, 8, GOLD);

    // Shoulders
    DrawSphere((Vector3){ -0.42f, 1.4f + bob, 0 }, 0.18f, DARKBLUE);
    DrawSphereWires((Vector3){ -0.42f, 1.4f + bob, 0 }, 0.19f, 6, 6, GOLD);
    DrawSphere((Vector3){  0.42f, 1.4f + bob, 0 }, 0.18f, DARKBLUE);
    DrawSphereWires((Vector3){  0.42f, 1.4f + bob, 0 }, 0.19f, 6, 6, GOLD);

    // Head
    DrawSphere((Vector3){ 0, 1.7f + bob, 0 }, 0.28f, BEIGE);

    // Glowing Eyes
    DrawSphere((Vector3){ -0.09f, 1.72f + bob, -0.22f }, 0.05f, CYAN);
    DrawSphere((Vector3){  0.09f, 1.72f + bob, -0.22f }, 0.05f, CYAN);

    // Wizard Hat Brim
    DrawCylinder((Vector3){ 0, 1.82f + bob, 0 }, 0.64f, 0.64f, 0.06f, 14, DARKBLUE);
    DrawCylinderWires((Vector3){ 0, 1.82f + bob, 0 }, 0.65f, 0.65f, 0.06f, 14, GOLD);

    // Golden Star on Hat
    DrawSphere((Vector3){ 0, 1.95f + bob, -0.32f }, 0.08f, GOLD);

    // Wizard Hat Cone
    DrawCylinder((Vector3){ 0, 2.3f + bob, -0.08f }, 0.05f, 0.42f, 0.9f, 10, DARKBLUE);
    DrawCylinderWires((Vector3){ 0, 2.3f + bob, -0.08f }, 0.06f, 0.43f, 0.9f, 10, SKYBLUE);

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
        DrawCircle3D(pos, 1.25f, (Vector3){ 1, 0, 0 }, 90.0f, MAGENTA);
    }

    rlPopMatrix();
}

void DrawEnemyModel(const Enemy& e) {
    rlPushMatrix();
    rlTranslatef(e.pos.x, e.pos.y, e.pos.z);

    Color tint = (e.hurtTimer > 0.0f) ? RED : ((e.frozenTimer > 0.0f) ? SKYBLUE : WHITE);

    if (e.type == ENEMY_WISP) {
        float bob = sinf(e.animTimer * 6.0f) * 0.18f;
        DrawSphere((Vector3){ 0, bob, 0 }, 0.55f, ColorTint(DARKPURPLE, tint));
        DrawSphereWires((Vector3){ 0, bob, 0 }, 0.62f, 6, 6, ColorTint(PURPLE, tint));
        DrawSphere((Vector3){ 0, bob, 0 }, 0.28f, ColorTint(MAGENTA, tint));
        DrawSphere((Vector3){ -0.18f, bob + 0.1f, -0.4f }, 0.1f, RED);
        DrawSphere((Vector3){  0.18f, bob + 0.1f, -0.4f }, 0.1f, RED);
        for (int w = 1; w <= 3; w++) {
            float tailOffset = (float)w * 0.28f;
            float twBob = sinf(e.animTimer * 8.0f + w) * 0.12f;
            DrawSphere((Vector3){ 0, bob - tailOffset * 0.5f + twBob, tailOffset }, 0.22f / w, PURPLE);
        }
    } else if (e.type == ENEMY_GOLEM) {
        DrawCube((Vector3){ 0, 1.2f, 0 }, 1.5f, 1.8f, 1.2f, ColorTint(DARKGRAY, tint));
        DrawCubeWires((Vector3){ 0, 1.2f, 0 }, 1.52f, 1.82f, 1.22f, ColorTint(GRAY, tint));
        DrawCube((Vector3){ -0.4f, 2.1f, 0.4f }, 0.3f, 0.7f, 0.3f, RED);
        DrawCube((Vector3){  0.4f, 2.0f, 0.4f }, 0.3f, 0.6f, 0.3f, ORANGE);
        DrawCube((Vector3){ 0, 1.2f, -0.55f }, 0.5f, 0.5f, 0.2f, ColorTint(RED, tint));
        DrawSphere((Vector3){ -1.1f, 0.8f, 0 }, 0.45f, ColorTint(GRAY, tint));
        DrawSphere((Vector3){  1.1f, 0.8f, 0 }, 0.45f, ColorTint(GRAY, tint));
    } else if (e.type == ENEMY_MAGE) {
        DrawCylinder((Vector3){ 0, 1.0f, 0 }, 0.3f, 0.6f, 1.6f, 8, ColorTint(BLACK, tint));
        DrawSphere((Vector3){ 0, 1.9f, 0 }, 0.35f, ColorTint(PURPLE, tint));
        DrawSphere((Vector3){ 0, 1.9f, -0.28f }, 0.12f, ColorTint(VIOLET, tint));
    } else if (e.type == ENEMY_PYRO) {
        // Hellfire Imp: Crimson body, fiery wings, curved black horns
        float bob = sinf(e.animTimer * 8.0f) * 0.15f;
        DrawSphere((Vector3){ 0, bob, 0 }, 0.5f, ColorTint(MAROON, tint));
        DrawSphereWires((Vector3){ 0, bob, 0 }, 0.52f, 6, 6, ColorTint(ORANGE, tint));
        DrawSphere((Vector3){ 0, bob, -0.2f }, 0.2f, GOLD);
        // Horns
        DrawCylinderWires((Vector3){ -0.22f, bob + 0.45f, -0.1f }, 0.04f, 0.08f, 0.4f, 4, BLACK);
        DrawCylinderWires((Vector3){  0.22f, bob + 0.45f, -0.1f }, 0.04f, 0.08f, 0.4f, 4, BLACK);
        // Small wings
        float wingFlap = sinf(e.animTimer * 16.0f) * 0.3f;
        Vector3 wCenter = { 0, bob + 0.2f, 0.25f };
        DrawTriangle3D(wCenter, { -0.8f, bob + 0.5f + wingFlap, 0.35f }, { -0.6f, bob - 0.2f, 0.3f }, ORANGE);
        DrawTriangle3D(wCenter, {  0.6f, bob - 0.2f, 0.3f }, {  0.8f, bob + 0.5f + wingFlap, 0.35f }, ORANGE);
    } else if (e.type == ENEMY_FROST_WRAITH) {
        // Frost Wraith: Pale translucent ice ghost with frozen spires
        float bob = sinf(e.animTimer * 4.0f) * 0.2f;
        DrawCylinder((Vector3){ 0, bob + 0.8f, 0 }, 0.1f, 0.55f, 1.4f, 8, ColorAlpha(ColorTint(SKYBLUE, tint), 0.75f));
        DrawSphere((Vector3){ 0, bob + 1.6f, 0 }, 0.32f, ColorTint(WHITE, tint));
        DrawSphere((Vector3){ -0.1f, bob + 1.65f, -0.25f }, 0.06f, BLUE);
        DrawSphere((Vector3){  0.1f, bob + 1.65f, -0.25f }, 0.06f, BLUE);
        // Ice crystal crown
        DrawCylinderWires((Vector3){ 0, bob + 1.9f, 0 }, 0.35f, 0.35f, 0.15f, 6, WHITE);
    } else if (e.type == ENEMY_STORM_ELEMENT) {
        // Storm Elemental: crackling lightning orb with 4 spinning spikes
        float bob = sinf(e.animTimer * 10.0f) * 0.1f;
        DrawSphere((Vector3){ 0, bob, 0 }, 0.45f, ColorTint(YELLOW, tint));
        DrawSphereWires((Vector3){ 0, bob, 0 }, 0.52f, 8, 8, ColorTint(VIOLET, tint));
        // 4 electric spikes spinning
        for (int s = 0; s < 4; s++) {
            float a = e.animTimer * 6.0f + (float)s / 4.0f * 2.0f * PI;
            Vector3 spPos = { cosf(a) * 0.95f, bob + sinf(a * 2.0f) * 0.25f, sinf(a) * 0.95f };
            DrawCube(spPos, 0.18f, 0.18f, 0.18f, GOLD);
            DrawLine3D((Vector3){ 0, bob, 0 }, spPos, WHITE);
        }
    } else if (e.type == ENEMY_NECROMANCER) {
        // Necromancer: dark robed figure with bone horned mask & scythe
        DrawCylinder((Vector3){ 0, 1.0f, 0 }, 0.25f, 0.65f, 1.7f, 8, (Color){ 20, 16, 26, 255 });
        // Pale skull mask
        DrawSphere((Vector3){ 0, 1.95f, -0.15f }, 0.3f, ColorTint(LIGHTGRAY, tint));
        DrawSphere((Vector3){ -0.09f, 1.98f, -0.38f }, 0.06f, LIME);
        DrawSphere((Vector3){  0.09f, 1.98f, -0.38f }, 0.06f, LIME);
        // Bone scythe in hand
        DrawCylinder((Vector3){ 0.7f, 1.1f, -0.1f }, 0.035f, 0.035f, 2.0f, 6, GRAY);
        DrawCube((Vector3){ 0.7f, 2.0f, -0.3f }, 0.05f, 0.1f, 0.55f, WHITE);
    } else if (e.type == ENEMY_BOSS) {
        float pulse = sinf(e.animTimer * 4.0f) * 0.15f;
        DrawSphere((Vector3){ 0, 0, 0 }, 1.9f + pulse, ColorTint(BLACK, tint));
        DrawSphereWires((Vector3){ 0, 0, 0 }, 2.1f + pulse, 12, 12, ColorTint(PURPLE, tint));
        DrawSphere((Vector3){ 0, 0, -1.3f }, 0.65f, ColorTint(RED, tint));
        DrawSphere((Vector3){ 0, 0, -1.6f }, 0.28f, GOLD);

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
        DrawCubeWires((Vector3){ 0, 0, 0 }, e.radius * 2.2f, e.radius * 2.2f, e.radius * 2.2f, SKYBLUE);
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
        DrawCube((Vector3){0, 0, 0}, d.size, d.size * 1.2f, d.size * 0.8f, (Color){ 45, 38, 62, 255 });
        DrawCubeWires((Vector3){0, 0, 0}, d.size * 1.02f, d.size * 1.22f, d.size * 0.82f, (Color){ 90, 75, 125, 255 });
        rlPopMatrix();
    }

    // Top floor surface is precisely at y = 0.0f
    DrawCircle3D((Vector3){ 0, 0.0f, 0 }, arenaRadius, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 30, 26, 44, 255 });

    // Island rim only extends downward from y = 0.0f to y = -2.0f
    DrawCylinderEx((Vector3){ 0, -2.0f, 0 }, (Vector3){ 0, 0.0f, 0 }, arenaRadius + 1.2f, arenaRadius, 32, (Color){ 24, 18, 34, 255 });
    DrawCylinderWiresEx((Vector3){ 0, -2.0f, 0 }, (Vector3){ 0, 0.0f, 0 }, arenaRadius + 1.2f, arenaRadius, 32, (Color){ 70, 50, 100, 255 });

    // Tapered floating rock underside descending down to y = -9.0f
    DrawCylinderEx((Vector3){ 0, -9.0f, 0 }, (Vector3){ 0, -2.0f, 0 }, 4.0f, arenaRadius + 1.2f, 16, (Color){ 14, 10, 22, 255 });
    DrawCylinderWiresEx((Vector3){ 0, -9.0f, 0 }, (Vector3){ 0, -2.0f, 0 }, 4.0f, arenaRadius + 1.2f, 16, (Color){ 45, 32, 60, 255 });

    // Rune Rings painted at y = 0.02f on top of the floor
    DrawRuneRing((Vector3){ 0, 0.02f, 0 }, 6.0f, ColorAlpha(CYAN, 0.85f), t * 0.4f);
    DrawRuneRing((Vector3){ 0, 0.02f, 0 }, 14.0f, ColorAlpha(VIOLET, 0.75f), -t * 0.25f);
    DrawRuneRing((Vector3){ 0, 0.02f, 0 }, 25.0f, ColorAlpha(MAGENTA, 0.65f), t * 0.15f);
    DrawRuneRing((Vector3){ 0, 0.02f, 0 }, arenaRadius - 1.5f, ColorAlpha(GOLD, 0.8f), -t * 0.1f);

    int pillars = 8;
    for (int i = 0; i < pillars; i++) {
        float a = (float)i / (float)pillars * 2.0f * PI;
        Vector3 pPos = { cosf(a) * (arenaRadius - 3.5f), 3.0f, sinf(a) * (arenaRadius - 3.5f) };
        DrawCube(pPos, 1.4f, 6.0f, 1.4f, (Color){ 45, 40, 60, 255 });
        DrawCubeWires(pPos, 1.42f, 6.02f, 1.42f, (Color){ 90, 80, 120, 255 });

        float pulse = 0.5f + 0.5f * sinf(t * 3.0f + (float)i);
        Color rCol = (i % 2 == 0) ? ColorAlpha(CYAN, pulse) : ColorAlpha(MAGENTA, pulse);
        DrawCube((Vector3){ pPos.x, 3.2f, pPos.z }, 1.45f, 1.2f, 1.45f, rCol);

        Vector3 topPos = { pPos.x, pPos.y + 4.2f + sinf(t * 2.0f + i) * 0.3f, pPos.z };
        DrawCrystal(topPos, rCol, 0.6f, t * 1.5f + i);
    }
}

// -----------------------------------------------------------------------------
// Interactive UI Helpers
// -----------------------------------------------------------------------------
bool DrawArcaneButton(Rectangle bounds, const char* label, const char* sublabel, Color primaryColor, bool disabled = false) {
    Vector2 m = GetMousePosition();
    bool hovered = !disabled && CheckCollisionPointRec(m, bounds);
    bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    Color bgCol = disabled ? (Color){ 30, 25, 40, 255 } :
                  hovered  ? ColorAlpha(primaryColor, 0.35f) : (Color){ 24, 18, 36, 230 };
    Color borderCol = disabled ? (Color){ 60, 50, 80, 255 } :
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

void DrawArcaneSlider(Rectangle bounds, float* value, float minVal, float maxVal, const char* label, const char* valueFmt) {
    Vector2 m = GetMousePosition();
    bool hovered = CheckCollisionPointRec(m, bounds);

    Rectangle track = { bounds.x, bounds.y + bounds.height * 0.55f, bounds.width, 10.0f };
    DrawRectangleRounded(track, 0.5f, 4, (Color){ 20, 16, 32, 255 });
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
    snprintf(buf, sizeof(buf), valueFmt, *value);
    int vw = MeasureText(buf, 16);
    DrawText(buf, (int)(bounds.x + bounds.width - vw), (int)bounds.y, 16, GOLD);
}

// -----------------------------------------------------------------------------
// Update Gameplay Logic
// -----------------------------------------------------------------------------
void UpdateGameplay(float dt) {
    float t = (float)GetTime();

    if (G.screenShake > 0.0f) {
        G.screenShake -= dt * 2.0f;
        if (G.screenShake < 0.0f) G.screenShake = 0.0f;
    }
    if (G.hitFlash > 0.0f) {
        G.hitFlash -= dt * 2.5f;
        if (G.hitFlash < 0.0f) G.hitFlash = 0.0f;
    }
    if (G.invulnerableTimer > 0.0f) G.invulnerableTimer -= dt;
    if (G.hitmarkerTimer > 0.0f) G.hitmarkerTimer -= dt;

    if (G.combo > 0) {
        G.comboTimer -= dt;
        if (G.comboTimer <= 0.0f) G.combo = 0;
    }

    if (G.cdFireball > 0.0f) G.cdFireball -= dt;
    if (G.cdFrost > 0.0f) G.cdFrost -= dt;
    if (G.cdThunder > 0.0f) G.cdThunder -= dt;
    if (G.cdBlink > 0.0f) G.cdBlink -= dt;

    // Passive Mana Regen
    if (G.playerMana < G.playerMaxMana) {
        G.playerMana += 14.0f * G.perkManaRegenMult * dt;
        if (G.playerMana > G.playerMaxMana) G.playerMana = G.playerMaxMana;
    }

    // Open Settings / Pause Panel (ESC with cooldown debounce)
    if (G.escCooldown > 0.0f) G.escCooldown -= dt;
    if (IsKeyPressed(KEY_ESCAPE) && G.escCooldown <= 0.0f) {
        G.previousState = STATE_PLAYING;
        G.state = STATE_SETTINGS;
        G.escCooldown = 0.35f;
        EnableCursor();
        return;
    }

    // Toggle mouse capture (TAB)
    if (IsKeyPressed(KEY_TAB)) {
        G.mouseCaptured = !G.mouseCaptured;
        if (G.mouseCaptured) DisableCursor();
        else EnableCursor();
    }

    // Camera Look
    if (G.mouseCaptured) {
        Vector2 mouseDelta = GetMouseDelta();
        float sens = 0.0032f * G.settings.mouseSensitivity;
        G.cameraYaw -= mouseDelta.x * sens;
        G.cameraPitch += mouseDelta.y * sens;

        // Extended vertical pitch range (-48 deg up to +85 deg down)
        if (G.cameraPitch < -0.85f) G.cameraPitch = -0.85f;
        if (G.cameraPitch > 1.48f) G.cameraPitch = 1.48f;
    }

    float wheel = GetMouseWheelMove();
    G.cameraDistance -= wheel * 1.2f;
    if (G.cameraDistance < 4.0f) G.cameraDistance = 4.0f;
    if (G.cameraDistance > 18.0f) G.cameraDistance = 18.0f;

    // Movement
    Vector3 moveInput = { 0, 0, 0 };
    if (IsKeyDown(KEY_W)) moveInput.z -= 1.0f;
    if (IsKeyDown(KEY_S)) moveInput.z += 1.0f;
    if (IsKeyDown(KEY_A)) moveInput.x -= 1.0f;
    if (IsKeyDown(KEY_D)) moveInput.x += 1.0f;

    bool isMoving = (Vector3Length(moveInput) > 0.1f);
    if (isMoving) {
        moveInput = Vector3Normalize(moveInput);
        float moveX = moveInput.x * cosf(-G.cameraYaw) - moveInput.z * sinf(-G.cameraYaw);
        float moveZ = moveInput.x * sinf(-G.cameraYaw) + moveInput.z * cosf(-G.cameraYaw);
        Vector3 targetDir = { moveX, 0.0f, moveZ };

        float moveSpeed = 10.5f * G.perkSpeedMult;
        G.playerVel = Vector3Lerp(G.playerVel, Vector3Scale(targetDir, moveSpeed), 14.0f * dt);

        float targetYaw = atan2f(-targetDir.x, -targetDir.z);
        float diff = targetYaw - G.playerYaw;
        while (diff > PI) diff -= 2.0f * PI;
        while (diff < -PI) diff += 2.0f * PI;
        G.playerYaw += diff * 12.0f * dt;

        if (rand() % 4 == 0) {
            Vector3 sparkPos = { G.playerPos.x, 0.05f, G.playerPos.z };
            AddParticle(sparkPos, {0, 0.4f, 0}, CYAN, 0.2f, 0.35f);
        }
    } else {
        G.playerVel = Vector3Lerp(G.playerVel, {0, 0, 0}, 16.0f * dt);
    }

    G.playerPos = Vector3Add(G.playerPos, Vector3Scale(G.playerVel, dt));

    float arenaRadius = 33.5f;
    float distCenter = sqrtf(G.playerPos.x * G.playerPos.x + G.playerPos.z * G.playerPos.z);
    if (distCenter > arenaRadius) {
        G.playerPos.x = (G.playerPos.x / distCenter) * arenaRadius;
        G.playerPos.z = (G.playerPos.z / distCenter) * arenaRadius;
        G.playerVel = { 0, 0, 0 };
    }

    // Camera Orbit
    Vector3 targetPos = Vector3Add(G.playerPos, { 0, 1.6f, 0 });
    if (G.settings.screenShake && G.screenShake > 0.0f) {
        targetPos.x += ((float)rand() / (float)RAND_MAX - 0.5f) * G.screenShake * 0.8f;
        targetPos.y += ((float)rand() / (float)RAND_MAX - 0.5f) * G.screenShake * 0.8f;
        targetPos.z += ((float)rand() / (float)RAND_MAX - 0.5f) * G.screenShake * 0.8f;
    }

    Vector3 camOffset = {
        sinf(G.cameraYaw) * cosf(G.cameraPitch) * G.cameraDistance,
        sinf(G.cameraPitch) * G.cameraDistance,
        cosf(G.cameraYaw) * cosf(G.cameraPitch) * G.cameraDistance
    };
    G.camera.position = Vector3Add(targetPos, camOffset);
    // Prevent camera from clipping beneath the arena floor when looking high up
    if (G.camera.position.y < 0.45f) {
        G.camera.position.y = 0.45f;
    }
    G.camera.target = targetPos;
    G.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    G.camera.fovy = G.settings.cameraFov;
    G.camera.projection = CAMERA_PERSPECTIVE;

    // Spells Input (Raycast aim towards 3D crosshair target point)
    Vector3 camForward = Vector3Normalize(Vector3Subtract(G.camera.target, G.camera.position));
    Vector3 aimPoint = Vector3Add(G.camera.position, Vector3Scale(camForward, 45.0f));

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && G.cdFireball <= 0.0f) {
        Vector3 staffTip = Vector3Add(G.playerPos, { 0, 1.6f, 0 });
        Vector3 aimDir = Vector3Normalize(Vector3Subtract(aimPoint, staffTip));
        CastFireball(staffTip, aimDir);
    }

    if ((IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_Q)) && G.cdFrost <= 0.0f) {
        CastFrostNova();
    }

    if (IsKeyPressed(KEY_E) && G.cdThunder <= 0.0f) {
        CastThunderstrike();
    }

    if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_LEFT_SHIFT)) && G.cdBlink <= 0.0f) {
        CastBlink();
    }

    // Orbiting Shields
    G.shieldAngle += 3.2f * dt;
    for (int s = 0; s < 3; s++) {
        float sAngle = G.shieldAngle + (float)s / 3.0f * 2.0f * PI;
        Vector3 shieldPos = {
            G.playerPos.x + cosf(sAngle) * 2.2f,
            G.playerPos.y + 1.2f + sinf(t * 4.0f + s) * 0.2f,
            G.playerPos.z + sinf(sAngle) * 2.2f
        };
        for (auto& e : G.enemies) {
            if (!e.active) continue;
            if (Vector3Distance(shieldPos, e.pos) < 1.4f) {
                e.hp -= 40.0f * dt;
                e.hurtTimer = 0.1f;
                G.hitmarkerTimer = 0.12f;
                AddParticle(shieldPos, {0, 1.0f, 0}, CYAN, 0.25f, 0.3f);
            }
        }
    }

    // Update Frost Novas
    for (auto& fn : G.frostNovas) {
        if (!fn.active) continue;
        fn.radius += fn.speed * dt;
        if (fn.radius >= fn.maxRadius) {
            fn.active = false;
            continue;
        }

        for (auto& e : G.enemies) {
            if (!e.active) continue;
            float dist = Vector3Distance(fn.center, e.pos);
            if (fabsf(dist - fn.radius) < 1.8f) {
                // Frost wraith is immune to freeze!
                if (e.type == ENEMY_FROST_WRAITH) {
                    // Takes partial damage, does not freeze
                    e.hp -= fn.damage * 0.4f;
                } else if (e.frozenTimer <= 0.0f) {
                    e.frozenTimer = fn.duration;
                    e.hp -= fn.damage;
                }
                e.hurtTimer = 0.2f;
                G.hitmarkerTimer = 0.15f;
                AddDamageText(Vector3Add(e.pos, {0, 1.8f, 0}), (int)fn.damage, SKYBLUE);

                for (int i = 0; i < 14; i++) {
                    Vector3 vel = {
                        ((float)rand() / (float)RAND_MAX - 0.5f) * 4.0f,
                        1.0f + (float)rand() / (float)RAND_MAX * 4.0f,
                        ((float)rand() / (float)RAND_MAX - 0.5f) * 4.0f
                    };
                    AddParticle(e.pos, vel, WHITE, 0.3f, 0.5f);
                }
            }
        }
    }

    // Update Lightnings
    for (auto& lb : G.lightnings) {
        if (!lb.active) continue;
        lb.timer -= dt;
        if (lb.timer <= 0.0f) lb.active = false;
    }

    // Update Projectiles
    for (auto& p : G.projectiles) {
        if (!p.active) continue;
        p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
        p.life -= dt;
        if (p.life <= 0.0f || p.pos.y < 0.0f) {
            p.active = false;
            if (p.type == 0) TriggerExplosion(p.pos, 5.5f * G.perkExplosionRadius, p.damage, ORANGE);
            continue;
        }

        if (p.type == 0) {
            AddParticle(p.pos, {0, 0, 0}, GOLD, 0.25f, 0.25f);
            AddParticle(p.pos, {0, 0, 0}, ORANGE, 0.2f, 0.35f);
        } else if (p.type == 1) {
            AddParticle(p.pos, {0, 0, 0}, p.color, 0.22f, 0.25f);
        }

        if (p.type == 0) {
            for (auto& e : G.enemies) {
                if (!e.active) continue;
                Vector3 enemyCenter = e.pos;
                if (e.type == ENEMY_GOLEM) enemyCenter.y += 1.1f;
                else if (e.type == ENEMY_MAGE || e.type == ENEMY_NECROMANCER) enemyCenter.y += 1.0f;

                if (Vector3Distance(p.pos, enemyCenter) <= (p.radius + e.radius)) {
                    p.active = false;
                    G.hitmarkerTimer = 0.15f;
                    TriggerExplosion(p.pos, 5.5f * G.perkExplosionRadius, p.damage, ORANGE);
                    break;
                }
            }
        } else if (p.type == 1) {
            // Player Hurtbox centered at torso (y = 1.15f)
            Vector3 playerCenter = Vector3Add(G.playerPos, { 0, 1.15f, 0 });
            if (Vector3Distance(p.pos, playerCenter) <= (p.radius + 1.15f)) {
                p.active = false;
                if (G.invulnerableTimer <= 0.0f) {
                    PlaySound(G.sndHurt);
                    G.playerHp -= p.damage;
                    G.hitFlash = 0.5f;
                    if (G.settings.screenShake) G.screenShake = 0.4f;
                    AddDamageText(Vector3Add(G.playerPos, {0, 2.0f, 0}), (int)p.damage, RED);
                }
            }
        }
    }

    // Update Enemies (8 Distinct AI Behaviors with Separation & Smart Movement)
    for (size_t ei = 0; ei < G.enemies.size(); ei++) {
        auto& e = G.enemies[ei];
        if (!e.active) continue;
        e.animTimer += dt;
        if (e.hurtTimer > 0.0f) e.hurtTimer -= dt;

        if (e.frozenTimer > 0.0f) {
            e.frozenTimer -= dt;
            continue;
        }

        // Separation force: push away from nearby living enemies to avoid clumping
        for (size_t oj = 0; oj < G.enemies.size(); oj++) {
            if (ei == oj || !G.enemies[oj].active) continue;
            Vector3 diff = Vector3Subtract(e.pos, G.enemies[oj].pos);
            diff.y = 0.0f;
            float d = Vector3Length(diff);
            float minDist = e.radius + G.enemies[oj].radius + 0.45f;
            if (d > 0.01f && d < minDist) {
                float push = (1.0f - d / minDist) * 4.5f * dt;
                Vector3 pushDir = Vector3Normalize(diff);
                e.pos = Vector3Add(e.pos, Vector3Scale(pushDir, push));
            }
        }

        Vector3 toPlayer = Vector3Subtract(G.playerPos, e.pos);
        toPlayer.y = 0.0f;
        float distToPlayer = Vector3Length(toPlayer);

        if (distToPlayer > 0.1f) {
            Vector3 dir = Vector3Normalize(toPlayer);
            Vector3 strafe = { -dir.z, 0.0f, dir.x };

            if (e.type == ENEMY_WISP) {
                // Wisp: swooping sinusoidal flight, orbits at 8-11m
                e.pos.y = 1.7f + sinf(e.animTimer * 4.0f) * 0.35f;
                if (distToPlayer > 10.0f) e.pos = Vector3Add(e.pos, Vector3Scale(dir, e.speed * dt));
                else if (distToPlayer < 6.5f) e.pos = Vector3Subtract(e.pos, Vector3Scale(dir, e.speed * 0.8f * dt));
                e.pos = Vector3Add(e.pos, Vector3Scale(strafe, sinf(e.animTimer * 3.0f) * 4.2f * dt));
            } else if (e.type == ENEMY_GOLEM) {
                // Golem: ground brute, charges when within 8m
                e.pos.y = 0.0f;
                float gSpeed = (distToPlayer < 8.0f) ? (e.speed * 1.45f) : e.speed;
                if (distToPlayer > 2.0f) e.pos = Vector3Add(e.pos, Vector3Scale(dir, gSpeed * dt));
            } else if (e.type == ENEMY_MAGE) {
                // Mage: hovers at 1.2m, retreats if player gets close
                e.pos.y = 1.2f;
                if (distToPlayer > 14.0f) e.pos = Vector3Add(e.pos, Vector3Scale(dir, e.speed * dt));
                else if (distToPlayer < 7.0f) e.pos = Vector3Subtract(e.pos, Vector3Scale(dir, e.speed * 1.1f * dt));
            } else if (e.type == ENEMY_PYRO) {
                // Hellfire Imp: high-speed erratic bat darting
                e.pos.y = 1.6f + fabsf(sinf(e.animTimer * 5.0f)) * 1.6f;
                float dart = sinf(e.animTimer * 7.0f);
                if (distToPlayer > 7.0f) e.pos = Vector3Add(e.pos, Vector3Scale(dir, e.speed * dt));
                else if (distToPlayer < 4.0f) e.pos = Vector3Subtract(e.pos, Vector3Scale(dir, e.speed * dt));
                e.pos = Vector3Add(e.pos, Vector3Scale(strafe, dart * 6.0f * dt));
            } else if (e.type == ENEMY_FROST_WRAITH) {
                // Frost Wraith: smooth eerie orbit and vertical levitation
                e.pos.y = 1.8f + sinf(e.animTimer * 2.2f) * 0.7f;
                if (distToPlayer > 12.0f) e.pos = Vector3Add(e.pos, Vector3Scale(dir, e.speed * dt));
                else if (distToPlayer < 8.0f) e.pos = Vector3Subtract(e.pos, Vector3Scale(dir, e.speed * 0.7f * dt));
                e.pos = Vector3Add(e.pos, Vector3Scale(strafe, 3.6f * dt));
            } else if (e.type == ENEMY_STORM_ELEMENT) {
                // Storm Elemental: rapid burst dash with lightning sparks
                e.pos.y = 1.1f + sinf(e.animTimer * 10.0f) * 0.2f;
                float burst = (fmodf(e.animTimer, 2.2f) < 0.7f) ? 1.9f : 0.8f;
                if (distToPlayer > 1.8f) e.pos = Vector3Add(e.pos, Vector3Scale(dir, e.speed * burst * dt));
                if (rand() % 3 == 0) AddParticle(e.pos, {0, 0.5f, 0}, YELLOW, 0.2f, 0.25f);
            } else if (e.type == ENEMY_NECROMANCER) {
                // Necromancer: backline commander, retreats if rushed
                e.pos.y = 0.0f;
                if (distToPlayer > 16.0f) e.pos = Vector3Add(e.pos, Vector3Scale(dir, e.speed * dt));
                else if (distToPlayer < 10.0f) e.pos = Vector3Subtract(e.pos, Vector3Scale(dir, e.speed * dt));
            } else if (e.type == ENEMY_BOSS) {
                // Void Archon Boss: hovers gracefully between 2.2m and 3.6m altitude
                e.pos.y = 2.8f + sinf(e.animTimer * 1.6f) * 0.8f;
                if (distToPlayer > 13.0f) e.pos = Vector3Add(e.pos, Vector3Scale(dir, e.speed * dt));
                else if (distToPlayer < 8.5f) e.pos = Vector3Subtract(e.pos, Vector3Scale(dir, e.speed * 0.9f * dt));
                e.pos = Vector3Add(e.pos, Vector3Scale(strafe, sinf(e.animTimer * 0.9f) * 2.5f * dt));
            }
        }

        // Keep enemies on the arena platform
        float enemyDistCenter = sqrtf(e.pos.x * e.pos.x + e.pos.z * e.pos.z);
        if (enemyDistCenter > arenaRadius - 1.2f) {
            e.pos.x = (e.pos.x / enemyDistCenter) * (arenaRadius - 1.2f);
            e.pos.z = (e.pos.z / enemyDistCenter) * (arenaRadius - 1.2f);
        }

        e.attackCooldown -= dt;
        if (e.attackCooldown <= 0.0f) {
            Vector3 aimPlayerTorso = Vector3Normalize(Vector3Subtract(Vector3Add(G.playerPos, {0, 1.2f, 0}), e.pos));

            if (e.type == ENEMY_WISP && distToPlayer < 18.0f) {
                Projectile vp;
                vp.pos = e.pos;
                vp.vel = Vector3Scale(aimPlayerTorso, 19.0f);
                vp.radius = 0.38f;
                vp.damage = e.attackDamage;
                vp.type = 1;
                vp.life = 3.0f;
                vp.active = true;
                vp.color = PURPLE;
                G.projectiles.push_back(vp);
                e.attackCooldown = 2.4f;
            } else if (e.type == ENEMY_GOLEM && distToPlayer < 2.8f) {
                if (G.invulnerableTimer <= 0.0f) {
                    PlaySound(G.sndHurt);
                    G.playerHp -= e.attackDamage;
                    G.hitFlash = 0.6f;
                    if (G.settings.screenShake) G.screenShake = 0.5f;
                    AddDamageText(Vector3Add(G.playerPos, {0, 2.0f, 0}), (int)e.attackDamage, RED);
                }
                e.attackCooldown = 1.8f;
            } else if (e.type == ENEMY_MAGE && distToPlayer < 24.0f) {
                // Mage: teleport away if player is too close, else shoot
                if (distToPlayer < 6.0f) {
                    float a = (float)rand() / (float)RAND_MAX * 2.0f * PI;
                    float r = 12.0f + (float)rand() / (float)RAND_MAX * 8.0f;
                    e.pos = (Vector3){ cosf(a) * r, 1.2f, sinf(a) * r };
                }
                Projectile vp;
                vp.pos = e.pos;
                vp.vel = Vector3Scale(aimPlayerTorso, 15.0f);
                vp.radius = 0.45f;
                vp.damage = e.attackDamage;
                vp.type = 1;
                vp.life = 3.5f;
                vp.active = true;
                vp.color = MAGENTA;
                G.projectiles.push_back(vp);
                e.attackCooldown = 3.2f;
            } else if (e.type == ENEMY_PYRO && distToPlayer < 20.0f) {
                // Rapid flame burst aimed at player
                Projectile pp;
                pp.pos = e.pos;
                pp.vel = Vector3Scale(aimPlayerTorso, 24.0f);
                pp.radius = 0.36f;
                pp.damage = e.attackDamage;
                pp.type = 1;
                pp.life = 2.4f;
                pp.active = true;
                pp.color = ORANGE;
                G.projectiles.push_back(pp);
                e.attackCooldown = 1.6f;
            } else if (e.type == ENEMY_FROST_WRAITH && distToPlayer < 22.0f) {
                // Frost bolt that chills
                Projectile fp;
                fp.pos = e.pos;
                fp.vel = Vector3Scale(aimPlayerTorso, 17.0f);
                fp.radius = 0.42f;
                fp.damage = e.attackDamage;
                fp.type = 1;
                fp.life = 3.0f;
                fp.active = true;
                fp.color = SKYBLUE;
                G.projectiles.push_back(fp);
                e.attackCooldown = 2.6f;
            } else if (e.type == ENEMY_STORM_ELEMENT && distToPlayer < 18.0f) {
                // High-speed shock dash toward player
                Vector3 dir = Vector3Normalize(toPlayer);
                e.pos = Vector3Add(e.pos, Vector3Scale(dir, 7.0f));
                if (Vector3Distance(e.pos, G.playerPos) < 2.2f && G.invulnerableTimer <= 0.0f) {
                    PlaySound(G.sndHurt);
                    G.playerHp -= e.attackDamage;
                    G.hitFlash = 0.5f;
                    AddDamageText(Vector3Add(G.playerPos, {0, 2.0f, 0}), (int)e.attackDamage, YELLOW);
                }
                e.attackCooldown = 2.2f;
            } else if (e.type == ENEMY_NECROMANCER) {
                // Summon mini-wisp minion
                Enemy minion;
                minion.pos = Vector3Add(e.pos, { ((float)rand() / (float)RAND_MAX - 0.5f) * 2.0f, 0, ((float)rand() / (float)RAND_MAX - 0.5f) * 2.0f });
                minion.vel = { 0, 0, 0 };
                minion.type = ENEMY_WISP;
                minion.hp = minion.maxHp = 25.0f;
                minion.speed = 6.5f;
                minion.radius = 0.7f;
                minion.attackCooldown = 2.0f;
                minion.attackDamage = 14.0f;
                minion.pos.y = 1.6f;
                minion.frozenTimer = 0.0f;
                minion.hurtTimer = 0.0f;
                minion.animTimer = 0.0f;
                minion.active = true;
                G.enemies.push_back(minion);
                e.attackCooldown = 4.5f;
            } else if (e.type == ENEMY_BOSS) {
                // Boss 5-way spread barrage
                for (int b = -2; b <= 2; b++) {
                    float bAngle = (float)b * 0.20f;
                    Vector3 spreadDir = {
                        aimPlayerTorso.x * cosf(bAngle) - aimPlayerTorso.z * sinf(bAngle),
                        aimPlayerTorso.y,
                        aimPlayerTorso.x * sinf(bAngle) + aimPlayerTorso.z * cosf(bAngle)
                    };
                    Projectile bp;
                    bp.pos = e.pos;
                    bp.vel = Vector3Scale(spreadDir, 18.0f);
                    bp.radius = 0.55f;
                    bp.damage = e.attackDamage;
                    bp.type = 1;
                    bp.life = 4.0f;
                    bp.active = true;
                    bp.color = RED;
                    G.projectiles.push_back(bp);
                }
                e.attackCooldown = 2.6f;
            }
        }

        if (e.hp <= 0.0f) {
            e.active = false;
            G.kills++;
            G.combo++;
            G.comboTimer = 3.5f;
            int killScore = (e.type == ENEMY_BOSS) ? 1000 : ((e.type == ENEMY_GOLEM) ? 150 : 80);
            G.score += killScore * (1 + G.combo / 4);

            int gemCount = (e.type == ENEMY_BOSS) ? 12 : ((e.type == ENEMY_GOLEM) ? 4 : 2);
            for (int g = 0; g < gemCount; g++) {
                Vector3 gPos = {
                    e.pos.x + ((float)rand() / (float)RAND_MAX - 0.5f) * 1.5f,
                    0.5f,
                    e.pos.z + ((float)rand() / (float)RAND_MAX - 0.5f) * 1.5f
                };
                ManaGem gem = { gPos, 15.0f, 18.0f, true };
                G.gems.push_back(gem);
            }

            for (int p = 0; p < 25; p++) {
                Vector3 vel = {
                    ((float)rand() / (float)RAND_MAX - 0.5f) * 6.0f,
                    1.0f + (float)rand() / (float)RAND_MAX * 5.0f,
                    ((float)rand() / (float)RAND_MAX - 0.5f) * 6.0f
                };
                AddParticle(e.pos, vel, PURPLE, 0.4f, 0.6f);
            }

            if (e.type == ENEMY_BOSS) {
                G.bossActive = false;
                PlaySound(G.sndLevelUp);
                SaveGameToFile();
            }
        }
    }

    // Update Gems
    for (auto& gem : G.gems) {
        if (!gem.active) continue;
        gem.life -= dt;
        if (gem.life <= 0.0f) { gem.active = false; continue; }

        float dist = Vector3Distance(gem.pos, G.playerPos);
        float magnetDist = 8.5f;
        if (dist < magnetDist) {
            Vector3 toPlayer = Vector3Normalize(Vector3Subtract(G.playerPos, gem.pos));
            float magnetSpeed = (1.0f - dist / magnetDist) * 18.0f + 8.0f;
            gem.pos = Vector3Add(gem.pos, Vector3Scale(toPlayer, magnetSpeed * dt));
        }

        if (dist < 1.3f) {
            gem.active = false;
            PlaySound(G.sndGem);
            G.playerXp += (int)gem.value;
            G.playerMana = fminf(G.playerMaxMana, G.playerMana + 10.0f);
            AddDamageText(Vector3Add(G.playerPos, {0, 1.8f, 0}), (int)gem.value, GOLD);

            if (G.playerXp >= G.playerNextXp) {
                TriggerLevelUp();
            }
        }
    }

    // Update Particles
    for (auto& p : G.particles) {
        if (!p.active) continue;
        p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
        p.vel.y -= 4.0f * dt;
        p.life -= dt;
        if (p.life <= 0.0f) p.active = false;
    }

    // Update Damage Texts
    for (auto& dtItem : G.damageTexts) {
        if (!dtItem.active) continue;
        dtItem.pos.y += 1.8f * dt;
        dtItem.life -= dt;
        if (dtItem.life <= 0.0f) dtItem.active = false;
    }

    // Infinite Wave Spawner Progression
    if (G.waveIntroTimer > 0.0f) {
        G.waveIntroTimer -= dt;
    } else {
        if (G.enemiesToSpawn > 0) {
            G.spawnTimer -= dt;
            if (G.spawnTimer <= 0.0f) {
                G.spawnTimer = 1.1f - fminf(0.7f, G.wave * 0.04f);

                EnemyType tToSpawn = ENEMY_WISP;
                int r = rand() % 100;
                if (G.wave <= 2) {
                    if (r < 40) tToSpawn = ENEMY_WISP;
                    else if (r < 75) tToSpawn = ENEMY_GOLEM;
                    else tToSpawn = ENEMY_PYRO;
                } else if (G.wave <= 4) {
                    if (r < 25) tToSpawn = ENEMY_WISP;
                    else if (r < 45) tToSpawn = ENEMY_GOLEM;
                    else if (r < 65) tToSpawn = ENEMY_PYRO;
                    else if (r < 82) tToSpawn = ENEMY_FROST_WRAITH;
                    else tToSpawn = ENEMY_MAGE;
                } else {
                    // Wave 6+ infinite progression with all enemy types
                    if (r < 18) tToSpawn = ENEMY_WISP;
                    else if (r < 32) tToSpawn = ENEMY_GOLEM;
                    else if (r < 48) tToSpawn = ENEMY_PYRO;
                    else if (r < 62) tToSpawn = ENEMY_FROST_WRAITH;
                    else if (r < 76) tToSpawn = ENEMY_STORM_ELEMENT;
                    else if (r < 90) tToSpawn = ENEMY_MAGE;
                    else tToSpawn = ENEMY_NECROMANCER;
                }

                SpawnEnemy(tToSpawn);
                G.enemiesToSpawn--;
            }
        } else {
            // Check if all enemies defeated
            bool anyAlive = false;
            for (auto& e : G.enemies) {
                if (e.active) { anyAlive = true; break; }
            }
            if (!anyAlive && !G.bossActive) {
                // Wave completed celebration!
                PlaySound(G.sndLevelUp);
                G.playerHp = fminf(G.playerMaxHp, G.playerHp + 30.0f);
                G.playerMana = G.playerMaxMana;
                StartWave(G.wave + 1);
            }
        }
    }

    if (G.playerHp <= 0.0f) {
        G.state = STATE_GAMEOVER;
        EnableCursor();
    }
}

// -----------------------------------------------------------------------------
// 2D Arcane HUD
// -----------------------------------------------------------------------------
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

    DrawRectangleRounded((Rectangle){ (float)barX - 10, (float)barY - 10, (float)barW + 20, 112 }, 0.15f, 6, ColorAlpha((Color){ 18, 14, 28, 255 }, 0.85f));
    DrawRectangleRoundedLines((Rectangle){ (float)barX - 10, (float)barY - 10, (float)barW + 20, 112 }, 0.15f, 6, ColorAlpha(GOLD, 0.6f));

    char lvlBuf[32];
    snprintf(lvlBuf, sizeof(lvlBuf), "LVL %d", G.playerLevel);
    DrawRectangleRounded((Rectangle){ (float)barX, (float)barY, 65, 24 }, 0.3f, 4, DARKPURPLE);
    DrawRectangleRoundedLines((Rectangle){ (float)barX, (float)barY, 65, 24 }, 0.3f, 4, MAGENTA);
    DrawText(lvlBuf, barX + 10, barY + 5, 16, GOLD);

    char scoreBuf[48];
    snprintf(scoreBuf, sizeof(scoreBuf), "SCORE: %d | KILLS: %d", G.score, G.kills);
    DrawText(scoreBuf, barX + 75, barY + 6, 13, RAYWHITE);

    float hpPercent = fmaxf(0.0f, G.playerHp / G.playerMaxHp);
    DrawRectangle(barX, barY + 32, barW, 16, (Color){ 50, 15, 20, 255 });
    DrawRectangle(barX, barY + 32, (int)(barW * hpPercent), 16, (Color){ 220, 35, 50, 255 });
    DrawRectangleLines(barX, barY + 32, barW, 16, ColorAlpha(RED, 0.6f));
    char hpBuf[32];
    snprintf(hpBuf, sizeof(hpBuf), "HP %d/%d", (int)G.playerHp, (int)G.playerMaxHp);
    DrawText(hpBuf, barX + 8, barY + 33, 13, WHITE);

    float manaPercent = fmaxf(0.0f, G.playerMana / G.playerMaxMana);
    DrawRectangle(barX, barY + 54, barW, 14, (Color){ 10, 30, 60, 255 });
    DrawRectangle(barX, barY + 54, (int)(barW * manaPercent), 14, (Color){ 0, 175, 240, 255 });
    DrawRectangleLines(barX, barY + 54, barW, 14, ColorAlpha(CYAN, 0.6f));
    char manaBuf[32];
    snprintf(manaBuf, sizeof(manaBuf), "MANA %d/%d", (int)G.playerMana, (int)G.playerMaxMana);
    DrawText(manaBuf, barX + 8, barY + 55, 12, WHITE);

    float xpPercent = fminf(1.0f, (float)G.playerXp / (float)G.playerNextXp);
    DrawRectangle(barX, barY + 74, barW, 8, (Color){ 40, 35, 15, 255 });
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
        DrawRectangleRounded((Rectangle){ (float)(screenW / 2 - tw / 2 - 16), 18, (float)(tw + 32), 34 }, 0.3f, 4, ColorAlpha(DARKPURPLE, 0.85f));
        DrawRectangleRoundedLines((Rectangle){ (float)(screenW / 2 - tw / 2 - 16), 18, (float)(tw + 32), 34 }, 0.3f, 4, GOLD);
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

                DrawRectangle(bbX, bbY, bbW, bbH, (Color){ 35, 10, 25, 255 });
                DrawRectangle(bbX, bbY, (int)(bbW * bHpP), bbH, (Color){ 190, 20, 70, 255 });
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

    DrawCircle(radarCX, radarCY, radarR, ColorAlpha((Color){ 15, 12, 28, 255 }, 0.85f));
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

        DrawRectangleRounded((Rectangle){ (float)sx, (float)sy, (float)slotSize, (float)slotSize }, 0.2f, 4, ColorAlpha((Color){ 20, 16, 35, 255 }, 0.9f));
        DrawRectangleRoundedLines((Rectangle){ (float)sx, (float)sy, (float)slotSize, (float)slotSize }, 0.2f, 4, slots[i].color);

        if (i == 0) DrawCircle(sx + slotSize / 2, sy + slotSize / 2 - 4, 14, ORANGE);
        else if (i == 1) DrawPoly((Vector2){ (float)(sx + slotSize / 2), (float)(sy + slotSize / 2 - 4) }, 6, 15, 0, SKYBLUE);
        else if (i == 2) {
            int mx = sx + slotSize / 2;
            int my = sy + slotSize / 2 - 4;
            DrawLineEx((Vector2){ (float)mx - 4, (float)my - 12 }, (Vector2){ (float)mx + 2, (float)my - 2 }, 3.0f, PURPLE);
            DrawLineEx((Vector2){ (float)mx + 2, (float)my - 2 }, (Vector2){ (float)mx - 2, (float)my + 2 }, 3.0f, PURPLE);
            DrawLineEx((Vector2){ (float)mx - 2, (float)my + 2 }, (Vector2){ (float)mx + 6, (float)my + 12 }, 3.0f, PURPLE);
        } else if (i == 3) {
            DrawRing((Vector2){ (float)(sx + slotSize / 2), (float)(sy + slotSize / 2 - 4) }, 7.0f, 13.0f, 0, 360, 16, MAGENTA);
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
        DrawRectangleRounded((Rectangle){ (float)(sx + slotSize / 2 - kw / 2 - 4), (float)(sy + slotSize - 14), (float)(kw + 8), 14 }, 0.3f, 2, (Color){ 35, 25, 55, 255 });
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
    DrawRectangle(0, 0, screenW, screenH, ColorAlpha((Color){ 10, 8, 18, 255 }, 0.9f));

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
    if (DrawArcaneButton((Rectangle){ (float)btnX, (float)startY, (float)btnW, (float)btnH }, "CONTINUE GAME", contSub, GOLD, !hasSave)) {
        if (LoadGameFromFile()) {
            G.state = STATE_PLAYING;
            if (G.mouseCaptured) DisableCursor();
        }
    }

    if (DrawArcaneButton((Rectangle){ (float)btnX, (float)(startY + btnH + gap), (float)btnW, (float)btnH }, "NEW GAME", "START FROM WAVE 1 (FRESH RUN)", CYAN)) {
        ResetGame();
        G.state = STATE_PLAYING;
        if (G.mouseCaptured) DisableCursor();
    }

    if (DrawArcaneButton((Rectangle){ (float)btnX, (float)(startY + (btnH + gap) * 2), (float)btnW, (float)btnH }, "SETTINGS", "AUDIO, DISPLAY, SENSITIVITY & CONTROLS", PURPLE)) {
        G.previousState = STATE_TITLE;
        G.escCooldown = 0.3f;
        G.state = STATE_SETTINGS;
    }

    if (DrawArcaneButton((Rectangle){ (float)btnX, (float)(startY + (btnH + gap) * 3), (float)btnW, (float)btnH }, "EXIT TO DESKTOP", "QUIT APPLICATION", RED)) {
        CloseWindow();
        exit(0);
    }

    DrawText("Native C++ Edition | Raylib 6.0 Engine", screenW / 2 - 120, screenH - 28, 12, ColorAlpha(LIGHTGRAY, 0.5f));
}

void DrawSettingsMenu(int screenW, int screenH) {
    float dt = GetFrameTime();
    if (G.escCooldown > 0.0f) G.escCooldown -= dt;

    DrawRectangle(0, 0, screenW, screenH, ColorAlpha((Color){ 12, 10, 24, 255 }, 0.92f));

    const char* title = "GAME SETTINGS";
    int tw = MeasureText(title, 36);
    DrawText(title, screenW / 2 - tw / 2, screenH / 2 - 200, 36, GOLD);

    int panelW = 500;
    int panelX = screenW / 2 - panelW / 2;
    int startY = screenH / 2 - 130;

    // 1. Master Volume
    float oldVol = G.settings.masterVolume;
    DrawArcaneSlider((Rectangle){ (float)panelX, (float)startY, (float)panelW, 40.0f }, &G.settings.masterVolume, 0.0f, 1.0f, "Master Volume:", "%d%%");
    if (fabsf(oldVol - G.settings.masterVolume) > 0.01f) {
        ApplyMasterVolume();
    }

    // 2. Mouse Sensitivity
    DrawArcaneSlider((Rectangle){ (float)panelX, (float)(startY + 60), (float)panelW, 40.0f }, &G.settings.mouseSensitivity, 0.4f, 2.5f, "Mouse Sensitivity:", "%.1fx");

    // 3. Camera FOV
    DrawArcaneSlider((Rectangle){ (float)panelX, (float)(startY + 120), (float)panelW, 40.0f }, &G.settings.cameraFov, 45.0f, 75.0f, "Camera Field of View (FOV):", "%.0f deg");

    // 4. Screen Shake Toggle
    char ssText[48];
    snprintf(ssText, sizeof(ssText), "Screen Shake: %s", G.settings.screenShake ? "ON" : "OFF");
    if (DrawArcaneButton((Rectangle){ (float)panelX, (float)(startY + 180), (float)(panelW / 2 - 8), 44.0f }, ssText, "", G.settings.screenShake ? CYAN : GRAY)) {
        G.settings.screenShake = !G.settings.screenShake;
    }

    // 5. Crosshair Style
    const char* chStyles[] = { "Arcane Ring", "Classic Cross", "Minimal Dot" };
    char chText[64];
    snprintf(chText, sizeof(chText), "Crosshair: %s", chStyles[G.settings.crosshairStyle]);
    if (DrawArcaneButton((Rectangle){ (float)(panelX + panelW / 2 + 8), (float)(startY + 180), (float)(panelW / 2 - 8), 44.0f }, chText, "", PURPLE)) {
        G.settings.crosshairStyle = (G.settings.crosshairStyle + 1) % 3;
    }

    // 6. Fullscreen
    char fsText[48];
    snprintf(fsText, sizeof(fsText), "Display Mode: %s", IsWindowFullscreen() ? "FULLSCREEN" : "WINDOWED");
    if (DrawArcaneButton((Rectangle){ (float)panelX, (float)(startY + 235), (float)panelW, 44.0f }, fsText, "Toggle Shortcut: F11", GOLD)) {
        ToggleFullscreen();
        G.settings.isFullscreen = IsWindowFullscreen();
    }

    // ESC Debounced Check
    bool escPressed = (IsKeyPressed(KEY_ESCAPE) && G.escCooldown <= 0.0f);

    // Navigation / Resume / Save Buttons
    if (G.previousState == STATE_PLAYING) {
        int bWidth = 236;
        if (DrawArcaneButton((Rectangle){ (float)panelX, (float)(startY + 295), (float)bWidth, 48.0f }, "RESUME GAME", "BACK TO BATTLE (ESC)", GREEN) || escPressed) {
            SaveSettingsToFile();
            G.escCooldown = 0.35f;
            G.state = STATE_PLAYING;
            if (G.mouseCaptured) DisableCursor();
        }

        if (DrawArcaneButton((Rectangle){ (float)(panelX + panelW - bWidth), (float)(startY + 295), (float)bWidth, 48.0f }, "SAVE & QUIT", "RETURN TO MAIN MENU", ORANGE)) {
            SaveSettingsToFile();
            SaveGameToFile();
            G.escCooldown = 0.35f;
            G.state = STATE_TITLE;
            EnableCursor();
        }
    } else {
        if (DrawArcaneButton((Rectangle){ (float)(screenW / 2 - 130), (float)(startY + 295), 260.0f, 48.0f }, "BACK TO MAIN MENU", "APPLY & RETURN", GREEN) || escPressed) {
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
        Color bgCol = hovered ? ColorAlpha((Color){ 45, 30, 70, 255 }, 0.95f) : ColorAlpha((Color){ 25, 18, 42, 255 }, 0.9f);

        DrawRectangleRounded(cardRect, 0.12f, 6, bgCol);
        DrawRectangleRoundedLines(cardRect, 0.12f, 6, borderCol);

        char keyBuf[8];
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
    if (DrawArcaneButton((Rectangle){ (float)(screenW / 2 - btnW / 2), (float)(screenH / 2 + 10), (float)btnW, (float)btnH }, "RETRY (PRESS R)", "RESTART FROM WAVE 1", GOLD) || IsKeyPressed(KEY_R)) {
        ResetGame();
        G.state = STATE_PLAYING;
        if (G.mouseCaptured) DisableCursor();
    }

    if (DrawArcaneButton((Rectangle){ (float)(screenW / 2 - btnW / 2), (float)(screenH / 2 + 70), (float)btnW, (float)btnH }, "MAIN MENU", "RETURN TO TITLE SCREEN", PURPLE)) {
        G.state = STATE_TITLE;
        EnableCursor();
    }
}


// -----------------------------------------------------------------------------
// MAIN ENTRY POINT
// -----------------------------------------------------------------------------
int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Arcane Realm 3D: Chronicles of the Mage");
    SetExitKey(KEY_NULL); // Prevent Raylib from closing window on ESC!
    SetTargetFPS(60);

    InitAudioDevice();
    G.sndFireball  = GenSound(44100, 0.22f, WaveFireball);
    G.sndExplosion = GenSound(44100, 0.55f, WaveExplosion);
    G.sndFrost     = GenSound(44100, 0.35f, WaveFrost);
    G.sndThunder   = GenSound(44100, 0.55f, WaveThunder);
    G.sndBlink     = GenSound(44100, 0.20f, WaveBlink);
    G.sndGem       = GenSound(44100, 0.14f, WaveGem);
    G.sndLevelUp   = GenSound(44100, 0.65f, WaveLevelUp);
    G.sndHurt      = GenSound(44100, 0.18f, WaveHurt);
    G.sndClick     = GenSound(44100, 0.08f, WaveButtonClick);

    LoadSettingsFromFile();
    InitPerks();
    ResetGame();

    G.crystals[0] = GameContext::Crystal((Vector3){  18.0f, 6.0f,  18.0f }, PURPLE, 1.2f, 1.8f, 0.8f);
    G.crystals[1] = GameContext::Crystal((Vector3){ -18.0f, 7.0f,  18.0f }, CYAN,   1.5f, 2.2f, 1.0f);
    G.crystals[2] = GameContext::Crystal((Vector3){  18.0f, 6.5f, -18.0f }, GOLD,   1.0f, 1.5f, 0.7f);
    G.crystals[3] = GameContext::Crystal((Vector3){ -18.0f, 8.0f, -18.0f }, MAGENTA,1.4f, 2.0f, 0.9f);
    G.crystals[4] = GameContext::Crystal((Vector3){   0.0f, 10.0f,  0.0f }, SKYBLUE,0.8f, 1.2f, 0.6f);

    for (int i = 0; i < 16; i++) {
        G.debris[i].orbitRadius = 40.0f + fmodf((float)i * 3.7f, 18.0f);
        G.debris[i].orbitSpeed = 0.08f + ((float)rand() / (float)RAND_MAX) * 0.12f;
        G.debris[i].angle = (float)i / 16.0f * 2.0f * PI;
        G.debris[i].height = -4.0f + fmodf((float)i * 2.5f, 12.0f);
        G.debris[i].size = 0.6f + ((float)rand() / (float)RAND_MAX) * 0.9f;
        G.debris[i].rotSpeed = 0.5f + ((float)rand() / (float)RAND_MAX) * 1.5f;
        G.debris[i].rotAxis = Vector3Normalize({ (float)rand(), (float)rand(), (float)rand() });
    }

    Color nebCols[6] = { (Color){ 70, 20, 110, 255 }, (Color){ 20, 50, 120, 255 }, (Color){ 90, 20, 70, 255 },
                         (Color){ 15, 80, 100, 255 }, (Color){ 80, 40, 120, 255 }, (Color){ 30, 20, 60, 255 } };
    for (int i = 0; i < 6; i++) {
        float a = (float)i / 6.0f * 2.0f * PI;
        // Deep space cosmic backdrop, never overlapping the arena floor or players
        G.nebulas[i].pos = (Vector3){ cosf(a) * 95.0f, -55.0f - fmodf((float)i * 3.0f, 10.0f), sinf(a) * 95.0f };
        G.nebulas[i].radius = 36.0f;
        G.nebulas[i].color = nebCols[i];
    }

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
        ClearBackground((Color){ 10, 8, 18, 255 });

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
            DrawCircle3D(fn.center, fn.radius, (Vector3){ 1, 0, 0 }, 90.0f, ColorAlpha(SKYBLUE, 0.45f));
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
                DrawLine3D((Vector3){ prev.x + 0.05f, prev.y, prev.z }, (Vector3){ cur.x + 0.05f, cur.y, cur.z }, PURPLE);
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

        // --- 2D UI & Menus ---
        if (G.state == STATE_PLAYING) {
            DrawArcaneHUD(curW, curH);
        } else if (G.state == STATE_SETTINGS) {
            DrawSettingsMenu(curW, curH);
        } else if (G.state == STATE_PERK_SELECT) {
            DrawArcaneHUD(curW, curH);
            DrawPerkSelectionModal(curW, curH);
        } else if (G.state == STATE_TITLE) {
            DrawTitleScreen(curW, curH);
        } else if (G.state == STATE_GAMEOVER) {
            DrawGameOverScreen(curW, curH);
        }

        EndDrawing();
    }

    // Cleanup
    UnloadSound(G.sndFireball);
    UnloadSound(G.sndExplosion);
    UnloadSound(G.sndFrost);
    UnloadSound(G.sndThunder);
    UnloadSound(G.sndBlink);
    UnloadSound(G.sndGem);
    UnloadSound(G.sndLevelUp);
    UnloadSound(G.sndHurt);
    UnloadSound(G.sndClick);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
