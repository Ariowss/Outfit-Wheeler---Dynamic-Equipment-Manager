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
#include "thumbnail/ItemThumbnailService.h"
#include <windows.h>
#include <SKSE/SKSE.h>
#include <RE/B/BipedAnim.h>
#include <vector>
#include <algorithm>
#include <mutex>
#include <unordered_set>
#include <atomic>

static std::atomic<uint64_t> g_lastEquipTimestamp{ 0 };

bool IsPureApparel(RE::TESForm* a_form) noexcept {
    if (!a_form) return false;
    if (a_form->GetFormType() != RE::FormType::Armor) {
        return false;
    }
    auto* armor = a_form->As<RE::TESObjectARMO>();
    if (!armor) return false;

    if (!armor->GetPlayable()) return false;
    if ((armor->formFlags & 0x00000004) != 0) return false;

    const char* name = armor->GetName();
    if (!name || name[0] == '\0') return false;

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
    static std::atomic<RE::FormID> s_cachedWardrobeFormID{ 0 };
    RE::FormID cachedID = s_cachedWardrobeFormID.load(std::memory_order_relaxed);
    if (cachedID != 0) {
        auto* form = RE::TESForm::LookupByID(cachedID);
        if (form) {
            auto* refr = form->As<RE::TESObjectREFR>();
            if (refr) return refr;
        }
    }

    if (g_manager.wardrobeContainerFormID != 0) {
        auto* form = RE::TESForm::LookupByID(g_manager.wardrobeContainerFormID);
        if (form) {
            auto* refr = form->As<RE::TESObjectREFR>();
            if (refr) {
                s_cachedWardrobeFormID.store(g_manager.wardrobeContainerFormID, std::memory_order_relaxed);
                return refr;
            }
        }
    }

    auto* wardrobe = RE::TESForm::LookupByEditorID<RE::TESObjectREFR>("OutfitWheeler_WardrobeChestRef");
    if (wardrobe) {
        s_cachedWardrobeFormID.store(wardrobe->GetFormID(), std::memory_order_relaxed);
        return wardrobe;
    }

    auto* dataHandler = RE::TESDataHandler::GetSingleton();
    if (dataHandler) {
        auto* ref = dataHandler->LookupForm<RE::TESObjectREFR>(0x00000002, "OutfitWheeler.esp");
        if (ref) { s_cachedWardrobeFormID.store(ref->GetFormID(), std::memory_order_relaxed); return ref; }
        ref = dataHandler->LookupForm<RE::TESObjectREFR>(0x00000002, "PrismaOutfitWheel.esp");
        if (ref) { s_cachedWardrobeFormID.store(ref->GetFormID(), std::memory_order_relaxed); return ref; }

        ref = dataHandler->LookupForm<RE::TESObjectREFR>(0x00000800, "OutfitWheeler.esp");
        if (ref) { s_cachedWardrobeFormID.store(ref->GetFormID(), std::memory_order_relaxed); return ref; }
        ref = dataHandler->LookupForm<RE::TESObjectREFR>(0x00000801, "OutfitWheeler.esp");
        if (ref) { s_cachedWardrobeFormID.store(ref->GetFormID(), std::memory_order_relaxed); return ref; }
        ref = dataHandler->LookupForm<RE::TESObjectREFR>(0x00000802, "OutfitWheeler.esp");
        if (ref) { s_cachedWardrobeFormID.store(ref->GetFormID(), std::memory_order_relaxed); return ref; }
        ref = dataHandler->LookupForm<RE::TESObjectREFR>(0x00000800, "PrismaOutfitWheel.esp");
        if (ref) { s_cachedWardrobeFormID.store(ref->GetFormID(), std::memory_order_relaxed); return ref; }
        ref = dataHandler->LookupForm<RE::TESObjectREFR>(0x00000801, "PrismaOutfitWheel.esp");
        if (ref) { s_cachedWardrobeFormID.store(ref->GetFormID(), std::memory_order_relaxed); return ref; }
        ref = dataHandler->LookupForm<RE::TESObjectREFR>(0x00000802, "PrismaOutfitWheel.esp");
        if (ref) { s_cachedWardrobeFormID.store(ref->GetFormID(), std::memory_order_relaxed); return ref; }
    }

    return nullptr;
}

static int SafeValidateAndCleanExtraData(RE::ExtraDataList* a_xList) {
    if (!a_xList) return 0;
    __try {
        uintptr_t uList = reinterpret_cast<uintptr_t>(a_xList);
        if (uList < 0x10000 || uList >= 0x7FFFFFFFFFFF) return 0;

        auto* baseList = reinterpret_cast<RE::BaseExtraList*>(a_xList);
        RE::BSExtraData* node = baseList->GetData();
        int depth = 0;

        while (node && depth++ < 64) {
            uintptr_t uNode = reinterpret_cast<uintptr_t>(node);
            if (uNode < 0x10000 || uNode >= 0x7FFFFFFFFFFF) return 0;

            uintptr_t vtbl = *reinterpret_cast<uintptr_t*>(node);
            if (vtbl < 0x10000 || vtbl >= 0x7FFFFFFFFFFF) {
                // Null or invalid vtable pointer: root cause of save game crash call [rax+0x08]
                return 0;
            }

            auto type = node->GetType();
            if (static_cast<uint32_t>(type) > 0x100) {
                return 0;
            }

            node = node->next;
        }

        if (depth >= 64) return 0;
        return 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

static bool HasMeaningfulContainerExtraData(RE::ExtraDataList* a_xList) {
    if (!a_xList) return false;
    __try {
        if (a_xList->HasType(RE::ExtraDataType::kEnchantment) ||
            a_xList->HasType(RE::ExtraDataType::kTextDisplayData) ||
            a_xList->HasType(RE::ExtraDataType::kUniqueID) ||
            a_xList->HasType(RE::ExtraDataType::kHealth) ||
            a_xList->HasType(RE::ExtraDataType::kCharge) ||
            a_xList->HasType(RE::ExtraDataType::kPoison)) {
            return true;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    return false;
}

static void SafeStripContainerWornFlags(RE::ExtraDataList* a_xList) {
    if (!a_xList) return;
    __try {
        if (a_xList->HasType(RE::ExtraDataType::kWorn)) {
            a_xList->RemoveByType(RE::ExtraDataType::kWorn);
        }
        if (a_xList->HasType(RE::ExtraDataType::kWornLeft)) {
            a_xList->RemoveByType(RE::ExtraDataType::kWornLeft);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static int SafeProbeEntryData(RE::InventoryEntryData* a_entry, RE::TESBoundObject** a_outObj, RE::BSSimpleList<RE::ExtraDataList*>** a_outExtra) {
    if (!a_entry) return 0;
    __try {
        uintptr_t uEntry = reinterpret_cast<uintptr_t>(a_entry);
        if (uEntry < 0x10000 || uEntry >= 0x7FFFFFFFFFFF) return 0;
        auto* obj = a_entry->object;
        if (!obj) return 0;
        uintptr_t uObj = reinterpret_cast<uintptr_t>(obj);
        if (uObj < 0x10000 || uObj >= 0x7FFFFFFFFFFF) return 0;
        *a_outObj = obj;
        *a_outExtra = a_entry->extraLists;
        return 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

void SanitizeWardrobeContainer() {
    static std::mutex s_sanitizeMutex;
    std::lock_guard<std::mutex> lock(s_sanitizeMutex);

    auto* wardrobe = GetWardrobeContainer();
    if (!wardrobe) return;

    wardrobe->InitInventoryIfRequired();

    auto* xChanges = wardrobe->extraList.GetByType<RE::ExtraContainerChanges>();
    if (!xChanges || !xChanges->changes) return;

    auto* invChanges = xChanges->changes;
    if (!invChanges->entryList) return;

    std::vector<RE::InventoryEntryData*> validEntries;

    for (auto* entry : *invChanges->entryList) {
        RE::TESBoundObject* obj = nullptr;
        RE::BSSimpleList<RE::ExtraDataList*>* xLists = nullptr;
        if (SafeProbeEntryData(entry, &obj, &xLists) != 1) continue;

        if (xLists) {
            std::vector<RE::ExtraDataList*> validLists;
            for (auto* xList : *xLists) {
                if (SafeValidateAndCleanExtraData(xList) == 1) {
                    SafeStripContainerWornFlags(xList);
                    if (HasMeaningfulContainerExtraData(xList)) {
                        validLists.push_back(xList);
                    }
                }
            }

            xLists->clear();
            for (auto* validXList : validLists) {
                xLists->push_front(validXList);
            }

            if (xLists->empty()) {
                delete entry->extraLists;
                entry->extraLists = nullptr;
            }
        }

        validEntries.push_back(entry);
    }

    invChanges->entryList->clear();
    for (auto* entry : validEntries) {
        invChanges->entryList->push_front(entry);
    }
}

std::vector<InventoryWornEntry> GetEquippedInventoryEntries(RE::Actor* a_actor) {
    std::vector<InventoryWornEntry> equipped;
    auto actor = a_actor ? a_actor : ResolveActiveTargetActor();
    if (!actor) return equipped;

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

    auto inventory = actor->GetInventory();
    equipped.reserve(16);
    std::unordered_set<RE::FormID> addedFormIDs;

    for (const auto& [item, data] : inventory) {
        if (item && data.first > 0 && IsPureApparel(item) && data.second) {
            auto* bound = item->As<RE::TESBoundObject>();
            if (!bound) continue;

            auto* entryData = data.second.get();
            bool foundExtraWorn = false;
            if (entryData && entryData->extraLists) {
                for (auto* extraList : *entryData->extraLists) {
                    if (extraList && (extraList->HasType(RE::ExtraDataType::kWorn) || extraList->HasType(RE::ExtraDataType::kWornLeft))) {
                        InventoryWornEntry entry;
                        entry.object = bound;
                        entry.extraList = extraList;
                        equipped.push_back(entry);
                        foundExtraWorn = true;
                        addedFormIDs.insert(bound->GetFormID());
                    }
                }
            }
            if (!foundExtraWorn && (entryData->IsWorn() || bipedWornFormIDs.contains(bound->GetFormID()))) {
                InventoryWornEntry entry;
                entry.object = bound;
                entry.extraList = nullptr;
                equipped.push_back(entry);
                addedFormIDs.insert(bound->GetFormID());
            }
        }
    }

    for (RE::FormID fID : bipedWornFormIDs) {
        if (addedFormIDs.find(fID) == addedFormIDs.end()) {
            auto* form = RE::TESForm::LookupByID(fID);
            if (form && IsPureApparel(form)) {
                auto* bound = form->As<RE::TESBoundObject>();
                if (bound) {
                    InventoryWornEntry entry;
                    entry.object = bound;
                    entry.extraList = nullptr;
                    equipped.push_back(entry);
                    addedFormIDs.insert(fID);
                }
            }
        }
    }

    return equipped;
}

std::string GetTrueItemDisplayName(RE::ExtraDataList* a_xList, RE::TESBoundObject* a_boundObj) {
    if (!a_boundObj) return "Apparel";
    if (a_xList) {
        const char* dispName = a_xList->GetDisplayName(a_boundObj);
        if (dispName && dispName[0] != '\0') {
            return std::string(dispName);
        }
    }
    const char* baseName = a_boundObj->GetName();
    return (baseName && baseName[0] != '\0') ? std::string(baseName) : "Apparel";
}

std::string GetTrueItemCustomName(RE::ExtraDataList* a_xList) {
    if (!a_xList) return "";
    auto* xText = a_xList->GetExtraTextDisplayData();
    if (xText && xText->displayName.c_str() && xText->displayName.c_str()[0] != '\0') {
        return std::string(xText->displayName.c_str());
    }
    return "";
}

RE::EnchantmentItem* GetTrueItemEnchantment(RE::ExtraDataList* a_xList, RE::TESBoundObject* a_boundObj) {
    if (a_xList) {
        auto* xEnch = a_xList->GetByType<RE::ExtraEnchantment>();
        if (xEnch && xEnch->enchantment) {
            return xEnch->enchantment;
        }
        auto* xRef = a_xList->GetByType<RE::ExtraReferenceHandle>();
        auto ref = xRef ? xRef->GetOriginalReference() : nullptr;
        if (ref && !ref->IsDeleted()) {
            auto* refEnch = ref->extraList.GetByType<RE::ExtraEnchantment>();
            if (refEnch && refEnch->enchantment) {
                return refEnch->enchantment;
            }
        }
    }
    if (a_boundObj) {
        auto* arm = a_boundObj->As<RE::TESObjectARMO>();
        if (arm && arm->formEnchanting) {
            return arm->formEnchanting;
        }
        auto* wep = a_boundObj->As<RE::TESObjectWEAP>();
        if (wep && wep->formEnchanting) {
            return wep->formEnchanting;
        }
    }
    return nullptr;
}

bool MatchesWornEntry(const InventoryWornEntry& entry, const OutfitItem& target) {
    if (!entry.object || entry.object->GetFormID() != target.formID) return false;

    auto* boundObj = entry.object->As<RE::TESBoundObject>();
    auto* wornEnch = GetTrueItemEnchantment(entry.extraList, boundObj);
    RE::FormID wornEnchID = wornEnch ? wornEnch->GetFormID() : 0;

    if (target.enchantmentID != 0) {
        if (wornEnchID != target.enchantmentID) return false;
    } else {
        if (entry.extraList) {
            auto* xEnch = entry.extraList->GetByType<RE::ExtraEnchantment>();
            if (xEnch && xEnch->enchantment && xEnch->enchantment->GetFormID() != 0) {
                return false;
            }
        }
    }

    if (target.uniqueID != 0 && entry.extraList) {
        auto* xUID = entry.extraList->GetByType<RE::ExtraUniqueID>();
        if (xUID && xUID->uniqueID != target.uniqueID) return false;
    }

    if (!target.customName.empty() && entry.extraList) {
        std::string wornCustom = GetTrueItemCustomName(entry.extraList);
        if (!wornCustom.empty() && wornCustom != target.customName) return false;
    }

    if (entry.extraList) {
        bool entryIsLeft = entry.extraList->HasType(RE::ExtraDataType::kWornLeft);
        if (target.isWornLeft != entryIsLeft && (entry.extraList->HasType(RE::ExtraDataType::kWorn) || entryIsLeft)) {
            return false;
        }
    }

    return true;
}

RE::ExtraDataList* FindBestMatchingExtraList(RE::InventoryEntryData* entryData, const OutfitItem& target, RE::TESBoundObject* boundObj) {
    if (!entryData || !entryData->extraLists || entryData->extraLists->empty()) {
        return nullptr;
    }

    RE::ExtraDataList* bestCandidate = nullptr;
    int highestScore = -1;

    for (auto* xList : *entryData->extraLists) {
        if (!xList || xList->HasType(RE::ExtraDataType::kWorn) || xList->HasType(RE::ExtraDataType::kWornLeft)) {
            continue;
        }

        auto* candEnch = GetTrueItemEnchantment(xList, boundObj);
        RE::FormID candEnchID = candEnch ? candEnch->GetFormID() : 0;

        if (target.enchantmentID != 0) {
            if (candEnchID != target.enchantmentID) {
                continue;
            }
        } else {
            if (candEnchID != 0) {
                continue;
            }
        }

        int score = 100;

        if (target.uniqueID != 0) {
            auto* xUID = xList->GetByType<RE::ExtraUniqueID>();
            if (xUID && xUID->uniqueID == target.uniqueID) {
                score += 20000;
            }
        }

        std::string candCustom = GetTrueItemCustomName(xList);
        if (!target.customName.empty()) {
            if (candCustom == target.customName) {
                score += 10000;
            } else if (candCustom.empty()) {
                score += 1000;
            } else {
                continue;
            }
        } else {
            if (candCustom.empty()) {
                score += 5000;
            } else {
                score += 500;
            }
        }

        if (target.enchantmentID != 0 && candEnchID == target.enchantmentID) {
            score += 5000;
        }

        if (score > highestScore) {
            highestScore = score;
            bestCandidate = xList;
        }
    }

    if (!bestCandidate && target.enchantmentID == 0) {
        for (auto* xList : *entryData->extraLists) {
            if (xList && !xList->HasType(RE::ExtraDataType::kWorn) && !xList->HasType(RE::ExtraDataType::kWornLeft)) {
                auto* candEnch = GetTrueItemEnchantment(xList, boundObj);
                if (!candEnch) {
                    return xList;
                }
            }
        }
    }

    return bestCandidate;
}

void AutoStashInactiveSlotItems(RE::Actor* a_actor) {
    if (!g_autoStashEnabled) return;
    auto* targetActor = a_actor ? a_actor : ResolveActiveTargetActor();
    if (!targetActor || !targetActor->Is3DLoaded() || targetActor->IsDead()) return;

    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || (targetActor != player && !targetActor->IsPlayerRef())) return;

    auto* wardrobe = GetWardrobeContainer();
    if (!wardrobe) return;

    std::unordered_set<RE::FormID> uniqueRegisteredIDs;
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        RE::FormID canonicalID = (targetActor == RE::PlayerCharacter::GetSingleton()) ? 0x00000007 : targetActor->GetFormID();
        const auto& profile = g_manager.GetProfile(canonicalID);
        for (const auto& wheel : profile.wheels) {
            for (const auto& slot : wheel.slots) {
                if (slot.outfit.has_value()) {
                    for (RE::FormID fID : slot.outfit->itemIDs) {
                        if (fID != 0) uniqueRegisteredIDs.insert(fID);
                    }
                    for (const auto& itm : slot.outfit->items) {
                        if (itm.formID != 0) uniqueRegisteredIDs.insert(itm.formID);
                    }
                }
            }
        }
    }

    if (uniqueRegisteredIDs.empty()) return;

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
                        if (it->second.second && it->second.second->extraLists && !it->second.second->extraLists->empty()) {
                            std::vector<RE::ExtraDataList*> xListsToMove;
                            for (auto* xList : *it->second.second->extraLists) {
                                if (xList && SafeValidateAndCleanExtraData(xList) == 1 && HasMeaningfulContainerExtraData(xList)) {
                                    SafeStripContainerWornFlags(xList);
                                    xListsToMove.push_back(xList);
                                }
                            }
                            for (auto* xList : xListsToMove) {
                                targetActor->RemoveItem(boundObj, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, xList, wardrobe);
                                count--;
                                if (count <= 0) break;
                            }
                        }
                        if (count > 0) {
                            targetActor->RemoveItem(boundObj, count, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, wardrobe);
                        }
                    }
                }
            }
        }
    }
    SanitizeWardrobeContainer();
}

void PerformOutfitSwap(RE::Actor* targetActor, RE::ActorEquipManager* equipManager, const std::vector<OutfitItem>& targetItems) {
    if (!targetActor || !equipManager || !targetActor->Is3DLoaded() || targetActor->IsDead()) return;

    auto actorState = targetActor->AsActorState();
    if (actorState && (actorState->IsBleedingOut() || targetActor->IsInKillMove())) {
        return;
    }

    if (!targetActor->IsPlayerRef()) {
        auto* base = targetActor->GetActorBase();
        auto npcKw = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("ActorTypeNPC");
        if (base && npcKw && !base->HasKeyword(npcKw) && !targetActor->HasKeyword(npcKw)) {
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

    std::unordered_set<RE::FormID> registeredWheelIDs;
    if (g_autoStashEnabled && wardrobe) {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        RE::FormID canonicalID = (targetActor == player) ? 0x00000007 : targetActor->GetFormID();
        const auto& profile = g_manager.GetProfile(canonicalID);
        for (const auto& wheel : profile.wheels) {
            for (const auto& slot : wheel.slots) {
                if (slot.outfit.has_value()) {
                    for (RE::FormID fID : slot.outfit->itemIDs) {
                        if (fID != 0) registeredWheelIDs.insert(fID);
                    }
                    for (const auto& itm : slot.outfit->items) {
                        if (itm.formID != 0) registeredWheelIDs.insert(itm.formID);
                    }
                }
            }
        }
    }

    if (targetActor != player && targetActor->IsPlayerTeammate()) {
        auto* base = targetActor->GetActorBase();
        if (base) {
            base->defaultOutfit = nullptr;
            base->sleepOutfit = nullptr;
        }
    }

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
                RE::ExtraDataList* xList = FindBestMatchingExtraList(it->second.second.get(), itm, boundObj);
                wardrobe->RemoveItem(boundObj, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, xList, targetActor);
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
                    RE::ExtraDataList* xList = FindBestMatchingExtraList(playerIt->second.second.get(), itm, boundObj);
                    player->RemoveItem(boundObj, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, xList, targetActor);
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
            if (itm.enchantmentID != 0 && !targetExtra) {
                auto* baseEnch = GetTrueItemEnchantment(nullptr, boundObj);
                if (!baseEnch || baseEnch->GetFormID() != itm.enchantmentID) {
                    SKSE::log::warn("PerformOutfitSwap: Custom enchanted item {:08X} (ench {:08X}) not found with extra data, equipping base instance", itm.formID, itm.enchantmentID);
                }
            }
            const RE::BGSEquipSlot* targetSlot = nullptr;
            if (itm.isWornLeft) {
                targetSlot = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x00013F44); // LeftHand
            }
            equipManager->EquipObject(targetActor, boundObj, targetExtra, 1, targetSlot, false, false, true, true);
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

        size_t ringCount = 0;
        for (const auto& entry : equipped) {
            if (entry.object && IsPureApparel(entry.object)) {
                OutfitItem itm;
                itm.formID = entry.object->GetFormID();

                bool isRing = false;
                if (auto* arm = entry.object ? entry.object->As<RE::TESObjectARMO>() : nullptr) {
                    if (arm->HasPartOf(RE::BIPED_MODEL::BipedObjectSlot::kRing)) {
                        isRing = true;
                    }
                }

                if (entry.extraList) {
                    if (entry.extraList->HasType(RE::ExtraDataType::kWornLeft)) {
                        itm.isWornLeft = true;
                    }

                    auto* xUID = entry.extraList->GetByType<RE::ExtraUniqueID>();
                    if (xUID) {
                        itm.uniqueID = xUID->uniqueID;
                    }

                    auto* bound = entry.object ? entry.object->As<RE::TESBoundObject>() : nullptr;
                    auto* ench = GetTrueItemEnchantment(entry.extraList, bound);
                    if (ench) {
                        itm.enchantmentID = ench->GetFormID();
                    }

                    itm.customName = GetTrueItemCustomName(entry.extraList);
                }

                if (isRing) {
                    if (ringCount > 0) {
                        itm.isWornLeft = true;
                    }
                    ringCount++;
                }

                if (itm.enchantmentID == 0 && entry.object) {
                    auto* bound = entry.object->As<RE::TESBoundObject>();
                    auto* ench = GetTrueItemEnchantment(nullptr, bound);
                    if (ench) {
                        itm.enchantmentID = ench->GetFormID();
                    }
                }

                if (entry.object) {
                    if (auto* bound = entry.object->As<RE::TESBoundObject>()) {
                        OutfitWheeler::Thumbnail::ItemThumbnailService::GetSingleton().QueueItem(bound);
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
    if (now - g_lastEquipTimestamp.load(std::memory_order_relaxed) < 250) {
        return;
    }
    g_lastEquipTimestamp.store(now, std::memory_order_relaxed);

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
                        }
                        break;
                    }
                }
            }
            if (targetActor == player) {
                AutoStashInactiveSlotItems(targetActor);
            }
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
                        RE::ExtraDataList* xList = FindBestMatchingExtraList(it->second.second.get(), itm, boundObj);
                        wardrobe->RemoveItem(boundObj, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, xList, targetActor);
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
                            RE::ExtraDataList* xList = FindBestMatchingExtraList(playerIt->second.second.get(), itm, boundObj);
                            player->RemoveItem(boundObj, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, xList, targetActor);
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
                    if (itm.enchantmentID != 0 && !targetExtra) {
                        auto* baseEnch = GetTrueItemEnchantment(nullptr, boundObj);
                        if (!baseEnch || baseEnch->GetFormID() != itm.enchantmentID) {
                            SKSE::log::warn("EquipSlotImpl (Addon): Custom enchanted item {:08X} not found with extra data, equipping base instance", itm.formID);
                        }
                    }
                    const RE::BGSEquipSlot* targetSlot = nullptr;
                    if (itm.isWornLeft) {
                        targetSlot = RE::TESForm::LookupByID<RE::BGSEquipSlot>(0x00013F44); // LeftHand
                    }
                    equipManager->EquipObject(targetActor, boundObj, targetExtra, 1, targetSlot, false, false, true, true);
                }
            }

            AutoStashInactiveSlotItems(targetActor);
            RE::DebugNotification(("Equipped Addon " + targetOutfit.name).c_str());
        }
        return;
    }

    size_t regularMatches = 0;
    for (const auto& itm : targetItems) {
        for (const auto& entry : currentlyWornEntries) {
            if (MatchesWornEntry(entry, itm)) {
                regularMatches++;
                break;
            }
        }
    }

    std::string clickedOutfitID = std::to_string(wheelIdx) + "_" + std::to_string(slotIdx);
    bool isCurrentlyActive = (AdaptiveWardrobeManager::GetSingleton()->GetCurrentOutfitID(targetActorID) == clickedOutfitID && regularMatches > 0);
    bool allRegularWorn = (!targetItems.empty() && (regularMatches == targetItems.size() || isCurrentlyActive));
    if (allRegularWorn) {
        for (const auto& entry : currentlyWornEntries) {
            if (!entry.object || !IsPureApparel(entry.object)) continue;
            for (const auto& itm : targetItems) {
                if (MatchesWornEntry(entry, itm) || (entry.object->GetFormID() == itm.formID)) {
                    auto* bound = entry.object->As<RE::TESBoundObject>();
                    if (bound) {
                        equipManager->UnequipObject(targetActor, bound, entry.extraList, 1, nullptr, false, false, true, true);
                    }
                    break;
                }
            }
        }
        ContextFlags curCtx = ExecutionQueue::GetSingleton()->GatherCurrentContext();
        AdaptiveWardrobeManager::GetSingleton()->SetCurrentOutfitID(targetActorID, "");
        AdaptiveWardrobeManager::GetSingleton()->SetManualOverride(targetActorID, curCtx);
        RE::DebugNotification(("Unequipped " + targetOutfit.name).c_str());
        return;
    }

    PerformOutfitSwap(targetActor, equipManager, targetItems);

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

    std::vector<OutfitItem> slotOutfitItems;
    std::string outfitName = "Outfit";
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        const auto& wheels = g_manager.GetActiveWheels();
        if (wheelIdx < wheels.size() && slotIdx < wheels[wheelIdx].slots.size()) {
            if (wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
                const auto& o = *wheels[wheelIdx].slots[slotIdx].outfit;
                outfitName = o.name;
                slotOutfitItems = o.items;
                if (slotOutfitItems.empty() && !o.itemIDs.empty()) {
                    for (const auto& fID : o.itemIDs) {
                        OutfitItem itm;
                        itm.formID = fID;
                        slotOutfitItems.push_back(itm);
                    }
                }
            }
        }
    }
    if (slotOutfitItems.empty()) return;

    auto currentlyWorn = GetEquippedInventoryEntries(targetActor);
    for (const auto& targetItem : slotOutfitItems) {
        for (const auto& entry : currentlyWorn) {
            if (entry.object && IsPureApparel(entry.object) && MatchesWornEntry(entry, targetItem)) {
                auto* bound = entry.object->As<RE::TESBoundObject>();
                if (bound) {
                    equipManager->UnequipObject(targetActor, bound, entry.extraList, 1, nullptr, false, false, true, true);
                    break;
                }
            }
        }
        auto* form = RE::TESForm::LookupByID(targetItem.formID);
        if (form && IsPureApparel(form)) {
            auto* bound = form->As<RE::TESBoundObject>();
            if (bound) {
                auto actorInv = targetActor->GetInventory();
                auto it = actorInv.find(bound);
                if (it != actorInv.end() && it->second.first > 0) {
                    RE::ExtraDataList* xList = FindBestMatchingExtraList(it->second.second.get(), targetItem, bound);
                    if (xList) {
                        SafeStripContainerWornFlags(xList);
                        if (!HasMeaningfulContainerExtraData(xList) || SafeValidateAndCleanExtraData(xList) != 1) {
                            xList = nullptr;
                        }
                    }
                    targetActor->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, xList, wardrobe);
                }
            }
        }
    }
    SanitizeWardrobeContainer();

    RE::DebugNotification(("Stored " + outfitName + " in wardrobe!").c_str());
}

void RetrieveSlotOutfit(size_t wheelIdx, size_t slotIdx) {
    auto targetActor = ResolveActiveTargetActor();
    auto* wardrobe = GetWardrobeContainer();
    if (!targetActor || !wardrobe) return;

    std::vector<OutfitItem> slotOutfitItems;
    std::string outfitName = "Outfit";
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        const auto& wheels = g_manager.GetActiveWheels();
        if (wheelIdx < wheels.size() && slotIdx < wheels[wheelIdx].slots.size()) {
            if (wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
                const auto& o = *wheels[wheelIdx].slots[slotIdx].outfit;
                outfitName = o.name;
                slotOutfitItems = o.items;
                if (slotOutfitItems.empty() && !o.itemIDs.empty()) {
                    for (const auto& fID : o.itemIDs) {
                        OutfitItem itm;
                        itm.formID = fID;
                        slotOutfitItems.push_back(itm);
                    }
                }
            }
        }
    }
    if (slotOutfitItems.empty()) return;

    for (const auto& targetItem : slotOutfitItems) {
        auto* form = RE::TESForm::LookupByID(targetItem.formID);
        if (form && IsPureApparel(form)) {
            auto* bound = form->As<RE::TESBoundObject>();
            if (bound) {
                auto wardrobeInv = wardrobe->GetInventory();
                auto it = wardrobeInv.find(bound);
                if (it != wardrobeInv.end() && it->second.first > 0) {
                    RE::ExtraDataList* xList = FindBestMatchingExtraList(it->second.second.get(), targetItem, bound);
                    wardrobe->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, xList, targetActor);
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

    std::vector<OutfitItem> wheelItems;
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        const auto& wheels = g_manager.GetActiveWheels();
        if (wheelIdx < wheels.size()) {
            for (const auto& slot : wheels[wheelIdx].slots) {
                if (slot.outfit.has_value()) {
                    if (!slot.outfit->items.empty()) {
                        for (const auto& itm : slot.outfit->items) {
                            wheelItems.push_back(itm);
                        }
                    } else {
                        for (RE::FormID fID : slot.outfit->itemIDs) {
                            OutfitItem itm;
                            itm.formID = fID;
                            wheelItems.push_back(itm);
                        }
                    }
                }
            }
        }
    }
    if (wheelItems.empty()) return;

    auto currentlyWorn = GetEquippedInventoryEntries(targetActor);
    for (const auto& targetItem : wheelItems) {
        bool isWorn = false;
        for (const auto& entry : currentlyWorn) {
            if (entry.object && MatchesWornEntry(entry, targetItem)) {
                isWorn = true;
                break;
            }
        }
        if (!isWorn) {
            auto* form = RE::TESForm::LookupByID(targetItem.formID);
            if (form && IsPureApparel(form)) {
                auto* boundObj = form->As<RE::TESBoundObject>();
                if (boundObj) {
                    auto actorInv = targetActor->GetInventory();
                    auto it = actorInv.find(boundObj);
                    if (it != actorInv.end() && it->second.first > 0) {
                        RE::ExtraDataList* xList = FindBestMatchingExtraList(it->second.second.get(), targetItem, boundObj);
                        if (xList) {
                            SafeStripContainerWornFlags(xList);
                            if (!HasMeaningfulContainerExtraData(xList) || SafeValidateAndCleanExtraData(xList) != 1) {
                                xList = nullptr;
                            }
                        }
                        targetActor->RemoveItem(boundObj, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, xList, wardrobe);
                    }
                }
            }
        }
    }
    SanitizeWardrobeContainer();

    RE::DebugNotification("Wheel outfits stored in wardrobe!");
}

void RetrieveWheelOutfits(size_t wheelIdx) {
    auto targetActor = ResolveActiveTargetActor();
    auto* wardrobe = GetWardrobeContainer();
    if (!targetActor || !wardrobe) return;

    std::vector<OutfitItem> wheelItems;
    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        const auto& wheels = g_manager.GetActiveWheels();
        if (wheelIdx < wheels.size()) {
            for (const auto& slot : wheels[wheelIdx].slots) {
                if (slot.outfit.has_value()) {
                    if (!slot.outfit->items.empty()) {
                        for (const auto& itm : slot.outfit->items) {
                            wheelItems.push_back(itm);
                        }
                    } else {
                        for (RE::FormID fID : slot.outfit->itemIDs) {
                            OutfitItem itm;
                            itm.formID = fID;
                            wheelItems.push_back(itm);
                        }
                    }
                }
            }
        }
    }
    if (wheelItems.empty()) return;

    for (const auto& targetItem : wheelItems) {
        auto* form = RE::TESForm::LookupByID(targetItem.formID);
        if (form && IsPureApparel(form)) {
            auto* bound = form->As<RE::TESBoundObject>();
            if (bound) {
                auto wardrobeInv = wardrobe->GetInventory();
                auto it = wardrobeInv.find(bound);
                if (it != wardrobeInv.end() && it->second.first > 0) {
                    RE::ExtraDataList* xList = FindBestMatchingExtraList(it->second.second.get(), targetItem, bound);
                    wardrobe->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, xList, targetActor);
                }
            }
        }
    }

    RE::DebugNotification("Retrieved wheel outfits to inventory!");
}