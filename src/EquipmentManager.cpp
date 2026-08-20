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
#include <algorithm>
#include <mutex>

static uint64_t g_lastEquipTimestamp = 0;

bool IsPureApparel(RE::TESForm* a_form) noexcept {
    if (!a_form) return false;
    if (a_form->GetFormType() != RE::FormType::Armor) {
        return false;
    }
    auto* armor = a_form->As<RE::TESObjectARMO>();
    if (!armor) return false;

    if ((armor->formFlags & 0x00000004) || !armor->GetPlayable()) {
        return false;
    }

    if (armor->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kShield)) {
        return false;
    }
    return true;
}

bool IsItemInAnyWheelOutfit(RE::FormID a_formID) {
    std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
    for (const auto& wheel : g_manager.wheels) {
        for (const auto& slot : wheel.slots) {
            if (slot.outfit.has_value()) {
                for (RE::FormID fID : slot.outfit->itemIDs) {
                    if (fID == a_formID) return true;
                }
            }
        }
    }
    return false;
}

RE::TESObjectREFR* GetWardrobeContainer() {
    auto* wardrobe = RE::TESForm::LookupByEditorID<RE::TESObjectREFR>("OutfitWheeler_WardrobeChestRef");
    if (wardrobe) {
        return wardrobe;
    }

    auto* dataHandler = RE::TESDataHandler::GetSingleton();
    if (dataHandler) {
        auto* ref = dataHandler->LookupForm<RE::TESObjectREFR>(0x00000800, "OutfitWheeler.esp");
        if (ref) return ref;
        ref = dataHandler->LookupForm<RE::TESObjectREFR>(0x00000801, "OutfitWheeler.esp");
        if (ref) return ref;
    }

    return nullptr;
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

void PerformOutfitSwap(RE::PlayerCharacter* player, RE::ActorEquipManager* equipManager, const std::vector<RE::FormID>& targetItemIDs) {
    if (!player || !equipManager || !player->Is3DLoaded() || player->IsDead()) return;

    auto actorState = player->AsActorState();
    if (actorState && (actorState->GetSitSleepState() != RE::SIT_SLEEP_STATE::kNormal ||
                       actorState->IsBleedingOut() ||
                       player->IsInKillMove() ||
                       player->IsOnMount())) {
        return;
    }

    auto race = player->GetRace();
    if (race) {
        auto npcKw = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("ActorTypeNPC");
        if (npcKw && !race->HasKeyword(npcKw)) {
            return;
        }
    }

    auto* wardrobe = GetWardrobeContainer();

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
                equipManager->UnequipObject(player, entry.object, entry.extraList, 1, nullptr, false, false, true, true);
                if (g_autoStashEnabled && wardrobe && IsItemInAnyWheelOutfit(formID)) {
                    player->RemoveItem(entry.object, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, wardrobe);
                }
            }
        }
    }

    if (wardrobe) {
        auto wardrobeInv = wardrobe->GetInventory();
        for (RE::FormID formID : targetList) {
            auto* form = RE::TESForm::LookupByID(formID);
            if (!form || !IsPureApparel(form)) continue;
            auto* boundObj = form->As<RE::TESBoundObject>();
            if (!boundObj) continue;

            auto it = wardrobeInv.find(boundObj);
            if (it != wardrobeInv.end() && it->second.first > 0) {
                wardrobe->RemoveItem(boundObj, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, player);
                it->second.first--;
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
                auto* entryData = it->second.second.get();
                RE::ExtraDataList* targetExtra = nullptr;

                if (entryData && entryData->extraLists && !entryData->extraLists->empty()) {
                    for (auto* xList : *entryData->extraLists) {
                        if (xList && !xList->HasType(RE::ExtraDataType::kWorn) && !xList->HasType(RE::ExtraDataType::kWornLeft)) {
                            targetExtra = xList;
                            break;
                        }
                    }
                }

                if (entryData && entryData->IsWorn() && !targetExtra) {
                    continue;
                }

                equipManager->EquipObject(player, boundObj, targetExtra, 1, nullptr, false, false, true, true);
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
                RE::FormID fID = entry.object->GetFormID();
                if (std::find(outfit.itemIDs.begin(), outfit.itemIDs.end(), fID) == outfit.itemIDs.end()) {
                    outfit.itemIDs.push_back(fID);
                }
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

    std::string clickedOutfitID = std::to_string(wheelIdx) + "_" + std::to_string(slotIdx);
    bool isCurrentRecordedOutfit = (AdaptiveWardrobeManager::GetSingleton()->GetCurrentOutfitID() == clickedOutfitID);
    bool isExactItemMatch = (!sanitizedTargetIDs.empty() && matchCount == sanitizedTargetIDs.size() && matchCount == currentlyWornEntries.size());

    bool isAlreadyWorn = isCurrentRecordedOutfit || isExactItemMatch;

    if (isAlreadyWorn) {
        auto* wardrobe = GetWardrobeContainer();
        for (const auto& entry : currentlyWornEntries) {
            if (entry.object && IsPureApparel(entry.object)) {
                RE::FormID formID = entry.object->GetFormID();
                if (std::find(sanitizedTargetIDs.begin(), sanitizedTargetIDs.end(), formID) != sanitizedTargetIDs.end()) {
                    equipManager->UnequipObject(player, entry.object, entry.extraList, 1, nullptr, false, false, true, true);
                    if (g_autoStashEnabled && wardrobe && IsItemInAnyWheelOutfit(formID)) {
                        player->RemoveItem(entry.object, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, wardrobe);
                    }
                }
            }
        }
        AdaptiveWardrobeManager::GetSingleton()->SetCurrentOutfitID("");
        RE::DebugNotification(("Unequipped " + targetOutfit.name).c_str());
        return;
    }

    PerformOutfitSwap(player, equipManager, sanitizedTargetIDs);

    AdaptiveWardrobeManager::GetSingleton()->RecordOutfitEquipped(clickedOutfitID, 0);

    RE::DebugNotification(("Equipped " + targetOutfit.name).c_str());
}

void RetrieveAllOutfitsToPlayer() {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto* wardrobe = GetWardrobeContainer();
    if (!player || !wardrobe) return;

    size_t count = 0;
    auto wardrobeInv = wardrobe->GetInventory();
    for (const auto& [item, data] : wardrobeInv) {
        if (item && data.first > 0) {
            wardrobe->RemoveItem(item, data.first, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, player);
            count += data.first;
        }
    }

    SKSE::log::info("RetrieveAllOutfitsToPlayer: Retrieved {} total items to player", count);
    RE::DebugNotification("All outfits retrieved to player inventory!");
}

void StashAllOutfitsToWardrobe() {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto* wardrobe = GetWardrobeContainer();
    if (!player || !wardrobe) return;

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

    std::vector<RE::FormID> uniqueRegisteredIDs;
    uniqueRegisteredIDs.reserve(allRegisteredIDs.size());
    for (RE::FormID fID : allRegisteredIDs) {
        if (std::find(uniqueRegisteredIDs.begin(), uniqueRegisteredIDs.end(), fID) == uniqueRegisteredIDs.end()) {
            uniqueRegisteredIDs.push_back(fID);
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

    size_t count = 0;
    auto playerInv = player->GetInventory();
    for (RE::FormID fID : uniqueRegisteredIDs) {
        if (std::find(currentlyWornIDs.begin(), currentlyWornIDs.end(), fID) == currentlyWornIDs.end()) {
            auto* form = RE::TESForm::LookupByID(fID);
            if (form && IsPureApparel(form)) {
                auto* boundObj = form->As<RE::TESBoundObject>();
                if (boundObj) {
                    auto it = playerInv.find(boundObj);
                    if (it != playerInv.end() && it->second.first > 0) {
                        player->RemoveItem(boundObj, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, wardrobe);
                        it->second.first--;
                        count++;
                    }
                }
            }
        }
    }

    SKSE::log::info("StashAllOutfitsToWardrobe: Stored {} items in wardrobe", count);
    RE::DebugNotification("All inactive outfits stored in wardrobe!");
}

void StashSlotOutfit(size_t wheelIdx, size_t slotIdx) {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto equipManager = RE::ActorEquipManager::GetSingleton();
    auto* wardrobe = GetWardrobeContainer();
    if (!player || !equipManager) return;

    if (!wardrobe) {
        SKSE::log::error("StashSlotOutfit: OutfitWheeler_WardrobeChestRef not found! Make sure OutfitWheeler.esp is enabled.");
        RE::DebugNotification("OutfitWheeler.esp is not active!");
        return;
    }

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

    std::vector<RE::FormID> uniqueSlotItems;
    uniqueSlotItems.reserve(slotItems.size());
    for (RE::FormID fID : slotItems) {
        if (std::find(uniqueSlotItems.begin(), uniqueSlotItems.end(), fID) == uniqueSlotItems.end()) {
            uniqueSlotItems.push_back(fID);
        }
    }

    auto currentlyWorn = GetEquippedInventoryEntries();
    for (const auto& entry : currentlyWorn) {
        if (entry.object && IsPureApparel(entry.object)) {
            RE::FormID formID = entry.object->GetFormID();
            if (std::find(uniqueSlotItems.begin(), uniqueSlotItems.end(), formID) != uniqueSlotItems.end()) {
                equipManager->UnequipObject(player, entry.object, entry.extraList, 1, nullptr, false, false, true, true);
            }
        }
    }

    size_t count = 0;
    auto playerInv = player->GetInventory();
    for (RE::FormID fID : uniqueSlotItems) {
        auto* form = RE::TESForm::LookupByID(fID);
        if (form && IsPureApparel(form)) {
            auto* bound = form->As<RE::TESBoundObject>();
            if (bound) {
                auto it = playerInv.find(bound);
                if (it != playerInv.end() && it->second.first > 0) {
                    player->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, wardrobe);
                    it->second.first--;
                    count++;
                }
            }
        }
    }

    SKSE::log::info("StashSlotOutfit: Slot {} ({}) stored {} items in wardrobe", slotIdx, outfitName, count);
    RE::DebugNotification(("Stored " + outfitName + " in wardrobe!").c_str());
}

void RetrieveSlotOutfit(size_t wheelIdx, size_t slotIdx) {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto* wardrobe = GetWardrobeContainer();
    if (!player || !wardrobe) return;

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

    std::vector<RE::FormID> uniqueSlotItems;
    uniqueSlotItems.reserve(slotItems.size());
    for (RE::FormID fID : slotItems) {
        if (std::find(uniqueSlotItems.begin(), uniqueSlotItems.end(), fID) == uniqueSlotItems.end()) {
            uniqueSlotItems.push_back(fID);
        }
    }

    size_t count = 0;
    auto wardrobeInv = wardrobe->GetInventory();
    for (RE::FormID fID : uniqueSlotItems) {
        auto* form = RE::TESForm::LookupByID(fID);
        if (form && IsPureApparel(form)) {
            auto* bound = form->As<RE::TESBoundObject>();
            if (bound) {
                auto it = wardrobeInv.find(bound);
                if (it != wardrobeInv.end() && it->second.first > 0) {
                    wardrobe->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, player);
                    it->second.first--;
                    count++;
                }
            }
        }
    }

    SKSE::log::info("RetrieveSlotOutfit: Slot {} ({}) retrieved {} items to player", slotIdx, outfitName, count);
    RE::DebugNotification(("Retrieved " + outfitName + " to inventory!").c_str());
}

void StashWheelOutfits(size_t wheelIdx) {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto equipManager = RE::ActorEquipManager::GetSingleton();
    auto* wardrobe = GetWardrobeContainer();
    if (!player || !equipManager || !wardrobe) return;

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

    std::vector<RE::FormID> uniqueWheelIDs;
    uniqueWheelIDs.reserve(wheelItemIDs.size());
    for (RE::FormID fID : wheelItemIDs) {
        if (std::find(uniqueWheelIDs.begin(), uniqueWheelIDs.end(), fID) == uniqueWheelIDs.end()) {
            uniqueWheelIDs.push_back(fID);
        }
    }

    auto currentlyWorn = GetEquippedInventoryEntries();
    std::vector<RE::FormID> currentlyWornIDs;
    currentlyWornIDs.reserve(currentlyWorn.size());
    for (const auto& entry : currentlyWorn) {
        if (entry.object && IsPureApparel(entry.object)) {
            currentlyWornIDs.push_back(entry.object->GetFormID());
        }
    }

    size_t count = 0;
    auto playerInv = player->GetInventory();
    for (RE::FormID fID : uniqueWheelIDs) {
        if (std::find(currentlyWornIDs.begin(), currentlyWornIDs.end(), fID) == currentlyWornIDs.end()) {
            auto* form = RE::TESForm::LookupByID(fID);
            if (form && IsPureApparel(form)) {
                auto* boundObj = form->As<RE::TESBoundObject>();
                if (boundObj) {
                    auto it = playerInv.find(boundObj);
                    if (it != playerInv.end() && it->second.first > 0) {
                        player->RemoveItem(boundObj, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, wardrobe);
                        it->second.first--;
                        count++;
                    }
                }
            }
        }
    }

    SKSE::log::info("StashWheelOutfits: Wheel {} stored {} items in wardrobe", wheelIdx, count);
    RE::DebugNotification("Wheel outfits stored in wardrobe!");
}

void RetrieveWheelOutfits(size_t wheelIdx) {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto* wardrobe = GetWardrobeContainer();
    if (!player || !wardrobe) return;

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

    std::vector<RE::FormID> uniqueWheelIDs;
    uniqueWheelIDs.reserve(wheelItemIDs.size());
    for (RE::FormID fID : wheelItemIDs) {
        if (std::find(uniqueWheelIDs.begin(), uniqueWheelIDs.end(), fID) == uniqueWheelIDs.end()) {
            uniqueWheelIDs.push_back(fID);
        }
    }

    size_t count = 0;
    auto wardrobeInv = wardrobe->GetInventory();
    for (RE::FormID fID : uniqueWheelIDs) {
        auto* form = RE::TESForm::LookupByID(fID);
        if (form && IsPureApparel(form)) {
            auto* bound = form->As<RE::TESBoundObject>();
            if (bound) {
                auto it = wardrobeInv.find(bound);
                if (it != wardrobeInv.end() && it->second.first > 0) {
                    wardrobe->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, player);
                    it->second.first--;
                    count++;
                }
            }
        }
    }

    SKSE::log::info("RetrieveWheelOutfits: Wheel {} retrieved {} items to player", wheelIdx, count);
    RE::DebugNotification("Retrieved wheel outfits to inventory!");
}