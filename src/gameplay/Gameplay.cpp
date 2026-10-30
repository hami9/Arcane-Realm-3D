#include "gameplay/Gameplay.h"
#include "gameplay/EnemySystem.h"
#include "persistence/SaveSystem.h"
#include "raymath.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

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

void AddDamageText(Vector3 pos, int damage, Color color, bool crit) {
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

void TriggerExplosion(Vector3 pos, float radius, float damage, Color coreColor, bool shatterFrozen) {
    if (radius <= 0.0f || damage <= 0.0f) return;
    if (IsAudioDeviceReady()) PlaySound(G.sndExplosion);
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
        if (!e.active || e.hp <= 0.0f) continue;
        float dist = Vector3Distance(pos, e.pos);
        if (dist <= radius) {
            float falloff = 1.0f - (dist / radius) * 0.5f;
            const bool shattered = shatterFrozen && e.frozenTimer > 0.0f && e.type != ENEMY_FROST_WRAITH;
            float dmg = damage * falloff * (shattered ? 1.5f : 1.0f);
            if (shattered) {
                e.frozenTimer = 0.0f;
                for (int i = 0; i < 8; ++i) {
                    const float angle = static_cast<float>(i) / 8.0f * 2.0f * PI;
                    AddParticle(e.pos, {cosf(angle) * 5.0f, 2.0f, sinf(angle) * 5.0f}, SKYBLUE, 0.3f, 0.5f);
                }
            }
            e.hp -= dmg;
            e.hurtTimer = 0.2f;
            G.hitmarkerTimer = 0.15f;
            AddDamageText(Vector3Add(e.pos, {0, 1.8f, 0}), (int)dmg, shattered ? SKYBLUE : ORANGE, shattered || dmg > 50);

            Vector3 push = Vector3Normalize(Vector3Subtract(e.pos, pos));
            e.pos = Vector3Add(e.pos, Vector3Scale(push, 2.8f * falloff));
        }
    }
}

// -----------------------------------------------------------------------------
// Spells
// -----------------------------------------------------------------------------
void CastFireball(Vector3 from, Vector3 dir) {
    if (G.cdFireball > 0.0f || G.playerMana < 12.0f) return;
    G.playerMana -= 12.0f;
    if (IsAudioDeviceReady()) PlaySound(G.sndFireball);
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
    if (G.cdFrost > 0.0f || G.playerMana < 30.0f) return;
    G.playerMana -= 30.0f;
    if (IsAudioDeviceReady()) PlaySound(G.sndFrost);
    G.cdFrost = 5.0f;

    FrostNova fn;
    fn.center = G.playerPos;
    fn.radius = 1.0f;
    fn.maxRadius = 14.5f;
    fn.speed = 22.0f;
    fn.damage = GetFrostDamage();
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

void UpdateFrostNovas(float dt) {
    for (auto& nova : G.frostNovas) {
        if (!nova.active) continue;
        const float previousRadius = nova.radius;
        nova.radius = std::min(nova.maxRadius, nova.radius + nova.speed * dt);

        for (std::size_t index = 0; index < G.enemies.size(); ++index) {
            auto& enemy = G.enemies[index];
            if (!enemy.active || enemy.hp <= 0.0f) continue;
            if (std::find(nova.hitEnemyIndices.begin(), nova.hitEnemyIndices.end(), index) != nova.hitEnemyIndices.end()) continue;
            const float distance = Vector3Distance(nova.center, enemy.pos);
            // Sweep the moving ring so a long frame cannot skip a target.
            if (distance < previousRadius - 1.8f || distance > nova.radius + 1.8f) continue;
            nova.hitEnemyIndices.push_back(index);

            const float damage = nova.damage * (enemy.type == ENEMY_FROST_WRAITH ? 0.4f : 1.0f);
            enemy.hp -= damage;
            if (enemy.type != ENEMY_FROST_WRAITH) {
                enemy.frozenTimer = std::max(enemy.frozenTimer, nova.duration);
                if (enemy.attackWindup > 0.0f) {
                    enemy.attackWindup = 0.0f;
                    enemy.attackCooldown = 1.0f;
                }
            }
            enemy.hurtTimer = 0.2f;
            G.hitmarkerTimer = 0.15f;
            AddDamageText(Vector3Add(enemy.pos, {0, 1.8f, 0}), static_cast<int>(damage), SKYBLUE);
            for (int i = 0; i < 14; i++) {
                Vector3 velocity = {
                    (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 4.0f,
                    1.0f + static_cast<float>(rand()) / RAND_MAX * 4.0f,
                    (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 4.0f
                };
                AddParticle(enemy.pos, velocity, WHITE, 0.3f, 0.5f);
            }
        }
        if (nova.radius >= nova.maxRadius) nova.active = false;
    }
}

void CastThunderstrike() {
    if (G.cdThunder > 0.0f || G.playerMana < 40.0f) return;
    G.playerMana -= 40.0f;
    if (IsAudioDeviceReady()) PlaySound(G.sndThunder);
    G.cdThunder = 6.5f;
    if (G.settings.screenShake) G.screenShake = 0.6f;

    Enemy* target = nullptr;
    float closestDist = 999.0f;
    for (auto& e : G.enemies) {
        if (!e.active || e.hp <= 0.0f) continue;
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
                if (!candidate.active || candidate.hp <= 0.0f) continue;
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

float GetBlinkCooldown() {
    return std::max(1.2f, 2.8f / std::max(1.0f, G.perkSpeedMult));
}

float GetFrostDamage() {
    // The existing saved duration bonus also identifies the number of frost ranks.
    return 35.0f * (1.0f + 0.3f * std::max(0.0f, G.perkFrostDuration / 2.0f));
}

void CastBlink() {
    if (G.cdBlink > 0.0f) return;
    if (IsAudioDeviceReady()) PlaySound(G.sndBlink);
    G.cdBlink = GetBlinkCooldown();
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
        { "Arcane Triad", "MULTISHOT", "+2 fireballs per cast", "Stacks up to 7 fireballs.", 1 },
        { "Supernova Blast", "EXPLOSIVE FORCE", "+60% base blast radius", "Covers a wider impact area.", 2 },
        { "Absolute Zero", "PERMAFROST", "+2s freeze ring duration", "and boosts ice damage +30%.", 3 },
        { "Chain Shock", "CHAIN LIGHTNING", "+3 additional chain targets", "Arcs between nearby enemies.", 4 },
        { "Fleetfoot Sorcerer", "ETHEREAL STRIDE", "+30% movement speed bonus", "and lower dash cooldown.", 5 },
        { "Mana Wellspring", "INFINITE FOUNT", "+100% base mana recharge", "+40 current and max mana.", 6 }
    };
}

void TriggerLevelUp() {
    // One choice belongs to one level. Excess XP waits until that choice resolves.
    if (G.state == STATE_PERK_SELECT || G.playerNextXp <= 0 || G.playerXp < G.playerNextXp) return;

    std::vector<Perk> pool;
    for (const auto& perk : G.availablePerks) {
        if (perk.id != 1 || G.perkFireMultishot < 7) pool.push_back(perk);
    }
    if (pool.empty()) return;

    if (IsAudioDeviceReady()) PlaySound(G.sndLevelUp);
    G.playerLevel++;
    G.playerXp -= G.playerNextXp;
    G.playerNextXp = static_cast<int>(G.playerNextXp * 1.5f);
    G.playerHp = G.playerMaxHp;
    G.playerMana = G.playerMaxMana;

    G.currentPerkChoices.clear();
    for (int i = 0; i < 3 && !pool.empty(); i++) {
        int idx = rand() % pool.size();
        G.currentPerkChoices.push_back(pool[idx]);
        pool.erase(pool.begin() + idx);
    }
    G.state = STATE_PERK_SELECT;
    if (IsWindowReady()) EnableCursor();
}

void ApplyPerk(int perkId) {
    if (G.state != STATE_PERK_SELECT) return;
    const auto offered = std::find_if(G.currentPerkChoices.begin(), G.currentPerkChoices.end(),
        [perkId](const Perk& perk) { return perk.id == perkId; });
    if (offered == G.currentPerkChoices.end()) return;

    if (perkId == 1) G.perkFireMultishot = std::min(7, G.perkFireMultishot + 2);
    else if (perkId == 2) G.perkExplosionRadius += 0.6f;
    else if (perkId == 3) G.perkFrostDuration += 2.0f;
    else if (perkId == 4) G.perkChainCount += 3;
    else if (perkId == 5) G.perkSpeedMult += 0.3f;
    else if (perkId == 6) {
        G.perkManaRegenMult += 1.0f;
        G.playerMaxMana += 40.0f;
        G.playerMana = std::min(G.playerMaxMana, G.playerMana + 40.0f);
    }
    else return;

    G.currentPerkChoices.clear();
    G.state = STATE_PLAYING;
    // Persist the chosen reward before consuming XP for the next pending choice.
    SaveGameToFile();
    if (G.playerXp >= G.playerNextXp) TriggerLevelUp();
    if (G.state == STATE_PLAYING && G.mouseCaptured && IsWindowReady()) DisableCursor();
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

void StartWave(int w, bool saveProgress) {
    G.wave = w;
    G.waveIntroTimer = 2.8f;
    G.spawnTimer = 0.4f;

    // Clean up corpses and active projectiles from previous wave
    G.enemies.clear();
    G.projectiles.clear();
    G.frostNovas.clear();
    G.lightnings.clear();

    if (w % 5 == 0) {
        // Boss Wave every 5 waves!
        G.enemiesToSpawn = 8 + (w / 5) * 3;
        SpawnEnemy(ENEMY_BOSS);
    } else {
        G.enemiesToSpawn = 8 + w * 3;
        G.bossActive = false;
    }
    if (saveProgress) SaveGameToFile();
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
    // Keep the title scene valid before the first gameplay update.
    G.camera.target = Vector3{0.0f, 1.0f, 0.0f};
    G.camera.position = Vector3{0.0f, 1.0f + sinf(G.cameraPitch) * G.cameraDistance,
                                cosf(G.cameraPitch) * G.cameraDistance};
    G.camera.up = Vector3{0.0f, 1.0f, 0.0f};
    G.camera.fovy = G.settings.cameraFov;
    G.camera.projection = CAMERA_PERSPECTIVE;
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

    StartWave(1, false);
}

void UpdateGameplay(float dt) {
    if (G.state == STATE_PLAYING && G.playerXp >= G.playerNextXp) {
        TriggerLevelUp();
        if (G.state == STATE_PERK_SELECT) return;
    }
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
        if (IsWindowReady()) EnableCursor();
        return;
    }

    // Toggle mouse capture (TAB)
    if (IsKeyPressed(KEY_TAB)) {
        G.mouseCaptured = !G.mouseCaptured;
        if (IsWindowReady()) {
            if (G.mouseCaptured) DisableCursor();
            else EnableCursor();
        }
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
    G.camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
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
            if (!e.active || e.hp <= 0.0f) continue;
            if (Vector3Distance(shieldPos, e.pos) < 1.4f) {
                e.hp -= 40.0f * dt;
                e.hurtTimer = 0.1f;
                G.hitmarkerTimer = 0.12f;
                AddParticle(shieldPos, {0, 1.0f, 0}, CYAN, 0.25f, 0.3f);
            }
        }
    }

    UpdateFrostNovas(dt);

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
            if (p.type == 0) TriggerExplosion(p.pos, 5.5f * G.perkExplosionRadius, p.damage, ORANGE, true);
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
                if (!e.active || e.hp <= 0.0f) continue;
                Vector3 enemyCenter = e.pos;
                if (e.type == ENEMY_GOLEM) enemyCenter.y += 1.1f;
                else if (e.type == ENEMY_MAGE || e.type == ENEMY_NECROMANCER) enemyCenter.y += 1.0f;

                if (Vector3Distance(p.pos, enemyCenter) <= (p.radius + e.radius)) {
                    p.active = false;
                    G.hitmarkerTimer = 0.15f;
                    TriggerExplosion(p.pos, 5.5f * G.perkExplosionRadius, p.damage, ORANGE, true);
                    break;
                }
            }
        } else if (p.type == 1) {
            // Player Hurtbox centered at torso (y = 1.15f)
            Vector3 playerCenter = Vector3Add(G.playerPos, { 0, 1.15f, 0 });
            if (Vector3Distance(p.pos, playerCenter) <= (p.radius + 1.15f)) {
                p.active = false;
                if (G.invulnerableTimer <= 0.0f) {
                    if (IsAudioDeviceReady()) PlaySound(G.sndHurt);
                    G.playerHp -= p.damage;
                    G.invulnerableTimer = 0.35f;
                    G.hitFlash = 0.5f;
                    if (G.settings.screenShake) G.screenShake = 0.4f;
                    AddDamageText(Vector3Add(G.playerPos, {0, 2.0f, 0}), (int)p.damage, RED);
                }
            }
        }
    }

    UpdateEnemies(dt, arenaRadius);

    // Resolve defeat before pickups, level-up healing or the wave-clear reward.
    if (G.playerHp <= 0.0f) {
        G.state = STATE_GAMEOVER;
        if (IsWindowReady()) EnableCursor();
        return;
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
            if (IsAudioDeviceReady()) PlaySound(G.sndGem);
            G.playerXp += (int)gem.value;
            G.playerMana = fminf(G.playerMaxMana, G.playerMana + 10.0f);
            AddDamageText(Vector3Add(G.playerPos, {0, 1.8f, 0}), (int)gem.value, GOLD);

            if (G.playerXp >= G.playerNextXp) {
                TriggerLevelUp();
                if (G.state == STATE_PERK_SELECT) return;
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
                if (IsAudioDeviceReady()) PlaySound(G.sndLevelUp);
                G.playerHp = fminf(G.playerMaxHp, G.playerHp + 30.0f);
                G.playerMana = G.playerMaxMana;
                StartWave(G.wave + 1);
            }
        }
    }

}
