#pragma once

#include <cstddef>

inline constexpr float GolemSlamWindup = 0.7f;
inline constexpr float GolemSlamRadius = 3.2f;
inline constexpr std::size_t MaxSummonedEnemyPopulation = 48;

void UpdateEnemies(float dt, float arenaRadius);
