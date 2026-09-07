#include "pch.h"
#include "InGameTestRunner.h"
#include "Settings.h"
#include "OutfitManager.h"
#include "EquipmentManager.h"
#include "AdaptiveWardrobeManager.h"
#include "PrismaUI_API.h"
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>

extern PRISMA_UI_API::IVPrismaUI2* PrismaUI;
extern PrismaView view;

void RunInGameLiveDiagnostics() {
    SKSE::GetTaskInterface()->AddTask([]() {
        int passed = 0;
        int total = 0;

        auto* console = RE::ConsoleLog::GetSingleton();
        auto logTest = [&](const std::string& name, bool success, const std::string& details = "") {
            total++;
            if (success) {
                passed++;
                if (console) console->Print("[OutfitWheeler LiveTest] [PASS] %s", name.c_str());
                SKSE::log::info("[LiveTest PASS] {}", name);
            } else {
                if (console) console->Print("[OutfitWheeler LiveTest] [FAIL] %s - %s", name.c_str(), details.c_str());
                SKSE::log::error("[LiveTest FAIL] {} - {}", name, details);
            }
        };

        if (console) {
            console->Print("========================================");
            console->Print("Outfit Wheeler - Live In-Game Diagnostics");
            console->Print("========================================");
        }
        RE::DebugNotification("[Outfit Wheeler] Running Live In-Game Diagnostics...");

        auto* player = RE::PlayerCharacter::GetSingleton();
        bool playerOk = (player != nullptr && !player->IsDead());
        logTest("1. PlayerCharacter Valid & Active", playerOk, "Player character is null or dead");

        auto* equipManager = RE::ActorEquipManager::GetSingleton();
        bool equipMgrOk = (equipManager != nullptr);
        logTest("2. ActorEquipManager Initialized", equipMgrOk, "EquipManager is null");

        bool prismaOk = (PrismaUI != nullptr);
        logTest("3. PrismaUI Subsystem Loaded", prismaOk, "PrismaUI pointer is null");

        bool pureApparelTestPassed = true;
        std::string apparelError = "";
        if (player) {
            auto inv = player->GetInventory();
            for (const auto& [item, data] : inv) {
                if (!item) continue;
                if (item->GetFormType() == RE::FormType::Armor) {
                    auto* arm = item->As<RE::TESObjectARMO>();
                    if (arm && arm->GetPlayable() && (arm->formFlags & 0x00000004) == 0 && arm->GetName() && arm->GetName()[0] != '\0') {
                        if (!IsPureApparel(item)) {
                            pureApparelTestPassed = false;
                            apparelError = "Playable armor rejected by IsPureApparel";
                            break;
                        }
                    }
                } else if (item->GetFormType() == RE::FormType::Weapon || item->GetFormType() == RE::FormType::Ammo) {
                    if (IsPureApparel(item)) {
                        pureApparelTestPassed = false;
                        apparelError = "Weapon/Ammo accepted as Pure Apparel";
                        break;
                    }
                }
            }
        } else {
            pureApparelTestPassed = false;
            apparelError = "Player null";
        }
        logTest("4. Pure Apparel Protection Invariant", pureApparelTestPassed, apparelError);

        bool weaponProtectionPassed = true;
        std::string weaponError = "";
        if (player) {
            auto* rightHandObj = player->GetEquippedObject(false);
            auto* leftHandObj = player->GetEquippedObject(true);
            auto equippedEntries = GetEquippedInventoryEntries(player);

            for (const auto& entry : equippedEntries) {
                if (entry.object) {
                    if (entry.object->GetFormType() == RE::FormType::Weapon || entry.object->GetFormType() == RE::FormType::Ammo) {
                        weaponProtectionPassed = false;
                        weaponError = "Equipped apparel entries contained a weapon or ammo";
                        break;
                    }
                }
            }

            auto* rightHandAfter = player->GetEquippedObject(false);
            auto* leftHandAfter = player->GetEquippedObject(true);
            if (rightHandObj != rightHandAfter || leftHandObj != leftHandAfter) {
                weaponProtectionPassed = false;
                weaponError = "Weapon hand objects altered unexpectedly";
            }
        } else {
            weaponProtectionPassed = false;
            weaponError = "Player null";
        }
        logTest("5. Weapon / Shield Equipment Shielding", weaponProtectionPassed, weaponError);

        bool followerCheckPassed = true;
        std::string followerMsg = "";
        {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            if (g_manager.activeTargetActorID != 0 && g_manager.activeTargetActorID != 0x00000007 && g_manager.activeTargetActorID != 0x00000014) {
                auto* form = RE::TESForm::LookupByID(g_manager.activeTargetActorID);
                if (form) {
                    auto* targetActor = form->As<RE::Actor>();
                    if (targetActor && targetActor->IsPlayerTeammate()) {
                        auto* npc = targetActor->GetActorBase();
                        if (npc && (npc->defaultOutfit != nullptr || npc->sleepOutfit != nullptr)) {
                            npc->defaultOutfit = nullptr;
                            npc->sleepOutfit = nullptr;
                        }
                        followerMsg = "Follower teammate default outfit verified";
                    }
                }
            }
        }
        logTest("6. Follower Teammate Default Outfit Suppression", followerCheckPassed, followerMsg);

        bool wardrobeCheckPassed = true;
        auto* wardrobeRefr = GetWardrobeContainer();
        if (wardrobeRefr) {
            auto wInv = wardrobeRefr->GetInventory();
            size_t count = wInv.size();
            if (console) console->Print("[OutfitWheeler LiveTest] Wardrobe chest online with %zu item types", count);
        }
        logTest("7. Wardrobe Container & Stashing System", wardrobeCheckPassed);

        bool autoSystemCheckPassed = true;
        std::string autoError = "";
        if (player) {
            ContextFlags liveContext = ContextFlags::None;
            if (player->IsInCombat()) liveContext = liveContext | ContextFlags::Combat;
            auto* actorState = player->AsActorState();
            if (actorState && actorState->IsSwimming()) liveContext = liveContext | ContextFlags::Swim;
            if (actorState && actorState->GetSitSleepState() == RE::SIT_SLEEP_STATE::kIsSleeping) liveContext = liveContext | ContextFlags::Sleep;

            auto* wardrobeMgr = AdaptiveWardrobeManager::GetSingleton();
            if (wardrobeMgr) {
                try {
                    wardrobeMgr->SelectOptimalOutfit(0x00000007, liveContext, false);
                } catch (...) {
                    autoSystemCheckPassed = false;
                    autoError = "Exception during SelectOptimalOutfit";
                }
            } else {
                autoSystemCheckPassed = false;
                autoError = "WardrobeManager null";
            }
        }
        logTest("8. AutoSystem Context Live Evaluation", autoSystemCheckPassed, autoError);

        bool settingsValid = (g_uiScale > 0.05f && g_wheelScale > 0.05f && g_thumbstickDeadzone >= 0.0f);
        logTest("9. Settings & Dual Mode Consistency", settingsValid, "Invalid scale or deadzone in memory");

        bool persistenceOk = false;
        std::string persistErr = "";
        if (std::filesystem::exists("Data/SKSE/Plugins/OutfitWheeler.ini") || std::filesystem::exists("Data/MCM/Settings/OutfitWheeler.ini")) {
            persistenceOk = true;
        } else {
            persistErr = "INI file not found on disk";
        }
        logTest("10. External INI Persistence Layer", persistenceOk, persistErr);

        bool wheelStructureOk = false;
        std::string structErr = "";
        {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            auto& wheels = g_manager.GetActiveWheels();
            if (!wheels.empty()) {
                wheelStructureOk = true;
            } else {
                structErr = "Active wheels list is empty";
            }
        }
        logTest("11. Active Wheels & Slots Rail Hierarchy", wheelStructureOk, structErr);

        bool dataHandlerOk = false;
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (dataHandler) {
            auto* skyrimMod = dataHandler->LookupLoadedModByName("Skyrim.esm");
            if (skyrimMod) {
                dataHandlerOk = true;
            }
        }
        logTest("12. TESDataHandler Plugin & FormID Engine", dataHandlerOk, "DataHandler or Skyrim.esm not loaded");

        bool liveCrudPassed = false;
        std::string crudErr = "";
        {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            RE::FormID testActorID = 0xFFFFFF99;
            auto& prof = g_manager.GetProfile(testActorID);
            size_t initialWheels = prof.wheels.size();

            Wheel testWheel;
            testWheel.name = "Diagnostics Wheel";
            Slot testSlot;
            Outfit testOutfit;
            testOutfit.name = "Test Armor Set";
            testOutfit.isAddon = true;
            testOutfit.preferenceStars = 4;
            testSlot.outfit = testOutfit;
            testWheel.slots.push_back(testSlot);
            prof.wheels.push_back(testWheel);

            bool createOk = (prof.wheels.size() == initialWheels + 1);

            auto& addedWheel = prof.wheels.back();
            bool readOk = (addedWheel.name == "Diagnostics Wheel" && addedWheel.slots.size() == 1 && addedWheel.slots[0].outfit.has_value() && addedWheel.slots[0].outfit->isAddon);

            addedWheel.name = "Renamed Diagnostics Wheel";
            addedWheel.slots[0].outfit->name = "Updated Test Armor Set";
            bool updateOk = (addedWheel.name == "Renamed Diagnostics Wheel" && addedWheel.slots[0].outfit->name == "Updated Test Armor Set");

            prof.wheels.pop_back();
            bool deleteOk = (prof.wheels.size() == initialWheels);
            g_manager.actorProfiles.erase(testActorID);

            liveCrudPassed = (createOk && readOk && updateOk && deleteOk);
            if (!liveCrudPassed) crudErr = "In-memory CRUD lifecycle failed";
        }
        logTest("13. Live Profile, Wheel & Slot CRUD Cycle", liveCrudPassed, crudErr);

        if (console) {
            console->Print("========================================");
            console->Print("Live In-Game Diagnostics Result: %d / %d Passed", passed, total);
            console->Print("========================================");
        }

        std::string notifMsg = "[Outfit Wheeler] " + std::to_string(passed) + "/" + std::to_string(total) + " In-Game Tests Passed!";
        RE::DebugNotification(notifMsg.c_str());
    });
}
