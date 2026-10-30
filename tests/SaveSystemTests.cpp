#include "core/GameContext.h"
#include "gameplay/Gameplay.h"
#include "persistence/SaveSystem.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
namespace fs = std::filesystem;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct IsolatedWorkingDirectory {
    fs::path previous = fs::current_path();
    fs::path temporary = fs::temp_directory_path() /
        ("arcane-save-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

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

std::vector<char> ReadBytes(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    Check(file.good(), "Could not read test save");
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

template<typename T>
void WriteLegacyRecord(const fs::path& path, const T& record, std::size_t bytes = sizeof(T)) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(&record), static_cast<std::streamsize>(bytes));
    Check(file.good(), "Could not write legacy fixture");
}

SaveData SavedRun() {
    SaveData data{};
    data.magic = 0x4152434E;
    data.wave = 3;
    data.level = 4;
    data.xp = 18;
    data.nextXp = 150;
    data.score = 1234;
    data.kills = 42;
    data.hp = 73.0f;
    data.maxHp = 100.0f;
    data.mana = 48.0f;
    data.maxMana = 100.0f;
    data.perkFireMultishot = 2;
    data.perkExplosionRadius = 1.5f;
    data.perkFrostDuration = 0.5f;
    data.perkChainCount = 2;
    data.perkSpeedMult = 1.25f;
    data.perkManaRegenMult = 1.4f;
    return data;
}

void CheckLoadedRun(const SaveData& data) {
    Check(G.state == STATE_PLAYING, "Continue should enter gameplay");
    Check(G.wave == data.wave && G.playerLevel == data.level && G.playerXp == data.xp &&
        G.playerNextXp == data.nextXp && G.score == data.score && G.kills == data.kills,
        "Continue lost progression fields");
    Check(G.playerHp == data.hp && G.playerMaxHp == data.maxHp &&
        G.playerMana == data.mana && G.playerMaxMana == data.maxMana,
        "Continue lost health or mana");
    Check(G.perkFireMultishot == data.perkFireMultishot &&
        G.perkExplosionRadius == data.perkExplosionRadius &&
        G.perkFrostDuration == data.perkFrostDuration && G.perkChainCount == data.perkChainCount &&
        G.perkSpeedMult == data.perkSpeedMult && G.perkManaRegenMult == data.perkManaRegenMult,
        "Continue lost perk fields");
}

void TestStartupAndContinue() {
    Check(!HasSaveGame(), "An empty working directory should have no Continue option");
    ResetGame();
    Check(!fs::exists("savegame.dat"), "Startup/reset created an unsolicited save");

    const SaveData legacy = SavedRun();
    WriteLegacyRecord("savegame.dat", legacy);
    const auto beforeStartup = ReadBytes("savegame.dat");
    ResetGame();
    Check(ReadBytes("savegame.dat") == beforeStartup, "Startup/reset overwrote the previous run");
    Check(HasSaveGame(), "Legacy native save was not recognized");
    Check(LoadGameFromFile(), "Continue could not load a valid legacy save");
    CheckLoadedRun(legacy);
    Check(ReadBytes("savegame.dat") == beforeStartup, "Continue modified the on-disk save");

    SaveGameToFile();
    G.score = 0;
    G.playerHp = 1.0f;
    Check(LoadGameFromFile(), "Written save did not round-trip");
    CheckLoadedRun(legacy);

    G.score = 9876;
    StartWave(4);
    G.score = 0;
    Check(LoadGameFromFile() && G.wave == 4 && G.score == 9876,
        "Normal wave progression stopped autosaving");
}

void TestInvalidSaves() {
    auto rejected = [](const SaveData& record, std::size_t bytes) {
        WriteLegacyRecord("savegame.dat", record, bytes);
        G.state = STATE_PAUSED;
        G.score = 919;
        Check(!HasSaveGame(), "Continue accepted an invalid save");
        Check(!LoadGameFromFile(), "Invalid save loaded successfully");
        Check(G.state == STATE_PAUSED && G.score == 919, "Invalid save mutated the current run");
    };
    SaveData data = SavedRun();
    rejected(data, sizeof(data) - 1);
    data.magic = 0;
    rejected(data, sizeof(data));
    data = SavedRun();
    data.hp = std::numeric_limits<float>::quiet_NaN();
    rejected(data, sizeof(data));
    data = SavedRun();
    data.hp = 0.0f;
    rejected(data, sizeof(data));
    data = SavedRun();
    data.wave = 0;
    rejected(data, sizeof(data));
}

void CheckDefaultSettings() {
    Check(G.settings.masterVolume == 0.8f && G.settings.mouseSensitivity == 1.0f &&
        G.settings.cameraFov == 54.0f && G.settings.screenShake &&
        G.settings.crosshairStyle == 0 && !G.settings.isFullscreen,
        "Missing/truncated settings did not restore full defaults");
}

void TestSettings() {
    LoadSettingsFromFile();
    CheckDefaultSettings();
    G.settings = {0.25f, 1.8f, 65.0f, false, 2, true};
    SaveSettingsToFile();
    G.settings = {};
    LoadSettingsFromFile();
    Check(G.settings.masterVolume == 0.25f && G.settings.mouseSensitivity == 1.8f &&
        G.settings.cameraFov == 65.0f && !G.settings.screenShake &&
        G.settings.crosshairStyle == 2 && G.settings.isFullscreen,
        "Legacy settings did not round-trip");

    GameSettings settings{0.2f, 2.0f, 70.0f, false, 1, true};
    WriteLegacyRecord("settings.dat", settings, sizeof(float));
    LoadSettingsFromFile();
    CheckDefaultSettings();

    settings = {std::numeric_limits<float>::quiet_NaN(), 100.0f,
        std::numeric_limits<float>::infinity(), true, 100, false};
    WriteLegacyRecord("settings.dat", settings);
    LoadSettingsFromFile();
    Check(G.settings.masterVolume == 0.8f && G.settings.cameraFov == 54.0f &&
        G.settings.mouseSensitivity == 2.5f && G.settings.crosshairStyle == 2,
        "Invalid setting values were not repaired");

    settings = {-2.0f, -10.0f, -30.0f, true, -1, false};
    WriteLegacyRecord("settings.dat", settings);
    LoadSettingsFromFile();
    Check(G.settings.masterVolume == 0.0f && G.settings.mouseSensitivity == 0.4f &&
        G.settings.cameraFov == 45.0f && G.settings.crosshairStyle == 0,
        "Settings lower bounds were not enforced");
}
} // namespace

int main() {
    try {
        IsolatedWorkingDirectory isolated;
        TestStartupAndContinue();
        TestInvalidSaves();
        TestSettings();
        std::cout << "Save/settings regressions passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
