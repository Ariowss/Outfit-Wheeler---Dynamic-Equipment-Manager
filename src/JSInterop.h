#pragma once
#include <nlohmann/json.hpp>
#include "PrismaUI_API.h"
#include <RE/Skyrim.h>

using json = nlohmann::json;

extern RE::NiPoint3 g_stagePosition;
extern float g_stageAngleZ;
extern RE::FormID g_currentStudioActorID;

void OnDomReadyCallbackFunc(PrismaView v);
void OnConsoleMessageCallbackFunc(PrismaView v, PRISMA_UI_API::ConsoleMessageLevel level, const char* message);

void InitializeStudioStage();
void SetupStudioCameraAndControls(bool enableStudio);
void RestoreStudioActors();

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
void SetSlotTypeListener(const char* data);
void ToggleItemEquipListener(const char* data);
void TransferItemListener(const char* data);
void TransferAllInventoryListener(const char* data);
void OrbitCameraListener(const char* data);
void ZoomCameraListener(const char* data);
void PanCameraListener(const char* data);
void SelectTargetActorListener(const char* data);
void RetrieveAllOutfitsListener(const char* data);
void StashAllOutfitsListener(const char* data);
void StashSlotOutfitListener(const char* data);
void RetrieveSlotOutfitListener(const char* data);
void StashWheelOutfitsListener(const char* data);
void RetrieveWheelOutfitsListener(const char* data);
void AddItemToSlotListener(const char* data);
void RemoveItemFromSlotListener(const char* data);
void MoveSlotListener(const char* data);
void MoveWheelListener(const char* data);