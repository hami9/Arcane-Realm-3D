#pragma once

#include "raylib.h"
#include <cstddef>
#include <vector>

#ifndef CYAN
inline constexpr Color CYAN{0, 225, 255, 255};
#endif

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
    // Enemies keep their vector indices until the wave ends.
    std::vector<std::size_t> hitEnemyIndices;
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
    // Runtime-only attack preview; legacy saved-run layouts stay unchanged.
    float attackWindup = 0.0f;
    Vector3 attackOrigin{};
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

// One shared context during the incremental migration from the original single file.
extern GameContext G;
