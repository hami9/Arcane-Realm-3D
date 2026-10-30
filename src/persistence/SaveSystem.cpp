#include "core/GameContext.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstddef>

#include "persistence/SaveSystem.h"
#include "gameplay/Gameplay.h"

namespace {
constexpr int SaveMagic = 0x4152434E;

float SanitizeSetting(float value, float minimum, float maximum, float fallback) {
    return std::isfinite(value) ? std::clamp(value, minimum, maximum) : fallback;
}

bool IsValidSave(const SaveData& data) {
    return data.magic == SaveMagic && data.wave > 0 && data.level > 0 &&
        data.xp >= 0 && data.nextXp > 0 && data.score >= 0 && data.kills >= 0 &&
        std::isfinite(data.hp) && std::isfinite(data.maxHp) &&
        data.hp > 0.0f && data.maxHp >= data.hp &&
        std::isfinite(data.mana) && std::isfinite(data.maxMana) &&
        data.mana >= 0.0f && data.maxMana > 0.0f && data.mana <= data.maxMana &&
        data.perkFireMultishot > 0 && data.perkChainCount > 0 &&
        std::isfinite(data.perkExplosionRadius) && data.perkExplosionRadius > 0.0f &&
        std::isfinite(data.perkFrostDuration) && data.perkFrostDuration >= 0.0f &&
        std::isfinite(data.perkSpeedMult) && data.perkSpeedMult > 0.0f &&
        std::isfinite(data.perkManaRegenMult) && data.perkManaRegenMult > 0.0f;
}

bool ReadSave(SaveData& data) {
    FILE* f = fopen("savegame.dat", "rb");
    if (!f) return false;
    const size_t count = fread(&data, sizeof(data), 1, f);
    fclose(f);
    return count == 1 && IsValidSave(data);
}
} // namespace

void ApplyMasterVolume() {
    if (IsAudioDeviceReady()) SetMasterVolume(G.settings.masterVolume);
}

void SaveSettingsToFile() {
    FILE* f = fopen("settings.dat", "wb");
    if (f) {
        fwrite(&G.settings, sizeof(GameSettings), 1, f);
        fclose(f);
    }
}

void LoadSettingsFromFile() {
    G.settings = {0.8f, 1.0f, 54.0f, true, 0, false};

    FILE* f = fopen("settings.dat", "rb");
    if (f) {
        GameSettings loaded{};
        const size_t count = fread(&loaded, sizeof(loaded), 1, f);
        fclose(f);
        if (count == 1) {
            loaded.masterVolume = SanitizeSetting(loaded.masterVolume, 0.0f, 1.0f, 0.8f);
            loaded.mouseSensitivity = SanitizeSetting(loaded.mouseSensitivity, 0.4f, 2.5f, 1.0f);
            loaded.cameraFov = SanitizeSetting(loaded.cameraFov, 45.0f, 75.0f, 54.0f);
            loaded.crosshairStyle = std::clamp(loaded.crosshairStyle, 0, 2);
            // Canonicalize native bool representations before reading the fields.
            const auto* bytes = reinterpret_cast<const unsigned char*>(&loaded);
            loaded.screenShake = bytes[offsetof(GameSettings, screenShake)] != 0;
            loaded.isFullscreen = bytes[offsetof(GameSettings, isFullscreen)] != 0;
            G.settings = loaded;
        }
    }
    ApplyMasterVolume();
}

bool HasSaveGame() {
    SaveData data{};
    return ReadSave(data);
}

void SaveGameToFile() {
    SaveData data{};
    data.magic = SaveMagic;
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

bool LoadGameFromFile() {
    SaveData data{};
    if (!ReadSave(data)) return false;

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

    StartWave(G.wave, false);

    G.state = STATE_PLAYING;
    if (G.mouseCaptured && IsWindowReady()) DisableCursor();
    return true;
}
