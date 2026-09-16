#include "pch.h"
#include "JSInterop.h"
#include "Settings.h"
#include "OutfitManager.h"
#include "EquipmentManager.h"
#include "AdaptiveWardrobeManager.h"
#include "InputHandler.h"
#include "thumbnail/ItemThumbnailService.h"
#include <algorithm>
#include <chrono>
#include <sstream>
#include <fstream>
#include <cmath>
#include <unordered_map>
#include <cctype>
#include <SKSE/SKSE.h>
#include <RE/B/BipedAnim.h>
#include <RE/P/PlayerInputHandler.h>
#include <RE/P/PlayerControls.h>
#include <RE/T/TogglePOVHandler.h>
#include <RE/F/FirstPersonState.h>

extern PRISMA_UI_API::IVPrismaUI2* PrismaUI;
extern PrismaView view;

struct StudioActorTransform {
    RE::NiPoint3 position;
    float angleZ{ 0.0f };
};

RE::NiPoint3 g_stagePosition;
float g_stageAngleZ = 0.0f;
RE::FormID g_currentStudioActorID = 0x00000007;
static std::unordered_map<RE::FormID, StudioActorTransform> g_originalFollowerTransforms;

static float g_savedFreeRotationX = 0.0f;
static float g_savedFreeRotationY = 0.0f;
static float g_savedZoomOffset = 1.0f;
static bool g_wasFirstPerson = false;
static bool g_hasSavedCameraState = false;

static RE::TESObjectREFR* ResolveContainerFromSourceString(const std::string& src) {
    std::string upper = src;
    std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    if (upper == "CLOUD" || upper == "WARDROBE") {
        return GetWardrobeContainer();
    }

    if (upper == "0X00000007" || upper == "0X00000014" || upper == "PLAYER" || upper.empty()) {
        return RE::PlayerCharacter::GetSingleton();
    }

    if (upper == "FOLLOWER") {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        if (g_manager.activeTargetActorID != 0 && g_manager.activeTargetActorID != 0x00000007 && g_manager.activeTargetActorID != 0x00000014) {
            auto* form = RE::TESForm::LookupByID(g_manager.activeTargetActorID);
            auto* actor = form ? form->As<RE::Actor>() : nullptr;
            if (actor) return actor;
        }
    }

    RE::FormID fID = 0;
    try {
        if (upper.rfind("0X", 0) == 0) {
            fID = static_cast<RE::FormID>(std::stoul(upper.substr(2), nullptr, 16));
        } else {
            fID = static_cast<RE::FormID>(std::stoul(upper, nullptr, 16));
        }
    } catch (...) {}

    if (fID == 0x00000007 || fID == 0x00000014 || fID == 0) {
        return RE::PlayerCharacter::GetSingleton();
    }

    auto* form = RE::TESForm::LookupByID(fID);
    if (!form) return nullptr;

    auto* refr = form->As<RE::TESObjectREFR>();
    if (refr) return refr;

    auto* actor = form->As<RE::Actor>();
    if (actor) return actor;

    return nullptr;
}

static void SetActorRestrainedAndDontMove(RE::Actor* a_actor, bool a_enable) {
    if (!a_actor) return;
    auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
    if (!vm) return;
    auto* handlePolicy = vm->GetObjectHandlePolicy();
    if (!handlePolicy) return;
    RE::VMHandle handle = handlePolicy->GetHandleForObject(RE::Actor::FORMTYPE, a_actor);
    if (handle == handlePolicy->EmptyHandle()) return;

    auto args1 = RE::MakeFunctionArguments<bool>(std::move(a_enable));
    RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback1;
    vm->DispatchMethodCall(handle, "Actor", "SetRestrained", args1, callback1);

    auto args2 = RE::MakeFunctionArguments<bool>(std::move(a_enable));
    RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback2;
    vm->DispatchMethodCall(handle, "Actor", "SetDontMove", args2, callback2);
}

void OnDomReadyCallbackFunc(PrismaView v) {
    SKSE::log::info("OnDomReadyCallbackFunc fired for View ID: {}", v);
    SKSE::GetTaskInterface()->AddTask([v]() {
        if (PrismaUI && v != 0) {
            PrismaUI->Unfocus(v);
            PrismaUI->Hide(v);
        }
        if (g_gameFullyLoaded.load(std::memory_order_acquire)) {
            auto controlMap = RE::ControlMap::GetSingleton();
            if (controlMap) {
                controlMap->ToggleControls(RE::ControlMap::UEFlag::kAll, true);
            }
        }
    });
}

void OnConsoleMessageCallbackFunc(PrismaView v, PRISMA_UI_API::ConsoleMessageLevel level, const char* message) {
    SKSE::log::info("[JS CONSOLE] [Level {}] [View {}]: {}", static_cast<uint8_t>(level), v, message ? message : "");
}

void InitializeStudioStage() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return;

    float playerAngleZ = player->GetAngleZ();
    RE::NiPoint3 playerPos = player->GetPosition();

    float forwardOffset = 160.0f;
    g_stagePosition.x = playerPos.x + forwardOffset * std::sin(playerAngleZ);
    g_stagePosition.y = playerPos.y + forwardOffset * std::cos(playerAngleZ);
    g_stagePosition.z = playerPos.z;
    g_stageAngleZ = playerAngleZ;

    g_currentStudioActorID = 0x00000007;
    g_originalFollowerTransforms.clear();

    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        g_manager.activeTargetActorID = 0x00000007;
    }
}

void SetCameraInputHandlingEnabled(bool a_enabled) {
    auto controlMap = RE::ControlMap::GetSingleton();
    if (controlMap) {
        using UEFlag = RE::ControlMap::UEFlag;
        uint32_t flags = static_cast<uint32_t>(UEFlag::kPOVSwitch) |
                         static_cast<uint32_t>(UEFlag::kWheelZoom);
        controlMap->ToggleControls(static_cast<UEFlag>(flags), a_enabled);
    }

    auto playerControls = RE::PlayerControls::GetSingleton();
    if (playerControls) {
        playerControls->blockPlayerInput = !a_enabled;
        if (playerControls->togglePOVHandler) {
            playerControls->togglePOVHandler->SetInputEventHandlingEnabled(a_enabled);
        }
    }

    auto cam = RE::PlayerCamera::GetSingleton();
    if (!cam) return;

    auto& rtData = cam->GetRuntimeData();
    auto& firstPtr = rtData.cameraStates[RE::CameraStates::kFirstPerson];
    if (firstPtr) {
        auto* firstState = static_cast<RE::FirstPersonState*>(firstPtr.get());
        if (firstState) {
            static_cast<RE::PlayerInputHandler*>(firstState)->SetInputEventHandlingEnabled(a_enabled);
        }
    }
    auto& tpPtr = rtData.cameraStates[RE::CameraStates::kThirdPerson];
    if (tpPtr) {
        auto* tpState = static_cast<RE::ThirdPersonState*>(tpPtr.get());
        if (tpState) {
            static_cast<RE::PlayerInputHandler*>(tpState)->SetInputEventHandlingEnabled(a_enabled);
        }
    }
    auto& mountPtr = rtData.cameraStates[RE::CameraStates::kMount];
    if (mountPtr) {
        auto* mountState = static_cast<RE::ThirdPersonState*>(mountPtr.get());
        if (mountState) {
            static_cast<RE::PlayerInputHandler*>(mountState)->SetInputEventHandlingEnabled(a_enabled);
        }
    }

    if (cam->currentState) {
        if (cam->currentState->id == RE::CameraState::kFirstPerson) {
            auto* fpState = static_cast<RE::FirstPersonState*>(cam->currentState.get());
            if (fpState) {
                static_cast<RE::PlayerInputHandler*>(fpState)->SetInputEventHandlingEnabled(a_enabled);
            }
        } else if (cam->currentState->id == RE::CameraState::kThirdPerson ||
            cam->currentState->id == RE::CameraState::kMount ||
            cam->currentState->id == RE::CameraState::kAutoVanity) {
            auto* tpState = static_cast<RE::ThirdPersonState*>(cam->currentState.get());
            if (tpState) {
                static_cast<RE::PlayerInputHandler*>(tpState)->SetInputEventHandlingEnabled(a_enabled);
            }
        }
    }
}

void SetupStudioCameraAndControls(bool enableStudio) {
    auto controlMap = RE::ControlMap::GetSingleton();
    if (controlMap) {
        using UEFlag = RE::ControlMap::UEFlag;
        uint32_t flags = static_cast<uint32_t>(UEFlag::kFighting) |
                         static_cast<uint32_t>(UEFlag::kMovement) |
                         static_cast<uint32_t>(UEFlag::kJumping) |
                         static_cast<uint32_t>(UEFlag::kSneaking) |
                         static_cast<uint32_t>(UEFlag::kActivate) |
                         static_cast<uint32_t>(UEFlag::kPOVSwitch) |
                         static_cast<uint32_t>(UEFlag::kWheelZoom);
        controlMap->ToggleControls(static_cast<UEFlag>(flags), !enableStudio);
    }
    SetCameraInputHandlingEnabled(!enableStudio);

    auto player = RE::PlayerCharacter::GetSingleton();
    auto cam = RE::PlayerCamera::GetSingleton();
    if (cam) {
        if (enableStudio) {
            if (cam->currentState) {
                if (cam->currentState->id == RE::CameraState::kFirstPerson) {
                    g_wasFirstPerson = true;
                    cam->ForceThirdPerson();
                } else {
                    g_wasFirstPerson = false;
                }

                if (cam->currentState->id == RE::CameraState::kThirdPerson || cam->currentState->id == RE::CameraState::kAutoVanity) {
                    auto* tpState = static_cast<RE::ThirdPersonState*>(cam->currentState.get());
                    if (tpState) {
                        if (!g_hasSavedCameraState) {
                            g_savedFreeRotationX = tpState->freeRotation.x;
                            g_savedFreeRotationY = tpState->freeRotation.y;
                            g_savedZoomOffset = tpState->targetZoomOffset;
                            g_hasSavedCameraState = true;
                        }
                        tpState->freeRotation.x = 3.14159265f;
                        tpState->freeRotation.y = 0.0f;
                        tpState->targetZoomOffset = 0.45f;
                    }
                }
            }
            if (player) {
                cam->cameraTarget = player->GetHandle();
            }
        } else {
            RestoreStudioActors();
            if (controlMap) {
                controlMap->ToggleControls(RE::ControlMap::UEFlag::kAll, true);
            }
            if (g_hasSavedCameraState && cam->currentState) {
                if (cam->currentState->id == RE::CameraState::kThirdPerson || cam->currentState->id == RE::CameraState::kAutoVanity) {
                    auto* tpState = static_cast<RE::ThirdPersonState*>(cam->currentState.get());
                    if (tpState) {
                        tpState->freeRotation.x = g_savedFreeRotationX;
                        tpState->freeRotation.y = g_savedFreeRotationY;
                        tpState->targetZoomOffset = g_savedZoomOffset;
                    }
                }
                if (g_wasFirstPerson) {
                    cam->ForceFirstPerson();
                }
                g_hasSavedCameraState = false;
            }
        }
    }
}

void RestoreStudioActors() {
    auto player = RE::PlayerCharacter::GetSingleton();

    for (const auto& [actorID, transform] : g_originalFollowerTransforms) {
        auto* form = RE::TESForm::LookupByID(actorID);
        auto* actor = form ? form->As<RE::Actor>() : nullptr;
        if (actor) {
            SetActorRestrainedAndDontMove(actor, false);
            actor->SetPosition(transform.position, true);
            actor->data.angle.z = transform.angleZ;
            actor->SetAngle(actor->data.angle);
            actor->SetGraphVariableFloat("Speed", 0.0f);
            actor->SetGraphVariableFloat("Direction", 0.0f);
            actor->SetGraphVariableBool("bIsMoving", false);
            actor->SetGraphVariableBool("bIsWalking", false);
            actor->SetGraphVariableBool("bIsRunning", false);
            actor->SetGraphVariableBool("bIsSprinting", false);
            actor->SetGraphVariableBool("IsMoving", false);
            actor->StopMoving(0.0f);
            actor->NotifyAnimationGraph("IdleForceDefaultState");
        }
    }

    g_originalFollowerTransforms.clear();
    g_currentStudioActorID = 0x00000007;

    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        g_manager.activeTargetActorID = 0x00000007;
    }

    auto cam = RE::PlayerCamera::GetSingleton();
    if (cam) {
        if (player) {
            cam->cameraTarget = player->GetHandle();
        }
        if (g_wasFirstPerson) {
            cam->ForceFirstPerson();
            g_wasFirstPerson = false;
        } else {
            cam->ForceThirdPerson();
            if (cam->currentState && (cam->currentState->id == RE::CameraState::kThirdPerson || cam->currentState->id == RE::CameraState::kAutoVanity)) {
                auto* tpState = static_cast<RE::ThirdPersonState*>(cam->currentState.get());
                if (tpState && g_hasSavedCameraState) {
                    tpState->freeRotation.x = g_savedFreeRotationX;
                    tpState->freeRotation.y = g_savedFreeRotationY;
                    tpState->targetZoomOffset = g_savedZoomOffset;
                }
            }
        }
        g_hasSavedCameraState = false;
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
            if (cam->currentState->id == RE::CameraState::kThirdPerson || cam->currentState->id == RE::CameraState::kAutoVanity) {
                auto* tpState = static_cast<RE::ThirdPersonState*>(cam->currentState.get());
                if (tpState) {
                    tpState->freeRotation.x += (deltaX * 0.005f);
                    tpState->freeRotation.y = std::clamp(tpState->freeRotation.y - (deltaY * 0.005f), -1.2f, 1.2f);
                }
            }
        });
    } catch (...) {}
}

void ZoomCameraListener(const char*) {
}

void PanCameraListener(const char* data) {
    try {
        json req = json::parse(data);
        float moveX = req.value("moveX", 0.0f);
        float moveY = req.value("moveY", 0.0f);

        SKSE::GetTaskInterface()->AddTask([moveX, moveY]() {
            auto cam = RE::PlayerCamera::GetSingleton();
            if (!cam || !cam->currentState) return;
            if (cam->currentState->id == RE::CameraState::kThirdPerson || cam->currentState->id == RE::CameraState::kAutoVanity) {
                auto* tpState = static_cast<RE::ThirdPersonState*>(cam->currentState.get());
                if (tpState) {
                    tpState->freeRotation.x += (moveX * 0.05f);
                    tpState->freeRotation.y = std::clamp(tpState->freeRotation.y - (moveY * 0.05f), -1.2f, 1.2f);
                }
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
                            SetActorRestrainedAndDontMove(prevActor, false);
                            prevActor->SetPosition(it->second.position, true);
                            prevActor->data.angle.z = it->second.angleZ;
                            prevActor->SetAngle(prevActor->data.angle);
                            prevActor->SetGraphVariableFloat("Speed", 0.0f);
                            prevActor->SetGraphVariableFloat("Direction", 0.0f);
                            prevActor->SetGraphVariableBool("bIsMoving", false);
                            prevActor->SetGraphVariableBool("bIsWalking", false);
                            prevActor->SetGraphVariableBool("bIsRunning", false);
                            prevActor->SetGraphVariableBool("bIsSprinting", false);
                            prevActor->SetGraphVariableBool("IsMoving", false);
                            prevActor->StopMoving(0.0f);
                            prevActor->NotifyAnimationGraph("IdleForceDefaultState");
                        }
                    }
                }

                if (targetID == 0x00000007 || targetID == 0 || targetID == 0x00000014) {
                    auto cam = RE::PlayerCamera::GetSingleton();
                    if (cam) {
                        cam->cameraTarget = player->GetHandle();
                        cam->ForceThirdPerson();
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
                        if (g_originalFollowerTransforms.find(targetID) == g_originalFollowerTransforms.end()) {
                            StudioActorTransform tf;
                            tf.position = follower->GetPosition();
                            tf.angleZ = follower->GetAngleZ();
                            g_originalFollowerTransforms[targetID] = tf;
                        }

                        float playerAngleZ = player->GetAngleZ();
                        RE::NiPoint3 playerPos = player->GetPosition();

                        float forwardOffset = 160.0f;
                        g_stagePosition.x = playerPos.x + forwardOffset * std::sin(playerAngleZ);
                        g_stagePosition.y = playerPos.y + forwardOffset * std::cos(playerAngleZ);
                        g_stagePosition.z = playerPos.z;
                        g_stageAngleZ = playerAngleZ;

                        follower->SetPosition(g_stagePosition, true);
                        follower->data.angle.z = g_stageAngleZ;
                        follower->SetAngle(follower->data.angle);

                        SetActorRestrainedAndDontMove(follower, true);
                        follower->StopCombat();
                        follower->DrawWeaponMagicHands(false);
                        follower->StopMoving(0.0f);
                        follower->SetGraphVariableFloat("Speed", 0.0f);
                        follower->SetGraphVariableFloat("Direction", 0.0f);
                        follower->SetGraphVariableBool("bIsMoving", false);
                        follower->SetGraphVariableBool("bIsWalking", false);
                        follower->SetGraphVariableBool("bIsRunning", false);
                        follower->SetGraphVariableBool("bIsSprinting", false);
                        follower->SetGraphVariableBool("IsMoving", false);
                        follower->NotifyAnimationGraph("IdleStop_Loose");
                        follower->NotifyAnimationGraph("IdleStop");
                        follower->NotifyAnimationGraph("IdleForceDefaultState");

                        auto cam = RE::PlayerCamera::GetSingleton();
                        if (cam) {
                            cam->cameraTarget = follower->GetHandle();
                            cam->ForceThirdPerson();
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
        if (req.contains("wheelOffsetY")) g_wheelOffsetY = req.value("wheelOffsetY", 0.0f);
        if (req.contains("uiScale")) g_uiScale = req.value("uiScale", 1.0f);
        if (req.contains("editUiMode")) g_editUiMode = req.value("editUiMode", 1);
        if (req.contains("editKey")) {
            if (req["editKey"].is_number()) g_editKey = req["editKey"].get<uint32_t>();
            else if (req["editKey"].is_string()) { try { g_editKey = std::stoul(req["editKey"].get<std::string>()); } catch (...) {} }
        }
        if (req.contains("editModifierKey")) {
            if (req["editModifierKey"].is_number()) g_editModifierKey = req["editModifierKey"].get<uint32_t>();
            else if (req["editModifierKey"].is_string()) { try { g_editModifierKey = std::stoul(req["editModifierKey"].get<std::string>()); } catch (...) {} }
        }
        if (req.contains("playKey")) {
            if (req["playKey"].is_number()) g_playKey = req["playKey"].get<uint32_t>();
            else if (req["playKey"].is_string()) { try { g_playKey = std::stoul(req["playKey"].get<std::string>()); } catch (...) {} }
        }
        if (req.contains("playModifierKey")) {
            if (req["playModifierKey"].is_number()) g_playModifierKey = req["playModifierKey"].get<uint32_t>();
            else if (req["playModifierKey"].is_string()) { try { g_playModifierKey = std::stoul(req["playModifierKey"].get<std::string>()); } catch (...) {} }
        }
        if (req.contains("gamepadPlayKey")) {
            if (req["gamepadPlayKey"].is_number()) g_gamepadPlayKey = req["gamepadPlayKey"].get<uint32_t>();
            else if (req["gamepadPlayKey"].is_string()) { try { g_gamepadPlayKey = std::stoul(req["gamepadPlayKey"].get<std::string>()); } catch (...) {} }
        }
        if (req.contains("gamepadPlayModifier")) {
            if (req["gamepadPlayModifier"].is_number()) g_gamepadPlayModifier = req["gamepadPlayModifier"].get<uint32_t>();
            else if (req["gamepadPlayModifier"].is_string()) { try { g_gamepadPlayModifier = std::stoul(req["gamepadPlayModifier"].get<std::string>()); } catch (...) {} }
        }
        if (req.contains("thumbstickDeadzone")) g_thumbstickDeadzone = req.value("thumbstickDeadzone", 0.25f);

        if (req.contains("autoCloseOnEquip")) g_autoCloseOnEquip = req.value("autoCloseOnEquip", true);
        if (req.contains("autoStashEnabled")) g_autoStashEnabled = req.value("autoStashEnabled", false);
        if (req.contains("autoSystemEnabled")) g_autoSystemEnabled = req.value("autoSystemEnabled", false);
        if (req.contains("autoCityEnabled")) g_autoCityEnabled = req.value("autoCityEnabled", true);
        if (req.contains("autoHomeEnabled")) g_autoHomeEnabled = req.value("autoHomeEnabled", true);
        if (req.contains("autoCombatEnabled")) g_autoCombatEnabled = req.value("autoCombatEnabled", true);
        if (req.contains("autoSwimEnabled")) g_autoSwimEnabled = req.value("autoSwimEnabled", true);

        SaveINI();
        SaveDataInternal();

        SKSE::GetTaskInterface()->AddTask([editMode]() {
            if (editMode) {
                if (g_editUiMode == 0) {
                    SetupStudioCameraAndControls(false);
                } else {
                    InitializeStudioStage();
                    SetupStudioCameraAndControls(true);
                }
            }
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
        bool typing = req.value("typing", false);
        g_isTyping.store(typing, std::memory_order_release);
        SKSE::GetTaskInterface()->AddTask([typing]() {
            auto controlMap = RE::ControlMap::GetSingleton();
            if (controlMap) {
                controlMap->ToggleControls(RE::ControlMap::UEFlag::kAll, !typing);
            }
        });
    } catch (...) {}
}

void SaveOutfitListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = 0;
        if (req.contains("wheelIndex")) {
            if (req["wheelIndex"].is_number()) wheelIdx = req["wheelIndex"].get<size_t>();
            else if (req["wheelIndex"].is_string()) { try { wheelIdx = std::stoull(req["wheelIndex"].get<std::string>()); } catch (...) {} }
        }
        size_t slotIdx = 0;
        if (req.contains("slotIndex")) {
            if (req["slotIndex"].is_number()) slotIdx = req["slotIndex"].get<size_t>();
            else if (req["slotIndex"].is_string()) { try { slotIdx = std::stoull(req["slotIndex"].get<std::string>()); } catch (...) {} }
        }

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx]() {
            SaveOutfitToSlotImpl(wheelIdx, slotIdx);
            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJson(true);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (...) {}
}

void SaveOutfitToSlotListener(const char* data) {
    SaveOutfitListener(data);
}

void RenameSlotListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = 0;
        if (req.contains("wheelIndex")) {
            if (req["wheelIndex"].is_number()) wheelIdx = req["wheelIndex"].get<size_t>();
            else if (req["wheelIndex"].is_string()) { try { wheelIdx = std::stoull(req["wheelIndex"].get<std::string>()); } catch (...) {} }
        }
        size_t slotIdx = 0;
        if (req.contains("slotIndex")) {
            if (req["slotIndex"].is_number()) slotIdx = req["slotIndex"].get<size_t>();
            else if (req["slotIndex"].is_string()) { try { slotIdx = std::stoull(req["slotIndex"].get<std::string>()); } catch (...) {} }
        }
        std::string newName = req.value("newName", "");

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx, newName]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            auto& wheels = g_manager.GetActiveWheels();
            if (wheels.empty()) {
                Wheel defaultWheel;
                defaultWheel.name = "Wheel 1";
                wheels.push_back(defaultWheel);
            }
            size_t wIdx = wheelIdx;
            if (wIdx >= wheels.size()) wIdx = 0;
            while (wheels[wIdx].slots.size() <= slotIdx) {
                Slot s;
                wheels[wIdx].slots.push_back(s);
            }

            if (wheels[wIdx].slots[slotIdx].outfit.has_value()) {
                if (!newName.empty()) {
                    wheels[wIdx].slots[slotIdx].outfit->name = newName;
                }
            } else {
                Outfit o;
                o.name = newName.empty() ? ("Outfit " + std::to_string(slotIdx + 1)) : newName;
                o.contextTag = "";
                o.preferenceStars = 3;
                o.controlState = ControlState::Normal;
                o.isAddon = false;
                wheels[wIdx].slots[slotIdx].outfit = o;
            }

            SaveDataInternal();
            RE::DebugNotification(("Renamed outfit to " + newName).c_str());

            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJsonInternal(true);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (...) {}
}

void RenameWheelListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = 0;
        if (req.contains("wheelIndex")) {
            if (req["wheelIndex"].is_number()) wheelIdx = req["wheelIndex"].get<size_t>();
            else if (req["wheelIndex"].is_string()) { try { wheelIdx = std::stoull(req["wheelIndex"].get<std::string>()); } catch (...) {} }
        }
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
        bool keepOpen = req.value("keepOpen", false);

        bool shiftHeld = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) || ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0) || ((GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0);
        if (shiftHeld) keepOpen = true;

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx, editMode, keepOpen]() {
            EquipSlotImpl(wheelIdx, slotIdx);

            if (PrismaUI && view != 0) {
                if (editMode) {
                    std::string jsCmd = GetWheelJson(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                } else {
                    if (g_autoCloseOnEquip && !keepOpen) {
                        HideWheel();
                    } else {
                        std::string jsCmd = GetWheelJson(false);
                        PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                    }
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
    std::string initialContextTag = "";
    try {
        if (data && data[0] != 0) {
            json req = json::parse(data);
            editMode = req.value("editMode", true);
            initialContextTag = req.value("contextTag", "");
        }
    } catch (...) {}

    SKSE::GetTaskInterface()->AddTask([editMode, initialContextTag]() {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        auto& wheels = g_manager.GetActiveWheels();
        size_t activeIdx = g_manager.GetActiveWheelIndex();
        if (!wheels.empty() && activeIdx < wheels.size()) {
            Slot s;
            Outfit o;
            o.name = "Outfit " + std::to_string(wheels[activeIdx].slots.size() + 1);
            if (!initialContextTag.empty() && initialContextTag != "ALL") {
                o.contextTag = initialContextTag;
            }
            s.outfit = o;
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
                } else {
                    Outfit o;
                    o.name = "Outfit " + std::to_string(slotIdx + 1);
                    o.contextTag = tag;
                    o.preferenceStars = 3;
                    o.controlState = ControlState::Normal;
                    o.isAddon = false;
                    wheels[wheelIdx].slots[slotIdx].outfit = o;
                }

                SaveDataInternal();
                RE::DebugNotification(("Context Tag set to: " + (tag.empty() ? "None" : tag)).c_str());

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
                } else {
                    Outfit o;
                    o.name = "Outfit " + std::to_string(slotIdx + 1);
                    o.contextTag = "";
                    o.preferenceStars = stars;
                    o.controlState = ControlState::Normal;
                    o.isAddon = false;
                    wheels[wheelIdx].slots[slotIdx].outfit = o;
                }

                SaveDataInternal();
                RE::DebugNotification(("Rating set to " + std::to_string(stars) + " Stars").c_str());

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
                } else {
                    Outfit o;
                    o.name = "Outfit " + std::to_string(slotIdx + 1);
                    o.contextTag = "";
                    o.preferenceStars = 3;
                    o.controlState = ControlState::Normal;
                    o.isAddon = isAddon;
                    wheels[wheelIdx].slots[slotIdx].outfit = o;
                }

                SaveDataInternal();
                RE::DebugNotification(("Slot set to " + std::string(isAddon ? "Addon" : "Full Outfit")).c_str());

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

        uint16_t reqUniqueID = req.value("uniqueID", static_cast<uint16_t>(0));
        RE::FormID reqEnchID = req.value("enchantmentID", static_cast<RE::FormID>(0));
        std::string reqCustomName = req.value("customName", "");

        SKSE::GetTaskInterface()->AddTask([rawID, reqUniqueID, reqEnchID, reqCustomName]() {
            RE::Actor* targetActor = ResolveActiveTargetActor();
            auto* player = RE::PlayerCharacter::GetSingleton();
            auto equipManager = RE::ActorEquipManager::GetSingleton();
            if (!targetActor || !equipManager) return;

            auto* form = RE::TESForm::LookupByID(rawID);
            if (!form || !IsPureApparel(form)) return;
            auto* bound = form->As<RE::TESBoundObject>();
            if (!bound) return;

            OutfitItem itm;
            itm.formID = rawID;
            itm.uniqueID = reqUniqueID;
            itm.enchantmentID = reqEnchID;
            itm.customName = reqCustomName;

            auto inv = targetActor->GetInventory();
            auto it = inv.find(bound);

            if (it != inv.end() && it->second.first > 0) {
                auto* entryData = it->second.second.get();
                bool isWorn = false;
                RE::ExtraDataList* targetExtra = nullptr;

                if (entryData && entryData->extraLists) {
                    for (auto* xList : *entryData->extraLists) {
                        if (xList && (xList->HasType(RE::ExtraDataType::kWorn) || xList->HasType(RE::ExtraDataType::kWornLeft))) {
                            InventoryWornEntry entry{ bound, xList };
                            if (MatchesWornEntry(entry, itm)) {
                                isWorn = true;
                                targetExtra = xList;
                                break;
                            }
                        }
                    }
                }
                if (!isWorn && entryData && entryData->IsWorn() && itm.enchantmentID == 0) {
                    isWorn = true;
                }
                if (!isWorn) {
                    if (const auto& biped = targetActor->GetCurrentBiped()) {
                        for (size_t i = 0; i < 42; ++i) {
                            if (biped->objects[i].item && biped->objects[i].item->GetFormID() == bound->GetFormID()) {
                                if (itm.enchantmentID == 0) {
                                    isWorn = true;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (isWorn) {
                    equipManager->UnequipObject(targetActor, bound, targetExtra, 1, nullptr, false, false, true, true);
                    RE::DebugNotification(("Unequipped " + std::string(bound->GetName() ? bound->GetName() : "item")).c_str());
                } else {
                    targetExtra = FindBestMatchingExtraList(entryData, itm, bound);
                    if (itm.enchantmentID != 0 && !targetExtra) {
                        RE::DebugNotification("Matching enchanted item not found in inventory!");
                        return;
                    }
                    equipManager->EquipObject(targetActor, bound, targetExtra, 1, nullptr, false, false, true, true);
                    RE::DebugNotification(("Equipped " + std::string(bound->GetName() ? bound->GetName() : "item")).c_str());
                }
            } else {
                bool isAlreadyWorn = false;
                if (const auto& biped = targetActor->GetCurrentBiped()) {
                    for (size_t i = 0; i < 42; ++i) {
                        if (biped->objects[i].item && biped->objects[i].item->GetFormID() == bound->GetFormID()) {
                            isAlreadyWorn = true;
                            break;
                        }
                    }
                }
                if (isAlreadyWorn) {
                    equipManager->UnequipObject(targetActor, bound, nullptr, 1, nullptr, false, false, true, true);
                    RE::DebugNotification(("Unequipped " + std::string(bound->GetName() ? bound->GetName() : "item")).c_str());
                } else {
                    auto* wardrobe = GetWardrobeContainer();
                    bool transferred = false;

                    if (wardrobe) {
                        auto wInv = wardrobe->GetInventory();
                        auto wIt = wInv.find(bound);
                        if (wIt != wInv.end() && wIt->second.first > 0) {
                            wardrobe->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, targetActor);
                            equipManager->EquipObject(targetActor, bound, nullptr, 1, nullptr, false, false, true, true);
                            RE::DebugNotification(("Transferred from Wardrobe Chest & Equipped " + std::string(bound->GetName() ? bound->GetName() : "item")).c_str());
                            transferred = true;
                        }
                    }

                    if (!transferred && targetActor != player && player) {
                        auto pInv = player->GetInventory();
                        auto pIt = pInv.find(bound);
                        if (pIt != pInv.end() && pIt->second.first > 0) {
                            player->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, targetActor);
                            equipManager->EquipObject(targetActor, bound, nullptr, 1, nullptr, false, false, true, true);
                            RE::DebugNotification(("Transferred from Player & Equipped " + std::string(bound->GetName() ? bound->GetName() : "item")).c_str());
                            transferred = true;
                        }
                    }

                    if (!transferred) {
                        auto processLists = RE::ProcessLists::GetSingleton();
                        if (processLists) {
                            for (auto& handle : processLists->highActorHandles) {
                                auto otherActor = handle.get();
                                if (otherActor && otherActor.get() != targetActor && otherActor->IsPlayerTeammate() && !otherActor->IsDead()) {
                                    auto oInv = otherActor->GetInventory();
                                    auto oIt = oInv.find(bound);
                                    if (oIt != oInv.end() && oIt->second.first > 0) {
                                        otherActor->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, targetActor);
                                        equipManager->EquipObject(targetActor, bound, nullptr, 1, nullptr, false, false, true, true);
                                        RE::DebugNotification(("Transferred & Equipped " + std::string(bound->GetName() ? bound->GetName() : "item")).c_str());
                                        transferred = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJson(true);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (...) {}
}

void TransferItemListener(const char* data) {
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

        std::string fromSource = req.value("fromSource", "");
        std::string toTarget = req.value("toTarget", "");

        SKSE::GetTaskInterface()->AddTask([rawID, fromSource, toTarget]() {
            auto* form = RE::TESForm::LookupByID(rawID);
            if (!form || !IsPureApparel(form)) return;
            auto* bound = form->As<RE::TESBoundObject>();
            if (!bound) return;

            RE::TESObjectREFR* srcContainer = ResolveContainerFromSourceString(fromSource);
            RE::TESObjectREFR* dstContainer = ResolveContainerFromSourceString(toTarget);

            std::string dstName = "Target";
            if (dstContainer) {
                if (dstContainer == GetWardrobeContainer()) dstName = "Wardrobe Chest";
                else if (dstContainer == RE::PlayerCharacter::GetSingleton()) dstName = "Player";
                else dstName = (dstContainer->GetName() && dstContainer->GetName()[0] != 0) ? dstContainer->GetName() : "Follower";
            }

            if (srcContainer && dstContainer && srcContainer != dstContainer) {
                auto srcInv = srcContainer->GetInventory();
                auto it = srcInv.find(bound);
                if (it != srcInv.end() && it->second.first > 0) {
                    srcContainer->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, dstContainer);
                    RE::DebugNotification(("Transferred " + std::string(bound->GetName() ? bound->GetName() : "item") + " to " + dstName).c_str());
                }
            }

            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJson(true);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (...) {}
}

void TransferAllInventoryListener(const char* data) {
    try {
        json req = json::parse(data);
        std::string fromSource = req.value("fromSource", "");
        std::string toTarget = req.value("toTarget", "");

        SKSE::GetTaskInterface()->AddTask([fromSource, toTarget]() {
            RE::TESObjectREFR* srcContainer = ResolveContainerFromSourceString(fromSource);
            RE::TESObjectREFR* dstContainer = ResolveContainerFromSourceString(toTarget);

            std::string dstName = "Target";
            if (dstContainer) {
                if (dstContainer == GetWardrobeContainer()) dstName = "Wardrobe Chest";
                else if (dstContainer == RE::PlayerCharacter::GetSingleton()) dstName = "Player";
                else dstName = (dstContainer->GetName() && dstContainer->GetName()[0] != 0) ? dstContainer->GetName() : "Follower";
            }

            if (srcContainer && dstContainer && srcContainer != dstContainer) {
                auto srcInv = srcContainer->GetInventory();
                size_t totalCount = 0;
                for (const auto& [item, data] : srcInv) {
                    if (item && data.first > 0 && IsPureApparel(item)) {
                        auto* bound = item->As<RE::TESBoundObject>();
                        if (bound) {
                            srcContainer->RemoveItem(bound, data.first, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, dstContainer);
                            totalCount += data.first;
                        }
                    }
                }
                RE::DebugNotification(("Transferred " + std::to_string(totalCount) + " apparel items to " + dstName).c_str());
            }

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

void AddItemToSlotListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);
        RE::FormID rawID = req.value("rawID", static_cast<RE::FormID>(0));

        if (rawID == 0) {
            std::string idStr = req.value("id", "");
            if (idStr.rfind("0x", 0) == 0 || idStr.rfind("0X", 0) == 0) {
                try { rawID = static_cast<RE::FormID>(std::stoul(idStr.substr(2), nullptr, 16)); } catch (...) { rawID = 0; }
            }
        }

        if (rawID == 0) return;

        uint16_t reqUniqueID = req.value("uniqueID", static_cast<uint16_t>(0));
        RE::FormID reqEnchID = static_cast<RE::FormID>(req.value("enchantmentID", static_cast<uint32_t>(0)));
        std::string reqCustomName = req.value("customName", "");

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx, rawID, reqUniqueID, reqEnchID, reqCustomName]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            auto& wheels = g_manager.GetActiveWheels();
            if (wheelIdx < wheels.size() && slotIdx < wheels[wheelIdx].slots.size()) {
                if (!wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
                    Outfit o;
                    o.name = "Outfit " + std::to_string(slotIdx + 1);
                    o.contextTag = "";
                    o.preferenceStars = 3;
                    o.controlState = ControlState::Normal;
                    o.isAddon = false;
                    wheels[wheelIdx].slots[slotIdx].outfit = o;
                }

                auto& itemVec = wheels[wheelIdx].slots[slotIdx].outfit->itemIDs;
                auto& itemsList = wheels[wheelIdx].slots[slotIdx].outfit->items;

                OutfitItem newItm;
                newItm.formID = rawID;
                newItm.uniqueID = reqUniqueID;
                newItm.enchantmentID = reqEnchID;
                newItm.customName = reqCustomName;

                auto* form = RE::TESForm::LookupByID(rawID);
                auto* bound = form ? form->As<RE::TESBoundObject>() : nullptr;

                auto checkActorInventory = [&](RE::Actor* a_actor) -> bool {
                    if (!a_actor || !bound) return false;
                    auto inv = a_actor->GetInventory();
                    auto it = inv.find(bound);
                    if (it != inv.end() && it->second.first > 0 && it->second.second && it->second.second->extraLists) {
                        for (auto* xList : *it->second.second->extraLists) {
                            if (!xList) continue;

                            bool match = true;
                            if (reqUniqueID != 0) {
                                auto* xUID = xList->GetByType<RE::ExtraUniqueID>();
                                if (!xUID || xUID->uniqueID != reqUniqueID) match = false;
                            }
                            if (!reqCustomName.empty()) {
                                std::string candCustom = GetTrueItemCustomName(xList);
                                if (candCustom != reqCustomName) match = false;
                            }
                            if (reqEnchID != 0) {
                                auto* candEnch = GetTrueItemEnchantment(xList, bound);
                                if (!candEnch || candEnch->GetFormID() != reqEnchID) {
                                    match = false;
                                }
                            }

                            if (match) {
                                auto* xUID = xList->GetByType<RE::ExtraUniqueID>();
                                if (xUID) newItm.uniqueID = xUID->uniqueID;

                                auto* candEnch = GetTrueItemEnchantment(xList, bound);
                                if (candEnch) newItm.enchantmentID = candEnch->GetFormID();

                                std::string cName = GetTrueItemCustomName(xList);
                                if (!cName.empty()) newItm.customName = cName;

                                return true;
                            }
                        }
                    }
                    return false;
                };

                bool foundMatch = checkActorInventory(ResolveActiveTargetActor());
                if (!foundMatch && ResolveActiveTargetActor() != RE::PlayerCharacter::GetSingleton()) {
                    foundMatch = checkActorInventory(RE::PlayerCharacter::GetSingleton());
                }

                if (newItm.enchantmentID == 0 && bound) {
                    auto* arm = bound->As<RE::TESObjectARMO>();
                    if (arm && arm->formEnchanting) {
                        newItm.enchantmentID = arm->formEnchanting->GetFormID();
                    }
                }

                bool exists = false;
                for (const auto& existingItm : itemsList) {
                    if (existingItm.formID == newItm.formID &&
                        existingItm.uniqueID == newItm.uniqueID &&
                        existingItm.enchantmentID == newItm.enchantmentID &&
                        existingItm.customName == newItm.customName) {
                        exists = true;
                        break;
                    }
                }

                if (!exists) {
                    itemsList.push_back(newItm);
                    if (std::find(itemVec.begin(), itemVec.end(), rawID) == itemVec.end()) {
                        itemVec.push_back(rawID);
                    }
                    SaveDataInternal();
                    RE::DebugNotification("Item added to outfit!");
                }

                if (PrismaUI && view != 0) {
                    std::string jsCmd = GetWheelJsonInternal(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                }
            }
        });
    } catch (...) {}
}

void RemoveItemFromSlotListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t slotIdx = req.value("slotIndex", 0);
        RE::FormID rawID = req.value("rawID", static_cast<RE::FormID>(0));

        if (rawID == 0) {
            std::string idStr = req.value("id", "");
            if (idStr.rfind("0x", 0) == 0 || idStr.rfind("0X", 0) == 0) {
                try { rawID = static_cast<RE::FormID>(std::stoul(idStr.substr(2), nullptr, 16)); } catch (...) { rawID = 0; }
            }
        }

        if (rawID == 0) return;

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx, rawID]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            auto& wheels = g_manager.GetActiveWheels();
            if (wheelIdx < wheels.size() && slotIdx < wheels[wheelIdx].slots.size()) {
                if (wheels[wheelIdx].slots[slotIdx].outfit.has_value()) {
                    auto& itemVec = wheels[wheelIdx].slots[slotIdx].outfit->itemIDs;
                    itemVec.erase(std::remove(itemVec.begin(), itemVec.end(), rawID), itemVec.end());

                    auto& itemsList = wheels[wheelIdx].slots[slotIdx].outfit->items;
                    itemsList.erase(std::remove_if(itemsList.begin(), itemsList.end(), [rawID](const OutfitItem& itm) {
                        return itm.formID == rawID;
                    }), itemsList.end());

                    SaveDataInternal();
                    RE::DebugNotification("Item removed from outfit!");
                }

                if (PrismaUI && view != 0) {
                    std::string jsCmd = GetWheelJsonInternal(true);
                    PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
                }
            }
        });
    } catch (...) {}
}

void MoveSlotListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = req.value("wheelIndex", 0);
        size_t fromIdx = req.value("fromIndex", 0);
        size_t toIdx = req.value("toIndex", 0);
        bool editMode = req.value("editMode", true);

        SKSE::GetTaskInterface()->AddTask([wheelIdx, fromIdx, toIdx, editMode]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            auto& wheels = g_manager.GetActiveWheels();
            if (wheelIdx < wheels.size()) {
                auto& slots = wheels[wheelIdx].slots;
                if (fromIdx < slots.size() && toIdx < slots.size() && fromIdx != toIdx) {
                    auto slotItem = slots[fromIdx];
                    slots.erase(slots.begin() + fromIdx);
                    slots.insert(slots.begin() + toIdx, slotItem);

                    RE::FormID canonicalID = (g_manager.activeTargetActorID == 0x00000014 || g_manager.activeTargetActorID == 0) ? 0x00000007 : g_manager.activeTargetActorID;
                    AdaptiveWardrobeManager::GetSingleton()->SetCurrentOutfitID(canonicalID, "");

                    SaveDataInternal();
                    RE::DebugNotification("Reordered slots!");
                }
            }
            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJsonInternal(editMode);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (...) {}
}

void MoveWheelListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t fromIdx = req.value("fromIndex", 0);
        size_t toIdx = req.value("toIndex", 0);
        bool editMode = req.value("editMode", true);

        SKSE::GetTaskInterface()->AddTask([fromIdx, toIdx, editMode]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            auto& wheels = g_manager.GetActiveWheels();
            if (fromIdx < wheels.size() && toIdx < wheels.size() && fromIdx != toIdx) {
                auto wheelItem = wheels[fromIdx];
                wheels.erase(wheels.begin() + fromIdx);
                wheels.insert(wheels.begin() + toIdx, wheelItem);

                size_t activeIdx = g_manager.GetActiveWheelIndex();
                if (activeIdx == fromIdx) {
                    g_manager.GetActiveWheelIndex() = toIdx;
                } else if (fromIdx < activeIdx && toIdx >= activeIdx) {
                    g_manager.GetActiveWheelIndex()--;
                } else if (fromIdx > activeIdx && toIdx <= activeIdx) {
                    g_manager.GetActiveWheelIndex()++;
                }

                SaveDataInternal();
                RE::DebugNotification("Reordered wheels!");
            }
            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJsonInternal(editMode);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (...) {}
}

void ExportConfigListener(const char* data) {
    (void)data;
    try {
        SKSE::GetTaskInterface()->AddTask([]() {
            std::string jsonStr = ExportConfigToString();
            std::filesystem::create_directories("Data/SKSE/Plugins/backups");

            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::tm tm{};
            localtime_s(&tm, &now);
            char buf[64];
            std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm);
            std::string backupPath = "Data/SKSE/Plugins/backups/OutfitWheeler_" + std::string(buf) + ".json";

            std::ofstream out(backupPath);
            if (out.is_open()) {
                out << jsonStr;
                out.close();
                RE::DebugNotification(("Config backed up to: " + backupPath).c_str());
            }

            if (PrismaUI && view != 0) {
                json res;
                res["success"] = true;
                res["path"] = backupPath;
                res["configJson"] = jsonStr;
                PrismaUI->InteropCall(view, "onConfigExported", res.dump().c_str());
            }
        });
    } catch (...) {}
}

void ImportConfigListener(const char* data) {
    try {
        std::string jsonPayload = "";
        if (data && data[0] != '\0') {
            try {
                json req = json::parse(data);
                if (req.is_string()) {
                    jsonPayload = req.get<std::string>();
                } else if (req.contains("configJson") && req["configJson"].is_string()) {
                    jsonPayload = req["configJson"].get<std::string>();
                } else if (req.contains("config") && req["config"].is_object()) {
                    jsonPayload = req["config"].dump();
                } else if (req.contains("path") && req["path"].is_string()) {
                    std::ifstream in(req["path"].get<std::string>());
                    if (in.is_open()) {
                        std::stringstream ss;
                        ss << in.rdbuf();
                        jsonPayload = ss.str();
                    }
                } else if (req.is_object() && (req.contains("actors") || req.contains("wheels"))) {
                    jsonPayload = req.dump();
                }
            } catch (...) {
                jsonPayload = data;
            }
        }

        SKSE::GetTaskInterface()->AddTask([jsonPayload]() {
            bool success = ImportConfigFromString(jsonPayload);
            if (success) {
                RE::DebugNotification("Configuration successfully restored!");
            } else {
                RE::DebugNotification("Failed to restore configuration.");
            }

            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJsonInternal(true);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
            }
        });
    } catch (...) {}
}

void RequestThumbnailListener(const char* data) {
    try {
        json req = json::parse(data);
        std::vector<uint32_t> ids;
        bool highPriority = false;
        if (req.contains("highPriority") && req["highPriority"].is_boolean()) {
            highPriority = req["highPriority"].get<bool>();
        }
        if (req.contains("rawIDs") && req["rawIDs"].is_array()) {
            for (const auto& el : req["rawIDs"]) {
                if (el.is_number()) {
                    ids.push_back(el.get<uint32_t>());
                } else if (el.is_string()) {
                    try { ids.push_back(static_cast<uint32_t>(std::stoul(el.get<std::string>(), nullptr, 16))); } catch (...) {}
                }
            }
        } else if (req.is_array()) {
            for (const auto& el : req) {
                if (el.is_number()) {
                    ids.push_back(el.get<uint32_t>());
                } else if (el.is_string()) {
                    try { ids.push_back(static_cast<uint32_t>(std::stoul(el.get<std::string>(), nullptr, 16))); } catch (...) {}
                }
            }
        } else {
            uint32_t rawID = 0;
            if (req.contains("rawID") && req["rawID"].is_number()) {
                rawID = req["rawID"].get<uint32_t>();
            } else if (req.contains("id") && req["id"].is_string()) {
                std::string idStr = req["id"].get<std::string>();
                try { rawID = static_cast<uint32_t>(std::stoul(idStr, nullptr, 16)); } catch (...) {}
            }
            if (rawID != 0) {
                ids.push_back(rawID);
            }
        }

        if (!ids.empty()) {
            SKSE::GetTaskInterface()->AddTask([ids = std::move(ids), highPriority]() {
                auto& thumbService = OutfitWheeler::Thumbnail::ItemThumbnailService::GetSingleton();
                std::vector<RE::TESBoundObject*> bounds;
                bounds.reserve(ids.size());
                for (uint32_t rID : ids) {
                    auto* form = RE::TESForm::LookupByID(rID);
                    if (auto* bound = form ? form->As<RE::TESBoundObject>() : nullptr) {
                        bounds.push_back(bound);
                    }
                }
                if (!bounds.empty()) {
                    thumbService.QueueItems(bounds, highPriority);
                }
            });
        }
    } catch (...) {}
}

void NotifyThumbnailReady(uint32_t a_formID, const std::string& a_relPath) {
    if (!PrismaUI || view == 0) return;
    try {
        json msg = {
            { "formID", a_formID },
            { "rawID", a_formID },
            { "thumbnail", a_relPath }
        };
        PrismaUI->InteropCall(view, "onThumbnailReady", msg.dump().c_str());
    } catch (...) {}
}

void RegisterJSListeners(PrismaView v) {
    if (!PrismaUI || !v) return;

    PrismaUI->RegisterJSListener(v, "SaveSettings", SaveSettingsListener);
    PrismaUI->RegisterJSListener(v, "SetTypingMode", SetTypingModeListener);
    PrismaUI->RegisterJSListener(v, "SaveOutfit", SaveOutfitListener);
    PrismaUI->RegisterJSListener(v, "SaveOutfitToSlot", SaveOutfitToSlotListener);
    PrismaUI->RegisterJSListener(v, "RenameSlot", RenameSlotListener);
    PrismaUI->RegisterJSListener(v, "RenameWheel", RenameWheelListener);
    PrismaUI->RegisterJSListener(v, "ClearSlot", ClearSlotListener);
    PrismaUI->RegisterJSListener(v, "DeleteSlot", DeleteSlotListener);
    PrismaUI->RegisterJSListener(v, "EquipSlot", EquipSlotListener);
    PrismaUI->RegisterJSListener(v, "OnCloseWheel", OnCloseWheelListener);
    PrismaUI->RegisterJSListener(v, "ChangeWheel", ChangeWheelListener);
    PrismaUI->RegisterJSListener(v, "AddWheel", AddWheelListener);
    PrismaUI->RegisterJSListener(v, "AddSlot", AddSlotListener);
    PrismaUI->RegisterJSListener(v, "DeleteWheel", DeleteWheelListener);
    PrismaUI->RegisterJSListener(v, "SetSlotContext", SetSlotContextListener);
    PrismaUI->RegisterJSListener(v, "SetSlotRating", SetSlotRatingListener);
    PrismaUI->RegisterJSListener(v, "SetSlotType", SetSlotTypeListener);
    PrismaUI->RegisterJSListener(v, "ToggleItemEquip", ToggleItemEquipListener);
    PrismaUI->RegisterJSListener(v, "TransferItem", TransferItemListener);
    PrismaUI->RegisterJSListener(v, "TransferAllInventory", TransferAllInventoryListener);
    PrismaUI->RegisterJSListener(v, "OrbitCamera", OrbitCameraListener);
    PrismaUI->RegisterJSListener(v, "ZoomCamera", ZoomCameraListener);
    PrismaUI->RegisterJSListener(v, "PanCamera", PanCameraListener);
    PrismaUI->RegisterJSListener(v, "SelectTargetActor", SelectTargetActorListener);
    PrismaUI->RegisterJSListener(v, "RetrieveAllOutfits", RetrieveAllOutfitsListener);
    PrismaUI->RegisterJSListener(v, "StashAllOutfits", StashAllOutfitsListener);
    PrismaUI->RegisterJSListener(v, "StashSlotOutfit", StashSlotOutfitListener);
    PrismaUI->RegisterJSListener(v, "RetrieveSlotOutfit", RetrieveSlotOutfitListener);
    PrismaUI->RegisterJSListener(v, "StashWheelOutfits", StashWheelOutfitsListener);
    PrismaUI->RegisterJSListener(v, "RetrieveWheelOutfits", RetrieveWheelOutfitsListener);
    PrismaUI->RegisterJSListener(v, "AddItemToSlot", AddItemToSlotListener);
    PrismaUI->RegisterJSListener(v, "RemoveItemFromSlot", RemoveItemFromSlotListener);
    PrismaUI->RegisterJSListener(v, "MoveSlot", MoveSlotListener);
    PrismaUI->RegisterJSListener(v, "MoveWheel", MoveWheelListener);
    PrismaUI->RegisterJSListener(v, "ExportConfig", ExportConfigListener);
    PrismaUI->RegisterJSListener(v, "ImportConfig", ImportConfigListener);
    PrismaUI->RegisterJSListener(v, "RequestThumbnail", RequestThumbnailListener);
}