#include "Settings.h"
#include <windows.h>
#include <filesystem>
#include <string>

uint32_t g_editKey = 0x31;
uint32_t g_editModifierKey = 0x00;
uint32_t g_playKey = 0x2F;
uint32_t g_playModifierKey = 0x00;

uint32_t g_gamepadPlayKey = 0x0008;
uint32_t g_gamepadPlayModifier = 0x0100;
float g_thumbstickDeadzone = 0.25f;

float g_wheelScale = 1.0f;
float g_wheelOffsetX = 0.0f;
float g_wheelOffsetY = 0.0f;

bool g_autoCloseOnEquip = true;
bool g_autoStashEnabled = true;
bool g_autoSystemEnabled = true;
bool g_autoCityEnabled = true;
bool g_autoHomeEnabled = true;
bool g_autoCombatEnabled = true;
bool g_autoSwimEnabled = true;

static std::string GetIniPath() {
    std::filesystem::create_directories("Data/SKSE/Plugins");
    return std::filesystem::absolute("Data/SKSE/Plugins/OutfitWheeler.ini").string();
}

void LoadINI() {
    std::string path = GetIniPath();
    if (!std::filesystem::exists(path)) {
        SaveINI();
        return;
    }

    g_editKey = GetPrivateProfileIntA("Hotkeys", "kEditModeKey", 0x31, path.c_str());
    g_editModifierKey = GetPrivateProfileIntA("Hotkeys", "kEditModeModifier", 0x00, path.c_str());
    g_playKey = GetPrivateProfileIntA("Hotkeys", "kPlayModeKey", 0x2F, path.c_str());
    g_playModifierKey = GetPrivateProfileIntA("Hotkeys", "kPlayModeModifier", 0x00, path.c_str());

    g_gamepadPlayKey = GetPrivateProfileIntA("Gamepad", "kGamepadPlayKey", 0x0008, path.c_str());
    g_gamepadPlayModifier = GetPrivateProfileIntA("Gamepad", "kGamepadPlayModifier", 0x0100, path.c_str());

    g_autoCloseOnEquip = GetPrivateProfileIntA("AutoSystem", "bAutoCloseOnEquip", 1, path.c_str()) != 0;
    g_autoStashEnabled = GetPrivateProfileIntA("AutoSystem", "bAutoStashInactive", 1, path.c_str()) != 0;
    g_autoCityEnabled = GetPrivateProfileIntA("AutoSystem", "bAutoCity", 1, path.c_str()) != 0;
    g_autoHomeEnabled = GetPrivateProfileIntA("AutoSystem", "bAutoHome", 1, path.c_str()) != 0;
    g_autoCombatEnabled = GetPrivateProfileIntA("AutoSystem", "bAutoCombat", 1, path.c_str()) != 0;
    g_autoSwimEnabled = GetPrivateProfileIntA("AutoSystem", "bAutoSwim", 1, path.c_str()) != 0;

    char buffer[64];
    if (GetPrivateProfileStringA("Appearance", "fWheelScale", "1.0", buffer, sizeof(buffer), path.c_str())) {
        try { g_wheelScale = std::stof(buffer); } catch (...) { g_wheelScale = 1.0f; }
    }
    if (GetPrivateProfileStringA("Appearance", "fWheelOffsetX", "0.0", buffer, sizeof(buffer), path.c_str())) {
        try { g_wheelOffsetX = std::stof(buffer); } catch (...) { g_wheelOffsetX = 0.0f; }
    }
    if (GetPrivateProfileStringA("Appearance", "fWheelOffsetY", "0.0", buffer, sizeof(buffer), path.c_str())) {
        try { g_wheelOffsetY = std::stof(buffer); } catch (...) { g_wheelOffsetY = 0.0f; }
    }
}

void SaveINI() {
    std::string path = GetIniPath();

    WritePrivateProfileStringA("Hotkeys", "kEditModeKey", std::to_string(g_editKey).c_str(), path.c_str());
    WritePrivateProfileStringA("Hotkeys", "kEditModeModifier", std::to_string(g_editModifierKey).c_str(), path.c_str());
    WritePrivateProfileStringA("Hotkeys", "kPlayModeKey", std::to_string(g_playKey).c_str(), path.c_str());
    WritePrivateProfileStringA("Hotkeys", "kPlayModeModifier", std::to_string(g_playModifierKey).c_str(), path.c_str());

    WritePrivateProfileStringA("Gamepad", "kGamepadPlayKey", std::to_string(g_gamepadPlayKey).c_str(), path.c_str());
    WritePrivateProfileStringA("Gamepad", "kGamepadPlayModifier", std::to_string(g_gamepadPlayModifier).c_str(), path.c_str());

    WritePrivateProfileStringA("Appearance", "fWheelScale", std::to_string(g_wheelScale).c_str(), path.c_str());
    WritePrivateProfileStringA("Appearance", "fWheelOffsetX", std::to_string(g_wheelOffsetX).c_str(), path.c_str());
    WritePrivateProfileStringA("Appearance", "fWheelOffsetY", std::to_string(g_wheelOffsetY).c_str(), path.c_str());

    WritePrivateProfileStringA("AutoSystem", "bAutoCloseOnEquip", g_autoCloseOnEquip ? "1" : "0", path.c_str());
    WritePrivateProfileStringA("AutoSystem", "bAutoStashInactive", g_autoStashEnabled ? "1" : "0", path.c_str());
    WritePrivateProfileStringA("AutoSystem", "bAutoCity", g_autoCityEnabled ? "1" : "0", path.c_str());
    WritePrivateProfileStringA("AutoSystem", "bAutoHome", g_autoHomeEnabled ? "1" : "0", path.c_str());
    WritePrivateProfileStringA("AutoSystem", "bAutoCombat", g_autoCombatEnabled ? "1" : "0", path.c_str());
    WritePrivateProfileStringA("AutoSystem", "bAutoSwim", g_autoSwimEnabled ? "1" : "0", path.c_str());

    WritePrivateProfileStringA(NULL, NULL, NULL, path.c_str());
}