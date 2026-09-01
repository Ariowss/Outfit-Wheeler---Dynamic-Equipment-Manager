#include "pch.h"
#include "Settings.h"
#include <windows.h>
#include <filesystem>
#include <string>

float g_wheelScale = 1.0f;
float g_wheelOffsetX = 0.0f;
float g_wheelOffsetY = 0.0f;

uint32_t g_editKey = 0x31;
uint32_t g_editModifierKey = 0x00;
uint32_t g_playKey = 0x2F;
uint32_t g_playModifierKey = 0x00;

uint32_t g_gamepadPlayKey = 0x0008;
uint32_t g_gamepadPlayModifier = 0x0100;
float g_thumbstickDeadzone = 0.25f;

bool g_autoCloseOnEquip = true;
bool g_autoStashEnabled = false;
bool g_autoSystemEnabled = true;
bool g_autoCityEnabled = true;
bool g_autoHomeEnabled = true;
bool g_autoCombatEnabled = true;
bool g_autoSwimEnabled = true;

static std::string GetIniPath() {
    return std::filesystem::absolute("Data/MCM/Settings/OutfitWheeler.ini").string();
}

static float GetIniFloat(const char* section, const char* key, float defVal, const std::string& path) {
    char buf[64];
    GetPrivateProfileStringA(section, key, std::to_string(defVal).c_str(), buf, sizeof(buf), path.c_str());
    try {
        return std::stof(buf);
    } catch (...) {
        return defVal;
    }
}

void LoadINI() {
    std::string path = GetIniPath();
    if (!std::filesystem::exists("Data/MCM/Settings/OutfitWheeler.ini")) {
        return;
    }

    g_wheelScale = GetIniFloat("Appearance", "fWheelScale", 1.0f, path);
    g_wheelOffsetX = GetIniFloat("Appearance", "fWheelOffsetX", 0.0f, path);
    g_wheelOffsetY = GetIniFloat("Appearance", "fWheelOffsetY", 0.0f, path);

    g_editKey = static_cast<uint32_t>(GetPrivateProfileIntA("Controls", "uEditKey", 0x31, path.c_str()));
    g_editModifierKey = static_cast<uint32_t>(GetPrivateProfileIntA("Controls", "uEditModifierKey", 0x00, path.c_str()));
    g_playKey = static_cast<uint32_t>(GetPrivateProfileIntA("Controls", "uPlayKey", 0x2F, path.c_str()));
    g_playModifierKey = static_cast<uint32_t>(GetPrivateProfileIntA("Controls", "uPlayModifierKey", 0x00, path.c_str()));

    g_gamepadPlayKey = static_cast<uint32_t>(GetPrivateProfileIntA("Gamepad", "uGamepadPlayKey", 0x0008, path.c_str()));
    g_gamepadPlayModifier = static_cast<uint32_t>(GetPrivateProfileIntA("Gamepad", "uGamepadPlayModifier", 0x0100, path.c_str()));
    g_thumbstickDeadzone = GetIniFloat("Gamepad", "fThumbstickDeadzone", 0.25f, path);

    g_autoCloseOnEquip = GetPrivateProfileIntA("General", "bAutoCloseOnEquip", 1, path.c_str()) != 0;
    g_autoStashEnabled = GetPrivateProfileIntA("AdaptiveWardrobe", "bAutoStashEnabled", 0, path.c_str()) != 0;
    g_autoSystemEnabled = GetPrivateProfileIntA("AdaptiveWardrobe", "bAutoSystemEnabled", 1, path.c_str()) != 0;
    g_autoCityEnabled = GetPrivateProfileIntA("AdaptiveWardrobe", "bAutoCityEnabled", 1, path.c_str()) != 0;
    g_autoHomeEnabled = GetPrivateProfileIntA("AdaptiveWardrobe", "bAutoHomeEnabled", 1, path.c_str()) != 0;
    g_autoCombatEnabled = GetPrivateProfileIntA("AdaptiveWardrobe", "bAutoCombatEnabled", 1, path.c_str()) != 0;
    g_autoSwimEnabled = GetPrivateProfileIntA("AdaptiveWardrobe", "bAutoSwimEnabled", 1, path.c_str()) != 0;
}

void SaveINI() {
    std::filesystem::create_directories("Data/MCM/Settings");
    std::string path = GetIniPath();

    WritePrivateProfileStringA("Appearance", "fWheelScale", std::to_string(g_wheelScale).c_str(), path.c_str());
    WritePrivateProfileStringA("Appearance", "fWheelOffsetX", std::to_string(g_wheelOffsetX).c_str(), path.c_str());
    WritePrivateProfileStringA("Appearance", "fWheelOffsetY", std::to_string(g_wheelOffsetY).c_str(), path.c_str());

    WritePrivateProfileStringA("Controls", "uEditKey", std::to_string(g_editKey).c_str(), path.c_str());
    WritePrivateProfileStringA("Controls", "uEditModifierKey", std::to_string(g_editModifierKey).c_str(), path.c_str());
    WritePrivateProfileStringA("Controls", "uPlayKey", std::to_string(g_playKey).c_str(), path.c_str());
    WritePrivateProfileStringA("Controls", "uPlayModifierKey", std::to_string(g_playModifierKey).c_str(), path.c_str());

    WritePrivateProfileStringA("Gamepad", "uGamepadPlayKey", std::to_string(g_gamepadPlayKey).c_str(), path.c_str());
    WritePrivateProfileStringA("Gamepad", "uGamepadPlayModifier", std::to_string(g_gamepadPlayModifier).c_str(), path.c_str());
    WritePrivateProfileStringA("Gamepad", "fThumbstickDeadzone", std::to_string(g_thumbstickDeadzone).c_str(), path.c_str());

    WritePrivateProfileStringA("General", "bAutoCloseOnEquip", g_autoCloseOnEquip ? "1" : "0", path.c_str());
    WritePrivateProfileStringA("AdaptiveWardrobe", "bAutoStashEnabled", g_autoStashEnabled ? "1" : "0", path.c_str());
    WritePrivateProfileStringA("AdaptiveWardrobe", "bAutoSystemEnabled", g_autoSystemEnabled ? "1" : "0", path.c_str());
    WritePrivateProfileStringA("AdaptiveWardrobe", "bAutoCityEnabled", g_autoCityEnabled ? "1" : "0", path.c_str());
    WritePrivateProfileStringA("AdaptiveWardrobe", "bAutoHomeEnabled", g_autoHomeEnabled ? "1" : "0", path.c_str());
    WritePrivateProfileStringA("AdaptiveWardrobe", "bAutoCombatEnabled", g_autoCombatEnabled ? "1" : "0", path.c_str());
    WritePrivateProfileStringA("AdaptiveWardrobe", "bAutoSwimEnabled", g_autoSwimEnabled ? "1" : "0", path.c_str());
}