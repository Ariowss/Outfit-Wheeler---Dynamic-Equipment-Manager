#pragma once

#include <RE/Skyrim.h>
#include <atomic>
#include <cstdint>

extern std::atomic<bool> isMenuOpen;
extern std::atomic<bool> g_isTyping;
extern std::atomic<bool> g_isEditModeOpen;
extern std::atomic<bool> g_gameFullyLoaded;
extern std::atomic<bool> g_inputRegistered;
extern std::atomic<bool> g_hotkeysEnabled;
extern float g_lockedZoomOffset;
extern std::atomic<bool> g_hasLockedZoom;

void OpenWheel(bool isEditMode);
void ShowWheel(bool isEditMode);
void HideWheel();

class WheelInputHandler : public RE::BSTEventSink<RE::InputEvent*> {
public:
    static WheelInputHandler* GetSingleton();
    static void Register();

    static void OpenWheel(bool isEditMode);
    static void HideWheel();

    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override;
};