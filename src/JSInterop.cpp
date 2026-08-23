#include "pch.h"
#include "JSInterop.h"
#include "Settings.h"
#include "OutfitManager.h"
#include "EquipmentManager.h"
#include "AdaptiveWardrobeManager.h"
#include "InputHandler.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <SKSE/SKSE.h>

extern PRISMA_UI_API::IVPrismaUI2* PrismaUI;
extern PrismaView view;

struct StudioActorTransform {
    RE::NiPoint3 position;
    float angleZ{ 0.0f };
};

static RE::NiPoint3 g_stagePosition;
static float g_stageAngleZ = 0.0f;
static RE::FormID g_currentStudioActorID = 0x00000007;
static std::unordered_map<RE::FormID, StudioActorTransform> g_originalFollowerTransforms;
static bool g_isPlayerHidden = false;

void OnDomReadyCallbackFunc(PrismaView v) {
    SKSE::log::info("OnDomReadyCallbackFunc fired for View ID: {}", v);
}

void OnConsoleMessageCallbackFunc(PrismaView v, PRISMA_UI_API::ConsoleMessageLevel level, const char* message) {
    SKSE::log::info("[JS CONSOLE] [Level {}] [View {}]: {}", static_cast<uint8_t>(level), v, message ? message : "");
}

void InitializeStudioStage() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    g_stagePosition = player->GetPosition();
    g_stageAngleZ = player->GetAngleZ();
    g_currentStudioActorID = 0x00000007;
    g_originalFollowerTransforms.clear();
    g_isPlayerHidden = false;

    auto* root = player->Get3D();
    if (root) {
        root->CullNode(false);
    }
}

void RestoreStudioActors() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (player) {
        auto* root = player->Get3D();
        if (root) {
            root->CullNode(false);
        }
    }
    g_isPlayerHidden = false;

    for (const auto& [actorID, transform] : g_originalFollowerTransforms) {
        auto* form = RE::TESForm::LookupByID(actorID);
        auto* actor = form ? form->As<RE::Actor>() : nullptr;
        if (actor) {
            actor->SetPosition(transform.position, true);
            actor->data.angle.z = transform.angleZ;
            actor->NotifyAnimationGraph("IdleStop");
        }
    }

    g_originalFollowerTransforms.clear();
    g_currentStudioActorID = 0x00000007;

    auto cam = RE::PlayerCamera::GetSingleton();
    if (cam && player) {
        cam->cameraTarget = player->GetHandle();
    }
}

void OrbitCameraListener(const char* data) {
    try {
        json req = json::parse(data);
        float deltaX = req.value("deltaX", 0.0f);
        float deltaY = req.value("deltaY", 0.0f);

        SKSE::GetTaskInterface()->AddTask([deltaX, deltaY]() {
            auto cam = RE::PlayerCamera::GetSingleton();
            if (!cam || !cam->currentState) return;
            auto tpState = skyrim_cast<RE::ThirdPersonState*>(cam->currentState.get());
            if (tpState) {
                tpState->freeRotation.x += (deltaX * 0.005f);
                tpState->freeRotation.y = std::clamp(tpState->freeRotation.y - (deltaY * 0.005f), -1.2f, 1.2f);
            }
        });
    } catch (...) {}
}

void ZoomCameraListener(const char* data) {
    try {
        json req = json::parse(data);
        float deltaZoom = req.value("deltaZoom", 0.0f);

        SKSE::GetTaskInterface()->AddTask([deltaZoom]() {
            auto cam = RE::PlayerCamera::GetSingleton();
            if (!cam || !cam->currentState) return;
            auto tpState = skyrim_cast<RE::ThirdPersonState*>(cam->currentState.get());
            if (tpState) {
                tpState->targetZoomOffset = std::clamp(tpState->targetZoomOffset + (deltaZoom * 0.08f), 0.15f, 2.5f);
            }
        });
    } catch (...) {}
}

void PanCameraListener(const char* data) {
    try {
        json req = json::parse(data);
        float moveX = req.value("moveX", 0.0f);
        float moveY = req.value("moveY", 0.0f);

        SKSE::GetTaskInterface()->AddTask([moveX, moveY]() {
            auto cam = RE::PlayerCamera::GetSingleton();
            if (!cam || !cam->currentState) return;
            auto tpState = skyrim_cast<RE::ThirdPersonState*>(cam->currentState.get());
            if (tpState) {
                tpState->freeRotation.x += (moveX * 0.05f);
                tpState->freeRotation.y = std::clamp(tpState->freeRotation.y + (moveY * 0.05f), -1.2f, 1.2f);
            }
        });
    } catch (...) {}
}

void SelectTargetActorListener(const char* data) {
    try {
        json req = json::parse(data);
        std::string idStr = req.value("id", "0x00000007");
        RE::FormID targetID = 0x00000007;

        if (idStr.rfind("0x", 0) == 0 || idStr.rfind("0X", 0) == 0) {
            try {
                targetID = static_cast<RE::FormID>(std::stoul(idStr.substr(2), nullptr, 16));
            } catch (...) {
                targetID = 0x00000007;
            }
        }

        SKSE::GetTaskInterface()->AddTask([targetID]() {
            auto player = RE::PlayerCharacter::GetSingleton();
            if (!player) return;

            {
                std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
                g_manager.activeTargetActorID = targetID;
            }

            if (targetID != g_currentStudioActorID) {
                if (g_currentStudioActorID != 0x00000007 && g_currentStudioActorID != 0x00000014) {
                    auto it = g_originalFollowerTransforms.find(g_currentStudioActorID);
                    if (it != g_originalFollowerTransforms.end()) {
                        auto* prevForm = RE::TESForm::LookupByID(g_currentStudioActorID);
                        auto* prevActor = prevForm ? prevForm->As<RE::Actor>() : nullptr;
                        if (prevActor) {
                            prevActor->SetPosition(it->second.position, true);
                            prevActor->data.angle.z = it->second.angleZ;
                            prevActor->NotifyAnimationGraph("IdleStop");
                        }
                    }
                }

                if (targetID == 0x00000007 || targetID == 0 || targetID == 0x00000014) {
                    auto* root = player->Get3D();
                    if (root) {
                        root->CullNode(false);
                    }
                    g_isPlayerHidden = false;

                    auto cam = RE::PlayerCamera::GetSingleton();
                    if (cam) {
                        cam->cameraTarget = player->GetHandle();
                        if (cam->currentState && (cam->currentState->id == RE::CameraState::kThirdPerson || cam->currentState->id == RE::CameraState::kAutoVanity)) {
                            auto* tpState = static_cast<RE::ThirdPersonState*>(cam->currentState.get());
                            if (tpState) {
                                tpState->freeRotation.x = 3.14159265f;
                                tpState->freeRotation.y = 0.0f;
                                tpState->targetZoomOffset = 0.45f;
                            }
                        }
                    }
                    g_currentStudioActorID = 0x00000007;
                } else {
                    auto* form = RE::TESForm::LookupByID(targetID);
                    auto* follower = form ? form->As<RE::Actor>() : nullptr;

                    if (follower) {
                        auto* pRoot = player->Get3D();
                        if (pRoot) {
                            pRoot->CullNode(true);
                        }
                        g_isPlayerHidden = true;

                        if (g_originalFollowerTransforms.find(targetID) == g_originalFollowerTransforms.end()) {
                            StudioActorTransform tf;
                            tf.position = follower->GetPosition();
                            tf.angleZ = follower->GetAngleZ();
                            g_originalFollowerTransforms[targetID] = tf;
                        }

                        auto actorState = follower->AsActorState();
                        if (actorState) {
                            actorState->actorState1.sitSleepState = RE::SIT_SLEEP_STATE::kNormal;
                        }

                        follower->NotifyAnimationGraph("ForceDefaultState");
                        follower->NotifyAnimationGraph("IdleStop_Instant");
                        follower->NotifyAnimationGraph("IdleForceDefaultState");

                        follower->SetPosition(g_stagePosition, true);
                        follower->data.angle.z = g_stageAngleZ;

                        auto cam = RE::PlayerCamera::GetSingleton();
                        if (cam) {
                            cam->cameraTarget = follower->GetHandle();
                            if (cam->currentState && (cam->currentState->id == RE::CameraState::kThirdPerson || cam->currentState->id == RE::CameraState::kAutoVanity)) {
                                auto* tpState = static_cast<RE::ThirdPersonState*>(cam->currentState.get());
                                if (tpState) {
                                    tpState->freeRotation.x = 3.14159265f;
                                    tpState->freeRotation.y = 0.0f;
                                    tpState->targetZoomOffset = 0.45f;
                                }
                            }
                        }
                        g_currentStudioActorID = targetID;
                    }
                }
            }

            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJsonInternal(true);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (...) {}
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
    } catch (...) {}
}

void SetTypingModeListener(const char* data) {
    try {
        json req = json::parse(data);
        g_isTyping.store(req.value("typing", false), std::memory_order_release);
    } catch (...) {}
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
    } catch (...) {}
}

void RenameSlotListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);
        std::string newName = req.value("newName", "");

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx, newName]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            auto& wheels = g_manager.GetActiveWheels();
            if (wheelIdx < wheels.size() && slotIdx < wheels[wheelIdx].slots.size()) {
                if (wheels[wheelIdx].slots[slotIdx].outfit.has_value() && !newName.empty()) {
                    wheels[wheelIdx].slots[slotIdx].outfit->name = newName;
                    SaveDataInternal();
                    RE::DebugNotification(("Renamed outfit to " + newName).c_str());
                }
                if (PrismaUI && view != 0) {
                    std::string jsCmd = GetWheelJsonInternal(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                }
            }
        });
    } catch (...) {}
}

void RenameWheelListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        std::string newName = req.value("newName", "");

        SKSE::GetTaskInterface()->AddTask([wheelIdx, newName]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            auto& wheels = g_manager.GetActiveWheels();
            if (wheelIdx < wheels.size() && !newName.empty()) {
                wheels[wheelIdx].name = newName;
                SaveDataInternal();
                RE::DebugNotification(("Renamed wheel to " + newName).c_str());
                if (PrismaUI && view != 0) {
                    std::string jsCmd = GetWheelJsonInternal(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                }
            }
        });
    } catch (...) {}
}

void ClearSlotListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx]() {
            RetrieveSlotOutfit(wheelIdx, slotIdx);

            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            auto& wheels = g_manager.GetActiveWheels();
            if (wheelIdx < wheels.size() && slotIdx < wheels[wheelIdx].slots.size()) {
                wheels[wheelIdx].slots[slotIdx].outfit.reset();

                RE::FormID canonicalID = (g_manager.activeTargetActorID == 0x00000014 || g_manager.activeTargetActorID == 0) ? 0x00000007 : g_manager.activeTargetActorID;
                std::string thisSlotID = std::to_string(wheelIdx) + "_" + std::to_string(slotIdx);
                if (AdaptiveWardrobeManager::GetSingleton()->GetCurrentOutfitID(canonicalID) == thisSlotID) {
                    AdaptiveWardrobeManager::GetSingleton()->SetCurrentOutfitID(canonicalID, "");
                }

                SaveDataInternal();
                RE::DebugNotification("Cleared outfit data and returned items to inventory");

                if (PrismaUI && view != 0) {
                    std::string jsCmd = GetWheelJsonInternal(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                }
            }
        });
    } catch (...) {}
}

void DeleteSlotListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx]() {
            RetrieveSlotOutfit(wheelIdx, slotIdx);

            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            auto& wheels = g_manager.GetActiveWheels();
            if (wheelIdx < wheels.size() && slotIdx < wheels[wheelIdx].slots.size()) {
                if (wheels[wheelIdx].slots.size() > 1) {
                    wheels[wheelIdx].slots.erase(wheels[wheelIdx].slots.begin() + slotIdx);
                } else {
                    wheels[wheelIdx].slots[0].outfit.reset();
                }

                RE::FormID canonicalID = (g_manager.activeTargetActorID == 0x00000014 || g_manager.activeTargetActorID == 0) ? 0x00000007 : g_manager.activeTargetActorID;
                AdaptiveWardrobeManager::GetSingleton()->SetCurrentOutfitID(canonicalID, "");

                SaveDataInternal();
                RE::DebugNotification("Deleted slot and returned items to inventory!");

                if (PrismaUI && view != 0) {
                    std::string jsCmd = GetWheelJsonInternal(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                }
            }
        });
    } catch (...) {}
}

void EquipSlotListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);
        bool editMode = req.value("editMode", false);

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx, editMode]() {
            EquipSlotImpl(wheelIdx, slotIdx);

            if (PrismaUI && view != 0) {
                if (editMode) {
                    std::string jsCmd = GetWheelJson(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                } else {
                    HideWheel();
                }
            }
        });
    } catch (...) {}
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
            auto& wheels = g_manager.GetActiveWheels();
            if (wheels.empty()) return;
            int current = static_cast<int>(g_manager.GetActiveWheelIndex());
            int total = static_cast<int>(wheels.size());
            if (total <= 0) return;
            current = (current + delta) % total;
            if (current < 0) current += total;
            g_manager.GetActiveWheelIndex() = static_cast<size_t>(current);
            SaveDataInternal();

            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJsonInternal(editMode);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (...) {}
}

void AddWheelListener(const char* data) {
    bool editMode = true;
    try {
        if (data && data[0] != 0) {
            json req = json::parse(data);
            editMode = req.value("editMode", true);
        }
    } catch (...) {}

    SKSE::GetTaskInterface()->AddTask([editMode]() {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        auto& wheels = g_manager.GetActiveWheels();
        Wheel w;
        w.name = "Wheel " + std::to_string(wheels.size() + 1);
        for (size_t i = 0; i < 4; ++i) {
            Slot s;
            w.slots.push_back(s);
        }
        wheels.push_back(w);
        g_manager.GetActiveWheelIndex() = wheels.size() - 1;
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
        if (data && data[0] != 0) {
            json req = json::parse(data);
            editMode = req.value("editMode", true);
        }
    } catch (...) {}

    SKSE::GetTaskInterface()->AddTask([editMode]() {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        auto& wheels = g_manager.GetActiveWheels();
        size_t activeIdx = g_manager.GetActiveWheelIndex();
        if (!wheels.empty() && activeIdx < wheels.size()) {
            Slot s;
            wheels[activeIdx].slots.push_back(s);
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
        auto& wheels = g_manager.GetActiveWheels();
        if (wheels.size() > 1) {
            size_t delIdx = g_manager.GetActiveWheelIndex();
            RetrieveWheelOutfits(delIdx);

            wheels.erase(wheels.begin() + delIdx);
            if (g_manager.GetActiveWheelIndex() >= wheels.size()) {
                g_manager.GetActiveWheelIndex() = wheels.size() - 1;
            }

            RE::FormID canonicalID = (g_manager.activeTargetActorID == 0x00000014 || g_manager.activeTargetActorID == 0) ? 0x00000007 : g_manager.activeTargetActorID;
            AdaptiveWardrobeManager::GetSingleton()->SetCurrentOutfitID(canonicalID, "");
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
            auto& wheels = g_manager.GetActiveWheels();
            if (wheelIdx < wheels.size() && slotIdx < wheels[wheelIdx].slots.size()) {
                if (wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
                    wheels[wheelIdx].slots[slotIdx].outfit->contextTag = tag;
                    SaveDataInternal();
                    RE::DebugNotification(("Context Tag set to: " + (tag.empty() ? "None" : tag)).c_str());
                }
                if (PrismaUI && view != 0) {
                    std::string jsCmd = GetWheelJsonInternal(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                }
            }
        });
    } catch (...) {}
}

void SetSlotRatingListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);
        uint8_t stars = req.value("ratingStars", static_cast<uint8_t>(3));

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx, stars]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            auto& wheels = g_manager.GetActiveWheels();
            if (wheelIdx < wheels.size() && slotIdx < wheels[wheelIdx].slots.size()) {
                if (wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
                    wheels[wheelIdx].slots[slotIdx].outfit->preferenceStars = stars;
                    SaveDataInternal();
                    RE::DebugNotification(("Rating set to " + std::to_string(stars) + " Stars").c_str());
                }
                if (PrismaUI && view != 0) {
                    std::string jsCmd = GetWheelJsonInternal(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                }
            }
        });
    } catch (...) {}
}

void SetSlotTypeListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);
        bool isAddon = req.value("isAddon", false);

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx, isAddon]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            auto& wheels = g_manager.GetActiveWheels();
            if (wheelIdx < wheels.size() && slotIdx < wheels[wheelIdx].slots.size()) {
                if (wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
                    wheels[wheelIdx].slots[slotIdx].outfit->isAddon = isAddon;
                    SaveDataInternal();
                    RE::DebugNotification(("Slot set to " + std::string(isAddon ? "Addon" : "Full Outfit")).c_str());
                }
                if (PrismaUI && view != 0) {
                    std::string jsCmd = GetWheelJsonInternal(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                }
            }
        });
    } catch (...) {}
}

void ToggleItemEquipListener(const char* data) {
    try {
        json req = json::parse(data);
        RE::FormID rawID = req.value("rawID", static_cast<RE::FormID>(0));
        if (rawID == 0) {
            std::string idStr = req.value("id", "");
            if (idStr.rfind("0x", 0) == 0 || idStr.rfind("0X", 0) == 0) {
                try { rawID = static_cast<RE::FormID>(std::stoul(idStr.substr(2), nullptr, 16)); } catch (...) { rawID = 0; }
            }
        }
        if (rawID == 0) return;

        SKSE::GetTaskInterface()->AddTask([rawID]() {
            RE::Actor* targetActor = ResolveActiveTargetActor();
            auto equipManager = RE::ActorEquipManager::GetSingleton();
            if (!targetActor || !equipManager) return;

            auto* form = RE::TESForm::LookupByID(rawID);
            if (!form || !IsPureApparel(form)) return;
            auto* bound = form->As<RE::TESBoundObject>();
            if (!bound) return;

            auto inv = targetActor->GetInventory();
            auto it = inv.find(bound);
            if (it != inv.end() && it->second.first > 0) {
                auto* entryData = it->second.second.get();
                bool isWorn = false;
                RE::ExtraDataList* targetExtra = nullptr;

                if (entryData && entryData->extraLists) {
                    for (auto* xList : *entryData->extraLists) {
                        if (xList) {
                            if (xList->HasType(RE::ExtraDataType::kWorn) || xList->HasType(RE::ExtraDataType::kWornLeft)) {
                                isWorn = true;
                                targetExtra = xList;
                                break;
                            }
                        }
                    }
                }

                if (isWorn) {
                    equipManager->UnequipObject(targetActor, bound, targetExtra, 1, nullptr, false, false, true, true);
                    RE::DebugNotification(("Unequipped " + std::string(bound->GetName() ? bound->GetName() : "item")).c_str());
                } else {
                    equipManager->EquipObject(targetActor, bound, nullptr, 1, nullptr, false, false, true, true);
                    RE::DebugNotification(("Equipped " + std::string(bound->GetName() ? bound->GetName() : "item")).c_str());
                }
            } else {
                auto* wardrobe = GetWardrobeContainer();
                if (wardrobe) {
                    auto wInv = wardrobe->GetInventory();
                    auto wIt = wInv.find(bound);
                    if (wIt != wInv.end() && wIt->second.first > 0) {
                        wardrobe->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, targetActor);
                        equipManager->EquipObject(targetActor, bound, nullptr, 1, nullptr, false, false, true, true);
                        RE::DebugNotification(("Transferred & Equipped " + std::string(bound->GetName() ? bound->GetName() : "item")).c_str());
                    }
                }
            }

            targetActor->Update3DModel();

            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJson(true);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (...) {}
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
    } catch (...) {}
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
    } catch (...) {}
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
    } catch (...) {}
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
    } catch (...) {}
}