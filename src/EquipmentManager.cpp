#include "pch.h"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include "EquipmentManager.h"
#include "OutfitManager.h"
#include "AdaptiveWardrobeManager.h"
#include "ExecutionQueue.h"
#include "Settings.h"
#include <windows.h>
#include <SKSE/SKSE.h>
#include <set>
#include <map>
#include <mutex>

static std::mutex g_weightMutex;
static std::map<RE::FormID, float> g_originalWeights;
static uint64_t g_lastEquipTimestamp = 0;

bool IsPureApparel(RE::TESForm* a_form) {
    if (!a_form) return false;
    if (a_form->GetFormType() != RE::FormType::Armor) {
        return false;
    }
    auto* armor = a_form->As<RE::TESObjectARMO>();
    if (!armor) return false;

    if (armor->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kShield)) {
        return false;
    }
    return true;
}

static inline void StashItemToGhost(RE::TESBoundObject* a_obj) {
    if (!a_obj || !IsPureApparel(a_obj)) return;
    constexpr uint32_t kNonPlayable = 0x00000004;
    a_obj->formFlags |= kNonPlayable;
    auto* weightForm = a_obj->As<RE::TESWeightForm>();
    if (weightForm) {
        std::lock_guard<std::mutex> lock(g_weightMutex);
        if (g_originalWeights.find(a_obj->GetFormID()) == g_originalWeights.end() && weightForm->weight > 0.0f) {
            g_originalWeights[a_obj->GetFormID()] = weightForm->weight;
        }
        weightForm->weight = 0.0f;
    }
}

static inline void RetrieveItemFromGhost(RE::TESBoundObject* a_obj) {
    if (!a_obj) return;
    constexpr uint32_t kNonPlayable = 0x00000004;
    a_obj->formFlags &= ~kNonPlayable;
    auto* weightForm = a_obj->As<RE::TESWeightForm>();
    if (weightForm) {
        std::lock_guard<std::mutex> lock(g_weightMutex);
        auto it = g_originalWeights.find(a_obj->GetFormID());
        if (it != g_originalWeights.end()) {
            weightForm->weight = it->second;
        }
    }
}

std::vector<InventoryWornEntry> GetEquippedInventoryEntries() {
    std::vector<InventoryWornEntry> equipped;
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return equipped;

    auto inventory = player->GetInventory();
    for (const auto& [item, data] : inventory) {
        if (item && IsPureApparel(item) && data.second && data.second->extraLists) {
            for (auto* extraList : *data.second->extraLists) {
                if (extraList && (extraList->HasType(RE::ExtraDataType::kWorn) || extraList->HasType(RE::ExtraDataType::kWornLeft))) {
                    InventoryWornEntry entry;
                    entry.object = item;
                    entry.extraList = extraList;
                    equipped.push_back(entry);
                }
            }
        }
    }
    return equipped;
}

RE::TESObjectREFR* GetWardrobeContainer() {
    return nullptr;
}

void PerformOutfitSwap(RE::PlayerCharacter* player, RE::ActorEquipManager* equipManager, const std::vector<RE::FormID>& targetItemIDs) {
    if (!player || !equipManager || !player->Is3DLoaded()) return;

    auto actorState = player->AsActorState();
    if (actorState && (actorState->GetSitSleepState() != RE::SIT_SLEEP_STATE::kNormal || player->IsInKillMove())) {
        return;
    }

    std::set<RE::FormID> targetSet;
    for (RE::FormID fID : targetItemIDs) {
        auto* form = RE::TESForm::LookupByID(fID);
        if (form && IsPureApparel(form)) {
            targetSet.insert(fID);
        }
    }

    auto currentlyWornEntries = GetEquippedInventoryEntries();
    std::set<RE::FormID> currentlyWornSet;

    for (const auto& entry : currentlyWornEntries) {
        if (entry.object && IsPureApparel(entry.object)) {
            currentlyWornSet.insert(entry.object->GetFormID());
        }
    }

    for (const auto& entry : currentlyWornEntries) {
        if (entry.object && IsPureApparel(entry.object)) {
            RE::FormID formID = entry.object->GetFormID();
            if (targetSet.find(formID) == targetSet.end()) {
                equipManager->UnequipObject(player, entry.object, entry.extraList, 1, nullptr, false, false, false, true);
                if (g_autoStashEnabled) {
                    StashItemToGhost(entry.object);
                }
            }
        }
    }

    for (RE::FormID formID : targetSet) {
        auto* form = RE::TESForm::LookupByID(formID);
        if (form && IsPureApparel(form)) {
            auto* boundObj = form->As<RE::TESBoundObject>();
            if (boundObj) {
                RetrieveItemFromGhost(boundObj);
            }
        }
    }

    auto freshInventory = player->GetInventory();
    for (RE::FormID formID : targetSet) {
        if (currentlyWornSet.find(formID) == currentlyWornSet.end()) {
            auto* form = RE::TESForm::LookupByID(formID);
            if (!form || !IsPureApparel(form)) continue;

            auto* boundObj = form->As<RE::TESBoundObject>();
            if (!boundObj) continue;

            auto it = freshInventory.find(boundObj);
            if (it != freshInventory.end() && it->second.first > 0) {
                RE::ExtraDataList* targetExtra = nullptr;
                if (it->second.second && it->second.second->extraLists && !it->second.second->extraLists->empty()) {
                    targetExtra = it->second.second->extraLists->front();
                }
                equipManager->EquipObject(player, boundObj, targetExtra, 1, nullptr, false, false, false, true);
            }
        }
    }
}

void SaveOutfitToSlotImpl(size_t wheelIdx, size_t slotIdx) {
    auto equipped = GetEquippedInventoryEntries();
    if (equipped.empty()) {
        RE::DebugNotification("No equipped armor/clothing to save!");
        return;
    }

    std::lock_guard<std::mutex> lock(g_dataMutex);
    if (wheelIdx < g_manager.wheels.size() && slotIdx < g_manager.wheels[wheelIdx].slots.size()) {
        Outfit outfit;
        if (g_manager.wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
            outfit.name = g_manager.wheels[wheelIdx].slots[slotIdx].outfit->name;
            outfit.contextTag = g_manager.wheels[wheelIdx].slots[slotIdx].outfit->contextTag;
            outfit.preferenceStars = g_manager.wheels[wheelIdx].slots[slotIdx].outfit->preferenceStars;
            outfit.controlState = g_manager.wheels[wheelIdx].slots[slotIdx].outfit->controlState;
            outfit.usageCount = g_manager.wheels[wheelIdx].slots[slotIdx].outfit->usageCount;
            outfit.lastWornTimestamp = g_manager.wheels[wheelIdx].slots[slotIdx].outfit->lastWornTimestamp;
        } else {
            outfit.name = "Outfit " + std::to_string(slotIdx + 1);
            outfit.contextTag = "";
            outfit.preferenceStars = 3;
            outfit.controlState = ControlState::Normal;
            outfit.usageCount = 0;
            outfit.lastWornTimestamp = 0;
        }

        outfit.itemIDs.clear();
        for (const auto& entry : equipped) {
            if (entry.object && IsPureApparel(entry.object)) {
                outfit.itemIDs.push_back(entry.object->GetFormID());
            }
        }
        g_manager.wheels[wheelIdx].slots[slotIdx].outfit = outfit;
        SaveDataInternal();

        if (g_autoStashEnabled) {
            StashAllOutfitsToWardrobe();
        }

        RE::DebugNotification(("Saved " + outfit.name).c_str());
    }
}

void EquipSlotImpl(size_t wheelIdx, size_t slotIdx) {
    uint64_t now = GetTickCount64();
    if (now - g_lastEquipTimestamp < 350) {
        return;
    }
    g_lastEquipTimestamp = now;

    Outfit targetOutfit;
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        if (wheelIdx < g_manager.wheels.size() && slotIdx < g_manager.wheels[wheelIdx].slots.size()) {
            if (g_manager.wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
                targetOutfit = *g_manager.wheels[wheelIdx].slots[slotIdx].outfit;
                found = true;
            }
        }
    }

    if (!found) return;

    auto player = RE::PlayerCharacter::GetSingleton();
    auto equipManager = RE::ActorEquipManager::GetSingleton();
    if (!player || !equipManager) return;

    std::vector<RE::FormID> sanitizedTargetIDs;
    for (RE::FormID fID : targetOutfit.itemIDs) {
        auto* form = RE::TESForm::LookupByID(fID);
        if (form && IsPureApparel(form)) {
            sanitizedTargetIDs.push_back(fID);
        }
    }

    auto currentlyWornEntries = GetEquippedInventoryEntries();
    std::set<RE::FormID> currentlyWornSet;
    for (const auto& entry : currentlyWornEntries) {
        if (entry.object && IsPureApparel(entry.object)) {
            currentlyWornSet.insert(entry.object->GetFormID());
        }
    }

    size_t matchCount = 0;
    for (RE::FormID fID : sanitizedTargetIDs) {
        if (currentlyWornSet.find(fID) != currentlyWornSet.end()) {
            matchCount++;
        }
    }

    bool isAlreadyWorn = false;
    if (!sanitizedTargetIDs.empty()) {
        float ratio = static_cast<float>(matchCount) / static_cast<float>(sanitizedTargetIDs.size());
        if (ratio >= 0.5f || matchCount == sanitizedTargetIDs.size()) {
            isAlreadyWorn = true;
        }
    }

    if (isAlreadyWorn) {
        for (const auto& entry : currentlyWornEntries) {
            if (entry.object && IsPureApparel(entry.object)) {
                RE::FormID formID = entry.object->GetFormID();
                for (RE::FormID tID : sanitizedTargetIDs) {
                    if (formID == tID) {
                        equipManager->UnequipObject(player, entry.object, entry.extraList, 1, nullptr, false, false, false, true);
                        if (g_autoStashEnabled) {
                            StashItemToGhost(entry.object);
                        }
                        break;
                    }
                }
            }
        }
        AdaptiveWardrobeManager::GetSingleton()->SetCurrentOutfitID("");
        RE::DebugNotification(("Unequipped " + targetOutfit.name).c_str());
        return;
    }

    PerformOutfitSwap(player, equipManager, sanitizedTargetIDs);

    std::string adaptiveOutfitID = std::to_string(wheelIdx) + "_" + std::to_string(slotIdx);
    AdaptiveWardrobeManager::GetSingleton()->RecordOutfitEquipped(adaptiveOutfitID, 0);

    RE::DebugNotification(("Equipped " + targetOutfit.name).c_str());
}

void RetrieveAllOutfitsToPlayer() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        for (const auto& wheel : g_manager.wheels) {
            for (const auto& slot : wheel.slots) {
                if (slot.outfit.has_value()) {
                    for (RE::FormID fID : slot.outfit->itemIDs) {
                        auto* form = RE::TESForm::LookupByID(fID);
                        if (form && IsPureApparel(form)) {
                            auto* boundObj = form->As<RE::TESBoundObject>();
                            if (boundObj) {
                                RetrieveItemFromGhost(boundObj);
                            }
                        }
                    }
                }
            }
        }
    }

    RE::DebugNotification("All outfits visible in inventory with normal weight!");
}

void StashAllOutfitsToWardrobe() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    std::set<RE::FormID> allRegisteredIDs;
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        for (const auto& wheel : g_manager.wheels) {
            for (const auto& slot : wheel.slots) {
                if (slot.outfit.has_value()) {
                    for (RE::FormID fID : slot.outfit->itemIDs) {
                        auto* form = RE::TESForm::LookupByID(fID);
                        if (form && IsPureApparel(form)) {
                            allRegisteredIDs.insert(fID);
                        }
                    }
                }
            }
        }
    }

    auto currentlyWorn = GetEquippedInventoryEntries();
    std::set<RE::FormID> currentlyWornIDs;
    for (const auto& entry : currentlyWorn) {
        if (entry.object && IsPureApparel(entry.object)) {
            currentlyWornIDs.insert(entry.object->GetFormID());
        }
    }

    for (RE::FormID fID : allRegisteredIDs) {
        if (currentlyWornIDs.find(fID) == currentlyWornIDs.end()) {
            auto* form = RE::TESForm::LookupByID(fID);
            if (form && IsPureApparel(form)) {
                auto* boundObj = form->As<RE::TESBoundObject>();
                if (boundObj) {
                    StashItemToGhost(boundObj);
                }
            }
        }
    }

    RE::DebugNotification("All inactive outfits hidden and weight set to 0!");
}

void StashSlotOutfit(size_t wheelIdx, size_t slotIdx) {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto equipManager = RE::ActorEquipManager::GetSingleton();
    if (!player || !equipManager) return;

    std::vector<RE::FormID> slotItems;
    std::string outfitName = "Outfit";
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        if (wheelIdx < g_manager.wheels.size() && slotIdx < g_manager.wheels[wheelIdx].slots.size()) {
            if (g_manager.wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
                slotItems = g_manager.wheels[wheelIdx].slots[slotIdx].outfit->itemIDs;
                outfitName = g_manager.wheels[wheelIdx].slots[slotIdx].outfit->name;
            }
        }
    }
    if (slotItems.empty()) return;

    auto currentlyWorn = GetEquippedInventoryEntries();
    for (const auto& entry : currentlyWorn) {
        if (entry.object && IsPureApparel(entry.object)) {
            RE::FormID formID = entry.object->GetFormID();
            for (RE::FormID targetID : slotItems) {
                if (formID == targetID) {
                    equipManager->UnequipObject(player, entry.object, entry.extraList, 1, nullptr, false, false, false, true);
                    break;
                }
            }
        }
    }

    for (RE::FormID fID : slotItems) {
        auto* form = RE::TESForm::LookupByID(fID);
        if (form && IsPureApparel(form)) {
            auto* bound = form->As<RE::TESBoundObject>();
            if (bound) {
                StashItemToGhost(bound);
            }
        }
    }

    RE::DebugNotification(("Stored " + outfitName + " in wardrobe (Hidden & Weightless)!").c_str());
}

void RetrieveSlotOutfit(size_t wheelIdx, size_t slotIdx) {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    std::vector<RE::FormID> slotItems;
    std::string outfitName = "Outfit";
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        if (wheelIdx < g_manager.wheels.size() && slotIdx < g_manager.wheels[wheelIdx].slots.size()) {
            if (g_manager.wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
                slotItems = g_manager.wheels[wheelIdx].slots[slotIdx].outfit->itemIDs;
                outfitName = g_manager.wheels[wheelIdx].slots[slotIdx].outfit->name;
            }
        }
    }
    if (slotItems.empty()) return;

    for (RE::FormID fID : slotItems) {
        auto* form = RE::TESForm::LookupByID(fID);
        if (form && IsPureApparel(form)) {
            auto* bound = form->As<RE::TESBoundObject>();
            if (bound) {
                RetrieveItemFromGhost(bound);
            }
        }
    }

    RE::DebugNotification(("Retrieved " + outfitName + " to inventory!").c_str());
}

void StashWheelOutfits(size_t wheelIdx) {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto equipManager = RE::ActorEquipManager::GetSingleton();
    if (!player || !equipManager) return;

    std::set<RE::FormID> wheelItemIDs;
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        if (wheelIdx < g_manager.wheels.size()) {
            for (const auto& slot : g_manager.wheels[wheelIdx].slots) {
                if (slot.outfit.has_value()) {
                    for (RE::FormID fID : slot.outfit->itemIDs) {
                        auto* form = RE::TESForm::LookupByID(fID);
                        if (form && IsPureApparel(form)) {
                            wheelItemIDs.insert(fID);
                        }
                    }
                }
            }
        }
    }
    if (wheelItemIDs.empty()) return;

    auto currentlyWorn = GetEquippedInventoryEntries();
    std::set<RE::FormID> currentlyWornIDs;
    for (const auto& entry : currentlyWorn) {
        if (entry.object && IsPureApparel(entry.object)) {
            currentlyWornIDs.insert(entry.object->GetFormID());
        }
    }

    for (RE::FormID fID : wheelItemIDs) {
        if (currentlyWornIDs.find(fID) == currentlyWornIDs.end()) {
            auto* form = RE::TESForm::LookupByID(fID);
            if (form && IsPureApparel(form)) {
                auto* boundObj = form->As<RE::TESBoundObject>();
                if (boundObj) {
                    StashItemToGhost(boundObj);
                }
            }
        }
    }

    RE::DebugNotification("Wheel outfits stored in wardrobe (Hidden & Weightless)!");
}

void RetrieveWheelOutfits(size_t wheelIdx) {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    std::set<RE::FormID> wheelItemIDs;
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        if (wheelIdx < g_manager.wheels.size()) {
            for (const auto& slot : g_manager.wheels[wheelIdx].slots) {
                if (slot.outfit.has_value()) {
                    for (RE::FormID fID : slot.outfit->itemIDs) {
                        auto* form = RE::TESForm::LookupByID(fID);
                        if (form && IsPureApparel(form)) {
                            wheelItemIDs.insert(fID);
                        }
                    }
                }
            }
        }
    }
    if (wheelItemIDs.empty()) return;

    for (RE::FormID fID : wheelItemIDs) {
        auto* form = RE::TESForm::LookupByID(fID);
        if (form && IsPureApparel(form)) {
            auto* bound = form->As<RE::TESBoundObject>();
            if (bound) {
                RetrieveItemFromGhost(bound);
            }
        }
    }

    RE::DebugNotification("Retrieved wheel outfits to inventory!");
}