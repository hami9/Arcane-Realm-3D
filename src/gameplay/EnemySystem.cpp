#include "gameplay/EnemySystem.h"
#include "gameplay/Gameplay.h"
#include "persistence/SaveSystem.h"
#include "raymath.h"

#include <cmath>
#include <cstdlib>

namespace {
void ResolveEnemyDeath(Enemy& e) {
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
        if (IsAudioDeviceReady()) PlaySound(G.sndLevelUp);
        SaveGameToFile();
    }
    e.attackWindup = 0.0f;
}

void ResolveGolemSlam(Enemy& e) {
    const Vector2 player = {G.playerPos.x, G.playerPos.z};
    const Vector2 origin = {e.attackOrigin.x, e.attackOrigin.z};
    if (Vector2Distance(player, origin) <= GolemSlamRadius && G.invulnerableTimer <= 0.0f) {
        if (IsAudioDeviceReady()) PlaySound(G.sndHurt);
        G.playerHp -= e.attackDamage;
        G.invulnerableTimer = 0.35f;
        G.hitFlash = 0.6f;
        if (G.settings.screenShake) G.screenShake = 0.5f;
        AddDamageText(Vector3Add(G.playerPos, {0, 2.0f, 0}), (int)e.attackDamage, RED);
    }
    for (int i = 0; i < 20; ++i) {
        const float angle = 2.0f * PI * static_cast<float>(i) / 20.0f;
        const Vector3 edge = {e.attackOrigin.x + cosf(angle) * GolemSlamRadius,
                              0.08f, e.attackOrigin.z + sinf(angle) * GolemSlamRadius};
        AddParticle(edge, {0.0f, 1.0f, 0.0f}, ORANGE, 0.2f, 0.35f);
    }
    e.attackCooldown = 1.8f;
}
} // namespace

void UpdateEnemies(float dt, float arenaRadius) {
    // Commit summons after simulation so vector growth cannot invalidate e.
    std::vector<Enemy> pendingSummons;
    std::size_t livingEnemies = 0;
    for (const auto& enemy : G.enemies) {
        if (enemy.active && enemy.hp > 0.0f) ++livingEnemies;
    }

    // Update Enemies (8 Distinct AI Behaviors with Separation & Smart Movement)
    for (size_t ei = 0; ei < G.enemies.size(); ei++) {
        auto& e = G.enemies[ei];
        if (!e.active) continue;
        // Resolve lethal hits before freeze and AI: frozen corpses must still pay rewards.
        if (e.hp <= 0.0f) {
            ResolveEnemyDeath(e);
            continue;
        }
        e.animTimer += dt;
        if (e.hurtTimer > 0.0f) e.hurtTimer -= dt;

        if (e.frozenTimer > 0.0f) {
            e.frozenTimer = fmaxf(0.0f, e.frozenTimer - dt);
            if (e.attackWindup > 0.0f) {
                e.attackWindup = 0.0f;
                e.attackCooldown = 1.0f;
            }
            continue;
        }

        if (e.type == ENEMY_GOLEM && e.attackWindup > 0.0f) {
            e.attackWindup = fmaxf(0.0f, e.attackWindup - dt);
            if (e.attackWindup <= 0.0f) ResolveGolemSlam(e);
            continue; // A committed slam locks movement until impact.
        }

        // Separation force: push away from nearby living enemies to avoid clumping
        for (size_t oj = 0; oj < G.enemies.size(); oj++) {
            if (ei == oj || !G.enemies[oj].active || G.enemies[oj].hp <= 0.0f) continue;
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
                e.attackOrigin = {e.pos.x, 0.06f, e.pos.z};
                e.attackWindup = GolemSlamWindup;
            } else if (e.type == ENEMY_MAGE && distToPlayer < 24.0f) {
                // Mage: teleport away if player is too close, else shoot
                if (distToPlayer < 6.0f) {
                    float a = (float)rand() / (float)RAND_MAX * 2.0f * PI;
                    float r = 12.0f + (float)rand() / (float)RAND_MAX * 8.0f;
                    e.pos = Vector3{ cosf(a) * r, 1.2f, sinf(a) * r };
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
                    if (IsAudioDeviceReady()) PlaySound(G.sndHurt);
                    G.playerHp -= e.attackDamage;
                    G.invulnerableTimer = 0.35f;
                    G.hitFlash = 0.5f;
                    AddDamageText(Vector3Add(G.playerPos, {0, 2.0f, 0}), (int)e.attackDamage, YELLOW);
                }
                e.attackCooldown = 2.2f;
            } else if (e.type == ENEMY_NECROMANCER) {
                e.attackCooldown = 4.5f;
                if (livingEnemies + pendingSummons.size() >= MaxSummonedEnemyPopulation) continue;
                // New minions join next frame, after all current enemy references expire.
                Enemy minion{};
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
                pendingSummons.push_back(minion);
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
    }
    G.enemies.insert(G.enemies.end(), pendingSummons.begin(), pendingSummons.end());
}
