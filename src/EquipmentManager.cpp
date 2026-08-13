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
#include <SKSE/SKSE.h>
#include <set>

std::vector<InventoryWornEntry> GetEquippedInventoryEntries() {
    std::vector<InventoryWornEntry> equipped;
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return equipped;

    auto inventory = player->GetInventory();
    for (const auto& [item, data] : inventory) {
        if (item && item->IsArmor() && data.second && data.second->extraLists) {
            auto* armor = item->As<RE::TESObjectARMO>();
            if (armor && armor->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kShield)) {
                continue;
            }

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

void PerformOutfitSwap(RE::PlayerCharacter* player, RE::ActorEquipManager* equipManager, const std::vector<RE::FormID>& targetItemIDs) {
    if (!player || !equipManager) return;

    std::set<RE::FormID> targetSet(targetItemIDs.begin(), targetItemIDs.end());
    auto currentlyWornEntries = GetEquippedInventoryEntries();
    std::set<RE::FormID> currentlyWornSet;
    for (const auto& entry : currentlyWornEntries) {
        if (entry.object) {
            currentlyWornSet.insert(entry.object->GetFormID());
        }
    }

    for (const auto& entry : currentlyWornEntries) {
        if (entry.object && entry.object->IsArmor()) {
            RE::FormID formID = entry.object->GetFormID();
            if (targetSet.find(formID) == targetSet.end()) {
                equipManager->UnequipObject(player, entry.object, entry.extraList, 1, nullptr, false, false, false, true);
            }
        }
    }

    auto freshInventory = player->GetInventory();
    for (RE::FormID formID : targetItemIDs) {
        if (currentlyWornSet.find(formID) == currentlyWornSet.end()) {
            auto* form = RE::TESForm::LookupByID(formID);
            auto* boundObj = form ? form->As<RE::TESBoundObject>() : nullptr;
            if (!boundObj || !boundObj->IsArmor()) {
                continue;
            }
            auto it = freshInventory.find(boundObj);
            bool playerOwnsItem = (it != freshInventory.end()) && (it->second.first > 0);
            if (!playerOwnsItem) {
                continue;
            }

            RE::ExtraDataList* targetExtra = nullptr;
            if (it->second.second && it->second.second->extraLists && !it->second.second->extraLists->empty()) {
                targetExtra = it->second.second->extraLists->front();
            }
            equipManager->EquipObject(player, boundObj, targetExtra, 1, nullptr, false, false, false, true);
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
        for (const auto& entry : equipped) {
            if (entry.object && entry.object->IsArmor()) {
                outfit.itemIDs.push_back(entry.object->GetFormID());
            }
        }
        g_manager.wheels[wheelIdx].slots[slotIdx].outfit = outfit;
        SaveDataInternal();
        RE::DebugNotification(("Saved " + outfit.name).c_str());
    }
}

void EquipSlotImpl(size_t wheelIdx, size_t slotIdx) {
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

    PerformOutfitSwap(player, equipManager, targetOutfit.itemIDs);

    std::string adaptiveOutfitID = std::to_string(wheelIdx) + "_" + std::to_string(slotIdx);
    AdaptiveWardrobeManager::GetSingleton()->RecordOutfitEquipped(adaptiveOutfitID, 0);
    AdaptiveWardrobeManager::GetSingleton()->PauseAutoSystem(10);

    RE::DebugNotification(("Equipped " + targetOutfit.name).c_str());
}