#pragma once

#include "core/GameContext.h"

void AddParticle(Vector3 pos, Vector3 vel, Color color, float size, float life);
void AddDamageText(Vector3 pos, int damage, Color color, bool crit = false);
void TriggerExplosion(Vector3 pos, float radius, float damage, Color coreColor);
void CastFireball(Vector3 from, Vector3 dir);
void CastFrostNova();
void CastThunderstrike();
void CastBlink();
void InitPerks();
void TriggerLevelUp();
void ApplyPerk(int perkId);
void SpawnEnemy(EnemyType type);
void StartWave(int wave, bool saveProgress = true);
void ResetGame();
void UpdateGameplay(float dt);
