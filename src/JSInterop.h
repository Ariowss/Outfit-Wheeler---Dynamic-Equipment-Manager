#ifndef JS_INTEROP_H
#define JS_INTEROP_H

#include "PrismaUI_API.h"

void OnDomReadyCallbackFunc(PrismaView v);
void OnConsoleMessageCallbackFunc(PrismaView v, PRISMA_UI_API::ConsoleMessageLevel level, const char* message);
void SaveSettingsListener(const char* data);
void SetTypingModeListener(const char* data);
void SaveOutfitListener(const char* data);
void RenameSlotListener(const char* data);
void RenameWheelListener(const char* data);
void ClearSlotListener(const char* data);
void DeleteSlotListener(const char* data);
void EquipSlotListener(const char* data);
void OnCloseWheelListener(const char* data);
void ChangeWheelListener(const char* data);
void AddWheelListener(const char* data);
void AddSlotListener(const char* data);
void DeleteWheelListener(const char* data);
void SetSlotContextListener(const char* data);
void SetSlotRatingListener(const char* data);

#endif