#ifndef SETTINGS_H
#define SETTINGS_H

#include <cstdint>
#include <string>

extern uint32_t g_editKey;
extern uint32_t g_editModifierKey;
extern uint32_t g_playKey;
extern uint32_t g_playModifierKey;
extern float g_wheelScale;
extern float g_wheelOffsetX;

extern bool g_autoSystemEnabled;
extern bool g_autoCityEnabled;
extern bool g_autoHomeEnabled;
extern bool g_autoCombatEnabled;
extern bool g_autoSwimEnabled;

void LoadINI();
void SaveINI();

#endif