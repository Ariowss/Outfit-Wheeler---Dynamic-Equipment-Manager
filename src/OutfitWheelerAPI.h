#pragma once

#include <cstdint>
#include <windows.h>

#ifdef OUTFITWHEELER_EXPORTS
    #define OUTFITWHEELER_API extern "C" __declspec(dllexport)
#else
    #define OUTFITWHEELER_API extern "C" __declspec(dllimport)
#endif

OUTFITWHEELER_API bool OutfitWheeler_Open();
OUTFITWHEELER_API bool OutfitWheeler_OpenEdit();
OUTFITWHEELER_API void OutfitWheeler_Close();
OUTFITWHEELER_API bool OutfitWheeler_IsMenuOpen();
OUTFITWHEELER_API void OutfitWheeler_SetHotkeyEnabled(bool a_enabled);
OUTFITWHEELER_API bool OutfitWheeler_IsHotkeyEnabled();

OUTFITWHEELER_API bool OpenOutfitWheeler();
OUTFITWHEELER_API void CloseOutfitWheeler();
OUTFITWHEELER_API bool IsOutfitWheelerOpen();
OUTFITWHEELER_API void SetOutfitWheelerHotkeyEnabled(bool a_enabled);
OUTFITWHEELER_API bool IsOutfitWheelerHotkeyEnabled();

OUTFITWHEELER_API bool YourMod_Open();
OUTFITWHEELER_API void YourMod_Close();
OUTFITWHEELER_API bool YourMod_IsMenuOpen();
OUTFITWHEELER_API void YourMod_SetHotkeyEnabled(bool a_enabled);

#ifndef OUTFITWHEELER_EXPORTS

namespace OutfitWheelerAPI
{
    using FnOpen = bool (*)();
    using FnOpenEdit = bool (*)();
    using FnClose = void (*)();
    using FnIsMenuOpen = bool (*)();
    using FnSetHotkeyEnabled = void (*)(bool);
    using FnIsHotkeyEnabled = bool (*)();

    struct Interface
    {
        FnOpen Open{ nullptr };
        FnOpenEdit OpenEdit{ nullptr };
        FnClose Close{ nullptr };
        FnIsMenuOpen IsMenuOpen{ nullptr };
        FnSetHotkeyEnabled SetHotkeyEnabled{ nullptr };
        FnIsHotkeyEnabled IsHotkeyEnabled{ nullptr };

        bool IsLoaded() const { return Open != nullptr && Close != nullptr; }

        bool Initialize()
        {
            const char* dllNames[] = {
                "OutfitWheeler-DynamicEquipmentManager.dll",
                "OutfitWheeler.dll",
                "OutfitWheeler-DynamicEquipmentManager",
                "OutfitWheeler"
            };

            HMODULE hMod = nullptr;
            for (const char* name : dllNames) {
                hMod = GetModuleHandleA(name);
                if (hMod) break;
            }

            if (!hMod) return false;

            Open = reinterpret_cast<FnOpen>(GetProcAddress(hMod, "OutfitWheeler_Open"));
            if (!Open) Open = reinterpret_cast<FnOpen>(GetProcAddress(hMod, "OpenOutfitWheeler"));
            if (!Open) Open = reinterpret_cast<FnOpen>(GetProcAddress(hMod, "YourMod_Open"));

            OpenEdit = reinterpret_cast<FnOpenEdit>(GetProcAddress(hMod, "OutfitWheeler_OpenEdit"));

            Close = reinterpret_cast<FnClose>(GetProcAddress(hMod, "OutfitWheeler_Close"));
            if (!Close) Close = reinterpret_cast<FnClose>(GetProcAddress(hMod, "CloseOutfitWheeler"));
            if (!Close) Close = reinterpret_cast<FnClose>(GetProcAddress(hMod, "YourMod_Close"));

            IsMenuOpen = reinterpret_cast<FnIsMenuOpen>(GetProcAddress(hMod, "OutfitWheeler_IsMenuOpen"));
            if (!IsMenuOpen) IsMenuOpen = reinterpret_cast<FnIsMenuOpen>(GetProcAddress(hMod, "IsOutfitWheelerOpen"));
            if (!IsMenuOpen) IsMenuOpen = reinterpret_cast<FnIsMenuOpen>(GetProcAddress(hMod, "YourMod_IsMenuOpen"));

            SetHotkeyEnabled = reinterpret_cast<FnSetHotkeyEnabled>(GetProcAddress(hMod, "OutfitWheeler_SetHotkeyEnabled"));
            if (!SetHotkeyEnabled) SetHotkeyEnabled = reinterpret_cast<FnSetHotkeyEnabled>(GetProcAddress(hMod, "SetOutfitWheelerHotkeyEnabled"));
            if (!SetHotkeyEnabled) SetHotkeyEnabled = reinterpret_cast<FnSetHotkeyEnabled>(GetProcAddress(hMod, "YourMod_SetHotkeyEnabled"));

            IsHotkeyEnabled = reinterpret_cast<FnIsHotkeyEnabled>(GetProcAddress(hMod, "OutfitWheeler_IsHotkeyEnabled"));
            if (!IsHotkeyEnabled) IsHotkeyEnabled = reinterpret_cast<FnIsHotkeyEnabled>(GetProcAddress(hMod, "IsOutfitWheelerHotkeyEnabled"));

            return IsLoaded();
        }
    };

    inline Interface& Get()
    {
        static Interface s_interface;
        return s_interface;
    }
}

#endif
