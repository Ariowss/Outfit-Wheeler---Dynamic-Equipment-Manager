#include "Settings.h"
#include <windows.h>
#include <filesystem>
#include <string>
#include <SKSE/SKSE.h>

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

bool g_autoStashEnabled = false;
bool g_autoSystemEnabled = true;
bool g_autoCityEnabled = true;
bool g_autoHomeEnabled = true;
bool g_autoCombatEnabled = true;
bool g_autoSwimEnabled = true;

static const std::string g_mcmIniPath = "Data/MCM/Settings/OutfitWheeler.ini";
static const std::string g_fallbackIniPath = "Data/SKSE/Plugins/PrismaOutfitWheel.ini";

void LoadINI() {
    std::string targetPath = g_mcmIniPath;
    if (!std::filesystem::exists(targetPath)) {
        targetPath = g_fallbackIniPath;
    }

    if (std::filesystem::exists(targetPath)) {
        g_editKey = GetPrivateProfileIntA("Hotkeys", "kEditModeKey", 0x31, targetPath.c_str());
        g_editModifierKey = GetPrivateProfileIntA("Hotkeys", "kEditModeModifier", 0x00, targetPath.c_str());
        g_playKey = GetPrivateProfileIntA("Hotkeys", "kPlayModeKey", 0x2F, targetPath.c_str());
        g_playModifierKey = GetPrivateProfileIntA("Hotkeys", "kPlayModeModifier", 0x00, targetPath.c_str());

        g_gamepadPlayKey = GetPrivateProfileIntA("Gamepad", "kGamepadPlayKey", 0x0008, targetPath.c_str());
        g_gamepadPlayModifier = GetPrivateProfileIntA("Gamepad", "kGamepadPlayModifier", 0x0100, targetPath.c_str());

        g_autoStashEnabled = GetPrivateProfileIntA("General", "bAutoStashInactive", 0, targetPath.c_str()) != 0;
        g_autoSystemEnabled = GetPrivateProfileIntA("AutoSystem", "bMasterAutoSystem", 1, targetPath.c_str()) != 0;
        g_autoCityEnabled = GetPrivateProfileIntA("AutoSystem", "bAutoCity", 1, targetPath.c_str()) != 0;
        g_autoHomeEnabled = GetPrivateProfileIntA("AutoSystem", "bAutoHome", 1, targetPath.c_str()) != 0;
        g_autoCombatEnabled = GetPrivateProfileIntA("AutoSystem", "bAutoCombat", 1, targetPath.c_str()) != 0;
        g_autoSwimEnabled = GetPrivateProfileIntA("AutoSystem", "bAutoSwim", 1, targetPath.c_str()) != 0;

        char buffer[32];
        if (GetPrivateProfileStringA("General", "fWheelScale", "1.0", buffer, sizeof(buffer), targetPath.c_str())) {
            try { g_wheelScale = std::stof(buffer); } catch (...) { g_wheelScale = 1.0f; }
        }
        if (GetPrivateProfileStringA("General", "fWheelOffsetX", "0.0", buffer, sizeof(buffer), targetPath.c_str())) {
            try { g_wheelOffsetX = std::stof(buffer); } catch (...) { g_wheelOffsetX = 0.0f; }
        }
        if (GetPrivateProfileStringA("General", "fWheelOffsetY", "0.0", buffer, sizeof(buffer), targetPath.c_str())) {
            try { g_wheelOffsetY = std::stof(buffer); } catch (...) { g_wheelOffsetY = 0.0f; }
        }
        if (GetPrivateProfileStringA("Gamepad", "fThumbstickDeadzone", "0.25", buffer, sizeof(buffer), targetPath.c_str())) {
            try { g_thumbstickDeadzone = std::stof(buffer); } catch (...) { g_thumbstickDeadzone = 0.25f; }
        }
    } else {
        SaveINI();
    }
}

void SaveINI() {
    std::filesystem::create_directories("Data/MCM/Settings");
    WritePrivateProfileStringA("Hotkeys", "kEditModeKey", std::to_string(g_editKey).c_str(), g_mcmIniPath.c_str());
    WritePrivateProfileStringA("Hotkeys", "kEditModeModifier", std::to_string(g_editModifierKey).c_str(), g_mcmIniPath.c_str());
    WritePrivateProfileStringA("Hotkeys", "kPlayModeKey", std::to_string(g_playKey).c_str(), g_mcmIniPath.c_str());
    WritePrivateProfileStringA("Hotkeys", "kPlayModeModifier", std::to_string(g_playModifierKey).c_str(), g_mcmIniPath.c_str());

    WritePrivateProfileStringA("Gamepad", "kGamepadPlayKey", std::to_string(g_gamepadPlayKey).c_str(), g_mcmIniPath.c_str());
    WritePrivateProfileStringA("Gamepad", "kGamepadPlayModifier", std::to_string(g_gamepadPlayModifier).c_str(), g_mcmIniPath.c_str());
    WritePrivateProfileStringA("Gamepad", "fThumbstickDeadzone", std::to_string(g_thumbstickDeadzone).c_str(), g_mcmIniPath.c_str());

    WritePrivateProfileStringA("General", "fWheelScale", std::to_string(g_wheelScale).c_str(), g_mcmIniPath.c_str());
    WritePrivateProfileStringA("General", "fWheelOffsetX", std::to_string(g_wheelOffsetX).c_str(), g_mcmIniPath.c_str());
    WritePrivateProfileStringA("General", "fWheelOffsetY", std::to_string(g_wheelOffsetY).c_str(), g_mcmIniPath.c_str());
    WritePrivateProfileStringA("General", "bAutoStashInactive", g_autoStashEnabled ? "1" : "0", g_mcmIniPath.c_str());

    WritePrivateProfileStringA("AutoSystem", "bMasterAutoSystem", g_autoSystemEnabled ? "1" : "0", g_mcmIniPath.c_str());
    WritePrivateProfileStringA("AutoSystem", "bAutoCity", g_autoCityEnabled ? "1" : "0", g_mcmIniPath.c_str());
    WritePrivateProfileStringA("AutoSystem", "bAutoHome", g_autoHomeEnabled ? "1" : "0", g_mcmIniPath.c_str());
    WritePrivateProfileStringA("AutoSystem", "bAutoCombat", g_autoCombatEnabled ? "1" : "0", g_mcmIniPath.c_str());
    WritePrivateProfileStringA("AutoSystem", "bAutoSwim", g_autoSwimEnabled ? "1" : "0", g_mcmIniPath.c_str());
}