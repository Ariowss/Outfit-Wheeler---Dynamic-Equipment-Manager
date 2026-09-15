#include "pch.h"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#pragma comment(lib, "xinput.lib")

#include "OutfitManager.h"
#include "EquipmentManager.h"
#include "AdaptiveWardrobeManager.h"
#include "ExecutionQueue.h"
#include "Settings.h"
#include "thumbnail/ItemThumbnailService.h"
#include "thumbnail/ThumbnailDiskCache.h"
#include <windows.h>
#include <xinput.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <set>
#include <unordered_set>
#include <RE/B/BipedAnim.h>
#include <SKSE/SKSE.h>

OutfitManager g_manager;
std::recursive_mutex g_dataMutex;

static const std::string g_savePath = "Data/SKSE/Plugins/PrismaOutfitWheel.json";
static const std::string g_fallbackSavePath1 = "Data/SKSE/Plugins/OutfitWheeler.json";
static const std::string g_fallbackSavePath2 = "Data/SKSE/Plugins/Wheeler.json";
static const std::string g_fallbackSavePath3 = "Data/SKSE/Plugins/PrismaOutfitWheeler.json";

static std::string ToLowerString(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return str;
}

static std::string FormatFormID(RE::FormID a_id) {
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%08X", a_id);
    return std::string(buf);
}

static std::string SanitizeUtf8(const std::string& str) {
    if (str.empty()) return "";
    std::string out;
    out.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(str[i]);
        if (c == '"' || c == '\\' || (c < 0x20 && c != ' ') || c == 0x7F) {
            out += ' ';
            continue;
        }
        if (c < 0x80) {
            out += static_cast<char>(c);
        } else if ((c & 0xE0) == 0xC0 && i + 1 < str.size() && (static_cast<unsigned char>(str[i + 1]) & 0xC0) == 0x80) {
            out += str[i];
            out += str[++i];
        } else if ((c & 0xF0) == 0xE0 && i + 2 < str.size() && (static_cast<unsigned char>(str[i + 1]) & 0xC0) == 0x80 && (static_cast<unsigned char>(str[i + 2]) & 0xC0) == 0x80) {
            out += str[i];
            out += str[++i];
            out += str[++i];
        } else if ((c & 0xF8) == 0xF0 && i + 3 < str.size() && (static_cast<unsigned char>(str[i + 1]) & 0xC0) == 0x80 && (static_cast<unsigned char>(str[i + 2]) & 0xC0) == 0x80 && (static_cast<unsigned char>(str[i + 3]) & 0xC0) == 0x80) {
            out += str[i];
            out += str[++i];
            out += str[++i];
            out += str[++i];
        } else {
            out += ' ';
        }
    }
    return out;
}

static bool ContainsTagToken(const std::string& str, const std::string_view token) {
    size_t pos = 0;
    while ((pos = str.find(token, pos)) != std::string::npos) {
        bool leftOk = (pos == 0) || !std::isalnum(static_cast<unsigned char>(str[pos - 1]));
        bool rightOk = (pos + token.size() >= str.size()) || !std::isalnum(static_cast<unsigned char>(str[pos + token.size()]));
        if (leftOk && rightOk) {
            return true;
        }
        pos += token.size();
    }
    return false;
}

static ContextFlags ParseContextTagToFlags(const std::string& tag) {
    std::string lower = ToLowerString(tag);
    ContextFlags flags = ContextFlags::None;

    if (ContainsTagToken(lower, "combat") || ContainsTagToken(lower, "fight")) flags |= ContextFlags::Combat;
    if (ContainsTagToken(lower, "swim")) flags |= ContextFlags::Swim;
    if (ContainsTagToken(lower, "in_water") || ContainsTagToken(lower, "water")) flags |= ContextFlags::InWater;
    if (ContainsTagToken(lower, "sleep") || ContainsTagToken(lower, "bed") || ContainsTagToken(lower, "pajama") || ContainsTagToken(lower, "nightwear") || ContainsTagToken(lower, "rest")) flags |= ContextFlags::Sleep;
    if (ContainsTagToken(lower, "mount") || ContainsTagToken(lower, "horse") || ContainsTagToken(lower, "ride") || ContainsTagToken(lower, "riding")) flags |= ContextFlags::Mount;

    if (ContainsTagToken(lower, "dungeon") || ContainsTagToken(lower, "tomb") || ContainsTagToken(lower, "ruin")) flags |= ContextFlags::Dungeon;
    if (ContainsTagToken(lower, "home") || ContainsTagToken(lower, "house")) flags |= ContextFlags::PlayerHome;
    if (ContainsTagToken(lower, "inn") || ContainsTagToken(lower, "tavern")) flags |= ContextFlags::Inn;
    if (ContainsTagToken(lower, "store") || ContainsTagToken(lower, "shop") || ContainsTagToken(lower, "market")) flags |= ContextFlags::Store;
    if (ContainsTagToken(lower, "guild") || ContainsTagToken(lower, "thief") || ContainsTagToken(lower, "brotherhood")) flags |= ContextFlags::GuildHall;
    if (ContainsTagToken(lower, "castle") || ContainsTagToken(lower, "palace")) flags |= ContextFlags::Castle;
    if (ContainsTagToken(lower, "temple") || ContainsTagToken(lower, "shrine") || ContainsTagToken(lower, "talos")) flags |= ContextFlags::Temple;
    if (ContainsTagToken(lower, "farm") || ContainsTagToken(lower, "mill")) flags |= ContextFlags::Farm;
    if (ContainsTagToken(lower, "jail") || ContainsTagToken(lower, "prison")) flags |= ContextFlags::Jail;
    if (ContainsTagToken(lower, "military") || ContainsTagToken(lower, "camp") || ContainsTagToken(lower, "war")) flags |= ContextFlags::Military;

    if (ContainsTagToken(lower, "city")) flags |= ContextFlags::City;
    if (ContainsTagToken(lower, "town") || ContainsTagToken(lower, "village")) flags |= ContextFlags::Town;
    if (ContainsTagToken(lower, "world") || ContainsTagToken(lower, "adventure") || ContainsTagToken(lower, "mountain") || ContainsTagToken(lower, "nature")) flags |= ContextFlags::World;

    if (ContainsTagToken(lower, "night")) flags |= ContextFlags::Night;
    if (ContainsTagToken(lower, "rain")) flags |= ContextFlags::Rain;
    if (ContainsTagToken(lower, "snow")) flags |= ContextFlags::Snow;
    if (ContainsTagToken(lower, "interior")) flags |= ContextFlags::Interior;

    return flags;
}

OutfitManager::OutfitManager() {
    GetProfile(0x00000007);
}

ActorWheelProfile& OutfitManager::GetProfile(RE::FormID a_id) {
    RE::FormID canonicalID = (a_id == 0x00000014 || a_id == 0) ? 0x00000007 : a_id;
    auto it = actorProfiles.find(canonicalID);
    if (it == actorProfiles.end()) {
        ActorWheelProfile prof;
        prof.activeWheelIndex = 0;
        Wheel defaultWheel;
        defaultWheel.name = "Wheel 1";
        for (size_t i = 0; i < 4; ++i) {
            Slot s;
            defaultWheel.slots.push_back(s);
        }
        prof.wheels.push_back(defaultWheel);
        actorProfiles[canonicalID] = prof;
        return actorProfiles[canonicalID];
    }
    return it->second;
}

ActorWheelProfile& OutfitManager::GetActiveProfile() {
    return GetProfile(activeTargetActorID);
}

std::vector<Wheel>& OutfitManager::GetActiveWheels() {
    return GetActiveProfile().wheels;
}

size_t& OutfitManager::GetActiveWheelIndex() {
    return GetActiveProfile().activeWheelIndex;
}

void SyncWheelOutfitsToAdaptiveWardrobe() {
    auto manager = AdaptiveWardrobeManager::GetSingleton();
    if (!manager) return;

    for (const auto& [actorID, profile] : g_manager.actorProfiles) {
        std::vector<AdaptiveOutfit> adaptiveList;
        size_t wIdx = 0;
        for (const auto& wheel : profile.wheels) {
            size_t sIdx = 0;
            for (const auto& slot : wheel.slots) {
                if (slot.outfit.has_value() && !slot.outfit->itemIDs.empty()) {
                    AdaptiveOutfit ao;
                    ao.id = std::to_string(wIdx) + "_" + std::to_string(sIdx);
                    ao.name = slot.outfit->name;
                    ao.itemIDs = slot.outfit->itemIDs;
                    ao.items = slot.outfit->items;
                    ao.preferenceStars = slot.outfit->preferenceStars;
                    ao.controlState = slot.outfit->controlState;
                    ao.dynamicVars.usageCount = slot.outfit->usageCount;
                    ao.dynamicVars.lastWornTimestamp = static_cast<uint32_t>(slot.outfit->lastWornTimestamp);

                    ContextFlags cFlags = ParseContextTagToFlags(slot.outfit->contextTag);
                    if (cFlags != ContextFlags::None && slot.outfit->preferenceStars > 0 && slot.outfit->controlState != ControlState::Excluded) {
                        SuitabilityRule rule;
                        rule.contextMask = cFlags;
                        rule.score = static_cast<uint8_t>(3 + slot.outfit->preferenceStars);
                        ao.suitabilities.push_back(rule);
                    } else {
                        ao.controlState = ControlState::Excluded;
                    }

                    adaptiveList.push_back(ao);
                }
                sIdx++;
            }
            wIdx++;
        }
        manager->LoadWardrobe(actorID, adaptiveList);
    }
}

static json SerializeFormID(RE::FormID formID) {
    if (formID == 0) {
        return json{ {"plugin", ""}, {"localID", "0x00000000"}, {"rawID", 0} };
    }

    auto* dataHandler = RE::TESDataHandler::GetSingleton();
    if (dataHandler) {
        uint8_t modIndex = static_cast<uint8_t>((formID >> 24) & 0xFF);
        const RE::TESFile* file = nullptr;
        uint32_t localID = 0;

        if (modIndex == 0xFE) {
            uint16_t lightIndex = static_cast<uint16_t>((formID >> 12) & 0x0FFF);
            file = dataHandler->LookupLoadedLightModByIndex(lightIndex);
            localID = formID & 0x00000FFF;
        } else {
            file = dataHandler->LookupLoadedModByIndex(modIndex);
            localID = formID & 0x00FFFFFF;
        }

        if (file && file->fileName && file->fileName[0] != 0) {
            return json{
                {"plugin", std::string(file->fileName)},
                {"localID", FormatFormID(localID)},
                {"rawID", static_cast<uint32_t>(formID)}
            };
        }
    }

    return json{
        {"plugin", "Skyrim.esm"},
        {"localID", FormatFormID(formID & 0x00FFFFFF)},
        {"rawID", static_cast<uint32_t>(formID)}
    };
}

static json SerializeOutfitItem(const OutfitItem& itm) {
    json j;
    if (itm.formID != 0) {
        j = SerializeFormID(itm.formID);
    } else {
        std::string pName = !itm.pluginName.empty() ? itm.pluginName : "Skyrim.esm";
        j = json{
            {"plugin", pName},
            {"localID", FormatFormID(itm.localFormID)},
            {"rawID", 0}
        };
    }
    j["uniqueID"] = itm.uniqueID;
    j["enchantmentID"] = static_cast<uint32_t>(itm.enchantmentID);
    std::string safeName = (itm.customName == "Apparel") ? "" : itm.customName;
    j["customName"] = SanitizeUtf8(safeName);
    return j;
}

static RE::FormID DeserializeFormID(const json& jItem) {
    if (jItem.is_number_integer() || jItem.is_number_unsigned()) {
        RE::FormID raw = jItem.get<RE::FormID>();
        auto* f = RE::TESForm::LookupByID(raw);
        if (f) return raw;
        return 0;
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
                const auto* regularMod = dataHandler->LookupLoadedModByName(plugin);
                if (regularMod && regularMod->compileIndex != 0xFF) {
                    RE::FormID fullID = (static_cast<RE::FormID>(regularMod->compileIndex) << 24) | (localID & 0x00FFFFFF);
                    auto* form = RE::TESForm::LookupByID(fullID);
                    if (form) return fullID;
                }

                const auto* lightMod = dataHandler->LookupLoadedLightModByName(plugin);
                if (lightMod && lightMod->smallFileCompileIndex != 0xFFFF) {
                    RE::FormID fullID = 0xFE000000 | (static_cast<RE::FormID>(lightMod->smallFileCompileIndex) << 12) | (localID & 0x00000FFF);
                    auto* form = RE::TESForm::LookupByID(fullID);
                    if (form) return fullID;
                }
            }
        }

        if (jItem.contains("rawID")) {
            RE::FormID rID = jItem.value("rawID", static_cast<RE::FormID>(0));
            auto* f = RE::TESForm::LookupByID(rID);
            if (f) return rID;
        }
    }
    return 0;
}

static OutfitItem DeserializeOutfitItem(const json& jItem) {
    OutfitItem itm;
    itm.formID = DeserializeFormID(jItem);
    if (jItem.is_object()) {
        itm.uniqueID = jItem.value("uniqueID", static_cast<uint16_t>(0));
        itm.enchantmentID = static_cast<RE::FormID>(jItem.value("enchantmentID", static_cast<uint32_t>(0)));
        itm.customName = jItem.value("customName", "");
        if (itm.customName == "Apparel") {
            itm.customName = "";
        }
        itm.pluginName = jItem.value("plugin", "");
        if (jItem.contains("localID")) {
            if (jItem.at("localID").is_number_unsigned() || jItem.at("localID").is_number_integer()) {
                itm.localFormID = jItem.at("localID").get<uint32_t>();
            } else if (jItem.at("localID").is_string()) {
                std::string s = jItem.at("localID").get<std::string>();
                if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
                    s = s.substr(2);
                }
                try {
                    itm.localFormID = static_cast<uint32_t>(std::stoul(s, nullptr, 16));
                } catch (...) {
                    itm.localFormID = 0;
                }
            }
        }
    }
    return itm;
}

static json SerializeProfile(const ActorWheelProfile& prof) {
    json root;
    root["activeWheel"] = prof.activeWheelIndex;
    json wheelsJson = json::array();
    for (const auto& wheel : prof.wheels) {
        json wJson;
        wJson["name"] = SanitizeUtf8(wheel.name);
        json slotsJson = json::array();
        for (const auto& slot : wheel.slots) {
            json sJson;
            if (slot.outfit.has_value()) {
                sJson["hasOutfit"] = (!slot.outfit->items.empty() || !slot.outfit->itemIDs.empty() || !slot.outfit->name.empty());
                sJson["name"] = SanitizeUtf8(slot.outfit->name);
                sJson["contextTag"] = SanitizeUtf8(slot.outfit->contextTag);
                sJson["preferenceStars"] = slot.outfit->preferenceStars;
                sJson["controlState"] = static_cast<uint8_t>(slot.outfit->controlState);
                sJson["isAddon"] = slot.outfit->isAddon;
                sJson["icon"] = slot.outfit->icon;
                sJson["usageCount"] = slot.outfit->usageCount;
                sJson["lastWornTimestamp"] = slot.outfit->lastWornTimestamp;

                json itemsArray = json::array();
                if (!slot.outfit->items.empty()) {
                    for (const auto& itm : slot.outfit->items) {
                        itemsArray.push_back(SerializeOutfitItem(itm));
                    }
                } else {
                    for (auto fID : slot.outfit->itemIDs) {
                        OutfitItem dummy;
                        dummy.formID = fID;
                        itemsArray.push_back(SerializeOutfitItem(dummy));
                    }
                }
                sJson["items"] = itemsArray;
            } else {
                sJson["hasOutfit"] = false;
                sJson["name"] = "";
                sJson["items"] = json::array();
                sJson["contextTag"] = "";
                sJson["preferenceStars"] = 3;
                sJson["controlState"] = 0;
                sJson["isAddon"] = false;
                sJson["icon"] = "";
                sJson["usageCount"] = 0;
                sJson["lastWornTimestamp"] = 0;
            }
            slotsJson.push_back(sJson);
        }
        wJson["slots"] = slotsJson;
        wheelsJson.push_back(wJson);
    }
    root["wheels"] = wheelsJson;
    return root;
}

static ActorWheelProfile DeserializeProfile(const json& profJson) {
    ActorWheelProfile prof;
    if (profJson.contains("activeWheel") && profJson["activeWheel"].is_number()) {
        prof.activeWheelIndex = profJson["activeWheel"].get<size_t>();
    }

    if (profJson.contains("wheels") && profJson["wheels"].is_array()) {
        prof.wheels.clear();
        for (const auto& wJson : profJson["wheels"]) {
            Wheel w;
            w.name = wJson.value("name", "Wheel");
            if (wJson.contains("slots") && wJson["slots"].is_array()) {
                size_t sIdx = 0;
                for (const auto& sJson : wJson["slots"]) {
                    Slot s;
                    bool hasOutfit = sJson.value("hasOutfit", false);
                    bool hasItems = sJson.contains("items") && sJson["items"].is_array() && !sJson["items"].empty();
                    bool hasName = sJson.contains("name") && !sJson["name"].get<std::string>().empty();

                    if (hasOutfit || hasItems || hasName) {
                        Outfit o;
                        o.name = sJson.value("name", "Outfit " + std::to_string(sIdx + 1));
                        o.contextTag = sJson.value("contextTag", "");
                        o.preferenceStars = sJson.value("preferenceStars", static_cast<uint8_t>(3));
                        o.controlState = static_cast<ControlState>(sJson.value("controlState", static_cast<uint8_t>(ControlState::Normal)));
                        o.isAddon = sJson.value("isAddon", false);
                        o.icon = sJson.value("icon", "");
                        o.usageCount = sJson.value("usageCount", 0u);
                        o.lastWornTimestamp = sJson.value("lastWornTimestamp", 0u);

                        if (hasItems) {
                            for (const auto& jItem : sJson["items"]) {
                                OutfitItem resolvedItm = DeserializeOutfitItem(jItem);
                                if (resolvedItm.formID != 0 || !resolvedItm.pluginName.empty() || resolvedItm.localFormID != 0) {
                                    o.items.push_back(resolvedItm);
                                    if (resolvedItm.formID != 0) {
                                        o.itemIDs.push_back(resolvedItm.formID);
                                    }
                                }
                            }
                        }
                        s.outfit = o;
                    } else {
                        Outfit o;
                        o.name = "Outfit " + std::to_string(sIdx + 1);
                        o.contextTag = "";
                        o.preferenceStars = 3;
                        o.controlState = ControlState::Normal;
                        o.isAddon = false;
                        o.icon = "";
                        s.outfit = o;
                    }
                    w.slots.push_back(s);
                    sIdx++;
                }
            }
            prof.wheels.push_back(w);
        }
    }

    if (prof.wheels.empty()) {
        Wheel defaultWheel;
        defaultWheel.name = "Wheel 1";
        for (size_t i = 0; i < 4; ++i) {
            Slot s;
            defaultWheel.slots.push_back(s);
        }
        prof.wheels.push_back(defaultWheel);
    }
    for (auto& w : prof.wheels) {
        if (w.slots.empty()) {
            for (size_t i = 0; i < 4; ++i) {
                Slot s;
                w.slots.push_back(s);
            }
        }
    }
    if (prof.activeWheelIndex >= prof.wheels.size()) {
        prof.activeWheelIndex = 0;
    }
    return prof;
}

void SaveDataInternal() {
    try {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        json root;
        root["activeTargetActorID"] = static_cast<uint32_t>(g_manager.activeTargetActorID);
        root["wardrobeContainerFormID"] = 0;

        json actorsJson = json::object();
        for (const auto& [actorID, profile] : g_manager.actorProfiles) {
            json actorNode;
            actorNode["profile"] = SerializeProfile(profile);
            actorNode["formMeta"] = SerializeFormID(actorID);
            actorsJson[FormatFormID(actorID)] = actorNode;
        }
        root["actors"] = actorsJson;

        json settingsJson;
        settingsJson["editKey"] = g_editKey;
        settingsJson["editModifierKey"] = g_editModifierKey;
        settingsJson["playKey"] = g_playKey;
        settingsJson["playModifierKey"] = g_playModifierKey;
        settingsJson["gamepadPlayKey"] = g_gamepadPlayKey;
        settingsJson["gamepadPlayModifier"] = g_gamepadPlayModifier;
        settingsJson["thumbstickDeadzone"] = g_thumbstickDeadzone;
        settingsJson["wheelScale"] = g_wheelScale;
        settingsJson["wheelOffsetX"] = g_wheelOffsetX;
        settingsJson["wheelOffsetY"] = g_wheelOffsetY;
        settingsJson["uiScale"] = g_uiScale;
        settingsJson["editUiMode"] = g_editUiMode;
        settingsJson["autoCloseOnEquip"] = g_autoCloseOnEquip.load(std::memory_order_relaxed);
        settingsJson["autoStashEnabled"] = g_autoStashEnabled.load(std::memory_order_relaxed);
        settingsJson["autoSystemEnabled"] = g_autoSystemEnabled.load(std::memory_order_relaxed);
        settingsJson["autoCityEnabled"] = g_autoCityEnabled.load(std::memory_order_relaxed);
        settingsJson["autoHomeEnabled"] = g_autoHomeEnabled.load(std::memory_order_relaxed);
        settingsJson["autoCombatEnabled"] = g_autoCombatEnabled.load(std::memory_order_relaxed);
        settingsJson["autoSwimEnabled"] = g_autoSwimEnabled.load(std::memory_order_relaxed);
        root["settings"] = settingsJson;

        std::filesystem::create_directories("Data/SKSE/Plugins");

        if (std::filesystem::exists(g_savePath)) {
            std::string bakPath = g_savePath + ".bak";
            std::error_code ecBak;
            std::filesystem::copy_file(g_savePath, bakPath, std::filesystem::copy_options::overwrite_existing, ecBak);
        }

        std::string tmpPath = g_savePath + ".tmp";
        {
            std::ofstream file(tmpPath);
            if (!file.is_open()) return;
            file << root.dump(-1, ' ', false, json::error_handler_t::replace);
        }

        std::error_code ec;
        std::filesystem::remove(g_savePath, ec);
        std::filesystem::rename(tmpPath, g_savePath, ec);
        if (ec) {
            std::filesystem::copy_file(tmpPath, g_savePath, std::filesystem::copy_options::overwrite_existing, ec);
            std::filesystem::remove(tmpPath, ec);
        }

        SyncWheelOutfitsToAdaptiveWardrobe();
    } catch (...) {}
}

void LoadData() {
    std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
    std::string targetPath = g_savePath;
    if (!std::filesystem::exists(targetPath)) {
        if (std::filesystem::exists(g_fallbackSavePath1)) {
            targetPath = g_fallbackSavePath1;
        } else if (std::filesystem::exists(g_fallbackSavePath2)) {
            targetPath = g_fallbackSavePath2;
        } else if (std::filesystem::exists(g_fallbackSavePath3)) {
            targetPath = g_fallbackSavePath3;
        } else if (std::filesystem::exists(g_savePath + ".bak")) {
            targetPath = g_savePath + ".bak";
        } else {
            return;
        }
    }

    std::ifstream file(targetPath);
    if (!file.is_open()) return;

    try {
        json root = json::parse(file, nullptr, false);
        if (root.is_discarded()) return;

        g_manager.wardrobeContainerFormID = 0;

        if (root.contains("actors") && root["actors"].is_object()) {
            g_manager.actorProfiles.clear();
            for (auto& [keyStr, actorDataJson] : root["actors"].items()) {
                RE::FormID aID = 0x00000007;

                if (actorDataJson.is_object() && actorDataJson.contains("formMeta")) {
                    RE::FormID resolvedActor = DeserializeFormID(actorDataJson["formMeta"]);
                    if (resolvedActor != 0) {
                        aID = resolvedActor;
                    }
                }

                if (aID == 0x00000007 && (keyStr.rfind("0x", 0) == 0 || keyStr.rfind("0X", 0) == 0)) {
                    try {
                        aID = static_cast<RE::FormID>(std::stoul(keyStr.substr(2), nullptr, 16));
                    } catch (...) {
                        aID = 0x00000007;
                    }
                }

                if (actorDataJson.is_object() && actorDataJson.contains("profile")) {
                    g_manager.actorProfiles[aID] = DeserializeProfile(actorDataJson["profile"]);
                } else {
                    g_manager.actorProfiles[aID] = DeserializeProfile(actorDataJson);
                }
            }
        } else if (root.contains("wheels")) {
            g_manager.actorProfiles.clear();
            g_manager.actorProfiles[0x00000007] = DeserializeProfile(root);
        }

        if (root.contains("settings") && root["settings"].is_object()) {
            const auto& s = root["settings"];
            if (s.contains("editKey") && s["editKey"].is_number()) g_editKey = s["editKey"].get<uint32_t>();
            if (s.contains("editModifierKey") && s["editModifierKey"].is_number()) g_editModifierKey = s["editModifierKey"].get<uint32_t>();
            if (s.contains("playKey") && s["playKey"].is_number()) g_playKey = s["playKey"].get<uint32_t>();
            if (s.contains("playModifierKey") && s["playModifierKey"].is_number()) g_playModifierKey = s["playModifierKey"].get<uint32_t>();
            if (s.contains("gamepadPlayKey") && s["gamepadPlayKey"].is_number()) g_gamepadPlayKey = s["gamepadPlayKey"].get<uint32_t>();
            if (s.contains("gamepadPlayModifier") && s["gamepadPlayModifier"].is_number()) g_gamepadPlayModifier = s["gamepadPlayModifier"].get<uint32_t>();
            if (s.contains("thumbstickDeadzone") && s["thumbstickDeadzone"].is_number()) g_thumbstickDeadzone = s["thumbstickDeadzone"].get<float>();
            if (s.contains("wheelScale") && s["wheelScale"].is_number()) g_wheelScale = s["wheelScale"].get<float>();
            if (s.contains("wheelOffsetX") && s["wheelOffsetX"].is_number()) g_wheelOffsetX = s["wheelOffsetX"].get<float>();
            if (s.contains("wheelOffsetY") && s["wheelOffsetY"].is_number()) g_wheelOffsetY = s["wheelOffsetY"].get<float>();
            if (s.contains("uiScale") && s["uiScale"].is_number()) g_uiScale = s["uiScale"].get<float>();
            if (s.contains("editUiMode") && s["editUiMode"].is_number()) g_editUiMode = s["editUiMode"].get<uint32_t>();
            if (s.contains("autoCloseOnEquip") && s["autoCloseOnEquip"].is_boolean()) g_autoCloseOnEquip.store(s["autoCloseOnEquip"].get<bool>(), std::memory_order_relaxed);
            if (s.contains("autoStashEnabled") && s["autoStashEnabled"].is_boolean()) g_autoStashEnabled.store(s["autoStashEnabled"].get<bool>(), std::memory_order_relaxed);
            if (s.contains("autoSystemEnabled") && s["autoSystemEnabled"].is_boolean()) g_autoSystemEnabled.store(s["autoSystemEnabled"].get<bool>(), std::memory_order_relaxed);
            if (s.contains("autoCityEnabled") && s["autoCityEnabled"].is_boolean()) g_autoCityEnabled.store(s["autoCityEnabled"].get<bool>(), std::memory_order_relaxed);
            if (s.contains("autoHomeEnabled") && s["autoHomeEnabled"].is_boolean()) g_autoHomeEnabled.store(s["autoHomeEnabled"].get<bool>(), std::memory_order_relaxed);
            if (s.contains("autoCombatEnabled") && s["autoCombatEnabled"].is_boolean()) g_autoCombatEnabled.store(s["autoCombatEnabled"].get<bool>(), std::memory_order_relaxed);
            if (s.contains("autoSwimEnabled") && s["autoSwimEnabled"].is_boolean()) g_autoSwimEnabled.store(s["autoSwimEnabled"].get<bool>(), std::memory_order_relaxed);
        }

        g_manager.GetProfile(0x00000007);
        SyncWheelOutfitsToAdaptiveWardrobe();
        if (g_gameFullyLoaded.load(std::memory_order_acquire)) {
            QueueAllLoadedItemThumbnails();
        }
    } catch (...) {}
}

void QueueAllLoadedItemThumbnails() {
    std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
    auto* dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) return;

    auto& thumbnailService = OutfitWheeler::Thumbnail::ItemThumbnailService::GetSingleton();
    size_t count = 0;
    for (const auto& [actorID, profile] : g_manager.actorProfiles) {
        for (const auto& wheel : profile.wheels) {
            for (const auto& slot : wheel.slots) {
                if (slot.outfit.has_value()) {
                    for (const auto& item : slot.outfit->items) {
                        if (item.formID != 0) {
                            auto* form = RE::TESForm::LookupByID(item.formID);
                            if (form && IsPureApparel(form)) {
                                if (auto* bound = form->As<RE::TESBoundObject>()) {
                                    thumbnailService.QueueItem(bound, true);
                                    count++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (count > 0) {
        logger::info("[Thumbnail] Queued {} outfit apparel items for 3D thumbnail background generation", count);
    }
}

void QueuePlayerInventoryThumbnails() {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    auto inv = player->GetInventory();
    auto& thumbnailService = OutfitWheeler::Thumbnail::ItemThumbnailService::GetSingleton();
    std::vector<RE::TESBoundObject*> wornItems;
    std::vector<RE::TESBoundObject*> otherItems;
    for (const auto& [item, data] : inv) {
        if (!item || data.first <= 0) continue;
        if (!IsPureApparel(item)) continue;
        auto* bound = item->As<RE::TESBoundObject>();
        if (!bound) continue;
        bool isWorn = data.second && data.second->IsWorn();
        if (isWorn) {
            wornItems.push_back(bound);
        } else {
            otherItems.push_back(bound);
        }
    }
    if (!wornItems.empty()) {
        thumbnailService.QueueItems(wornItems, true);
    }
    if (!otherItems.empty()) {
        thumbnailService.QueueItems(otherItems, false);
    }
    if (!wornItems.empty() || !otherItems.empty()) {
        logger::info("[Thumbnail] Queued {} worn and {} inventory apparel items for generation", wornItems.size(), otherItems.size());
    }
}

void QueueWardrobeInventoryThumbnails() {
    auto* wardrobe = GetWardrobeContainer();
    if (!wardrobe) return;

    auto inv = wardrobe->GetInventory();
    auto& thumbnailService = OutfitWheeler::Thumbnail::ItemThumbnailService::GetSingleton();
    std::vector<RE::TESBoundObject*> items;
    items.reserve(inv.size());
    for (const auto& [item, data] : inv) {
        if (!item || data.first <= 0) continue;
        if (!IsPureApparel(item)) continue;
        auto* bound = item->As<RE::TESBoundObject>();
        if (!bound) continue;
        items.push_back(bound);
    }
    if (!items.empty()) {
        thumbnailService.QueueItems(items, false);
        logger::info("[Thumbnail] Queued {} wardrobe chest apparel items for background generation", items.size());
    }
}

void QueueFollowerInventoryThumbnails() {
    auto* processLists = RE::ProcessLists::GetSingleton();
    if (!processLists) return;

    auto& thumbnailService = OutfitWheeler::Thumbnail::ItemThumbnailService::GetSingleton();
    std::vector<RE::TESBoundObject*> items;
    for (auto handle : processLists->highActorHandles) {
        auto actorPtr = handle.get();
        if (!actorPtr) continue;
        auto* actor = actorPtr.get();
        if (!actor || !actor->IsPlayerTeammate()) continue;
        auto inv = actor->GetInventory();
        for (const auto& [item, data] : inv) {
            if (!item || data.first <= 0) continue;
            if (!IsPureApparel(item)) continue;
            auto* bound = item->As<RE::TESBoundObject>();
            if (!bound) continue;
            items.push_back(bound);
        }
    }
    if (!items.empty()) {
        thumbnailService.QueueItems(items, false);
        logger::info("[Thumbnail] Queued {} follower apparel items for background generation", items.size());
    }
}

std::string ExportConfigToString() {
    try {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        json root;
        root["activeTargetActorID"] = static_cast<uint32_t>(g_manager.activeTargetActorID);
        root["wardrobeContainerFormID"] = 0;

        json actorsJson = json::object();
        for (const auto& [actorID, profile] : g_manager.actorProfiles) {
            json actorNode;
            actorNode["profile"] = SerializeProfile(profile);
            actorNode["formMeta"] = SerializeFormID(actorID);
            actorsJson[FormatFormID(actorID)] = actorNode;
        }
        root["actors"] = actorsJson;

        json settingsJson;
        settingsJson["editKey"] = g_editKey;
        settingsJson["editModifierKey"] = g_editModifierKey;
        settingsJson["playKey"] = g_playKey;
        settingsJson["playModifierKey"] = g_playModifierKey;
        settingsJson["gamepadPlayKey"] = g_gamepadPlayKey;
        settingsJson["gamepadPlayModifier"] = g_gamepadPlayModifier;
        settingsJson["thumbstickDeadzone"] = g_thumbstickDeadzone;
        settingsJson["wheelScale"] = g_wheelScale;
        settingsJson["wheelOffsetX"] = g_wheelOffsetX;
        settingsJson["wheelOffsetY"] = g_wheelOffsetY;
        settingsJson["uiScale"] = g_uiScale;
        settingsJson["editUiMode"] = g_editUiMode;
        settingsJson["autoCloseOnEquip"] = g_autoCloseOnEquip.load(std::memory_order_relaxed);
        settingsJson["autoStashEnabled"] = g_autoStashEnabled.load(std::memory_order_relaxed);
        settingsJson["autoSystemEnabled"] = g_autoSystemEnabled.load(std::memory_order_relaxed);
        settingsJson["autoCityEnabled"] = g_autoCityEnabled.load(std::memory_order_relaxed);
        settingsJson["autoHomeEnabled"] = g_autoHomeEnabled.load(std::memory_order_relaxed);
        settingsJson["autoCombatEnabled"] = g_autoCombatEnabled.load(std::memory_order_relaxed);
        settingsJson["autoSwimEnabled"] = g_autoSwimEnabled.load(std::memory_order_relaxed);
        root["settings"] = settingsJson;

        return root.dump(2, ' ', false, json::error_handler_t::replace);
    } catch (...) {
        return "{}";
    }
}

bool ImportConfigFromString(const std::string& jsonStr) {
    if (jsonStr.empty()) return false;
    try {
        json root = json::parse(jsonStr, nullptr, false);
        if (root.is_discarded()) return false;

        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        if (root.contains("actors") && root["actors"].is_object()) {
            g_manager.actorProfiles.clear();
            for (auto& [keyStr, actorDataJson] : root["actors"].items()) {
                RE::FormID aID = 0x00000007;

                if (actorDataJson.is_object() && actorDataJson.contains("formMeta")) {
                    RE::FormID resolvedActor = DeserializeFormID(actorDataJson["formMeta"]);
                    if (resolvedActor != 0) aID = resolvedActor;
                }

                if (aID == 0x00000007 && (keyStr.rfind("0x", 0) == 0 || keyStr.rfind("0X", 0) == 0)) {
                    try {
                        aID = static_cast<RE::FormID>(std::stoul(keyStr.substr(2), nullptr, 16));
                    } catch (...) { aID = 0x00000007; }
                }

                if (actorDataJson.is_object() && actorDataJson.contains("profile")) {
                    g_manager.actorProfiles[aID] = DeserializeProfile(actorDataJson["profile"]);
                } else {
                    g_manager.actorProfiles[aID] = DeserializeProfile(actorDataJson);
                }
            }
        } else if (root.contains("wheels")) {
            g_manager.actorProfiles.clear();
            g_manager.actorProfiles[0x00000007] = DeserializeProfile(root);
        }

        SaveDataInternal();
        return true;
    } catch (...) {
        return false;
    }
}

static bool CheckGamepadConnected() {
    typedef DWORD(WINAPI* XInputGetState_t)(DWORD, XINPUT_STATE*);
    static auto pXInputGetState = reinterpret_cast<XInputGetState_t>(GetProcAddress(GetModuleHandleA("xinput1_4.dll"), "XInputGetState"));
    if (!pXInputGetState) {
        pXInputGetState = reinterpret_cast<XInputGetState_t>(GetProcAddress(LoadLibraryA("xinput1_4.dll"), "XInputGetState"));
    }
    if (!pXInputGetState) {
        pXInputGetState = reinterpret_cast<XInputGetState_t>(GetProcAddress(LoadLibraryA("xinput9_1_0.dll"), "XInputGetState"));
    }
    if (pXInputGetState) {
        for (DWORD i = 0; i < 4; ++i) {
            XINPUT_STATE xState{};
            if (pXInputGetState(i, &xState) == ERROR_SUCCESS) {
                return true;
            }
        }
    }
    return false;
}

static json GetDynamicFollowersList() {
    json arr = json::array();
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return arr;

    auto* pAvOwner = player->AsActorValueOwner();
    float pCurWeight = pAvOwner ? pAvOwner->GetActorValue(RE::ActorValue::kInventoryWeight) : 0.0f;
    float pMaxWeight = pAvOwner ? pAvOwner->GetActorValue(RE::ActorValue::kCarryWeight) : 300.0f;
    char pWBuf[32];
    snprintf(pWBuf, sizeof(pWBuf), "%d / %d", static_cast<int>(pCurWeight), static_cast<int>(pMaxWeight));

    std::string pName = (player->GetActorBase() && player->GetActorBase()->GetName()) ? player->GetActorBase()->GetName() : "Player Character";
    arr.push_back(json{
        {"id", "0x00000007"},
        {"name", SanitizeUtf8(pName)},
        {"type", "PLAYER"},
        {"avatar", "D"},
        {"avatarUrl", ""},
        {"weight", std::string(pWBuf)}
    });

    auto processLists = RE::ProcessLists::GetSingleton();
    auto* npcKw = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("ActorTypeNPC");

    if (processLists) {
        std::set<RE::FormID> addedActors;
        addedActors.insert(0x00000007);
        addedActors.insert(0x00000014);

        for (auto& handle : processLists->highActorHandles) {
            auto actor = handle.get();
            if (!actor || actor.get() == player || actor->IsDead()) continue;

            auto* race = actor->GetRace();
            if (!race || (npcKw && !race->HasKeyword(npcKw))) continue;

            if (actor->IsPlayerTeammate() || actor->IsPlayerRef()) {
                RE::FormID fID = actor->GetFormID();
                if (addedActors.find(fID) != addedActors.end()) continue;
                addedActors.insert(fID);

                auto* base = actor->GetActorBase();
                if (!base) continue;

                auto* aAvOwner = actor->AsActorValueOwner();
                float curW = aAvOwner ? aAvOwner->GetActorValue(RE::ActorValue::kInventoryWeight) : 0.0f;
                float maxW = aAvOwner ? aAvOwner->GetActorValue(RE::ActorValue::kCarryWeight) : 300.0f;
                char aWBuf[32];
                snprintf(aWBuf, sizeof(aWBuf), "%d / %d", static_cast<int>(curW), static_cast<int>(maxW));

                std::string aName = base->GetName() ? base->GetName() : "Follower";
                std::string safeAName = SanitizeUtf8(aName);
                std::string av = safeAName.empty() ? "F" : std::string(1, safeAName[0]);

                arr.push_back(json{
                    {"id", FormatFormID(fID)},
                    {"name", safeAName},
                    {"type", "FOLLOWER"},
                    {"avatar", av},
                    {"avatarUrl", ""},
                    {"weight", std::string(aWBuf)}
                });
            }
        }
    }
    return arr;
}

static std::string GetApparelTypeString(RE::TESObjectARMO* armor) {
    if (!armor) return "Armor";
    uint32_t slotMask = static_cast<uint32_t>(armor->GetSlotMask());
    using Slot = RE::BGSBipedObjectForm::BipedObjectSlot;

    if (slotMask & (static_cast<uint32_t>(Slot::kHead) | static_cast<uint32_t>(Slot::kHair) | static_cast<uint32_t>(Slot::kCirclet))) {
        return "Helmet";
    }
    if (slotMask & static_cast<uint32_t>(Slot::kAmulet)) {
        return "Amulet";
    }
    if (slotMask & static_cast<uint32_t>(Slot::kRing)) {
        return "Ring";
    }
    if (slotMask & static_cast<uint32_t>(Slot::kHands)) {
        return "Gauntlets";
    }
    if (slotMask & static_cast<uint32_t>(Slot::kFeet)) {
        return "Boots";
    }
    if (slotMask & static_cast<uint32_t>(Slot::kShield)) {
        return "Shield";
    }
    if (slotMask & static_cast<uint32_t>(Slot::kBody)) {
        return "Cuirass";
    }
    return "Armor";
}

static json GetActorApparelInventoryJson(RE::Actor* actor, const std::string& sourceTag) {
    json arr = json::array();
    if (!actor || !actor->GetActorBase()) return arr;

    std::unordered_set<RE::FormID> bipedWornFormIDs;
    if (const auto& biped = actor->GetCurrentBiped()) {
        for (size_t i = 0; i < 42; ++i) {
            if (biped->objects[i].item) {
                bipedWornFormIDs.insert(biped->objects[i].item->GetFormID());
            }
        }
    }
    if (const auto& biped2 = actor->GetActorRuntimeData().biped) {
        for (size_t i = 0; i < 42; ++i) {
            if (biped2->objects[i].item) {
                bipedWornFormIDs.insert(biped2->objects[i].item->GetFormID());
            }
        }
    }
    std::unordered_set<RE::FormID> addedItemFormIDs;

    auto inv = actor->GetInventory();
    for (const auto& [item, data] : inv) {
        if (!item || data.first <= 0) continue;
        if (!IsPureApparel(item)) continue;

        addedItemFormIDs.insert(item->GetFormID());

        auto* armor = item->As<RE::TESObjectARMO>();
        if (!armor) continue;

        std::string thumbPath = "";
        std::string bMod = "Skyrim.esm";
        if (auto* bound = item->As<RE::TESBoundObject>()) {
            auto* bFile = bound->GetFile(0);
            if (bFile && !bFile->GetFilename().empty()) {
                bMod = bFile->GetFilename();
            }
            uint32_t bLoc = bound->GetLocalFormID();
            if (OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().HasThumbnail(bMod, bLoc)) {
                thumbPath = OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().GetRelativePath(bMod, bLoc);
            }
        }

        std::string typeStr = GetApparelTypeString(armor);
        std::string catStr = (typeStr == "Ring" || typeStr == "Amulet") ? "JEWELRY" : "ARMORS";
        std::string armorClassStr = armor->IsHeavyArmor() ? "Heavy" : (armor->IsLightArmor() ? "Light" : "Clothing");

        float armRating = static_cast<float>(armor->GetArmorRating());
        float wt = item->GetWeight();
        int val = item->GetGoldValue();

        auto* entryData = data.second.get();
        bool entryIsWorn = (entryData ? entryData->IsWorn() : false) || bipedWornFormIDs.contains(item->GetFormID());
        if (entryData && entryData->extraLists && !entryData->extraLists->empty()) {
            bool anyWorn = false;
            for (auto* xList : *entryData->extraLists) {
                if (xList && (xList->HasType(RE::ExtraDataType::kWorn) || xList->HasType(RE::ExtraDataType::kWornLeft))) {
                    anyWorn = true;
                    break;
                }
            }
            bool assignedWorn = false;
            int xListCount = 0;
            for (auto* xList : *entryData->extraLists) {
                if (!xList) continue;
                xListCount++;

                std::string displayName = GetTrueItemDisplayName(xList, armor);
                std::string customName = GetTrueItemCustomName(xList);

                uint16_t uniqueID = 0;
                auto* xUID = xList->GetByType<RE::ExtraUniqueID>();
                if (xUID) uniqueID = xUID->uniqueID;

                RE::FormID enchantmentID = 0;
                std::string enchName = "";
                std::vector<std::string> tags;
                if (armor->IsHeavyArmor()) tags.push_back("heavy");
                else if (armor->IsLightArmor()) tags.push_back("light");
                else if (armor->IsClothing()) tags.push_back("clothing");

                auto* ench = GetTrueItemEnchantment(xList, armor);
                if (ench) {
                    enchantmentID = ench->GetFormID();
                    enchName = SanitizeUtf8(ench->GetName() ? ench->GetName() : "Enchanted");
                    tags.push_back("enchanted");
                }

                bool isWorn = xList->HasType(RE::ExtraDataType::kWorn) || xList->HasType(RE::ExtraDataType::kWornLeft);
                if (!isWorn && !anyWorn && entryIsWorn && !assignedWorn) {
                    isWorn = true;
                    assignedWorn = true;
                }

                arr.push_back(json{
                    {"id", FormatFormID(item->GetFormID())},
                    {"rawID", static_cast<uint32_t>(item->GetFormID())},
                    {"uniqueID", uniqueID},
                    {"enchantmentID", static_cast<uint32_t>(enchantmentID)},
                    {"customName", SanitizeUtf8(customName)},
                    {"name", SanitizeUtf8(displayName)},
                    {"category", catStr},
                    {"type", typeStr},
                    {"armorClass", armorClassStr},
                    {"damage", 0},
                    {"armor", static_cast<int>(armRating)},
                    {"weight", wt},
                    {"value", val},
                    {"enchantment", enchName},
                    {"tags", tags},
                    {"equipped", isWorn},
                    {"count", 1},
                    {"source", sourceTag},
                    {"thumbnail", thumbPath},
                    {"modName", bMod}
                });
            }

            int plainCount = data.first - xListCount;
            if (plainCount > 0) {
                std::string rawName = item->GetName() ? item->GetName() : "Apparel";
                std::string enchName = "";
                std::vector<std::string> tags;
                if (armor->IsHeavyArmor()) tags.push_back("heavy");
                else if (armor->IsLightArmor()) tags.push_back("light");
                else if (armor->IsClothing()) tags.push_back("clothing");
                RE::FormID baseEnchID = 0;
                if (armor->formEnchanting) {
                    baseEnchID = armor->formEnchanting->GetFormID();
                    enchName = SanitizeUtf8(armor->formEnchanting->GetName() ? armor->formEnchanting->GetName() : "Enchanted");
                    tags.push_back("enchanted");
                }

                arr.push_back(json{
                    {"id", FormatFormID(item->GetFormID())},
                    {"rawID", static_cast<uint32_t>(item->GetFormID())},
                    {"uniqueID", 0},
                    {"enchantmentID", static_cast<uint32_t>(baseEnchID)},
                    {"customName", ""},
                    {"name", SanitizeUtf8(rawName)},
                    {"category", catStr},
                    {"type", typeStr},
                    {"armorClass", armorClassStr},
                    {"damage", 0},
                    {"armor", static_cast<int>(armRating)},
                    {"weight", wt},
                    {"value", val},
                    {"enchantment", enchName},
                    {"tags", tags},
                    {"equipped", false},
                    {"count", plainCount},
                    {"source", sourceTag},
                    {"thumbnail", thumbPath},
                    {"modName", bMod}
                });
            }
        } else {
            std::string rawName = item->GetName() ? item->GetName() : "Apparel";
            std::string enchName = "";
            std::vector<std::string> tags;
            if (armor->IsHeavyArmor()) tags.push_back("heavy");
            else if (armor->IsLightArmor()) tags.push_back("light");
            else if (armor->IsClothing()) tags.push_back("clothing");
            RE::FormID enchantmentID = 0;
            if (armor->formEnchanting) {
                enchantmentID = armor->formEnchanting->GetFormID();
                enchName = SanitizeUtf8(armor->formEnchanting->GetName() ? armor->formEnchanting->GetName() : "Enchanted");
                tags.push_back("enchanted");
            }

            bool isWorn = entryIsWorn;
            if (isWorn && data.first > 1) {
                arr.push_back(json{
                    {"id", FormatFormID(item->GetFormID())},
                    {"rawID", static_cast<uint32_t>(item->GetFormID())},
                    {"uniqueID", 0},
                    {"enchantmentID", static_cast<uint32_t>(enchantmentID)},
                    {"customName", ""},
                    {"name", SanitizeUtf8(rawName)},
                    {"category", catStr},
                    {"type", typeStr},
                    {"armorClass", armorClassStr},
                    {"damage", 0},
                    {"armor", static_cast<int>(armRating)},
                    {"weight", wt},
                    {"value", val},
                    {"enchantment", enchName},
                    {"tags", tags},
                    {"equipped", true},
                    {"count", 1},
                    {"source", sourceTag},
                    {"thumbnail", thumbPath},
                    {"modName", bMod}
                });
                arr.push_back(json{
                    {"id", FormatFormID(item->GetFormID())},
                    {"rawID", static_cast<uint32_t>(item->GetFormID())},
                    {"uniqueID", 0},
                    {"enchantmentID", static_cast<uint32_t>(enchantmentID)},
                    {"customName", ""},
                    {"name", SanitizeUtf8(rawName)},
                    {"category", catStr},
                    {"type", typeStr},
                    {"armorClass", armorClassStr},
                    {"damage", 0},
                    {"armor", static_cast<int>(armRating)},
                    {"weight", wt},
                    {"value", val},
                    {"enchantment", enchName},
                    {"tags", tags},
                    {"equipped", false},
                    {"count", data.first - 1},
                    {"source", sourceTag},
                    {"thumbnail", thumbPath},
                    {"modName", bMod}
                });
            } else {
                arr.push_back(json{
                    {"id", FormatFormID(item->GetFormID())},
                    {"rawID", static_cast<uint32_t>(item->GetFormID())},
                    {"uniqueID", 0},
                    {"enchantmentID", static_cast<uint32_t>(enchantmentID)},
                    {"customName", ""},
                    {"name", SanitizeUtf8(rawName)},
                    {"category", catStr},
                    {"type", typeStr},
                    {"armorClass", armorClassStr},
                    {"damage", 0},
                    {"armor", static_cast<int>(armRating)},
                    {"weight", wt},
                    {"value", val},
                    {"enchantment", enchName},
                    {"tags", tags},
                    {"equipped", isWorn},
                    {"count", data.first},
                    {"source", sourceTag},
                    {"thumbnail", thumbPath},
                    {"modName", bMod}
                });
            }
        }
    }
    for (RE::FormID fID : bipedWornFormIDs) {
        if (addedItemFormIDs.find(fID) == addedItemFormIDs.end()) {
            auto* f = RE::TESForm::LookupByID(fID);
            if (f && IsPureApparel(f)) {
                auto* armor = f->As<RE::TESObjectARMO>();
                if (!armor) continue;
                std::string thumbPath = "";
                std::string bMod = "Skyrim.esm";
                if (auto* bound = f->As<RE::TESBoundObject>()) {
                    auto* bFile = bound->GetFile(0);
                    if (bFile && !bFile->GetFilename().empty()) {
                        bMod = bFile->GetFilename();
                    }
                    uint32_t bLoc = bound->GetLocalFormID();
                    if (OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().HasThumbnail(bMod, bLoc)) {
                        thumbPath = OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().GetRelativePath(bMod, bLoc);
                    } else {
                        OutfitWheeler::Thumbnail::ItemThumbnailService::GetSingleton().QueueItem(bound, false);
                    }
                }
                std::string typeStr = GetApparelTypeString(armor);
                std::string catStr = (typeStr == "Ring" || typeStr == "Amulet") ? "JEWELRY" : "ARMORS";
                std::string armorClassStr = armor->IsHeavyArmor() ? "Heavy" : (armor->IsLightArmor() ? "Light" : "Clothing");
                float armRating = static_cast<float>(armor->GetArmorRating());
                float wt = f->GetWeight();
                int val = f->GetGoldValue();
                std::string rawName = f->GetName() ? f->GetName() : "Apparel";
                std::string enchName = "";
                std::vector<std::string> tags;
                if (armor->IsHeavyArmor()) tags.push_back("heavy");
                else if (armor->IsLightArmor()) tags.push_back("light");
                else if (armor->IsClothing()) tags.push_back("clothing");
                RE::FormID enchantmentID = 0;
                if (armor->formEnchanting) {
                    enchantmentID = armor->formEnchanting->GetFormID();
                    enchName = SanitizeUtf8(armor->formEnchanting->GetName() ? armor->formEnchanting->GetName() : "Enchanted");
                    tags.push_back("enchanted");
                }
                arr.push_back(json{
                    {"id", FormatFormID(fID)},
                    {"rawID", static_cast<uint32_t>(fID)},
                    {"uniqueID", 0},
                    {"enchantmentID", static_cast<uint32_t>(enchantmentID)},
                    {"customName", ""},
                    {"name", SanitizeUtf8(rawName)},
                    {"category", catStr},
                    {"type", typeStr},
                    {"armorClass", armorClassStr},
                    {"damage", 0},
                    {"armor", static_cast<int>(armRating)},
                    {"weight", wt},
                    {"value", val},
                    {"enchantment", enchName},
                    {"tags", tags},
                    {"equipped", true},
                    {"count", 1},
                    {"source", sourceTag},
                    {"thumbnail", thumbPath},
                    {"modName", bMod}
                });
                addedItemFormIDs.insert(fID);
            }
        }
    }
    return arr;
}

static json GetCloudWardrobeInventoryJson() {
    json arr = json::array();
    auto* wardrobe = GetWardrobeContainer();
    if (!wardrobe) return arr;

    auto inv = wardrobe->GetInventory();
    for (const auto& [item, data] : inv) {
        if (!item || data.first <= 0) continue;
        if (!IsPureApparel(item)) continue;

        auto* armor = item->As<RE::TESObjectARMO>();
        if (!armor) continue;

        std::string thumbPath = "";
        std::string bMod = "Skyrim.esm";
        if (auto* bound = item->As<RE::TESBoundObject>()) {
            auto* bFile = bound->GetFile(0);
            if (bFile && !bFile->GetFilename().empty()) {
                bMod = bFile->GetFilename();
            }
            uint32_t bLoc = bound->GetLocalFormID();
            if (OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().HasThumbnail(bMod, bLoc)) {
                thumbPath = OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().GetRelativePath(bMod, bLoc);
            }
        }

        std::string typeStr = GetApparelTypeString(armor);
        std::string catStr = (typeStr == "Ring" || typeStr == "Amulet") ? "JEWELRY" : "ARMORS";
        std::string armorClassStr = armor->IsHeavyArmor() ? "Heavy" : (armor->IsLightArmor() ? "Light" : "Clothing");

        float armRating = static_cast<float>(armor->GetArmorRating());
        float wt = item->GetWeight();
        int val = item->GetGoldValue();

        auto* entryData = data.second.get();
        if (entryData && entryData->extraLists && !entryData->extraLists->empty()) {
            int xListCount = 0;
            for (auto* xList : *entryData->extraLists) {
                if (!xList) continue;
                xListCount++;

                std::string displayName = GetTrueItemDisplayName(xList, armor);
                std::string customName = GetTrueItemCustomName(xList);

                uint16_t uniqueID = 0;
                auto* xUID = xList->GetByType<RE::ExtraUniqueID>();
                if (xUID) uniqueID = xUID->uniqueID;

                RE::FormID enchantmentID = 0;
                std::string enchName = "";
                std::vector<std::string> tags;
                if (armor->IsHeavyArmor()) tags.push_back("heavy");
                else if (armor->IsLightArmor()) tags.push_back("light");
                else if (armor->IsClothing()) tags.push_back("clothing");

                auto* ench = GetTrueItemEnchantment(xList, armor);
                if (ench) {
                    enchantmentID = ench->GetFormID();
                    enchName = SanitizeUtf8(ench->GetName() ? ench->GetName() : "Enchanted");
                    tags.push_back("enchanted");
                }

                arr.push_back(json{
                    {"id", FormatFormID(item->GetFormID())},
                    {"rawID", static_cast<uint32_t>(item->GetFormID())},
                    {"uniqueID", uniqueID},
                    {"enchantmentID", static_cast<uint32_t>(enchantmentID)},
                    {"customName", SanitizeUtf8(customName)},
                    {"name", SanitizeUtf8(displayName)},
                    {"category", catStr},
                    {"type", typeStr},
                    {"armorClass", armorClassStr},
                    {"damage", 0},
                    {"armor", static_cast<int>(armRating)},
                    {"weight", wt},
                    {"value", val},
                    {"enchantment", enchName},
                    {"tags", tags},
                    {"equipped", false},
                    {"count", 1},
                    {"source", "CLOUD"},
                    {"thumbnail", thumbPath},
                    {"modName", bMod}
                });
            }

            int plainCount = data.first - xListCount;
            if (plainCount > 0) {
                std::string rawName = item->GetName() ? item->GetName() : "Apparel";
                std::string enchName = "";
                std::vector<std::string> tags;
                if (armor->IsHeavyArmor()) tags.push_back("heavy");
                else if (armor->IsLightArmor()) tags.push_back("light");
                else if (armor->IsClothing()) tags.push_back("clothing");
                RE::FormID baseEnchID = 0;
                if (armor->formEnchanting) {
                    baseEnchID = armor->formEnchanting->GetFormID();
                    enchName = SanitizeUtf8(armor->formEnchanting->GetName() ? armor->formEnchanting->GetName() : "Enchanted");
                    tags.push_back("enchanted");
                }

                arr.push_back(json{
                    {"id", FormatFormID(item->GetFormID())},
                    {"rawID", static_cast<uint32_t>(item->GetFormID())},
                    {"uniqueID", 0},
                    {"enchantmentID", static_cast<uint32_t>(baseEnchID)},
                    {"customName", ""},
                    {"name", SanitizeUtf8(rawName)},
                    {"category", catStr},
                    {"type", typeStr},
                    {"armorClass", armorClassStr},
                    {"damage", 0},
                    {"armor", static_cast<int>(armRating)},
                    {"weight", wt},
                    {"value", val},
                    {"enchantment", enchName},
                    {"tags", tags},
                    {"equipped", false},
                    {"count", plainCount},
                    {"source", "CLOUD"},
                    {"thumbnail", thumbPath},
                    {"modName", bMod}
                });
            }
        } else {
            std::string rawName = item->GetName() ? item->GetName() : "Apparel";
            std::string enchName = "";
            std::vector<std::string> tags;
            if (armor->IsHeavyArmor()) tags.push_back("heavy");
            else if (armor->IsLightArmor()) tags.push_back("light");
            else if (armor->IsClothing()) tags.push_back("clothing");
            RE::FormID enchantmentID = 0;
            if (armor->formEnchanting) {
                enchantmentID = armor->formEnchanting->GetFormID();
                enchName = SanitizeUtf8(armor->formEnchanting->GetName() ? armor->formEnchanting->GetName() : "Enchanted");
                tags.push_back("enchanted");
            }

            arr.push_back(json{
                {"id", FormatFormID(item->GetFormID())},
                {"rawID", static_cast<uint32_t>(item->GetFormID())},
                {"uniqueID", 0},
                {"enchantmentID", static_cast<uint32_t>(enchantmentID)},
                {"customName", ""},
                {"name", SanitizeUtf8(rawName)},
                {"category", catStr},
                {"type", typeStr},
                {"armorClass", armorClassStr},
                {"damage", 0},
                {"armor", static_cast<int>(armRating)},
                {"weight", wt},
                {"value", val},
                {"enchantment", enchName},
                {"tags", tags},
                {"equipped", false},
                {"count", data.first},
                {"source", "CLOUD"},
                {"thumbnail", thumbPath},
                {"modName", bMod}
            });
        }
    }
    return arr;
}

std::string GetWheelJsonInternal(bool isEditMode) {
    try {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        auto& activeProfile = g_manager.GetActiveProfile();
        if (activeProfile.wheels.empty()) {
            Wheel defaultWheel;
            defaultWheel.name = "Wheel 1";
            for (size_t i = 0; i < 4; ++i) {
                Slot s;
                defaultWheel.slots.push_back(s);
            }
            activeProfile.wheels.push_back(defaultWheel);
            activeProfile.activeWheelIndex = 0;
        }
        if (activeProfile.activeWheelIndex >= activeProfile.wheels.size()) {
            activeProfile.activeWheelIndex = 0;
        }
        if (activeProfile.wheels[activeProfile.activeWheelIndex].slots.empty()) {
            for (size_t i = 0; i < 4; ++i) {
                Slot s;
                activeProfile.wheels[activeProfile.activeWheelIndex].slots.push_back(s);
            }
        }

        auto activeActor = ResolveActiveTargetActor();
        auto equippedEntries = GetEquippedInventoryEntries(activeActor);

        auto* wardrobe = GetWardrobeContainer();
        RE::TESObjectREFR::InventoryItemMap wardrobeInv;
        if (wardrobe) {
            wardrobeInv = wardrobe->GetInventory();
        }
        RE::TESObjectREFR::InventoryItemMap activeActorInv;
        if (activeActor) {
            activeActorInv = activeActor->GetInventory();
        }

        const auto& activeWheel = activeProfile.wheels[activeProfile.activeWheelIndex];
        RE::FormID canonicalTargetID = (g_manager.activeTargetActorID == 0x00000014 || g_manager.activeTargetActorID == 0) ? 0x00000007 : g_manager.activeTargetActorID;

        bool isGpConnected = CheckGamepadConnected();

        json data;
        data["editMode"] = isEditMode;
        data["wheelIndex"] = activeProfile.activeWheelIndex;
        data["wheelName"] = SanitizeUtf8(activeWheel.name);
        data["totalWheels"] = activeProfile.wheels.size();
        data["wheelScale"] = g_wheelScale;
        data["wheelOffsetX"] = g_wheelOffsetX;
        data["wheelOffsetY"] = g_wheelOffsetY;
        data["uiScale"] = g_uiScale;
        data["editUiMode"] = g_editUiMode;
        data["editKey"] = g_editKey;
        data["editModifierKey"] = g_editModifierKey;
        data["playKey"] = g_playKey;
        data["playModifierKey"] = g_playModifierKey;
        data["activeTargetActorId"] = FormatFormID(canonicalTargetID);

        data["gamepadPlayKey"] = g_gamepadPlayKey;
        data["gamepadPlayModifier"] = g_gamepadPlayModifier;
        data["thumbstickDeadzone"] = g_thumbstickDeadzone;
        data["gamepadConnected"] = isGpConnected;
        data["autoCloseOnEquip"] = g_autoCloseOnEquip.load(std::memory_order_relaxed);
        data["autoStashEnabled"] = g_autoStashEnabled.load(std::memory_order_relaxed);
        data["autoSystemEnabled"] = g_autoSystemEnabled.load(std::memory_order_relaxed);
        data["autoCityEnabled"] = g_autoCityEnabled.load(std::memory_order_relaxed);
        data["autoHomeEnabled"] = g_autoHomeEnabled.load(std::memory_order_relaxed);
        data["autoCombatEnabled"] = g_autoCombatEnabled.load(std::memory_order_relaxed);
        data["autoSwimEnabled"] = g_autoSwimEnabled.load(std::memory_order_relaxed);

        if (isEditMode) {
            auto player = RE::PlayerCharacter::GetSingleton();
            data["followers"] = GetDynamicFollowersList();

            json actorInvs = json::object();
            actorInvs["0x00000007"] = GetActorApparelInventoryJson(player, "0x00000007");

            auto processLists = RE::ProcessLists::GetSingleton();
            auto* npcKw = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("ActorTypeNPC");

            if (processLists) {
                for (auto& handle : processLists->highActorHandles) {
                    auto actor = handle.get();
                    if (!actor || actor.get() == player || !actor->IsPlayerTeammate() || actor->IsDead() || !actor->GetActorBase()) continue;
                    auto* race = actor->GetRace();
                    if (!race || (npcKw && !race->HasKeyword(npcKw))) continue;

                    std::string fIDStr = FormatFormID(actor->GetFormID());
                    actorInvs[fIDStr] = GetActorApparelInventoryJson(actor.get(), fIDStr);
                }
            }

            data["actorInventories"] = actorInvs;
            data["playerInventory"] = actorInvs["0x00000007"];

            std::string activeActorIDStr = FormatFormID(activeActor ? activeActor->GetFormID() : 0x00000007);
            if (activeActor && activeActor != player) {
                if (actorInvs.contains(activeActorIDStr)) {
                    data["followerInventory"] = actorInvs[activeActorIDStr];
                } else {
                    data["followerInventory"] = GetActorApparelInventoryJson(activeActor, activeActorIDStr);
                }
            } else {
                data["followerInventory"] = json::array();
            }

            data["cloudInventory"] = GetCloudWardrobeInventoryJson();
            data["inventory"] = data["playerInventory"];
        } else {
            data["followers"] = json::array();
            data["actorInventories"] = json::object();
            data["playerInventory"] = json::array();
            data["followerInventory"] = json::array();
            data["cloudInventory"] = json::array();
            data["inventory"] = json::array();
        }

        json slotsJson = json::array();
        size_t slotIdx = 0;
        for (const auto& slot : activeWheel.slots) {
            json sJson;
            sJson["index"] = slotIdx;
            bool hasRealOutfit = slot.outfit.has_value() && (!slot.outfit->items.empty() || !slot.outfit->itemIDs.empty() || !slot.outfit->name.empty());
            bool isAddon = slot.outfit.has_value() ? slot.outfit->isAddon : false;

            sJson["hasOutfit"] = hasRealOutfit;
            sJson["name"] = slot.outfit.has_value() ? SanitizeUtf8(slot.outfit->name) : ("Outfit " + std::to_string(slotIdx + 1));
            sJson["count"] = slot.outfit.has_value() ? (!slot.outfit->items.empty() ? slot.outfit->items.size() : slot.outfit->itemIDs.size()) : 0;
            sJson["icon"] = slot.outfit.has_value() ? slot.outfit->icon : "";
            sJson["contextTag"] = slot.outfit.has_value() ? SanitizeUtf8(slot.outfit->contextTag) : "";
            sJson["preferenceStars"] = slot.outfit.has_value() ? slot.outfit->preferenceStars : 3;
            sJson["controlState"] = slot.outfit.has_value() ? static_cast<uint8_t>(slot.outfit->controlState) : 0;
            sJson["isAddon"] = isAddon;

            json itemObjects = json::array();
            if (slot.outfit.has_value()) {
                if (!slot.outfit->items.empty()) {
                    for (const auto& itm : slot.outfit->items) {
                        auto* f = RE::TESForm::LookupByID(itm.formID);
                        if (f && IsPureApparel(f)) {
                            auto* arm = f->As<RE::TESObjectARMO>();
                            std::string tStr = GetApparelTypeString(arm);
                            std::string dName = (!itm.customName.empty() && itm.customName != "Apparel") ? itm.customName : (f->GetName() ? f->GetName() : "Apparel");
                            std::string cName = (itm.customName == "Apparel") ? "" : itm.customName;

                            std::string thumbPath = "";
                            if (auto* bound = f->As<RE::TESBoundObject>()) {
                                OutfitWheeler::Thumbnail::ItemThumbnailService::GetSingleton().QueueItem(bound, true);
                                auto* bFile = bound->GetFile(0);
                                std::string bMod = bFile ? std::string(bFile->GetFilename()) : "Skyrim.esm";
                                uint32_t bLoc = bound->GetLocalFormID();
                                if (OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().HasThumbnail(bMod, bLoc)) {
                                    thumbPath = OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().GetRelativePath(bMod, bLoc);
                                }
                            }

                            itemObjects.push_back(json{
                                {"id", FormatFormID(itm.formID)},
                                {"rawID", static_cast<uint32_t>(itm.formID)},
                                {"uniqueID", itm.uniqueID},
                                {"enchantmentID", static_cast<uint32_t>(itm.enchantmentID)},
                                {"customName", SanitizeUtf8(cName)},
                                {"name", SanitizeUtf8(dName)},
                                {"type", tStr},
                                {"thumbnail", thumbPath}
                            });
                        } else if (!itm.pluginName.empty() || itm.localFormID != 0) {
                            std::string dName = (!itm.customName.empty() && itm.customName != "Apparel") ? itm.customName : (!itm.pluginName.empty() ? ("Apparel (" + itm.pluginName + ")") : "Apparel");
                            std::string cName = (itm.customName == "Apparel") ? "" : itm.customName;

                            itemObjects.push_back(json{
                                {"id", "0x00000000"},
                                {"rawID", 0},
                                {"uniqueID", itm.uniqueID},
                                {"enchantmentID", static_cast<uint32_t>(itm.enchantmentID)},
                                {"customName", SanitizeUtf8(cName)},
                                {"name", SanitizeUtf8(dName)},
                                {"type", "Armor"},
                                {"isMissingMod", true},
                                {"plugin", itm.pluginName},
                                {"thumbnail", ""}
                            });
                        }
                    }
                } else {
                    for (auto fID : slot.outfit->itemIDs) {
                        auto* f = RE::TESForm::LookupByID(fID);
                        if (f && IsPureApparel(f)) {
                            auto* arm = f->As<RE::TESObjectARMO>();
                            std::string tStr = GetApparelTypeString(arm);

                            std::string thumbPath = "";
                            if (auto* bound = f->As<RE::TESBoundObject>()) {
                                OutfitWheeler::Thumbnail::ItemThumbnailService::GetSingleton().QueueItem(bound, true);
                                auto* bFile = bound->GetFile(0);
                                std::string bMod = bFile ? std::string(bFile->GetFilename()) : "Skyrim.esm";
                                uint32_t bLoc = bound->GetLocalFormID();
                                if (OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().HasThumbnail(bMod, bLoc)) {
                                    thumbPath = OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().GetRelativePath(bMod, bLoc);
                                }
                            }

                            itemObjects.push_back(json{
                                {"id", FormatFormID(fID)},
                                {"rawID", static_cast<uint32_t>(fID)},
                                {"name", SanitizeUtf8(f->GetName() ? f->GetName() : "Apparel")},
                                {"type", tStr},
                                {"thumbnail", thumbPath}
                            });
                        }
                    }
                }
            }
            sJson["items"] = itemObjects;

            bool isWorn = false;
            if (slot.outfit.has_value() && (!slot.outfit->itemIDs.empty() || !slot.outfit->items.empty())) {
                size_t matches = 0;
                const auto& targetItems = !slot.outfit->items.empty() ? slot.outfit->items : std::vector<OutfitItem>();

                if (!targetItems.empty()) {
                    size_t validCount = 0;
                    for (const auto& itm : targetItems) {
                        if (itm.formID == 0) continue;
                        validCount++;
                        for (const auto& entry : equippedEntries) {
                            if (MatchesWornEntry(entry, itm)) {
                                matches++;
                                break;
                            }
                        }
                    }
                    isWorn = (validCount > 0 && matches == validCount);
                } else {
                    size_t validCount = 0;
                    for (RE::FormID fID : slot.outfit->itemIDs) {
                        if (fID == 0) continue;
                        validCount++;
                        for (const auto& entry : equippedEntries) {
                            if (entry.object && entry.object->GetFormID() == fID) {
                                matches++;
                                break;
                            }
                        }
                    }
                    isWorn = (validCount > 0 && matches == validCount);
                }

                if (!isWorn && !isAddon) {
                    std::string currentActive = AdaptiveWardrobeManager::GetSingleton()->GetCurrentOutfitID(canonicalTargetID);
                    std::string thisSlotID = std::to_string(activeProfile.activeWheelIndex) + "_" + std::to_string(slotIdx);
                    if (!currentActive.empty() && currentActive == thisSlotID && matches > 0) {
                        isWorn = true;
                    }
                }
            }

            sJson["isWorn"] = isWorn;
            sJson["isEquipped"] = isWorn;

            std::vector<RE::FormID> outfitItemIDs;
            if (slot.outfit.has_value()) {
                if (!slot.outfit->items.empty()) {
                    for (const auto& itm : slot.outfit->items) {
                        if (itm.formID != 0) outfitItemIDs.push_back(itm.formID);
                    }
                } else {
                    for (RE::FormID fID : slot.outfit->itemIDs) {
                        if (fID != 0) outfitItemIDs.push_back(fID);
                    }
                }
            }

            bool inActorInv = false;
            size_t foundInActor = 0;
            if (activeActor && !outfitItemIDs.empty()) {
                for (RE::FormID fID : outfitItemIDs) {
                    auto* form = RE::TESForm::LookupByID(fID);
                    auto* bound = form ? form->As<RE::TESBoundObject>() : nullptr;
                    if (bound) {
                        auto it = activeActorInv.find(bound);
                        if (it != activeActorInv.end() && it->second.first > 0) {
                            foundInActor++;
                        }
                    }
                }
                if (foundInActor == outfitItemIDs.size()) {
                    inActorInv = true;
                }
            }

            bool inWardrobe = false;
            if (!outfitItemIDs.empty() && wardrobe) {
                size_t foundInWardrobe = 0;
                for (RE::FormID fID : outfitItemIDs) {
                    auto* form = RE::TESForm::LookupByID(fID);
                    auto* bound = form ? form->As<RE::TESBoundObject>() : nullptr;
                    if (bound) {
                        auto it = wardrobeInv.find(bound);
                        if (it != wardrobeInv.end() && it->second.first > 0) {
                            foundInWardrobe++;
                        }
                    }
                }
                if (foundInWardrobe > 0 && (foundInWardrobe == outfitItemIDs.size() || foundInWardrobe >= foundInActor)) {
                    inWardrobe = true;
                }
            }
            sJson["isInWardrobe"] = inWardrobe;

            slotsJson.push_back(sJson);
            slotIdx++;
        }
        data["slots"] = slotsJson;
        return data.dump(-1, ' ', false, json::error_handler_t::replace);
    } catch (...) {
        return "{}";
    }
}

std::string GetWheelJson(bool isEditMode) {
    std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
    return GetWheelJsonInternal(isEditMode);
}