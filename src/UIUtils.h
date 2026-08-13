#pragma once
#include <RE/Skyrim.h>

namespace Wheeler {

    inline RE::InventoryEntryData* GetSelectedInventoryItem() {
        auto ui = RE::UI::GetSingleton();
        if (!ui) return nullptr;

        auto invMenu = ui->GetMenu<RE::InventoryMenu>(RE::InventoryMenu::MENU_NAME);

        if (invMenu && invMenu->uiMovie) {
            RE::GFxValue indexVal;

            bool success = invMenu->uiMovie->GetVariable(&indexVal, "_root.Menu_mc.inventoryLists.itemList.iSelectedIndex");

            if (success && indexVal.IsNumber()) {
                std::size_t idx = static_cast<std::size_t>(indexVal.GetNumber());

                auto itemList = invMenu->GetRuntimeData().itemList;

                if (itemList && idx < itemList->items.size()) {
                    auto item = itemList->items[idx];
                    if (item) {
                        return item->data.objDesc;
                    }
                }
            }
        }
        return nullptr;
    }
}