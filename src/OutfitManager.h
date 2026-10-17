#pragma once
#include "AdaptiveWardrobeTypes.h"
#include <nlohmann/json.hpp>
#include <RE/Skyrim.h>
#include <vector>
#include <string>
#include <optional>
#include <mutex>
#include <unordered_map>

using json = nlohmann::json;

struct Outfit {
    std::string name;
    std::vector<RE::FormID> itemIDs;
    std::vector<OutfitItem> items;
    std::string contextTag;
    std::string icon{};
    uint8_t preferenceStars{ 3 };
    ControlState controlState{ ControlState::Normal };
    bool isAddon{ false };
    uint32_t usageCount{ 0 };
    uint32_t lastWornTimestamp{ 0 };
    std::string color{};
};

struct Slot {
    std::optional<Outfit> outfit;
};

struct Wheel {
    std::string name;
    std::string color{};
    std::vector<Slot> slots;
};

struct ActorWheelProfile {
    std::vector<Wheel> wheels;
    size_t activeWheelIndex{ 0 };
};

class OutfitManager {
public:
    OutfitManager();
    std::unordered_map<RE::FormID, ActorWheelProfile> actorProfiles;
    RE::FormID activeTargetActorID{ 0x00000007 };
    uint32_t wardrobeContainerFormID{ 0 };

    ActorWheelProfile& GetActiveProfile();
    ActorWheelProfile& GetProfile(RE::FormID a_id);
    std::vector<Wheel>& GetActiveWheels();
    size_t& GetActiveWheelIndex();
    void SortOutfitsAlphabetically();
};

extern OutfitManager g_manager;
extern std::recursive_mutex g_dataMutex;

void LoadData();
void SaveDataInternal();
void SaveDataAsync();
std::string ExportConfigToString();
bool ImportConfigFromString(const std::string& jsonStr);
std::string GetWheelJson(bool isEditMode);
std::string GetWheelJsonInternal(bool isEditMode);
void SyncWheelOutfitsToAdaptiveWardrobe();
void QueueAllLoadedItemThumbnails();
void QueuePlayerInventoryThumbnails();
void QueueWardrobeInventoryThumbnails();
void QueueFollowerInventoryThumbnails();
nlohmann::json GetDiscoveredPosePacksJson();

std::string FormatFormID(RE::FormID a_id);
std::string SanitizeUtf8(const std::string& str);
std::string GetApparelTypeString(RE::TESObjectARMO* armor);