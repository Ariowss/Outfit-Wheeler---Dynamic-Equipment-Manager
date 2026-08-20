#include "JSInterop.h"
#include "Settings.h"
#include "OutfitManager.h"
#include "EquipmentManager.h"
#include "AdaptiveWardrobeManager.h"
#include "InputHandler.h"
#include <SKSE/SKSE.h>

extern PRISMA_UI_API::IVPrismaUI2* PrismaUI;
extern PrismaView view;

void OnDomReadyCallbackFunc(PrismaView v) {
    SKSE::log::info("OnDomReadyCallbackFunc fired for View ID: {}", v);
}

void OnConsoleMessageCallbackFunc(PrismaView v, PRISMA_UI_API::ConsoleMessageLevel level, const char* message) {
    SKSE::log::info("[JS CONSOLE] [Level {}] [View {}]: {}", static_cast<uint8_t>(level), v, message ? message : "");
}

void SaveSettingsListener(const char* data) {
    try {
        json req = json::parse(data);
        bool editMode = req.value("editMode", false);
        if (req.contains("wheelScale")) g_wheelScale = req.value("wheelScale", 1.0f);
        if (req.contains("wheelOffsetX")) g_wheelOffsetX = req.value("wheelOffsetX", 0.0f);
        if (req.contains("editKey")) g_editKey = req.value("editKey", 0x31);
        if (req.contains("editModifierKey")) g_editModifierKey = req.value("editModifierKey", 0x00);
        if (req.contains("playKey")) g_playKey = req.value("playKey", 0x2F);
        if (req.contains("playModifierKey")) g_playModifierKey = req.value("playModifierKey", 0x00);

        if (req.contains("gamepadPlayKey")) g_gamepadPlayKey = req.value("gamepadPlayKey", 0x0008);
        if (req.contains("gamepadPlayModifier")) g_gamepadPlayModifier = req.value("gamepadPlayModifier", 0x0100);
        if (req.contains("thumbstickDeadzone")) g_thumbstickDeadzone = req.value("thumbstickDeadzone", 0.25f);

        if (req.contains("autoStashEnabled")) g_autoStashEnabled = req.value("autoStashEnabled", false);
        if (req.contains("autoSystemEnabled")) g_autoSystemEnabled = req.value("autoSystemEnabled", true);
        if (req.contains("autoCityEnabled")) g_autoCityEnabled = req.value("autoCityEnabled", true);
        if (req.contains("autoHomeEnabled")) g_autoHomeEnabled = req.value("autoHomeEnabled", true);
        if (req.contains("autoCombatEnabled")) g_autoCombatEnabled = req.value("autoCombatEnabled", true);
        if (req.contains("autoSwimEnabled")) g_autoSwimEnabled = req.value("autoSwimEnabled", true);

        SaveINI();

        SKSE::GetTaskInterface()->AddTask([editMode]() {
            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJson(editMode);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                RE::DebugNotification("Settings Saved!");
            }
        });
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in SaveSettings listener: {}", e.what());
    }
}

void SetTypingModeListener(const char* data) {
    try {
        json req = json::parse(data);
        g_isTyping.store(req.value("typing", false), std::memory_order_release);
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in SetTypingMode listener: {}", e.what());
    }
}

void SaveOutfitListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx]() {
            SaveOutfitToSlotImpl(wheelIdx, slotIdx);
            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJson(true);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in SaveOutfitToSlot listener: {}", e.what());
    }
}

void RenameSlotListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);
        std::string newName = req.value("newName", "");

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx, newName]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            if (wheelIdx < g_manager.wheels.size() && slotIdx < g_manager.wheels[wheelIdx].slots.size()) {
                if (g_manager.wheels[wheelIdx].slots[slotIdx].outfit.has_value() && !newName.empty()) {
                    g_manager.wheels[wheelIdx].slots[slotIdx].outfit->name = newName;
                    SaveDataInternal();
                    RE::DebugNotification(("Renamed outfit to " + newName).c_str());
                }
                if (PrismaUI && view != 0) {
                    std::string jsCmd = GetWheelJsonInternal(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                }
            }
        });
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in RenameSlot listener: {}", e.what());
    }
}

void RenameWheelListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        std::string newName = req.value("newName", "");

        SKSE::GetTaskInterface()->AddTask([wheelIdx, newName]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            if (wheelIdx < g_manager.wheels.size() && !newName.empty()) {
                g_manager.wheels[wheelIdx].name = newName;
                SaveDataInternal();
                RE::DebugNotification(("Renamed wheel to " + newName).c_str());
                if (PrismaUI && view != 0) {
                    std::string jsCmd = GetWheelJsonInternal(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                }
            }
        });
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in RenameWheel listener: {}", e.what());
    }
}

void ClearSlotListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx]() {
            RetrieveSlotOutfit(wheelIdx, slotIdx);

            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            if (wheelIdx < g_manager.wheels.size() && slotIdx < g_manager.wheels[wheelIdx].slots.size()) {
                g_manager.wheels[wheelIdx].slots[slotIdx].outfit.reset();

                std::string thisSlotID = std::to_string(wheelIdx) + "_" + std::to_string(slotIdx);
                if (AdaptiveWardrobeManager::GetSingleton()->GetCurrentOutfitID() == thisSlotID) {
                    AdaptiveWardrobeManager::GetSingleton()->SetCurrentOutfitID("");
                }

                SaveDataInternal();
                RE::DebugNotification("Cleared outfit data and returned items to inventory");

                if (PrismaUI && view != 0) {
                    std::string jsCmd = GetWheelJsonInternal(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                }
            }
        });
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in ClearSlot listener: {}", e.what());
    }
}

void DeleteSlotListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx]() {
            RetrieveSlotOutfit(wheelIdx, slotIdx);

            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            if (wheelIdx < g_manager.wheels.size() && slotIdx < g_manager.wheels[wheelIdx].slots.size()) {
                if (g_manager.wheels[wheelIdx].slots.size() > 1) {
                    g_manager.wheels[wheelIdx].slots.erase(g_manager.wheels[wheelIdx].slots.begin() + slotIdx);
                } else {
                    g_manager.wheels[wheelIdx].slots[0].outfit.reset();
                }

                AdaptiveWardrobeManager::GetSingleton()->SetCurrentOutfitID("");
                SaveDataInternal();
                RE::DebugNotification("Deleted slot and returned items to inventory!");

                if (PrismaUI && view != 0) {
                    std::string jsCmd = GetWheelJsonInternal(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                }
            }
        });
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in DeleteSlot listener: {}", e.what());
    }
}

void EquipSlotListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx]() {
            EquipSlotImpl(wheelIdx, slotIdx);
            if (PrismaUI && view != 0) {
                PrismaUI->Hide(view);
                isMenuOpen.store(false, std::memory_order_release);
            }
        });
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in EquipSlot listener: {}", e.what());
    }
}

void OnCloseWheelListener(const char*) {
    SKSE::GetTaskInterface()->AddTask([]() {
        HideWheel();
    });
}

void ChangeWheelListener(const char* data) {
    try {
        json req = json::parse(data);
        int delta = req.value("delta", 0);
        bool editMode = req.value("editMode", false);

        SKSE::GetTaskInterface()->AddTask([delta, editMode]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            if (g_manager.wheels.empty()) return;
            int current = static_cast<int>(g_manager.activeWheelIndex);
            int total = static_cast<int>(g_manager.wheels.size());
            if (total <= 0) return;
            current = (current + delta) % total;
            if (current < 0) current += total;
            g_manager.activeWheelIndex = static_cast<size_t>(current);
            SaveDataInternal();

            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJsonInternal(editMode);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in ChangeWheel listener: {}", e.what());
    }
}

void AddWheelListener(const char* data) {
    bool editMode = true;
    try {
        if (data && data[0] != '\0') {
            json req = json::parse(data);
            editMode = req.value("editMode", true);
        }
    } catch (...) {}

    SKSE::GetTaskInterface()->AddTask([editMode]() {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        Wheel w;
        w.name = "Wheel " + std::to_string(g_manager.wheels.size() + 1);
        for (size_t i = 0; i < 4; ++i) {
            Slot s;
            w.slots.push_back(s);
        }
        g_manager.wheels.push_back(w);
        g_manager.activeWheelIndex = g_manager.wheels.size() - 1;
        SaveDataInternal();

        if (PrismaUI && view != 0) {
            std::string jsCmd = GetWheelJsonInternal(editMode);
            PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            RE::DebugNotification("Created new wheel!");
        }
    });
}

void AddSlotListener(const char* data) {
    bool editMode = true;
    try {
        if (data && data[0] != '\0') {
            json req = json::parse(data);
            editMode = req.value("editMode", true);
        }
    } catch (...) {}

    SKSE::GetTaskInterface()->AddTask([editMode]() {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        if (!g_manager.wheels.empty() && g_manager.activeWheelIndex < g_manager.wheels.size()) {
            Slot s;
            g_manager.wheels[g_manager.activeWheelIndex].slots.push_back(s);
            SaveDataInternal();

            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJsonInternal(editMode);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                RE::DebugNotification("Added slot!");
            }
        }
    });
}

void DeleteWheelListener(const char*) {
    SKSE::GetTaskInterface()->AddTask([]() {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        if (g_manager.wheels.size() > 1) {
            size_t delIdx = g_manager.activeWheelIndex;
            RetrieveWheelOutfits(delIdx);

            g_manager.wheels.erase(g_manager.wheels.begin() + delIdx);
            if (g_manager.activeWheelIndex >= g_manager.wheels.size()) {
                g_manager.activeWheelIndex = g_manager.wheels.size() - 1;
            }

            AdaptiveWardrobeManager::GetSingleton()->SetCurrentOutfitID("");
            SaveDataInternal();

            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJsonInternal(true);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                RE::DebugNotification("Deleted wheel and returned items to inventory");
            }
        } else {
            RE::DebugNotification("Cannot delete the only wheel!");
        }
    });
}

void SetSlotContextListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);
        std::string tag = req.value("contextTag", "");

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx, tag]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            if (wheelIdx < g_manager.wheels.size() && slotIdx < g_manager.wheels[wheelIdx].slots.size()) {
                if (g_manager.wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
                    g_manager.wheels[wheelIdx].slots[slotIdx].outfit->contextTag = tag;
                    SaveDataInternal();
                    RE::DebugNotification(("Context Tag set to: " + (tag.empty() ? "None" : tag)).c_str());
                }
                if (PrismaUI && view != 0) {
                    std::string jsCmd = GetWheelJsonInternal(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                }
            }
        });
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in SetSlotContext listener: {}", e.what());
    }
}

void SetSlotRatingListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);
        uint8_t stars = req.value("ratingStars", static_cast<uint8_t>(3));

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx, stars]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            if (wheelIdx < g_manager.wheels.size() && slotIdx < g_manager.wheels[wheelIdx].slots.size()) {
                if (g_manager.wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
                    g_manager.wheels[wheelIdx].slots[slotIdx].outfit->preferenceStars = stars;
                    SaveDataInternal();
                    RE::DebugNotification(("Rating set to " + std::to_string(stars) + " Stars").c_str());
                }
                if (PrismaUI && view != 0) {
                    std::string jsCmd = GetWheelJsonInternal(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                }
            }
        });
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in SetSlotRating listener: {}", e.what());
    }
}

void RetrieveAllOutfitsListener(const char*) {
    SKSE::GetTaskInterface()->AddTask([]() {
        RetrieveAllOutfitsToPlayer();
        if (PrismaUI && view != 0) {
            std::string jsCmd = GetWheelJsonInternal(true);
            PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
        }
    });
}

void StashAllOutfitsListener(const char*) {
    SKSE::GetTaskInterface()->AddTask([]() {
        StashAllOutfitsToWardrobe();
        if (PrismaUI && view != 0) {
            std::string jsCmd = GetWheelJsonInternal(true);
            PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
        }
    });
}

void StashSlotOutfitListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx]() {
            StashSlotOutfit(wheelIdx, slotIdx);
            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJson(true);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in StashSlotOutfit listener: {}", e.what());
    }
}

void RetrieveSlotOutfitListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx]() {
            RetrieveSlotOutfit(wheelIdx, slotIdx);
            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJson(true);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in RetrieveSlotOutfit listener: {}", e.what());
    }
}

void StashWheelOutfitsListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);

        SKSE::GetTaskInterface()->AddTask([wheelIdx]() {
            StashWheelOutfits(wheelIdx);
            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJson(true);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in StashWheelOutfits listener: {}", e.what());
    }
}

void RetrieveWheelOutfitsListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);

        SKSE::GetTaskInterface()->AddTask([wheelIdx]() {
            RetrieveWheelOutfits(wheelIdx);
            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJson(true);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (const std::exception& e) {
        SKSE::log::error("Exception in RetrieveWheelOutfits listener: {}", e.what());
    }
}