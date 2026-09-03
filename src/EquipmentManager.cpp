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
#include <unordered_set>

static uint64_t g_lastEquipTimestamp = 0;

bool IsPureApparel(RE::TESForm* a_form) noexcept {
    if (!a_form) return false;
    if (a_form->GetFormType() != RE::FormType::Armor) {
        return false;
    }
    auto* armor = a_form->As<RE::TESObjectARMO>();
    if (!armor) return false;

    if (!armor->GetPlayable()) return false;
    if ((armor->formFlags & 0x00000004) != 0) return false;

    return true;
}

RE::Actor* ResolveActiveTargetActor() {
    std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
    if (g_manager.activeTargetActorID != 0 && g_manager.activeTargetActorID != 0x00000007 && g_manager.activeTargetActorID != 0x00000014) {
        auto* form = RE::TESForm::LookupByID(g_manager.activeTargetActorID);
        if (form) {
            auto* actor = form->As<RE::Actor>();
            if (actor && !actor->IsDead()) {
                return actor;
            }
        }
    }
    return RE::PlayerCharacter::GetSingleton();
}

bool IsItemInAnyWheelOutfit(RE::FormID a_formID) {
    std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
    for (const auto& [actorID, profile] : g_manager.actorProfiles) {
        for (const auto& wheel : profile.wheels) {
            for (const auto& slot : wheel.slots) {
                if (slot.outfit.has_value()) {
                    for (RE::FormID fID : slot.outfit->itemIDs) {
                        if (fID == a_formID) return true;
                    }
                    for (const auto& itm : slot.outfit->items) {
                        if (itm.formID == a_formID) return true;
                    }
                }
            }
        }
    }
    return false;
}

RE::TESObjectREFR* GetWardrobeContainer() {
    if (g_manager.wardrobeContainerFormID != 0) {
        auto* form = RE::TESForm::LookupByID(g_manager.wardrobeContainerFormID);
        if (form) {
            auto* refr = form->As<RE::TESObjectREFR>();
            if (refr) return refr;
        }
    }

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
        ref = dataHandler->LookupForm<RE::TESObjectREFR>(0x00000800, "PrismaOutfitWheel.esp");
        if (ref) return ref;
        ref = dataHandler->LookupForm<RE::TESObjectREFR>(0x00000801, "PrismaOutfitWheel.esp");
        if (ref) return ref;
    }

    return nullptr;
}

std::vector<InventoryWornEntry> GetEquippedInventoryEntries(RE::Actor* a_actor) {
    std::vector<InventoryWornEntry> equipped;
    auto actor = a_actor ? a_actor : ResolveActiveTargetActor();
    if (!actor) return equipped;

    auto inventory = actor->GetInventory();
    equipped.reserve(16);

    for (const auto& [item, data] : inventory) {
        if (item && data.first > 0 && IsPureApparel(item) && data.second) {
            auto* bound = item->As<RE::TESBoundObject>();
            if (!bound) continue;

            auto* entryData = data.second.get();
            if (entryData && entryData->extraLists) {
                for (auto* extraList : *entryData->extraLists) {
                    if (extraList && (extraList->HasType(RE::ExtraDataType::kWorn) || extraList->HasType(RE::ExtraDataType::kWornLeft))) {
                        InventoryWornEntry entry;
                        entry.object = bound;
                        entry.extraList = extraList;
                        equipped.push_back(entry);
                    }
                }
            }
        }
    }
    return equipped;
}

bool MatchesWornEntry(const InventoryWornEntry& entry, const OutfitItem& target) {
    if (!entry.object || entry.object->GetFormID() != target.formID) return false;
    if (!entry.extraList) {
        return target.uniqueID == 0 && target.customName.empty() && target.enchantmentID == 0;
    }

    if (target.uniqueID != 0) {
        auto* xUID = entry.extraList->GetByType<RE::ExtraUniqueID>();
        if (xUID && xUID->uniqueID == target.uniqueID) return true;
        return false;
    }

    if (!target.customName.empty()) {
        auto* xName = entry.extraList->GetByType<RE::ExtraTextDisplayData>();
        if (xName && xName->displayName.c_str() && target.customName == xName->displayName.c_str()) return true;
        return false;
    }

    if (target.enchantmentID != 0) {
        auto* xEnch = entry.extraList->GetByType<RE::ExtraEnchantment>();
        if (xEnch && xEnch->enchantment && xEnch->enchantment->GetFormID() == target.enchantmentID) return true;

        auto* arm = entry.object->As<RE::TESObjectARMO>();
        if (arm && arm->formEnchanting && arm->formEnchanting->GetFormID() == target.enchantmentID) return true;

        return false;
    }

    bool hasCustomEnch = entry.extraList->HasType(RE::ExtraDataType::kEnchantment);
    bool hasCustomName = entry.extraList->HasType(RE::ExtraDataType::kTextDisplayData);
    return !hasCustomEnch && !hasCustomName;
}

static RE::ExtraDataList* FindBestMatchingExtraList(RE::InventoryEntryData* entryData, const OutfitItem& target, RE::TESBoundObject* boundObj) {
    if (!entryData || !entryData->extraLists || entryData->extraLists->empty()) {
        return nullptr;
    }

    RE::ExtraDataList* bestCandidate = nullptr;
    int highestScore = -1;

    for (auto* xList : *entryData->extraLists) {
        if (!xList || xList->HasType(RE::ExtraDataType::kWorn) || xList->HasType(RE::ExtraDataType::kWornLeft)) {
            continue;
        }

        int score = 0;

        if (target.uniqueID != 0) {
            auto* xUID = xList->GetByType<RE::ExtraUniqueID>();
            if (xUID && xUID->uniqueID == target.uniqueID) {
                score += 1000;
            } else {
                continue;
            }
        }

        if (!target.customName.empty()) {
            auto* xName = xList->GetByType<RE::ExtraTextDisplayData>();
            if (xName && xName->displayName.c_str() && target.customName == xName->displayName.c_str()) {
                score += 500;
            } else {
                continue;
            }
        }

        if (target.enchantmentID != 0) {
            auto* xEnch = xList->GetByType<RE::ExtraEnchantment>();
            if (xEnch && xEnch->enchantment && xEnch->enchantment->GetFormID() == target.enchantmentID) {
                score += 250;
            } else {
                auto* arm = boundObj ? boundObj->As<RE::TESObjectARMO>() : nullptr;
                if (arm && arm->formEnchanting && arm->formEnchanting->GetFormID() == target.enchantmentID) {
                    score += 200;
                } else {
                    continue;
                }
            }
        }

        if (target.uniqueID == 0 && target.customName.empty() && target.enchantmentID == 0) {
            bool hasCustomEnch = xList->HasType(RE::ExtraDataType::kEnchantment);
            bool hasCustomName = xList->HasType(RE::ExtraDataType::kTextDisplayData);
            if (!hasCustomEnch && !hasCustomName) {
                score += 100;
            }
        }

        if (score > highestScore) {
            highestScore = score;
            bestCandidate = xList;
        }
    }

    return bestCandidate;
}

void AutoStashInactiveSlotItems(RE::Actor* a_actor) {
    if (!g_autoStashEnabled) return;
    auto* targetActor = a_actor ? a_actor : ResolveActiveTargetActor();
    if (!targetActor || !targetActor->Is3DLoaded() || targetActor->IsDead()) return;

    auto* wardrobe = GetWardrobeContainer();
    if (!wardrobe) return;

    std::vector<RE::FormID> registeredIDs;
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        RE::FormID canonicalID = (targetActor == RE::PlayerCharacter::GetSingleton()) ? 0x00000007 : targetActor->GetFormID();
        const auto& profile = g_manager.GetProfile(canonicalID);
        for (const auto& wheel : profile.wheels) {
            for (const auto& slot : wheel.slots) {
                if (slot.outfit.has_value()) {
                    for (RE::FormID fID : slot.outfit->itemIDs) {
                        if (fID != 0) registeredIDs.push_back(fID);
                    }
                    for (const auto& itm : slot.outfit->items) {
                        if (itm.formID != 0) registeredIDs.push_back(itm.formID);
                    }
                }
            }
        }
    }

    if (registeredIDs.empty()) return;

    std::vector<RE::FormID> uniqueRegisteredIDs;
    uniqueRegisteredIDs.reserve(registeredIDs.size());
    for (RE::FormID fID : registeredIDs) {
        if (std::find(uniqueRegisteredIDs.begin(), uniqueRegisteredIDs.end(), fID) == uniqueRegisteredIDs.end()) {
            uniqueRegisteredIDs.push_back(fID);
        }
    }

    auto currentlyWorn = GetEquippedInventoryEntries(targetActor);
    std::unordered_set<RE::FormID> currentlyWornIDs;
    for (const auto& entry : currentlyWorn) {
        if (entry.object) {
            currentlyWornIDs.insert(entry.object->GetFormID());
        }
    }

    auto actorInv = targetActor->GetInventory();
    for (RE::FormID fID : uniqueRegisteredIDs) {
        if (currentlyWornIDs.find(fID) == currentlyWornIDs.end()) {
            auto* form = RE::TESForm::LookupByID(fID);
            if (form && IsPureApparel(form)) {
                auto* boundObj = form->As<RE::TESBoundObject>();
                if (boundObj) {
                    auto it = actorInv.find(boundObj);
                    if (it != actorInv.end() && it->second.first > 0) {
                        int32_t count = it->second.first;
                        targetActor->RemoveItem(boundObj, count, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, wardrobe);
                    }
                }
            }
        }
    }
}

void PerformOutfitSwap(RE::Actor* targetActor, RE::ActorEquipManager* equipManager, const std::vector<OutfitItem>& targetItems) {
    if (!targetActor || !equipManager || !targetActor->Is3DLoaded() || targetActor->IsDead()) return;

    auto actorState = targetActor->AsActorState();
    if (actorState && (actorState->IsBleedingOut() || targetActor->IsInKillMove())) {
        return;
    }

    auto race = targetActor->GetRace();
    if (race) {
        auto npcKw = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("ActorTypeNPC");
        if (npcKw && !race->HasKeyword(npcKw)) {
            return;
        }
    }

    auto* wardrobe = GetWardrobeContainer();
    auto* player = RE::PlayerCharacter::GetSingleton();

    std::vector<OutfitItem> targetList;
    std::unordered_set<RE::FormID> targetFormIDs;
    targetList.reserve(targetItems.size());
    targetFormIDs.reserve(targetItems.size());

    for (const auto& itm : targetItems) {
        auto* form = RE::TESForm::LookupByID(itm.formID);
        if (form && IsPureApparel(form)) {
            targetList.push_back(itm);
            targetFormIDs.insert(itm.formID);
        }
    }

    auto currentlyWornEntries = GetEquippedInventoryEntries(targetActor);

    for (const auto& entry : currentlyWornEntries) {
        if (!entry.object || !IsPureApparel(entry.object)) continue;

        bool shouldKeep = false;
        for (const auto& targetItem : targetList) {
            if (MatchesWornEntry(entry, targetItem)) {
                shouldKeep = true;
                break;
            }
        }

        if (!shouldKeep) {
            auto* bound = entry.object->As<RE::TESBoundObject>();
            if (bound) {
                equipManager->UnequipObject(targetActor, bound, entry.extraList, 1, nullptr, false, false, true, true);
                if (g_autoStashEnabled && wardrobe && IsItemInAnyWheelOutfit(entry.object->GetFormID())) {
                    targetActor->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, wardrobe);
                }
            }
        }
    }

    if (wardrobe) {
        auto wardrobeInv = wardrobe->GetInventory();
        for (const auto& itm : targetList) {
            auto* form = RE::TESForm::LookupByID(itm.formID);
            if (!form || !IsPureApparel(form)) continue;
            auto* boundObj = form->As<RE::TESBoundObject>();
            if (!boundObj) continue;

            auto it = wardrobeInv.find(boundObj);
            if (it != wardrobeInv.end() && it->second.first > 0) {
                wardrobe->RemoveItem(boundObj, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, targetActor);
                it->second.first--;
            }
        }
    }

    if (targetActor != player && player) {
        auto playerInv = player->GetInventory();
        auto targetInv = targetActor->GetInventory();
        for (const auto& itm : targetList) {
            auto* form = RE::TESForm::LookupByID(itm.formID);
            if (!form || !IsPureApparel(form)) continue;
            auto* boundObj = form->As<RE::TESBoundObject>();
            if (!boundObj) continue;

            auto targetIt = targetInv.find(boundObj);
            if (targetIt == targetInv.end() || targetIt->second.first <= 0) {
                auto playerIt = playerInv.find(boundObj);
                if (playerIt != playerInv.end() && playerIt->second.first > 0) {
                    player->RemoveItem(boundObj, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, targetActor);
                    playerIt->second.first--;
                }
            }
        }
    }

    auto freshInventory = targetActor->GetInventory();
    for (const auto& itm : targetList) {
        auto* form = RE::TESForm::LookupByID(itm.formID);
        if (!form || !IsPureApparel(form)) continue;

        auto* boundObj = form->As<RE::TESBoundObject>();
        if (!boundObj) continue;

        auto it = freshInventory.find(boundObj);
        if (it != freshInventory.end() && it->second.first > 0) {
            auto* entryData = it->second.second.get();
            RE::ExtraDataList* targetExtra = FindBestMatchingExtraList(entryData, itm, boundObj);
            equipManager->EquipObject(targetActor, boundObj, targetExtra, 1, nullptr, false, true, true, true);
        }
    }

    AutoStashInactiveSlotItems(targetActor);
}

void PerformOutfitSwap(RE::Actor* targetActor, RE::ActorEquipManager* equipManager, const std::vector<RE::FormID>& targetItemIDs) {
    std::vector<OutfitItem> converted;
    converted.reserve(targetItemIDs.size());
    for (auto fID : targetItemIDs) {
        OutfitItem itm;
        itm.formID = fID;
        converted.push_back(itm);
    }
    PerformOutfitSwap(targetActor, equipManager, converted);
}

void SaveOutfitToSlotImpl(size_t wheelIdx, size_t slotIdx) {
    auto targetActor = ResolveActiveTargetActor();
    auto equipped = GetEquippedInventoryEntries(targetActor);
    if (equipped.empty()) {
        RE::DebugNotification("No equipped armor/clothing to save!");
        return;
    }

    std::string savedName = "Outfit";
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        auto& wheels = g_manager.GetActiveWheels();
        if (wheelIdx >= wheels.size()) return;
        if (slotIdx >= wheels[wheelIdx].slots.size()) return;

        Outfit outfit;
        if (wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
            outfit = *wheels[wheelIdx].slots[slotIdx].outfit;
        } else {
            outfit.name = "Outfit " + std::to_string(slotIdx + 1);
            outfit.contextTag = "";
            outfit.icon = "";
            outfit.preferenceStars = 3;
            outfit.controlState = ControlState::Normal;
            outfit.usageCount = 0;
            outfit.lastWornTimestamp = 0;
        }

        outfit.itemIDs.clear();
        outfit.items.clear();
        outfit.itemIDs.reserve(equipped.size());
        outfit.items.reserve(equipped.size());

        for (const auto& entry : equipped) {
            if (entry.object && IsPureApparel(entry.object)) {
                OutfitItem itm;
                itm.formID = entry.object->GetFormID();

                if (entry.extraList) {
                    auto* xUID = entry.extraList->GetByType<RE::ExtraUniqueID>();
                    if (xUID) {
                        itm.uniqueID = xUID->uniqueID;
                    }

                    auto* xEnch = entry.extraList->GetByType<RE::ExtraEnchantment>();
                    if (xEnch && xEnch->enchantment) {
                        itm.enchantmentID = xEnch->enchantment->GetFormID();
                    }

                    auto* xName = entry.extraList->GetByType<RE::ExtraTextDisplayData>();
                    if (xName && xName->displayName.c_str()) {
                        itm.customName = xName->displayName.c_str();
                    }
                }

                if (itm.enchantmentID == 0) {
                    auto* arm = entry.object->As<RE::TESObjectARMO>();
                    if (arm && arm->formEnchanting) {
                        itm.enchantmentID = arm->formEnchanting->GetFormID();
                    }
                }

                outfit.items.push_back(itm);
                outfit.itemIDs.push_back(itm.formID);
            }
        }

        wheels[wheelIdx].slots[slotIdx].outfit = outfit;
        savedName = outfit.name;
        SaveDataInternal();
    }

    RE::DebugNotification(("Saved " + savedName).c_str());
}

void EquipSlotImpl(size_t wheelIdx, size_t slotIdx) {
    uint64_t now = GetTickCount64();
    if (now - g_lastEquipTimestamp < 250) {
        return;
    }
    g_lastEquipTimestamp = now;

    Outfit targetOutfit;
    bool found = false;
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        const auto& wheels = g_manager.GetActiveWheels();
        if (wheelIdx < wheels.size() && slotIdx < wheels[wheelIdx].slots.size()) {
            if (wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
                targetOutfit = *wheels[wheelIdx].slots[slotIdx].outfit;
                found = true;
            }
        }
    }

    if (!found) return;

    auto targetActor = ResolveActiveTargetActor();
    auto equipManager = RE::ActorEquipManager::GetSingleton();
    if (!targetActor || !equipManager) return;

    RE::FormID targetActorID = (targetActor == RE::PlayerCharacter::GetSingleton()) ? 0x00000007 : targetActor->GetFormID();

    std::vector<OutfitItem> targetItems;
    if (!targetOutfit.items.empty()) {
        targetItems = targetOutfit.items;
    } else {
        for (auto fID : targetOutfit.itemIDs) {
            OutfitItem itm;
            itm.formID = fID;
            targetItems.push_back(itm);
        }
    }

    if (targetItems.empty()) return;

    auto currentlyWornEntries = GetEquippedInventoryEntries(targetActor);

    if (targetOutfit.isAddon) {
        size_t addonMatches = 0;
        for (const auto& entry : currentlyWornEntries) {
            for (const auto& itm : targetItems) {
                if (MatchesWornEntry(entry, itm)) {
                    addonMatches++;
                    break;
                }
            }
        }

        bool allAddonWorn = (addonMatches == targetItems.size());
        auto* wardrobe = GetWardrobeContainer();
        auto* player = RE::PlayerCharacter::GetSingleton();

        if (allAddonWorn) {
            for (const auto& entry : currentlyWornEntries) {
                if (!entry.object || !IsPureApparel(entry.object)) continue;
                for (const auto& itm : targetItems) {
                    if (MatchesWornEntry(entry, itm)) {
                        auto* bound = entry.object->As<RE::TESBoundObject>();
                        if (bound) {
                            equipManager->UnequipObject(targetActor, bound, entry.extraList, 1, nullptr, false, false, true, true);
                            if (wardrobe) {
                                targetActor->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, wardrobe);
                            }
                        }
                        break;
                    }
                }
            }
            AutoStashInactiveSlotItems(targetActor);
            RE::DebugNotification(("Unequipped Addon " + targetOutfit.name).c_str());
        } else {
            if (wardrobe) {
                auto wardrobeInv = wardrobe->GetInventory();
                for (const auto& itm : targetItems) {
                    auto* form = RE::TESForm::LookupByID(itm.formID);
                    if (!form || !IsPureApparel(form)) continue;
                    auto* boundObj = form->As<RE::TESBoundObject>();
                    if (!boundObj) continue;

                    auto it = wardrobeInv.find(boundObj);
                    if (it != wardrobeInv.end() && it->second.first > 0) {
                        wardrobe->RemoveItem(boundObj, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, targetActor);
                        it->second.first--;
                    }
                }
            }

            if (targetActor != player && player) {
                auto playerInv = player->GetInventory();
                auto targetInv = targetActor->GetInventory();
                for (const auto& itm : targetItems) {
                    auto* form = RE::TESForm::LookupByID(itm.formID);
                    if (!form || !IsPureApparel(form)) continue;
                    auto* boundObj = form->As<RE::TESBoundObject>();
                    if (!boundObj) continue;

                    auto targetIt = targetInv.find(boundObj);
                    if (targetIt == targetInv.end() || targetIt->second.first <= 0) {
                        auto playerIt = playerInv.find(boundObj);
                        if (playerIt != playerInv.end() && playerIt->second.first > 0) {
                            player->RemoveItem(boundObj, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, targetActor);
                            playerIt->second.first--;
                        }
                    }
                }
            }

            auto freshInventory = targetActor->GetInventory();
            for (const auto& itm : targetItems) {
                auto* form = RE::TESForm::LookupByID(itm.formID);
                if (!form || !IsPureApparel(form)) continue;

                auto* boundObj = form->As<RE::TESBoundObject>();
                if (!boundObj) continue;

                auto it = freshInventory.find(boundObj);
                if (it != freshInventory.end() && it->second.first > 0) {
                    auto* entryData = it->second.second.get();
                    RE::ExtraDataList* targetExtra = FindBestMatchingExtraList(entryData, itm, boundObj);
                    equipManager->EquipObject(targetActor, boundObj, targetExtra, 1, nullptr, false, true, true, true);
                }
            }

            AutoStashInactiveSlotItems(targetActor);
            RE::DebugNotification(("Equipped Addon " + targetOutfit.name).c_str());
        }
        return;
    }

    PerformOutfitSwap(targetActor, equipManager, targetItems);

    std::string clickedOutfitID = std::to_string(wheelIdx) + "_" + std::to_string(slotIdx);
    AdaptiveWardrobeManager::GetSingleton()->RecordOutfitEquipped(targetActorID, clickedOutfitID, 0);
    RE::DebugNotification(("Equipped " + targetOutfit.name).c_str());
}

void RetrieveAllOutfitsToPlayer() {
    auto targetActor = ResolveActiveTargetActor();
    auto* wardrobe = GetWardrobeContainer();
    if (!targetActor || !wardrobe) return;

    auto wardrobeInv = wardrobe->GetInventory();
    for (const auto& [item, data] : wardrobeInv) {
        if (item && data.first > 0) {
            wardrobe->RemoveItem(item, data.first, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, targetActor);
        }
    }

    RE::DebugNotification("All outfits retrieved to character inventory!");
}

void StashAllOutfitsToWardrobe() {
    auto targetActor = ResolveActiveTargetActor();
    if (!targetActor) return;
    AutoStashInactiveSlotItems(targetActor);
    RE::DebugNotification("All inactive outfits stored in wardrobe!");
}

void StashSlotOutfit(size_t wheelIdx, size_t slotIdx) {
    auto targetActor = ResolveActiveTargetActor();
    auto equipManager = RE::ActorEquipManager::GetSingleton();
    auto* wardrobe = GetWardrobeContainer();
    if (!targetActor || !equipManager || !wardrobe) return;

    std::vector<RE::FormID> slotItems;
    std::string outfitName = "Outfit";
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        const auto& wheels = g_manager.GetActiveWheels();
        if (wheelIdx < wheels.size() && slotIdx < wheels[wheelIdx].slots.size()) {
            if (wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
                slotItems = wheels[wheelIdx].slots[slotIdx].outfit->itemIDs;
                outfitName = wheels[wheelIdx].slots[slotIdx].outfit->name;
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

    auto currentlyWorn = GetEquippedInventoryEntries(targetActor);
    for (const auto& entry : currentlyWorn) {
        if (entry.object && IsPureApparel(entry.object)) {
            RE::FormID formID = entry.object->GetFormID();
            if (std::find(uniqueSlotItems.begin(), uniqueSlotItems.end(), formID) != uniqueSlotItems.end()) {
                auto* bound = entry.object->As<RE::TESBoundObject>();
                if (bound) {
                    equipManager->UnequipObject(targetActor, bound, entry.extraList, 1, nullptr, false, false, true, true);
                    targetActor->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, wardrobe);
                }
            }
        }
    }

    auto actorInv = targetActor->GetInventory();
    for (RE::FormID fID : uniqueSlotItems) {
        auto* form = RE::TESForm::LookupByID(fID);
        if (form && IsPureApparel(form)) {
            auto* bound = form->As<RE::TESBoundObject>();
            if (bound) {
                auto it = actorInv.find(bound);
                if (it != actorInv.end() && it->second.first > 0) {
                    targetActor->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, wardrobe);
                    it->second.first--;
                }
            }
        }
    }

    RE::DebugNotification(("Stored " + outfitName + " in wardrobe!").c_str());
}

void RetrieveSlotOutfit(size_t wheelIdx, size_t slotIdx) {
    auto targetActor = ResolveActiveTargetActor();
    auto* wardrobe = GetWardrobeContainer();
    if (!targetActor || !wardrobe) return;

    std::vector<RE::FormID> slotItems;
    std::string outfitName = "Outfit";
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        const auto& wheels = g_manager.GetActiveWheels();
        if (wheelIdx < wheels.size() && slotIdx < wheels[wheelIdx].slots.size()) {
            if (wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
                slotItems = wheels[wheelIdx].slots[slotIdx].outfit->itemIDs;
                outfitName = wheels[wheelIdx].slots[slotIdx].outfit->name;
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

    auto wardrobeInv = wardrobe->GetInventory();
    for (RE::FormID fID : uniqueSlotItems) {
        auto* form = RE::TESForm::LookupByID(fID);
        if (form && IsPureApparel(form)) {
            auto* bound = form->As<RE::TESBoundObject>();
            if (bound) {
                auto it = wardrobeInv.find(bound);
                if (it != wardrobeInv.end() && it->second.first > 0) {
                    wardrobe->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, targetActor);
                    it->second.first--;
                }
            }
        }
    }

    RE::DebugNotification(("Retrieved " + outfitName + " to inventory!").c_str());
}

void StashWheelOutfits(size_t wheelIdx) {
    auto targetActor = ResolveActiveTargetActor();
    auto equipManager = RE::ActorEquipManager::GetSingleton();
    auto* wardrobe = GetWardrobeContainer();
    if (!targetActor || !equipManager || !wardrobe) return;

    std::vector<RE::FormID> wheelItemIDs;
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        const auto& wheels = g_manager.GetActiveWheels();
        if (wheelIdx < wheels.size()) {
            for (const auto& slot : wheels[wheelIdx].slots) {
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

    auto currentlyWorn = GetEquippedInventoryEntries(targetActor);
    std::vector<RE::FormID> currentlyWornIDs;
    currentlyWornIDs.reserve(currentlyWorn.size());
    for (const auto& entry : currentlyWorn) {
        if (entry.object) {
            currentlyWornIDs.push_back(entry.object->GetFormID());
        }
    }

    auto actorInv = targetActor->GetInventory();
    for (RE::FormID fID : uniqueWheelIDs) {
        if (std::find(currentlyWornIDs.begin(), currentlyWornIDs.end(), fID) == currentlyWornIDs.end()) {
            auto* form = RE::TESForm::LookupByID(fID);
            if (form && IsPureApparel(form)) {
                auto* boundObj = form->As<RE::TESBoundObject>();
                if (boundObj) {
                    auto it = actorInv.find(boundObj);
                    if (it != actorInv.end() && it->second.first > 0) {
                        targetActor->RemoveItem(boundObj, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, wardrobe);
                        it->second.first--;
                    }
                }
            }
        }
    }

    RE::DebugNotification("Wheel outfits stored in wardrobe!");
}

void RetrieveWheelOutfits(size_t wheelIdx) {
    auto targetActor = ResolveActiveTargetActor();
    auto* wardrobe = GetWardrobeContainer();
    if (!targetActor || !wardrobe) return;

    std::vector<RE::FormID> wheelItemIDs;
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        const auto& wheels = g_manager.GetActiveWheels();
        if (wheelIdx < wheels.size()) {
            for (const auto& slot : wheels[wheelIdx].slots) {
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

    auto wardrobeInv = wardrobe->GetInventory();
    for (RE::FormID fID : uniqueWheelIDs) {
        auto* form = RE::TESForm::LookupByID(fID);
        if (form && IsPureApparel(form)) {
            auto* bound = form->As<RE::TESBoundObject>();
            if (bound) {
                auto it = wardrobeInv.find(bound);
                if (it != wardrobeInv.end() && it->second.first > 0) {
                    wardrobe->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, targetActor);
                    it->second.first--;
                }
            }
        }
    }

    RE::DebugNotification("Retrieved wheel outfits to inventory!");
}