#include "pch.h"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include "OutfitManager.h"
#include "EquipmentManager.h"
#include "AdaptiveWardrobeManager.h"
#include "Settings.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <set>
#include <SKSE/SKSE.h>

OutfitManager g_manager;
std::mutex g_dataMutex;

static const std::string g_savePath = "Data/SKSE/Plugins/PrismaOutfitWheel.json";

static std::string ToLowerString(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return str;
}

static ContextFlags ParseContextTagToFlags(const std::string& tag) {
    std::string lower = ToLowerString(tag);
    ContextFlags flags = ContextFlags::None;

    if (lower.find("city") != std::string::npos || lower.find("town") != std::string::npos) {
        flags = flags | ContextFlags::CityTown;
    }
    if (lower.find("home") != std::string::npos || lower.find("house") != std::string::npos) {
        flags = flags | ContextFlags::PlayerHome;
    }
    if (lower.find("combat") != std::string::npos || lower.find("fight") != std::string::npos) {
        flags = flags | ContextFlags::Combat;
    }
    if (lower.find("swim") != std::string::npos || lower.find("water") != std::string::npos) {
        flags = flags | ContextFlags::Swim;
    }

    return flags;
}

OutfitManager::OutfitManager() {
    Wheel defaultWheel;
    defaultWheel.name = "Wheel 1";
    wheels.push_back(defaultWheel);
}

void SyncWheelOutfitsToAdaptiveWardrobe() {
    std::vector<AdaptiveOutfit> adaptiveList;
    size_t wIdx = 0;
    for (const auto& wheel : g_manager.wheels) {
        size_t sIdx = 0;
        for (const auto& slot : wheel.slots) {
            if (slot.outfit.has_value()) {
                AdaptiveOutfit ao;
                ao.id = std::to_string(wIdx) + "_" + std::to_string(sIdx);
                ao.name = slot.outfit->name;
                ao.itemIDs = slot.outfit->itemIDs;
                ao.preferenceStars = slot.outfit->preferenceStars;
                ao.controlState = slot.outfit->controlState;
                ao.dynamicVars.usageCount = slot.outfit->usageCount;
                ao.dynamicVars.lastWornTimestamp = slot.outfit->lastWornTimestamp;

                ContextFlags cFlags = ParseContextTagToFlags(slot.outfit->contextTag);
                if (cFlags != ContextFlags::None) {
                    SuitabilityRule rule;
                    rule.contextMask = cFlags;
                    rule.score = static_cast<uint8_t>(3 + slot.outfit->preferenceStars);
                    ao.suitabilities.push_back(rule);
                }

                adaptiveList.push_back(ao);
            }
            sIdx++;
        }
        wIdx++;
    }
    AdaptiveWardrobeManager::GetSingleton()->LoadWardrobe(adaptiveList);
    SKSE::log::info("Synced {} wheel outfits into AdaptiveWardrobeManager", adaptiveList.size());
}

static json SerializeFormID(RE::FormID formID) {
    auto* form = RE::TESForm::LookupByID(formID);
    if (form) {
        auto* file = form->GetFile(0);
        if (file) {
            std::string modName = file->fileName;
            uint32_t localID = file->IsLight() ? (formID & 0x00000FFF) : (formID & 0x00FFFFFF);
            std::stringstream ss;
            ss << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << localID;
            return json{
                {"plugin", modName},
                {"localID", ss.str()},
                {"rawID", static_cast<uint32_t>(formID)}
            };
        }
    }
    std::stringstream ss;
    ss << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << formID;
    return json{
        {"plugin", "Skyrim.esm"},
        {"localID", ss.str()},
        {"rawID", static_cast<uint32_t>(formID)}
    };
}

static RE::FormID DeserializeFormID(const json& jItem) {
    if (jItem.is_number_integer() || jItem.is_number_unsigned()) {
        return jItem.get<RE::FormID>();
    }
    if (jItem.is_object()) {
        std::string plugin = jItem.value("plugin", "");
        uint32_t localID = 0;

        if (jItem.contains("localID")) {
            if (jItem.at("localID").is_number_unsigned() || jItem.at("localID").is_number_integer()) {
                localID = jItem.at("localID").get<uint32_t>();
            } else if (jItem.at("localID").is_string()) {
                std::string s = jItem.at("localID").get<std::string>();
                if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
                    s = s.substr(2);
                }
                try {
                    localID = static_cast<uint32_t>(std::stoul(s, nullptr, 16));
                } catch (...) {
                    localID = 0;
                }
            }
        }

        if (!plugin.empty() && localID != 0) {
            auto* dataHandler = RE::TESDataHandler::GetSingleton();
            if (dataHandler) {
                auto* boundObj = dataHandler->LookupForm<RE::TESBoundObject>(localID, plugin);
                if (boundObj) {
                    return boundObj->GetFormID();
                }
            }
        }

        if (jItem.contains("rawID")) {
            return jItem.value("rawID", static_cast<RE::FormID>(0));
        }
    }
    return 0;
}

void SaveDataInternal() {
    try {
        json root;
        root["activeWheel"] = g_manager.activeWheelIndex;
        json wheelsJson = json::array();
        for (const auto& wheel : g_manager.wheels) {
            json wJson;
            wJson["name"] = wheel.name;
            json slotsJson = json::array();
            for (const auto& slot : wheel.slots) {
                json sJson;
                if (slot.outfit.has_value()) {
                    sJson["hasOutfit"] = true;
                    sJson["name"] = slot.outfit->name;
                    sJson["contextTag"] = slot.outfit->contextTag;
                    sJson["preferenceStars"] = slot.outfit->preferenceStars;
                    sJson["controlState"] = static_cast<uint8_t>(slot.outfit->controlState);
                    sJson["usageCount"] = slot.outfit->usageCount;
                    sJson["lastWornTimestamp"] = slot.outfit->lastWornTimestamp;

                    json itemsArray = json::array();
                    for (auto fID : slot.outfit->itemIDs) {
                        itemsArray.push_back(SerializeFormID(fID));
                    }
                    sJson["items"] = itemsArray;
                } else {
                    sJson["hasOutfit"] = false;
                    sJson["name"] = "";
                    sJson["items"] = json::array();
                    sJson["contextTag"] = "";
                    sJson["preferenceStars"] = 3;
                    sJson["controlState"] = 0;
                    sJson["usageCount"] = 0;
                    sJson["lastWornTimestamp"] = 0;
                }
                slotsJson.push_back(sJson);
            }
            wJson["slots"] = slotsJson;
            wheelsJson.push_back(wJson);
        }
        root["wheels"] = wheelsJson;

        std::filesystem::create_directories("Data/SKSE/Plugins");
        std::string tmpPath = g_savePath + ".tmp";
        {
            std::ofstream file(tmpPath);
            if (!file.is_open()) {
                SKSE::log::error("Failed to open temporary save file at {}", tmpPath);
                return;
            }
            file << root.dump(4);
        }
        std::filesystem::rename(tmpPath, g_savePath);
        SKSE::log::info("Data successfully saved to {}", g_savePath);

        SyncWheelOutfitsToAdaptiveWardrobe();
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in SaveDataInternal: {}", e.what());
    }
}

void LoadData() {
    std::lock_guard<std::mutex> lock(g_dataMutex);
    if (!std::filesystem::exists(g_savePath)) {
        SKSE::log::info("No save file found at {}", g_savePath);
        return;
    }
    std::ifstream file(g_savePath);
    if (!file.is_open()) {
        SKSE::log::error("Failed to open save file for reading at {}", g_savePath);
        return;
    }

    try {
        json root = json::parse(file);
        if (root.contains("activeWheel") && root["activeWheel"].is_number()) {
            g_manager.activeWheelIndex = root["activeWheel"].get<size_t>();
        }
        if (root.contains("wheels") && root["wheels"].is_array()) {
            g_manager.wheels.clear();
            for (const auto& wJson : root["wheels"]) {
                Wheel w;
                w.name = wJson.value("name", "Wheel");
                if (wJson.contains("slots") && wJson["slots"].is_array()) {
                    for (const auto& sJson : wJson["slots"]) {
                        Slot s;
                        bool hasOutfit = sJson.value("hasOutfit", false);
                        bool hasItems = sJson.contains("items") && sJson["items"].is_array() && !sJson["items"].empty();

                        if (hasOutfit || hasItems) {
                            Outfit o;
                            o.name = sJson.value("name", "Outfit");
                            o.contextTag = sJson.value("contextTag", "");
                            o.preferenceStars = sJson.value("preferenceStars", static_cast<uint8_t>(3));
                            o.controlState = static_cast<ControlState>(sJson.value("controlState", static_cast<uint8_t>(ControlState::Normal)));
                            o.usageCount = sJson.value("usageCount", 0u);
                            o.lastWornTimestamp = sJson.value("lastWornTimestamp", 0u);

                            if (hasItems) {
                                for (const auto& jItem : sJson["items"]) {
                                    RE::FormID resolvedID = DeserializeFormID(jItem);
                                    if (resolvedID != 0) {
                                        o.itemIDs.push_back(resolvedID);
                                    }
                                }
                            }
                            s.outfit = o;
                        }
                        w.slots.push_back(s);
                    }
                }
                g_manager.wheels.push_back(w);
            }
        }
        if (g_manager.wheels.empty()) {
            Wheel defaultWheel;
            defaultWheel.name = "Wheel 1";
            g_manager.wheels.push_back(defaultWheel);
        }
        if (g_manager.activeWheelIndex >= g_manager.wheels.size()) {
            g_manager.activeWheelIndex = 0;
        }
        SKSE::log::info("Successfully loaded data from disk");

        SyncWheelOutfitsToAdaptiveWardrobe();
    } catch (const std::exception& e) {
        SKSE::log::error("Exception parsing JSON data: {}", e.what());
    }
}

std::string GetWheelJsonInternal(bool isEditMode) {
    if (g_manager.wheels.empty()) {
        Wheel defaultWheel;
        defaultWheel.name = "Wheel 1";
        g_manager.wheels.push_back(defaultWheel);
        g_manager.activeWheelIndex = 0;
    }
    if (g_manager.activeWheelIndex >= g_manager.wheels.size()) {
        g_manager.activeWheelIndex = 0;
    }

    std::set<RE::FormID> currentlyWornSet;
    auto equippedEntries = GetEquippedInventoryEntries();
    for (const auto& entry : equippedEntries) {
        if (entry.object) {
            currentlyWornSet.insert(entry.object->GetFormID());
        }
    }

    const auto& activeWheel = g_manager.wheels[g_manager.activeWheelIndex];
    size_t bestMatchingSlotIndex = SIZE_MAX;
    float bestMatchRatio = 0.0f;
    size_t maxMatches = 0;

    size_t idx = 0;
    for (const auto& slot : activeWheel.slots) {
        if (slot.outfit.has_value() && !slot.outfit->itemIDs.empty()) {
            size_t matches = 0;
            for (RE::FormID fID : slot.outfit->itemIDs) {
                if (currentlyWornSet.find(fID) != currentlyWornSet.end()) {
                    matches++;
                }
            }
            if (matches > 0) {
                float ratio = static_cast<float>(matches) / static_cast<float>(slot.outfit->itemIDs.size());
                if (ratio > bestMatchRatio || (ratio == bestMatchRatio && matches > maxMatches)) {
                    bestMatchRatio = ratio;
                    maxMatches = matches;
                    bestMatchingSlotIndex = idx;
                }
            }
        }
        idx++;
    }

    json data;
    data["editMode"] = isEditMode;
    data["wheelIndex"] = g_manager.activeWheelIndex;
    data["wheelName"] = activeWheel.name;
    data["totalWheels"] = g_manager.wheels.size();
    data["wheelScale"] = g_wheelScale;
    data["wheelOffsetX"] = g_wheelOffsetX;
    data["editKey"] = g_editKey;
    data["editModifierKey"] = g_editModifierKey;
    data["playKey"] = g_playKey;
    data["playModifierKey"] = g_playModifierKey;

    data["autoSystemEnabled"] = g_autoSystemEnabled;
    data["autoCityEnabled"] = g_autoCityEnabled;
    data["autoHomeEnabled"] = g_autoHomeEnabled;
    data["autoCombatEnabled"] = g_autoCombatEnabled;
    data["autoSwimEnabled"] = g_autoSwimEnabled;

    json slotsJson = json::array();
    size_t slotIdx = 0;
    for (const auto& slot : activeWheel.slots) {
        json sJson;
        sJson["index"] = slotIdx;
        sJson["hasOutfit"] = slot.outfit.has_value();
        sJson["name"] = slot.outfit.has_value() ? slot.outfit->name : "Empty";
        sJson["count"] = slot.outfit.has_value() ? slot.outfit->itemIDs.size() : 0;
        sJson["contextTag"] = slot.outfit.has_value() ? slot.outfit->contextTag : "";
        sJson["preferenceStars"] = slot.outfit.has_value() ? slot.outfit->preferenceStars : 3;
        sJson["controlState"] = slot.outfit.has_value() ? static_cast<uint8_t>(slot.outfit->controlState) : 0;

        bool isWorn = (slotIdx == bestMatchingSlotIndex && bestMatchRatio >= 0.5f);
        sJson["isWorn"] = isWorn;
        sJson["isEquipped"] = isWorn;

        slotsJson.push_back(sJson);
        slotIdx++;
    }
    data["slots"] = slotsJson;
    return data.dump();
}

std::string GetWheelJson(bool isEditMode) {
    std::lock_guard<std::mutex> lock(g_dataMutex);
    return GetWheelJsonInternal(isEditMode);
}