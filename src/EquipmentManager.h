#ifndef EQUIPMENT_MANAGER_H
#define EQUIPMENT_MANAGER_H

#include <RE/Skyrim.h>
#include <vector>

struct InventoryWornEntry {
    RE::TESBoundObject* object = nullptr;
    RE::ExtraDataList* extraList = nullptr;
};

std::vector<InventoryWornEntry> GetEquippedInventoryEntries();
void PerformOutfitSwap(RE::PlayerCharacter* player, RE::ActorEquipManager* equipManager, const std::vector<RE::FormID>& targetItemIDs);
void SaveOutfitToSlotImpl(size_t wheelIdx, size_t slotIdx);
void EquipSlotImpl(size_t wheelIdx, size_t slotIdx);

#endif