#pragma once

#include <RE/Skyrim.h>
#include <vector>

struct InventoryWornEntry {
    RE::TESForm* object{ nullptr };
    RE::ExtraDataList* extraList{ nullptr };
};

bool IsPureApparel(RE::TESForm* a_form) noexcept;
RE::Actor* ResolveActiveTargetActor();
bool IsItemInAnyWheelOutfit(RE::FormID a_formID);
RE::TESObjectREFR* GetWardrobeContainer();
std::vector<InventoryWornEntry> GetEquippedInventoryEntries(RE::Actor* a_actor);

void PerformOutfitSwap(RE::Actor* targetActor, RE::ActorEquipManager* equipManager, const std::vector<RE::FormID>& targetItemIDs);

void SaveOutfitToSlotImpl(size_t wheelIdx, size_t slotIdx);
void EquipSlotImpl(size_t wheelIdx, size_t slotIdx);
void RetrieveAllOutfitsToPlayer();
void StashAllOutfitsToWardrobe();
void StashSlotOutfit(size_t wheelIdx, size_t slotIdx);
void RetrieveSlotOutfit(size_t wheelIdx, size_t slotIdx);
void StashWheelOutfits(size_t wheelIdx);
void RetrieveWheelOutfits(size_t wheelIdx);