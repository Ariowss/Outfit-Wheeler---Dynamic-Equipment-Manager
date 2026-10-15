#include "Settings.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
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
float g_uiScale = 1.0f;
uint32_t g_editUiMode = 1;

std::atomic<bool> g_autoCloseOnEquip{ true };
std::atomic<bool> g_autoStashEnabled{ false };
std::atomic<bool> g_autoSystemEnabled{ false };
std::atomic<bool> g_autoCityEnabled{ false };
std::atomic<bool> g_autoHomeEnabled{ false };
std::atomic<bool> g_autoCombatEnabled{ false };
std::atomic<bool> g_autoSwimEnabled{ false };

std::string g_themeAccentColor = "#38bdf8";
std::string g_themeBgColor = "#070a0f";
std::string g_themeTextColor = "#f8fafc";
float g_themeDepth = 0.60f;
std::string g_themePreset = "sapphire";
std::atomic<bool> g_showItemPreviews{ true };

static std::string GetIniPath() {
    std::filesystem::create_directories("Data/SKSE/Plugins");
    return std::filesystem::absolute("Data/SKSE/Plugins/OutfitWheeler.ini").string();
}

static bool IniKeyExists(const char* section, const char* key, const char* iniPath) {
    char buf[32];
    return GetPrivateProfileStringA(section, key, "", buf, sizeof(buf), iniPath) > 0;
}

static uint32_t ReadIniKeyUint(const char* section, const char* key, uint32_t defaultVal, const char* iniPath) {
    char buf[64];
    if (GetPrivateProfileStringA(section, key, "", buf, sizeof(buf), iniPath)) {
        std::string s(buf);
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
        if (!s.empty()) {
            try {
                if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
                    return static_cast<uint32_t>(std::stoul(s, nullptr, 16));
                } else {
                    return static_cast<uint32_t>(std::stoul(s, nullptr, 10));
                }
            } catch (...) {}
        }
    }
    return defaultVal;
}

void LoadINI() {
    std::string path = GetIniPath();
    if (!std::filesystem::exists(path)) {
        std::error_code ec;
        if (std::filesystem::exists("Data/SKSE/Plugins/PrismaOutfitWheel.ini")) {
            std::filesystem::copy_file("Data/SKSE/Plugins/PrismaOutfitWheel.ini", path, std::filesystem::copy_options::overwrite_existing, ec);
            std::filesystem::remove("Data/SKSE/Plugins/PrismaOutfitWheel.ini", ec);
        } else if (std::filesystem::exists("Data/SKSE/Plugins/OutfitWheeler-DynamicEquipmentManager.ini")) {
            std::filesystem::copy_file("Data/SKSE/Plugins/OutfitWheeler-DynamicEquipmentManager.ini", path, std::filesystem::copy_options::overwrite_existing, ec);
            std::filesystem::remove("Data/SKSE/Plugins/OutfitWheeler-DynamicEquipmentManager.ini", ec);
        } else if (std::filesystem::exists("Data/MCM/Settings/OutfitWheeler.ini")) {
            std::filesystem::copy_file("Data/MCM/Settings/OutfitWheeler.ini", path, std::filesystem::copy_options::overwrite_existing, ec);
        } else {
            SaveINI();
            return;
        }
    }

    std::error_code ecClean;
    std::filesystem::remove("Data/SKSE/Plugins/PrismaOutfitWheel.ini", ecClean);
    std::filesystem::remove("Data/SKSE/Plugins/OutfitWheeler-DynamicEquipmentManager.ini", ecClean);

    if (IniKeyExists("Hotkeys", "kEditModeKey", path.c_str())) {
        g_editKey = ReadIniKeyUint("Hotkeys", "kEditModeKey", 0x31, path.c_str());
    } else {
        g_editKey = ReadIniKeyUint("General", "kEditModeKey", 0x31, path.c_str());
    }

    if (IniKeyExists("Hotkeys", "kEditModeModifier", path.c_str())) {
        g_editModifierKey = ReadIniKeyUint("Hotkeys", "kEditModeModifier", 0x00, path.c_str());
    } else {
        g_editModifierKey = ReadIniKeyUint("General", "kEditModeModifier", 0x00, path.c_str());
    }

    if (IniKeyExists("Hotkeys", "kPlayModeKey", path.c_str())) {
        g_playKey = ReadIniKeyUint("Hotkeys", "kPlayModeKey", 0x2F, path.c_str());
    } else {
        g_playKey = ReadIniKeyUint("General", "kPlayModeKey", 0x2F, path.c_str());
    }

    if (IniKeyExists("Hotkeys", "kPlayModeModifier", path.c_str())) {
        g_playModifierKey = ReadIniKeyUint("Hotkeys", "kPlayModeModifier", 0x00, path.c_str());
    } else {
        g_playModifierKey = ReadIniKeyUint("General", "kPlayModeModifier", 0x00, path.c_str());
    }

    if (IniKeyExists("Gamepad", "kGamepadPlayKey", path.c_str())) {
        g_gamepadPlayKey = ReadIniKeyUint("Gamepad", "kGamepadPlayKey", 0x0008, path.c_str());
    } else {
        g_gamepadPlayKey = ReadIniKeyUint("General", "kGamepadPlayKey", 0x0008, path.c_str());
    }

    if (IniKeyExists("Gamepad", "kGamepadPlayModifier", path.c_str())) {
        g_gamepadPlayModifier = ReadIniKeyUint("Gamepad", "kGamepadPlayModifier", 0x0100, path.c_str());
    } else {
        g_gamepadPlayModifier = ReadIniKeyUint("General", "kGamepadPlayModifier", 0x0100, path.c_str());
    }

    g_autoCloseOnEquip.store(GetPrivateProfileIntA("AutoSystem", "bAutoCloseOnEquip", 1, path.c_str()) != 0, std::memory_order_relaxed);
    g_autoStashEnabled.store(GetPrivateProfileIntA("AutoSystem", "bAutoStashInactive", 0, path.c_str()) != 0, std::memory_order_relaxed);
    g_autoSystemEnabled.store(GetPrivateProfileIntA("AutoSystem", "bAutoSystem", 0, path.c_str()) != 0, std::memory_order_relaxed);
    g_autoCityEnabled.store(GetPrivateProfileIntA("AutoSystem", "bAutoCity", 0, path.c_str()) != 0, std::memory_order_relaxed);
    g_autoHomeEnabled.store(GetPrivateProfileIntA("AutoSystem", "bAutoHome", 0, path.c_str()) != 0, std::memory_order_relaxed);
    g_autoCombatEnabled.store(GetPrivateProfileIntA("AutoSystem", "bAutoCombat", 0, path.c_str()) != 0, std::memory_order_relaxed);
    g_autoSwimEnabled.store(GetPrivateProfileIntA("AutoSystem", "bAutoSwim", 0, path.c_str()) != 0, std::memory_order_relaxed);

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
    if (GetPrivateProfileStringA("Appearance", "fUiScale", "1.0", buffer, sizeof(buffer), path.c_str())) {
        try { g_uiScale = std::stof(buffer); } catch (...) { g_uiScale = 1.0f; }
    }
    g_editUiMode = ReadIniKeyUint("Appearance", "uEditUiMode", 1, path.c_str());
    g_showItemPreviews.store(GetPrivateProfileIntA("Appearance", "bShowItemPreviews", 1, path.c_str()) != 0, std::memory_order_relaxed);
    if (GetPrivateProfileStringA("Gamepad", "fThumbstickDeadzone", "0.25", buffer, sizeof(buffer), path.c_str())) {
        try { g_thumbstickDeadzone = std::stof(buffer); } catch (...) { g_thumbstickDeadzone = 0.25f; }
    }
    char themeBuf[64];
    if (GetPrivateProfileStringA("Theme", "sThemeAccentColor", "#38bdf8", themeBuf, sizeof(themeBuf), path.c_str())) {
        g_themeAccentColor = themeBuf;
    }
    if (GetPrivateProfileStringA("Theme", "sThemeBgColor", "#070a0f", themeBuf, sizeof(themeBuf), path.c_str())) {
        g_themeBgColor = themeBuf;
    }
    if (GetPrivateProfileStringA("Theme", "sThemeTextColor", "#f8fafc", themeBuf, sizeof(themeBuf), path.c_str())) {
        g_themeTextColor = themeBuf;
    }
    if (GetPrivateProfileStringA("Theme", "sThemePreset", "sapphire", themeBuf, sizeof(themeBuf), path.c_str())) {
        g_themePreset = themeBuf;
    }
    if (GetPrivateProfileStringA("Theme", "fThemeDepth", "0.60", themeBuf, sizeof(themeBuf), path.c_str())) {
        try { g_themeDepth = std::stof(themeBuf); } catch (...) { g_themeDepth = 0.60f; }
    }
}

static void WriteSettingsToIniFile(const std::string& path) {
    try {
        std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
        std::ofstream out(path, std::ios::trunc);
        if (out.is_open()) {
            out << "[Theme]\n";
            out << "sThemeAccentColor=" << g_themeAccentColor << "\n";
            out << "sThemeBgColor=" << g_themeBgColor << "\n";
            out << "sThemeTextColor=" << g_themeTextColor << "\n";
            out << "sThemePreset=" << g_themePreset << "\n";
            out << "fThemeDepth=" << g_themeDepth << "\n\n";

            out << "[Hotkeys]\n";
            out << "kEditModeKey=" << g_editKey << "\n";
            out << "kEditModeModifier=" << g_editModifierKey << "\n";
            out << "kPlayModeKey=" << g_playKey << "\n";
            out << "kPlayModeModifier=" << g_playModifierKey << "\n\n";

            out << "[Gamepad]\n";
            out << "kGamepadPlayKey=" << g_gamepadPlayKey << "\n";
            out << "kGamepadPlayModifier=" << g_gamepadPlayModifier << "\n";
            out << "fThumbstickDeadzone=" << g_thumbstickDeadzone << "\n\n";

            out << "[Appearance]\n";
            out << "fWheelScale=" << g_wheelScale << "\n";
            out << "fWheelOffsetX=" << g_wheelOffsetX << "\n";
            out << "fWheelOffsetY=" << g_wheelOffsetY << "\n";
            out << "fUiScale=" << g_uiScale << "\n";
            out << "uEditUiMode=" << g_editUiMode << "\n";
            out << "bShowItemPreviews=" << (g_showItemPreviews.load(std::memory_order_relaxed) ? 1 : 0) << "\n\n";

            out << "[AutoSystem]\n";
            out << "bAutoCloseOnEquip=" << (g_autoCloseOnEquip.load(std::memory_order_relaxed) ? 1 : 0) << "\n";
            out << "bAutoStashInactive=" << (g_autoStashEnabled.load(std::memory_order_relaxed) ? 1 : 0) << "\n";
            out << "bAutoSystem=" << (g_autoSystemEnabled.load(std::memory_order_relaxed) ? 1 : 0) << "\n";
            out << "bAutoCity=" << (g_autoCityEnabled.load(std::memory_order_relaxed) ? 1 : 0) << "\n";
            out << "bAutoHome=" << (g_autoHomeEnabled.load(std::memory_order_relaxed) ? 1 : 0) << "\n";
            out << "bAutoCombat=" << (g_autoCombatEnabled.load(std::memory_order_relaxed) ? 1 : 0) << "\n";
            out << "bAutoSwim=" << (g_autoSwimEnabled.load(std::memory_order_relaxed) ? 1 : 0) << "\n\n";

            out << "[General]\n";
            out << "kEditModeKey=" << g_editKey << "\n";
            out << "kEditModeModifier=" << g_editModifierKey << "\n";
            out << "kPlayModeKey=" << g_playKey << "\n";
            out << "kPlayModeModifier=" << g_playModifierKey << "\n";
            out << "kGamepadPlayKey=" << g_gamepadPlayKey << "\n";
            out << "kGamepadPlayModifier=" << g_gamepadPlayModifier << "\n";
            out << "fWheelScale=" << g_wheelScale << "\n";
            out << "fWheelOffsetX=" << g_wheelOffsetX << "\n";
            out << "fWheelOffsetY=" << g_wheelOffsetY << "\n";
            out << "fUiScale=" << g_uiScale << "\n";
            out.flush();
        }
    } catch (...) {}
}

void SaveINI() {
    std::string path = GetIniPath();
    WriteSettingsToIniFile(path);
    if (std::filesystem::exists("Data/MCM/Settings")) {
        WriteSettingsToIniFile("Data/MCM/Settings/OutfitWheeler.ini");
    }

    // Clean up obsolete duplicate INI files so only OutfitWheeler.ini remains
    std::error_code ec;
    std::filesystem::remove("Data/SKSE/Plugins/PrismaOutfitWheel.ini", ec);
    std::filesystem::remove("Data/SKSE/Plugins/OutfitWheeler-DynamicEquipmentManager.ini", ec);
}