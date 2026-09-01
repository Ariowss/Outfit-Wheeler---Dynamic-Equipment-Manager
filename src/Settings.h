#pragma once
#include <stdint.h>

extern float g_wheelScale;
extern float g_wheelOffsetX;
extern float g_wheelOffsetY;

extern uint32_t g_editKey;
extern uint32_t g_editModifierKey;
extern uint32_t g_playKey;
extern uint32_t g_playModifierKey;

extern uint32_t g_gamepadPlayKey;
extern uint32_t g_gamepadPlayModifier;
extern float g_thumbstickDeadzone;

extern bool g_autoCloseOnEquip;
extern bool g_autoStashEnabled;
extern bool g_autoSystemEnabled;
extern bool g_autoCityEnabled;
extern bool g_autoHomeEnabled;
extern bool g_autoCombatEnabled;
extern bool g_autoSwimEnabled;

void LoadINI();
void SaveINI();