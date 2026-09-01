#include "pch.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

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
#include "Settings.h"
#include <windows.h>
#include <xinput.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <set>
#include <unordered_set>
#include <SKSE/SKSE.h>

OutfitManager g_manager;
std::recursive_mutex g_dataMutex;

static const std::string g_savePath = "Data/SKSE/Plugins/PrismaOutfitWheel.json";
static const std::string g_fallbackSavePath1 = "Data/SKSE/Plugins/OutfitWheeler.json";
static const std::string g_fallbackSavePath2 = "Data/SKSE/Plugins/Wheeler.json";

static std::string ToLowerString(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return str;
}

static std::string SanitizeUtf8(const std::string& str) {
    if (str.empty()) return "Apparel";
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
        } else {
            out += ' ';
        }
    }
    if (out.empty()) return "Apparel";
    return out;
}

static ContextFlags ParseContextTagToFlags(const std::string& tag) {
    std::string lower = ToLowerString(tag);
    ContextFlags flags = ContextFlags::None;

    if (lower.find("combat") != std::string::npos || lower.find("fight") != std::string::npos) flags |= ContextFlags::Combat;
    if (lower.find("swim") != std::string::npos) flags |= ContextFlags::Swim;
    if (lower.find("in_water") != std::string::npos || lower.find("water") != std::string::npos) flags |= ContextFlags::InWater;
    if (lower.find("sleep") != std::string::npos || lower.find("bed") != std::string::npos) flags |= ContextFlags::Sleep;
    if (lower.find("mount") != std::string::npos || lower.find("horse") != std::string::npos) flags |= ContextFlags::Mount;

    if (lower.find("dungeon") != std::string::npos || lower.find("tomb") != std::string::npos || lower.find("ruin") != std::string::npos) flags |= ContextFlags::Dungeon;
    if (lower.find("home") != std::string::npos || lower.find("house") != std::string::npos) flags |= ContextFlags::PlayerHome;
    if (lower.find("inn") != std::string::npos || lower.find("tavern") != std::string::npos) flags |= ContextFlags::Inn;
    if (lower.find("store") != std::string::npos || lower.find("shop") != std::string::npos || lower.find("market") != std::string::npos) flags |= ContextFlags::Store;
    if (lower.find("guild") != std::string::npos || lower.find("thief") != std::string::npos || lower.find("brotherhood") != std::string::npos) flags |= ContextFlags::GuildHall;
    if (lower.find("castle") != std::string::npos || lower.find("palace") != std::string::npos) flags |= ContextFlags::Castle;
    if (lower.find("temple") != std::string::npos || lower.find("shrine") != std::string::npos || lower.find("talos") != std::string::npos) flags |= ContextFlags::Temple;
    if (lower.find("farm") != std::string::npos || lower.find("mill") != std::string::npos) flags |= ContextFlags::Farm;
    if (lower.find("jail") != std::string::npos || lower.find("prison") != std::string::npos) flags |= ContextFlags::Jail;
    if (lower.find("military") != std::string::npos || lower.find("camp") != std::string::npos || lower.find("war") != std::string::npos) flags |= ContextFlags::Military;

    if (lower.find("city") != std::string::npos) flags |= ContextFlags::City;
    if (lower.find("town") != std::string::npos || lower.find("village") != std::string::npos) flags |= ContextFlags::Town;
    if (lower.find("world") != std::string::npos || lower.find("adventure") != std::string::npos || lower.find("mountain") != std::string::npos || lower.find("nature") != std::string::npos) flags |= ContextFlags::World;

    if (lower.find("night") != std::string::npos) flags |= ContextFlags::Night;
    if (lower.find("rain") != std::string::npos) flags |= ContextFlags::Rain;
    if (lower.find("snow") != std::string::npos) flags |= ContextFlags::Snow;
    if (lower.find("interior") != std::string::npos) flags |= ContextFlags::Interior;

    if (flags == ContextFlags::None) {
        flags = ContextFlags::Default;
    }

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
                    ao.preferenceStars = slot.outfit->preferenceStars;
                    ao.controlState = slot.outfit->controlState;
                    ao.dynamicVars.usageCount = slot.outfit->usageCount;
                    ao.dynamicVars.lastWornTimestamp = static_cast<uint32_t>(slot.outfit->lastWornTimestamp);

                    ContextFlags cFlags = ParseContextTagToFlags(slot.outfit->contextTag);
                    SuitabilityRule rule;
                    rule.contextMask = cFlags;
                    rule.score = static_cast<uint8_t>(3 + slot.outfit->preferenceStars);
                    ao.suitabilities.push_back(rule);

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
    auto* form = RE::TESForm::LookupByID(formID);
    if (form) {
        auto* file = form->GetFile(0);
        if (file && file->fileName && file->fileName[0] != 0) {
            std::string modName = file->fileName;
            if (!modName.empty()) {
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
                const RE::TESFile* modFile = nullptr;
                for (const auto* f : dataHandler->files) {
                    if (f && f->fileName && _stricmp(f->fileName, plugin.c_str()) == 0) {
                        modFile = f;
                        break;
                    }
                }

                if (modFile) {
                    RE::FormID resolvedFullID = 0;
                    if (modFile->IsLight()) {
                        resolvedFullID = dataHandler->LookupFormID(localID & 0x00000FFF, modFile->fileName);
                    } else {
                        resolvedFullID = dataHandler->LookupFormID(localID & 0x00FFFFFF, modFile->fileName);
                    }

                    if (resolvedFullID != 0) {
                        auto* f = RE::TESForm::LookupByID(resolvedFullID);
                        if (f) return resolvedFullID;
                    }
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
                sJson["hasOutfit"] = !slot.outfit->itemIDs.empty();
                sJson["name"] = SanitizeUtf8(slot.outfit->name);
                sJson["contextTag"] = SanitizeUtf8(slot.outfit->contextTag);
                sJson["preferenceStars"] = slot.outfit->preferenceStars;
                sJson["controlState"] = static_cast<uint8_t>(slot.outfit->controlState);
                sJson["isAddon"] = slot.outfit->isAddon;
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
                sJson["isAddon"] = false;
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
                    } else {
                        Outfit o;
                        o.name = "Outfit " + std::to_string(sIdx + 1);
                        o.contextTag = "";
                        o.preferenceStars = 3;
                        o.controlState = ControlState::Normal;
                        o.isAddon = false;
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
        prof.wheels.push_back(defaultWheel);
    }
    if (prof.activeWheelIndex >= prof.wheels.size()) {
        prof.activeWheelIndex = 0;
    }
    return prof;
}

void SaveDataInternal() {
    try {
        json root;
        root["activeTargetActorID"] = static_cast<uint32_t>(g_manager.activeTargetActorID);
        root["wardrobeContainerFormID"] = 0;

        json actorsJson = json::object();
        for (const auto& [actorID, profile] : g_manager.actorProfiles) {
            std::stringstream ss;
            ss << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << actorID;

            json actorNode;
            actorNode["profile"] = SerializeProfile(profile);
            actorNode["formMeta"] = SerializeFormID(actorID);

            actorsJson[ss.str()] = actorNode;
        }
        root["actors"] = actorsJson;

        std::filesystem::create_directories("Data/SKSE/Plugins");
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

        g_manager.GetProfile(0x00000007);
        SyncWheelOutfitsToAdaptiveWardrobe();
    } catch (...) {}
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
    std::stringstream pSs;
    pSs << static_cast<int>(pCurWeight) << " / " << static_cast<int>(pMaxWeight);

    std::string pName = (player->GetActorBase() && player->GetActorBase()->GetName()) ? player->GetActorBase()->GetName() : "Player Character";
    arr.push_back(json{
        {"id", "0x00000007"},
        {"name", SanitizeUtf8(pName)},
        {"type", "PLAYER"},
        {"avatar", "D"},
        {"avatarUrl", ""},
        {"weight", pSs.str()}
    });

    auto processLists = RE::ProcessLists::GetSingleton();
    if (processLists) {
        std::set<RE::FormID> addedActors;
        addedActors.insert(0x00000007);
        addedActors.insert(0x00000014);

        for (auto& handle : processLists->highActorHandles) {
            auto actor = handle.get();
            if (!actor || actor.get() == player || actor->IsDead()) continue;

            if (actor->IsPlayerTeammate() || actor->IsPlayerRef()) {
                RE::FormID fID = actor->GetFormID();
                if (addedActors.find(fID) != addedActors.end()) continue;
                addedActors.insert(fID);

                auto* base = actor->GetActorBase();
                if (!base) continue;

                auto* aAvOwner = actor->AsActorValueOwner();
                float curW = aAvOwner ? aAvOwner->GetActorValue(RE::ActorValue::kInventoryWeight) : 0.0f;
                float maxW = aAvOwner ? aAvOwner->GetActorValue(RE::ActorValue::kCarryWeight) : 300.0f;
                std::stringstream ssW;
                ssW << static_cast<int>(curW) << " / " << static_cast<int>(maxW);

                std::stringstream ssID;
                ssID << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << fID;

                std::string aName = base->GetName() ? base->GetName() : "Follower";
                std::string safeAName = SanitizeUtf8(aName);
                std::string av = safeAName.empty() ? "F" : std::string(1, safeAName[0]);

                arr.push_back(json{
                    {"id", ssID.str()},
                    {"name", safeAName},
                    {"type", "FOLLOWER"},
                    {"avatar", av},
                    {"avatarUrl", ""},
                    {"weight", ssW.str()}
                });
            }
        }
    }
    return arr;
}

static std::string GetApparelTypeString(RE::TESObjectARMO* armor) {
    if (!armor) return "Armor";
    if (armor->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kHead) || armor->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kHair) || armor->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kCirclet)) {
        return "Helmet";
    }
    if (armor->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kAmulet)) {
        return "Amulet";
    }
    if (armor->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kRing)) {
        return "Ring";
    }
    if (armor->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kBody)) {
        return "Cuirass";
    }
    if (armor->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kHands)) {
        return "Gauntlets";
    }
    if (armor->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kFeet)) {
        return "Boots";
    }
    if (armor->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kShield)) {
        return "Shield";
    }
    return "Apparel";
}

static json GetActorApparelInventoryJson(RE::Actor* actor, const std::string& sourceTag) {
    json arr = json::array();
    if (!actor || !actor->GetActorBase()) return arr;

    auto inv = actor->GetInventory();
    for (const auto& [item, data] : inv) {
        if (!item || data.first <= 0) continue;
        if (!IsPureApparel(item)) continue;

        auto* armor = item->As<RE::TESObjectARMO>();
        if (!armor) continue;

        std::stringstream ssID;
        ssID << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << item->GetFormID();

        std::string typeStr = GetApparelTypeString(armor);
        std::string catStr = (typeStr == "Ring" || typeStr == "Amulet") ? "JEWELRY" : "ARMORS";

        float armRating = static_cast<float>(armor->GetArmorRating());
        float wt = item->GetWeight();
        int val = item->GetGoldValue();

        std::string rawName = item->GetName() ? item->GetName() : "Apparel";
        std::string safeName = SanitizeUtf8(rawName);

        std::string enchName = "";
        std::vector<std::string> tags;

        if (armor->formEnchanting) {
            enchName = SanitizeUtf8(armor->formEnchanting->GetName() ? armor->formEnchanting->GetName() : "Enchanted");
            tags.push_back("enchanted");
        }

        bool isWorn = false;
        auto* entryData = data.second.get();
        if (entryData && entryData->extraLists) {
            for (auto* xList : *entryData->extraLists) {
                if (xList) {
                    if (xList->HasType(RE::ExtraDataType::kWorn) || xList->HasType(RE::ExtraDataType::kWornLeft)) {
                        isWorn = true;
                    }
                    if (xList->HasType(RE::ExtraDataType::kEnchantment)) {
                        tags.push_back("enchanted");
                    }
                    if (xList->HasType(RE::ExtraDataType::kOwnership)) {
                        tags.push_back("stolen");
                    }
                }
            }
        }

        arr.push_back(json{
            {"id", ssID.str()},
            {"rawID", static_cast<uint32_t>(item->GetFormID())},
            {"name", safeName},
            {"category", catStr},
            {"type", typeStr},
            {"damage", 0},
            {"armor", static_cast<int>(armRating)},
            {"weight", wt},
            {"value", val},
            {"enchantment", enchName},
            {"tags", tags},
            {"equipped", isWorn},
            {"count", data.first},
            {"source", sourceTag}
        });
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

        std::stringstream ssID;
        ssID << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << item->GetFormID();

        std::string typeStr = GetApparelTypeString(armor);
        std::string catStr = (typeStr == "Ring" || typeStr == "Amulet") ? "JEWELRY" : "ARMORS";

        float armRating = static_cast<float>(armor->GetArmorRating());
        float wt = item->GetWeight();
        int val = item->GetGoldValue();

        std::string rawName = item->GetName() ? item->GetName() : "Apparel";
        std::string safeName = SanitizeUtf8(rawName);

        std::string enchName = "";
        std::vector<std::string> tags;

        if (armor->formEnchanting) {
            enchName = SanitizeUtf8(armor->formEnchanting->GetName() ? armor->formEnchanting->GetName() : "Enchanted");
            tags.push_back("enchanted");
        }

        arr.push_back(json{
            {"id", ssID.str()},
            {"rawID", static_cast<uint32_t>(item->GetFormID())},
            {"name", safeName},
            {"category", catStr},
            {"type", typeStr},
            {"damage", 0},
            {"armor", static_cast<int>(armRating)},
            {"weight", wt},
            {"value", val},
            {"enchantment", enchName},
            {"tags", tags},
            {"equipped", false},
            {"count", data.first},
            {"source", "CLOUD"}
        });
    }
    return arr;
}

std::string GetWheelJsonInternal(bool isEditMode) {
    try {
        auto& activeProfile = g_manager.GetActiveProfile();
        if (activeProfile.wheels.empty()) {
            Wheel defaultWheel;
            defaultWheel.name = "Wheel 1";
            activeProfile.wheels.push_back(defaultWheel);
            activeProfile.activeWheelIndex = 0;
        }
        if (activeProfile.activeWheelIndex >= activeProfile.wheels.size()) {
            activeProfile.activeWheelIndex = 0;
        }

        auto activeActor = ResolveActiveTargetActor();
        std::unordered_set<RE::FormID> currentlyWornSet;
        if (activeActor && activeActor->GetActorBase()) {
            auto equippedEntries = GetEquippedInventoryEntries(activeActor);
            currentlyWornSet.reserve(equippedEntries.size());
            for (const auto& entry : equippedEntries) {
                if (entry.object && IsPureApparel(entry.object)) {
                    currentlyWornSet.insert(entry.object->GetFormID());
                }
            }
        }

        auto* wardrobe = GetWardrobeContainer();
        RE::TESObjectREFR::InventoryItemMap wardrobeInv;
        if (wardrobe) {
            wardrobeInv = wardrobe->GetInventory();
        }

        const auto& activeWheel = activeProfile.wheels[activeProfile.activeWheelIndex];
        RE::FormID canonicalTargetID = (g_manager.activeTargetActorID == 0x00000014 || g_manager.activeTargetActorID == 0) ? 0x00000007 : g_manager.activeTargetActorID;
        std::string currentActiveID = AdaptiveWardrobeManager::GetSingleton()->GetCurrentOutfitID(canonicalTargetID);

        bool isGpConnected = CheckGamepadConnected();

        std::stringstream targetIDStream;
        targetIDStream << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << canonicalTargetID;

        json data;
        data["editMode"] = isEditMode;
        data["wheelIndex"] = activeProfile.activeWheelIndex;
        data["wheelName"] = SanitizeUtf8(activeWheel.name);
        data["totalWheels"] = activeProfile.wheels.size();
        data["wheelScale"] = g_wheelScale;
        data["wheelOffsetX"] = g_wheelOffsetX;
        data["wheelOffsetY"] = g_wheelOffsetY;
        data["editKey"] = g_editKey;
        data["editModifierKey"] = g_editModifierKey;
        data["playKey"] = g_playKey;
        data["playModifierKey"] = g_playModifierKey;
        data["activeTargetActorId"] = targetIDStream.str();

        data["gamepadPlayKey"] = g_gamepadPlayKey;
        data["gamepadPlayModifier"] = g_gamepadPlayModifier;
        data["thumbstickDeadzone"] = g_thumbstickDeadzone;
        data["gamepadConnected"] = isGpConnected;

        data["autoStashEnabled"] = g_autoStashEnabled;
        data["autoSystemEnabled"] = g_autoSystemEnabled;
        data["autoCityEnabled"] = g_autoCityEnabled;
        data["autoHomeEnabled"] = g_autoHomeEnabled;
        data["autoCombatEnabled"] = g_autoCombatEnabled;
        data["autoSwimEnabled"] = g_autoSwimEnabled;

        if (isEditMode) {
            auto player = RE::PlayerCharacter::GetSingleton();
            data["followers"] = GetDynamicFollowersList();

            json actorInvs = json::object();
            actorInvs["0x00000007"] = GetActorApparelInventoryJson(player, "PLAYER");

            auto processLists = RE::ProcessLists::GetSingleton();
            if (processLists) {
                for (auto& handle : processLists->highActorHandles) {
                    auto actor = handle.get();
                    if (actor && actor.get() != player && actor->IsPlayerTeammate() && !actor->IsDead() && actor->GetActorBase()) {
                        std::stringstream ssID;
                        ssID << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << actor->GetFormID();
                        actorInvs[ssID.str()] = GetActorApparelInventoryJson(actor.get(), "FOLLOWER");
                    }
                }
            }

            data["actorInventories"] = actorInvs;
            data["playerInventory"] = actorInvs["0x00000007"];
            data["followerInventory"] = (activeActor && activeActor != player) ? GetActorApparelInventoryJson(activeActor, "FOLLOWER") : json::array();
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
            bool hasRealOutfit = slot.outfit.has_value() && !slot.outfit->itemIDs.empty();
            bool isAddon = slot.outfit.has_value() ? slot.outfit->isAddon : false;

            sJson["hasOutfit"] = hasRealOutfit;
            sJson["name"] = slot.outfit.has_value() ? SanitizeUtf8(slot.outfit->name) : ("Outfit " + std::to_string(slotIdx + 1));
            sJson["count"] = slot.outfit.has_value() ? slot.outfit->itemIDs.size() : 0;
            sJson["contextTag"] = slot.outfit.has_value() ? SanitizeUtf8(slot.outfit->contextTag) : "";
            sJson["preferenceStars"] = slot.outfit.has_value() ? slot.outfit->preferenceStars : 3;
            sJson["controlState"] = slot.outfit.has_value() ? static_cast<uint8_t>(slot.outfit->controlState) : 0;
            sJson["isAddon"] = isAddon;

            json itemObjects = json::array();
            if (slot.outfit.has_value()) {
                for (auto fID : slot.outfit->itemIDs) {
                    auto* f = RE::TESForm::LookupByID(fID);
                    if (f && IsPureApparel(f)) {
                        std::stringstream sID;
                        sID << "0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << fID;
                        auto* arm = f->As<RE::TESObjectARMO>();
                        std::string tStr = GetApparelTypeString(arm);

                        itemObjects.push_back(json{
                            {"id", sID.str()},
                            {"rawID", static_cast<uint32_t>(fID)},
                            {"name", SanitizeUtf8(f->GetName() ? f->GetName() : "Apparel")},
                            {"type", tStr}
                        });
                    }
                }
            }
            sJson["items"] = itemObjects;

            std::string thisSlotID = std::to_string(activeProfile.activeWheelIndex) + "_" + std::to_string(slotIdx);
            bool isWorn = false;

            if (slot.outfit.has_value() && !slot.outfit->itemIDs.empty()) {
                size_t matches = 0;
                for (RE::FormID fID : slot.outfit->itemIDs) {
                    if (currentlyWornSet.find(fID) != currentlyWornSet.end()) {
                        matches++;
                    }
                }

                if (isAddon) {
                    isWorn = (matches == slot.outfit->itemIDs.size());
                } else {
                    if (!currentActiveID.empty() && thisSlotID == currentActiveID) {
                        isWorn = true;
                    } else {
                        isWorn = (!currentlyWornSet.empty() && matches == slot.outfit->itemIDs.size() && matches == currentlyWornSet.size());
                    }
                }
            }

            sJson["isWorn"] = isWorn;
            sJson["isEquipped"] = isWorn;

            bool inWardrobe = false;
            if (slot.outfit.has_value() && !slot.outfit->itemIDs.empty() && wardrobe) {
                size_t foundInWardrobe = 0;
                for (RE::FormID fID : slot.outfit->itemIDs) {
                    auto* form = RE::TESForm::LookupByID(fID);
                    auto* bound = form ? form->As<RE::TESBoundObject>() : nullptr;
                    if (bound && wardrobeInv.find(bound) != wardrobeInv.end()) {
                        foundInWardrobe++;
                    }
                }
                if (foundInWardrobe > 0) {
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