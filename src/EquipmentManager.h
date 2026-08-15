#ifndef EQUIPMENT_MANAGER_H
#define EQUIPMENT_MANAGER_H

#include <vector>
#include <cstdint>

struct InventoryWornEntry {
    RE::TESBoundObject* object{ nullptr };
    RE::ExtraDataList* extraList{ nullptr };
};

bool IsPureApparel(RE::TESForm* a_form);
std::vector<InventoryWornEntry> GetEquippedInventoryEntries();
RE::TESObjectREFR* GetWardrobeContainer();
void PerformOutfitSwap(RE::PlayerCharacter* player, RE::ActorEquipManager* equipManager, const std::vector<RE::FormID>& targetItemIDs);
void SaveOutfitToSlotImpl(size_t wheelIdx, size_t slotIdx);
void EquipSlotImpl(size_t wheelIdx, size_t slotIdx);

void RetrieveAllOutfitsToPlayer();
void StashAllOutfitsToWardrobe();
void StashSlotOutfit(size_t wheelIdx, size_t slotIdx);
void RetrieveSlotOutfit(size_t wheelIdx, size_t slotIdx);
void StashWheelOutfits(size_t wheelIdx);
void RetrieveWheelOutfits(size_t wheelIdx);

#endif