#include "core/GameContext.h"
#include "gameplay/EnemySystem.h"
#include "gameplay/Gameplay.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
namespace fs = std::filesystem;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool Near(float actual, float expected) {
    return std::fabs(actual - expected) < 0.001f;
}

struct IsolatedWorkingDirectory {
    fs::path previous = fs::current_path();
    fs::path temporary = fs::temp_directory_path() /
        ("arcane-combat-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

    IsolatedWorkingDirectory() {
        fs::create_directories(temporary);
        fs::current_path(temporary);
    }
    ~IsolatedWorkingDirectory() {
        std::error_code error;
        fs::current_path(previous, error);
        fs::remove_all(temporary, error);
    }
};

void ResetFixture() {
    G = {};
    G.settings = {0.8f, 1.0f, 54.0f, true, 0, false};
    InitPerks();
    ResetGame();
    G.state = STATE_PLAYING;
    G.mouseCaptured = false;
}

Enemy MakeEnemy(EnemyType type, Vector3 position = {0.0f, 0.0f, 0.0f}) {
    Enemy enemy{};
    enemy.type = type;
    enemy.pos = position;
    enemy.hp = enemy.maxHp = 1000.0f;
    enemy.speed = 0.0f;
    enemy.radius = 0.8f;
    enemy.attackCooldown = 1000.0f;
    enemy.attackDamage = 20.0f;
    enemy.active = true;
    return enemy;
}

void OfferAndApply(int perkId) {
    const auto perk = std::find_if(G.availablePerks.begin(), G.availablePerks.end(),
        [perkId](const Perk& candidate) { return candidate.id == perkId; });
    Check(perk != G.availablePerks.end(), "Test requested an unknown perk");
    G.currentPerkChoices = {*perk};
    G.state = STATE_PERK_SELECT;
    ApplyPerk(perkId);
}

void TestShatter() {
    ResetFixture();
    Enemy frozen = MakeEnemy(ENEMY_GOLEM);
    frozen.frozenTimer = 3.0f;
    G.enemies = {frozen};
    TriggerExplosion({0.0f, 0.0f, 0.0f}, 5.0f, 40.0f, ORANGE, true);
    Check(Near(G.enemies[0].hp, 940.0f), "Fire did not shatter for 1.5x damage");
    Check(Near(G.enemies[0].frozenTimer, 0.0f), "Shatter did not consume freeze");
    TriggerExplosion({0.0f, 0.0f, 0.0f}, 5.0f, 40.0f, ORANGE, true);
    Check(Near(G.enemies[0].hp, 900.0f), "Shatter bonus repeated without a new freeze");

    G.enemies = {frozen};
    TriggerExplosion({0.0f, 0.0f, 0.0f}, 5.0f, 40.0f, PURPLE);
    Check(Near(G.enemies[0].hp, 960.0f) && Near(G.enemies[0].frozenTimer, 3.0f),
        "Non-fire explosion incorrectly consumed or amplified freeze");
}

void TestFrostHitsOnce() {
    ResetFixture();
    Enemy golem = MakeEnemy(ENEMY_GOLEM, {1.0f, 0.0f, 0.0f});
    golem.attackWindup = GolemSlamWindup;
    Enemy wraith = MakeEnemy(ENEMY_FROST_WRAITH, {1.0f, 0.0f, 0.0f});
    G.enemies = {golem, wraith};
    CastFrostNova();
    UpdateFrostNovas(0.01f);
    Check(Near(G.enemies[0].hp, 965.0f), "Frost did not damage a newly reached enemy");
    Check(G.enemies[0].frozenTimer > 0.0f && Near(G.enemies[0].attackWindup, 0.0f),
        "Frost did not immediately interrupt the incoming slam");
    Check(Near(G.enemies[1].hp, 986.0f) && Near(G.enemies[1].frozenTimer, 0.0f),
        "Frost wraith did not receive a single reduced hit without freeze");

    // A thawed or shattered target must not be hit again by the same expanding cast.
    G.enemies[0].frozenTimer = 0.0f;
    UpdateFrostNovas(0.01f);
    Check(Near(G.enemies[0].hp, 965.0f) && Near(G.enemies[0].frozenTimer, 0.0f),
        "The same nova damaged or refroze an enemy after thaw");
    Check(Near(G.enemies[1].hp, 986.0f), "The same nova damaged a frost wraith repeatedly");

    G.lightnings.push_back({{0.0f, 10.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 0.2f, true});
    StartWave(2, false);
    Check(G.frostNovas.empty() && G.lightnings.empty(),
        "A new wave retained effects or per-enemy frost hit history");
}

void TestBlinkAndPerks() {
    ResetFixture();
    G.playerMana = 0.0f;
    G.playerVel = {1.0f, 0.0f, 0.0f};
    CastBlink();
    Check(G.playerPos.x > 10.0f && Near(G.playerMana, 0.0f),
        "Blink could not evade with empty mana");
    Check(Near(G.cdBlink, 2.8f), "Blink did not start its base cooldown");
    const Vector3 afterFirst = G.playerPos;
    CastBlink();
    Check(Near(G.playerPos.x, afterFirst.x) && Near(G.playerPos.z, afterFirst.z),
        "Blink bypassed its active cooldown");
    G.cdBlink = 0.0f;
    G.playerPos = {32.0f, 0.0f, 0.0f};
    CastBlink();
    Check(std::hypot(G.playerPos.x, G.playerPos.z) <= 33.5f,
        "Blink escaped the arena boundary");

    const float originalCooldown = GetBlinkCooldown();
    OfferAndApply(5);
    Check(GetBlinkCooldown() < originalCooldown, "Fleetfoot did not reduce Blink cooldown");
    G.perkSpeedMult = 10.0f;
    Check(Near(GetBlinkCooldown(), 1.2f), "Blink cooldown fell below its minimum");

    OfferAndApply(3);
    Check(Near(G.perkFrostDuration, 2.0f) && Near(GetFrostDamage(), 45.5f),
        "Absolute Zero did not improve both duration and damage");
    G.playerMana = G.playerMaxMana;
    G.cdFrost = 0.0f;
    CastFrostNova();
    Check(Near(G.frostNovas.back().damage, 45.5f), "Frost cast ignored its damage perk");
    OfferAndApply(6);
    Check(Near(G.perkManaRegenMult, 2.0f) && Near(G.playerMaxMana, 140.0f),
        "Mana Wellspring did not grant its advertised regeneration and capacity");
}

void TestPerkChoicesAndQueuedLevels() {
    ResetFixture();
    OfferAndApply(1);
    Check(G.perkFireMultishot == 3, "First Arcane Triad did not grant three projectiles");
    OfferAndApply(1);
    Check(G.perkFireMultishot == 5, "Repeated Arcane Triad did not add projectiles");
    OfferAndApply(1);
    Check(G.perkFireMultishot == 7, "Arcane Triad did not reach its cap");
    G.playerXp = G.playerNextXp;
    TriggerLevelUp();
    Check(std::none_of(G.currentPerkChoices.begin(), G.currentPerkChoices.end(),
        [](const Perk& perk) { return perk.id == 1; }), "Capped Arcane Triad was offered again");

    ResetFixture();
    G.playerXp = 400;
    TriggerLevelUp();
    Check(G.state == STATE_PERK_SELECT && G.playerLevel == 2 && G.playerXp == 340,
        "Earned XP did not open the first level reward");
    const auto choices = G.currentPerkChoices;
    TriggerLevelUp();
    Check(G.playerLevel == 2 && G.playerXp == 340 &&
        G.currentPerkChoices.size() == choices.size() && G.currentPerkChoices[0].id == choices[0].id,
        "A pending level overwrote an unresolved reward");
    int selections = 0;
    while (G.state == STATE_PERK_SELECT && selections < 5) {
        Check(!G.currentPerkChoices.empty(), "Level reward has no choices");
        ApplyPerk(G.currentPerkChoices.front().id);
        ++selections;
    }
    Check(selections == 3 && G.playerLevel == 4 && G.playerXp == 115 && G.state == STATE_PLAYING,
        "Accumulated XP did not resolve one perk choice per earned level");
}

void BeginSlam() {
    ResetFixture();
    Enemy golem = MakeEnemy(ENEMY_GOLEM, {2.0f, 0.0f, 0.0f});
    golem.attackCooldown = 0.0f;
    G.enemies = {golem};
    UpdateEnemies(0.01f, 33.5f);
    Check(G.enemies[0].attackWindup > 0.0f && Near(G.playerHp, 100.0f),
        "Golem dealt damage before showing its windup");
}

void TestGolemCounterplay() {
    BeginSlam();
    const Vector3 origin = G.enemies[0].attackOrigin;
    const Vector3 position = G.enemies[0].pos;
    UpdateEnemies(0.3f, 33.5f);
    Check(Near(G.playerHp, 100.0f) && Near(G.enemies[0].pos.x, position.x),
        "Golem moved or hit before its windup completed");
    G.playerPos = {12.0f, 0.0f, 0.0f};
    UpdateEnemies(GolemSlamWindup, 33.5f);
    Check(Near(G.playerHp, 100.0f), "Dodging outside the warning circle did not avoid damage");
    Check(Near(G.enemies[0].attackOrigin.x, origin.x), "Committed slam tracked the player");

    BeginSlam();
    UpdateEnemies(GolemSlamWindup + 0.01f, 33.5f);
    Check(Near(G.playerHp, 80.0f) && G.invulnerableTimer > 0.0f,
        "A completed slam did not hit exactly once inside its warning circle");
    UpdateEnemies(0.05f, 33.5f);
    Check(Near(G.playerHp, 80.0f), "A resolved slam inflicted damage again");

    BeginSlam();
    G.invulnerableTimer = 1.0f;
    UpdateEnemies(GolemSlamWindup + 0.01f, 33.5f);
    Check(Near(G.playerHp, 100.0f), "Blink invulnerability did not block the slam");

    BeginSlam();
    G.enemies[0].frozenTimer = 1.0f;
    UpdateEnemies(0.05f, 33.5f);
    Check(Near(G.enemies[0].attackWindup, 0.0f), "Freeze did not cancel the warned attack");
    UpdateEnemies(1.1f, 33.5f);
    Check(Near(G.playerHp, 100.0f), "Canceled slam still resolved while frozen");
}

void TestDeathAndSummoning() {
    ResetFixture();
    Enemy corpse = MakeEnemy(ENEMY_GOLEM);
    corpse.hp = 0.0f;
    corpse.frozenTimer = 10.0f;
    corpse.attackWindup = GolemSlamWindup;
    G.enemies = {corpse};
    UpdateEnemies(0.05f, 33.5f);
    Check(!G.enemies[0].active && G.kills == 1 && G.gems.size() == 4 && Near(G.playerHp, 100.0f),
        "A frozen corpse attacked or failed to pay its kill rewards");
    UpdateEnemies(1.0f, 33.5f);
    Check(G.kills == 1 && G.gems.size() == 4, "Enemy death rewards were paid twice");

    ResetFixture();
    Enemy summoner = MakeEnemy(ENEMY_NECROMANCER, {16.0f, 0.0f, 0.0f});
    summoner.attackCooldown = 0.0f;
    G.enemies = {summoner};
    UpdateEnemies(0.05f, 33.5f);
    Check(G.enemies.size() == 2 && Near(G.enemies[0].attackCooldown, 4.5f),
        "Summoning lost the original enemy's cooldown or failed to create its minion");
    Check(G.enemies[1].type == ENEMY_WISP && Near(G.enemies[1].animTimer, 0.0f),
        "A deferred summon acted during its creation frame");

    ResetFixture();
    G.enemies.assign(MaxSummonedEnemyPopulation - 1, summoner);
    UpdateEnemies(0.05f, 33.5f);
    Check(G.enemies.size() == MaxSummonedEnemyPopulation,
        "Concurrent necromancers exceeded the active population cap");
    for (auto& enemy : G.enemies) enemy.attackCooldown = 0.0f;
    UpdateEnemies(0.05f, 33.5f);
    Check(G.enemies.size() == MaxSummonedEnemyPopulation,
        "Necromancers kept summoning at the population cap");
}

void TestLivingTargets() {
    ResetFixture();
    Enemy corpse = MakeEnemy(ENEMY_WISP, {1.0f, 0.0f, 0.0f});
    corpse.hp = 0.0f;
    Enemy first = MakeEnemy(ENEMY_GOLEM, {4.0f, 0.0f, 0.0f});
    Enemy fragile = MakeEnemy(ENEMY_GOLEM, {6.0f, 0.0f, 0.0f});
    fragile.hp = 10.0f;
    Enemy survivor = MakeEnemy(ENEMY_GOLEM, {15.0f, 0.0f, 0.0f});
    G.enemies = {corpse, first, fragile, survivor};
    CastThunderstrike();
    Check(Near(G.lightnings.front().end.x, 4.0f), "Thunder selected an already dead enemy");
    Check(G.enemies[2].hp <= 0.0f && Near(G.enemies[3].hp, 935.0f),
        "Thunder wasted its chain on an enemy killed by its initial blast");

    ResetFixture();
    corpse = MakeEnemy(ENEMY_WISP, {0.0f, 1.0f, 5.0f});
    corpse.hp = 0.0f;
    survivor = MakeEnemy(ENEMY_WISP, {0.0f, 1.0f, 10.0f});
    G.enemies = {corpse, survivor};
    Projectile fireball{};
    fireball.pos = corpse.pos;
    fireball.radius = 0.45f;
    fireball.damage = 45.0f;
    fireball.type = 0;
    fireball.life = 1.0f;
    fireball.active = true;
    fireball.color = ORANGE;
    G.projectiles = {fireball};
    UpdateGameplay(0.01f);
    Check(G.projectiles[0].active && Near(G.enemies[1].hp, 1000.0f),
        "A dead enemy absorbed a fireball before death cleanup");
}

void TestLethalHitPrecedesRewards() {
    for (bool collectibleGem : {false, true}) {
        ResetFixture();
        G.playerHp = 10.0f;
        G.waveIntroTimer = 0.0f;
        G.enemiesToSpawn = 0;
        Projectile bolt{};
        bolt.pos = {0.0f, 1.15f, 0.0f};
        bolt.radius = 0.4f;
        bolt.damage = collectibleGem ? 200.0f : 20.0f;
        bolt.type = 1;
        bolt.life = 1.0f;
        bolt.active = true;
        bolt.color = PURPLE;
        G.projectiles = {bolt};
        if (collectibleGem) G.gems.push_back({{0.0f, 0.0f, 0.0f}, 60.0f, 10.0f, true});
        UpdateGameplay(0.01f);
        Check(G.state == STATE_GAMEOVER && G.playerHp <= 0.0f,
            "A lethal hit was reversed by a same-frame level or wave reward");
        Check(G.playerXp == 0 && G.playerLevel == 1 && G.wave == 1,
            "A defeated player collected XP or advanced to another wave");
    }
}
} // namespace

int main() {
    try {
        IsolatedWorkingDirectory isolated;
        TestShatter();
        TestFrostHitsOnce();
        TestBlinkAndPerks();
        TestPerkChoicesAndQueuedLevels();
        TestGolemCounterplay();
        TestDeathAndSummoning();
        TestLivingTargets();
        TestLethalHitPrecedesRewards();
        std::cout << "Combat/progression regressions passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
