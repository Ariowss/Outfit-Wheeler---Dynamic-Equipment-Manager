#ifndef OUTFIT_MANAGER_H
#define OUTFIT_MANAGER_H

#include "AdaptiveWardrobeTypes.h"
#include <RE/Skyrim.h>
#include <nlohmann/json.hpp>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

using json = nlohmann::json;

struct Outfit {
    std::string name;
    std::vector<RE::FormID> itemIDs;
    std::string contextTag = "City";
    uint8_t preferenceStars = 3;
    ControlState controlState = ControlState::Normal;
    uint32_t usageCount = 0;
    uint32_t lastWornTimestamp = 0;
};

struct Slot {
    std::optional<Outfit> outfit;
};

struct Wheel {
    std::string name;
    std::vector<Slot> slots;
};

struct OutfitManager {
    std::vector<Wheel> wheels;
    size_t activeWheelIndex = 0;
    uint32_t wardrobeContainerFormID = 0;

    OutfitManager();
};

extern OutfitManager g_manager;
extern std::recursive_mutex g_dataMutex;

void SaveDataInternal();
void LoadData();
void SyncWheelOutfitsToAdaptiveWardrobe();
std::string GetWheelJsonInternal(bool isEditMode);
std::string GetWheelJson(bool isEditMode);

#endif