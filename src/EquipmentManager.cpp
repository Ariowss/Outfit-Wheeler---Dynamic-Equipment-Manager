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
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <mutex>

static std::mutex g_weightMutex;
static std::unordered_map<RE::FormID, float> g_originalWeights;
static uint64_t g_lastEquipTimestamp = 0;

bool IsPureApparel(RE::TESForm* a_form) noexcept {
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
    equipped.reserve(16);

    for (const auto& [item, data] : inventory) {
        if (item && data.first > 0 && IsPureApparel(item) && data.second) {
            auto* entryData = data.second.get();
            if (entryData && entryData->extraLists) {
                for (auto* extraList : *entryData->extraLists) {
                    if (extraList && (extraList->HasType(RE::ExtraDataType::kWorn) || extraList->HasType(RE::ExtraDataType::kWornLeft))) {
                        InventoryWornEntry entry;
                        entry.object = item;
                        entry.extraList = extraList;
                        equipped.push_back(entry);
                    }
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

    std::vector<RE::FormID> targetList;
    targetList.reserve(targetItemIDs.size());
    for (RE::FormID fID : targetItemIDs) {
        auto* form = RE::TESForm::LookupByID(fID);
        if (form && IsPureApparel(form)) {
            targetList.push_back(fID);
        }
    }

    auto currentlyWornEntries = GetEquippedInventoryEntries();
    std::vector<RE::FormID> currentlyWornList;
    currentlyWornList.reserve(currentlyWornEntries.size());

    for (const auto& entry : currentlyWornEntries) {
        if (entry.object && IsPureApparel(entry.object)) {
            currentlyWornList.push_back(entry.object->GetFormID());
        }
    }

    for (const auto& entry : currentlyWornEntries) {
        if (entry.object && IsPureApparel(entry.object)) {
            RE::FormID formID = entry.object->GetFormID();
            if (std::find(targetList.begin(), targetList.end(), formID) == targetList.end()) {
                equipManager->UnequipObject(player, entry.object, entry.extraList, 1, nullptr, false, false, false, true);
                if (g_autoStashEnabled) {
                    StashItemToGhost(entry.object);
                }
            }
        }
    }

    for (RE::FormID formID : targetList) {
        auto* form = RE::TESForm::LookupByID(formID);
        if (form && IsPureApparel(form)) {
            auto* boundObj = form->As<RE::TESBoundObject>();
            if (boundObj) {
                RetrieveItemFromGhost(boundObj);
            }
        }
    }

    auto freshInventory = player->GetInventory();
    for (RE::FormID formID : targetList) {
        if (std::find(currentlyWornList.begin(), currentlyWornList.end(), formID) == currentlyWornList.end()) {
            auto* form = RE::TESForm::LookupByID(formID);
            if (!form || !IsPureApparel(form)) continue;

            auto* boundObj = form->As<RE::TESBoundObject>();
            if (!boundObj) continue;

            auto it = freshInventory.find(boundObj);
            if (it != freshInventory.end() && it->second.first > 0) {
                equipManager->EquipObject(player, boundObj, nullptr, 1, nullptr, false, false, false, true);
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

    std::string savedName = "Outfit";
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        if (wheelIdx >= g_manager.wheels.size()) return;
        if (slotIdx >= g_manager.wheels[wheelIdx].slots.size()) return;

        Outfit outfit;
        if (g_manager.wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
            outfit = *g_manager.wheels[wheelIdx].slots[slotIdx].outfit;
        } else {
            outfit.name = "Outfit " + std::to_string(slotIdx + 1);
            outfit.contextTag = "";
            outfit.preferenceStars = 3;
            outfit.controlState = ControlState::Normal;
            outfit.usageCount = 0;
            outfit.lastWornTimestamp = 0;
        }

        outfit.itemIDs.clear();
        outfit.itemIDs.reserve(equipped.size());
        for (const auto& entry : equipped) {
            if (entry.object && IsPureApparel(entry.object)) {
                outfit.itemIDs.push_back(entry.object->GetFormID());
            }
        }
        g_manager.wheels[wheelIdx].slots[slotIdx].outfit = outfit;
        savedName = outfit.name;
        SaveDataInternal();
    }

    if (g_autoStashEnabled) {
        StashAllOutfitsToWardrobe();
    }

    RE::DebugNotification(("Saved " + savedName).c_str());
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
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
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
    sanitizedTargetIDs.reserve(targetOutfit.itemIDs.size());
    for (RE::FormID fID : targetOutfit.itemIDs) {
        auto* form = RE::TESForm::LookupByID(fID);
        if (form && IsPureApparel(form)) {
            sanitizedTargetIDs.push_back(fID);
        }
    }

    auto currentlyWornEntries = GetEquippedInventoryEntries();
    size_t matchCount = 0;

    for (const auto& entry : currentlyWornEntries) {
        if (entry.object && IsPureApparel(entry.object)) {
            RE::FormID formID = entry.object->GetFormID();
            if (std::find(sanitizedTargetIDs.begin(), sanitizedTargetIDs.end(), formID) != sanitizedTargetIDs.end()) {
                matchCount++;
            }
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
                if (std::find(sanitizedTargetIDs.begin(), sanitizedTargetIDs.end(), formID) != sanitizedTargetIDs.end()) {
                    equipManager->UnequipObject(player, entry.object, entry.extraList, 1, nullptr, false, false, false, true);
                    if (g_autoStashEnabled) {
                        StashItemToGhost(entry.object);
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
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
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

    std::vector<RE::FormID> allRegisteredIDs;
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        for (const auto& wheel : g_manager.wheels) {
            for (const auto& slot : wheel.slots) {
                if (slot.outfit.has_value()) {
                    for (RE::FormID fID : slot.outfit->itemIDs) {
                        allRegisteredIDs.push_back(fID);
                    }
                }
            }
        }
    }

    auto currentlyWorn = GetEquippedInventoryEntries();
    std::vector<RE::FormID> currentlyWornIDs;
    currentlyWornIDs.reserve(currentlyWorn.size());
    for (const auto& entry : currentlyWorn) {
        if (entry.object) {
            currentlyWornIDs.push_back(entry.object->GetFormID());
        }
    }

    for (RE::FormID fID : allRegisteredIDs) {
        if (std::find(currentlyWornIDs.begin(), currentlyWornIDs.end(), fID) == currentlyWornIDs.end()) {
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
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
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
            if (std::find(slotItems.begin(), slotItems.end(), formID) != slotItems.end()) {
                equipManager->UnequipObject(player, entry.object, entry.extraList, 1, nullptr, false, false, false, true);
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
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
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

    std::vector<RE::FormID> wheelItemIDs;
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        if (wheelIdx < g_manager.wheels.size()) {
            for (const auto& slot : g_manager.wheels[wheelIdx].slots) {
                if (slot.outfit.has_value()) {
                    for (RE::FormID fID : slot.outfit->itemIDs) {
                        wheelItemIDs.push_back(fID);
                    }
                }
            }
        }
    }
    if (wheelItemIDs.empty()) return;

    auto currentlyWorn = GetEquippedInventoryEntries();
    std::vector<RE::FormID> currentlyWornIDs;
    currentlyWornIDs.reserve(currentlyWorn.size());
    for (const auto& entry : currentlyWorn) {
        if (entry.object && IsPureApparel(entry.object)) {
            currentlyWornIDs.push_back(entry.object->GetFormID());
        }
    }

    for (RE::FormID fID : wheelItemIDs) {
        if (std::find(currentlyWornIDs.begin(), currentlyWornIDs.end(), fID) == currentlyWornIDs.end()) {
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

    std::vector<RE::FormID> wheelItemIDs;
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        if (wheelIdx < g_manager.wheels.size()) {
            for (const auto& slot : g_manager.wheels[wheelIdx].slots) {
                if (slot.outfit.has_value()) {
                    for (RE::FormID fID : slot.outfit->itemIDs) {
                        wheelItemIDs.push_back(fID);
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