#pragma once
#include <stdint.h>
#include <atomic>

extern float g_wheelScale;
extern float g_wheelOffsetX;
extern float g_wheelOffsetY;
extern float g_uiScale;
extern uint32_t g_editUiMode;

extern uint32_t g_editKey;
extern uint32_t g_editModifierKey;
extern uint32_t g_playKey;
extern uint32_t g_playModifierKey;

extern uint32_t g_gamepadPlayKey;
extern uint32_t g_gamepadPlayModifier;
extern float g_thumbstickDeadzone;

extern std::atomic<bool> g_autoCloseOnEquip;
extern std::atomic<bool> g_autoStashEnabled;
extern std::atomic<bool> g_autoSystemEnabled;
extern std::atomic<bool> g_autoCityEnabled;
extern std::atomic<bool> g_autoHomeEnabled;
extern std::atomic<bool> g_autoCombatEnabled;
extern std::atomic<bool> g_autoSwimEnabled;

void LoadINI();
void SaveINI();