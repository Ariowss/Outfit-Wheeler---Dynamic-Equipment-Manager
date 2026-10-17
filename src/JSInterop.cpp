#include "pch.h"
#include "JSInterop.h"
#include "Settings.h"
#include "OutfitManager.h"
#include "EquipmentManager.h"
#include "AdaptiveWardrobeManager.h"
#include "InputHandler.h"
#include "thumbnail/ItemThumbnailService.h"
#include "thumbnail/ThumbnailDiskCache.h"
#include "ModCatalogManager.h"
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
std::atomic<bool> g_isStudioActive{ false };
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

void SetActorRestrainedAndDontMove(RE::Actor* a_actor, bool a_enable) {
    if (!a_actor) return;
    if (a_enable && (a_actor->IsPlayerRef() || a_actor->GetFormID() == 0x00000007 || a_actor->GetFormID() == 0x00000014)) {
        return;
    }

    if (a_enable) {
        a_actor->SetLifeState(RE::ACTOR_LIFE_STATE::kRestrained);
        a_actor->GetActorRuntimeData().boolFlags.set(RE::Actor::BOOL_FLAGS::kMovementBlocked);
        a_actor->InitiateDoNothingPackage();
        a_actor->StopMoving(0.0f);
        a_actor->SetGraphVariableFloat("Speed", 0.0f);
        a_actor->SetGraphVariableFloat("Direction", 0.0f);
        a_actor->SetGraphVariableBool("bIsMoving", false);
        a_actor->SetGraphVariableBool("bIsWalking", false);
        a_actor->SetGraphVariableBool("bIsRunning", false);
        a_actor->SetGraphVariableBool("bIsSprinting", false);
        a_actor->SetGraphVariableBool("IsMoving", false);
    } else {
        a_actor->SetLifeState(RE::ACTOR_LIFE_STATE::kAlive);
        a_actor->GetActorRuntimeData().boolFlags.reset(RE::Actor::BOOL_FLAGS::kMovementBlocked);
        a_actor->StopMoving(0.0f);
        a_actor->SetGraphVariableFloat("Speed", 0.0f);
        a_actor->SetGraphVariableFloat("Direction", 0.0f);
        a_actor->SetGraphVariableBool("bIsMoving", false);
        a_actor->SetGraphVariableBool("bIsWalking", false);
        a_actor->SetGraphVariableBool("bIsRunning", false);
        a_actor->SetGraphVariableBool("bIsSprinting", false);
        a_actor->SetGraphVariableBool("IsMoving", false);
        a_actor->NotifyAnimationGraph("IdleForceDefaultState");
        a_actor->NotifyAnimationGraph("IdleStop");
        a_actor->NotifyAnimationGraph("IdleStop_Loose");
        a_actor->NotifyAnimationGraph("CameraPitchWalkRunStop");
        a_actor->EvaluatePackage(true, false);
    }

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
    if (!enableStudio) {
        if (!g_isStudioActive.exchange(false, std::memory_order_acq_rel)) {
            return;
        }
        RestoreStudioActors();
        if (!isMenuOpen.load(std::memory_order_acquire)) {
            auto controlMap = RE::ControlMap::GetSingleton();
            if (controlMap) {
                controlMap->ToggleControls(RE::ControlMap::UEFlag::kAll, true);
            }
            SetCameraInputHandlingEnabled(true);
        }
        auto cam = RE::PlayerCamera::GetSingleton();
        if (cam && g_hasSavedCameraState && cam->currentState) {
            if (cam->currentState->id == RE::CameraState::kThirdPerson || cam->currentState->id == RE::CameraState::kAutoVanity) {
                auto* tpState = static_cast<RE::ThirdPersonState*>(cam->currentState.get());
                if (tpState) {
                    tpState->freeRotation.x = g_savedFreeRotationX;
                    tpState->freeRotation.y = g_savedFreeRotationY;
                    tpState->targetZoomOffset = g_savedZoomOffset;
                    g_lockedZoomOffset = g_savedZoomOffset;
                }
            }
            if (g_wasFirstPerson) {
                cam->ForceFirstPerson();
            }
            g_hasSavedCameraState = false;
        }
        return;
    }

    g_isStudioActive.store(true, std::memory_order_release);
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
        controlMap->ToggleControls(static_cast<UEFlag>(flags), false);
    }
    SetCameraInputHandlingEnabled(false);

    auto player = RE::PlayerCharacter::GetSingleton();
    auto cam = RE::PlayerCamera::GetSingleton();
    if (cam) {
        if (cam->currentState) {
            if (cam->currentState->id == RE::CameraState::kFirstPerson) {
                g_wasFirstPerson = true;
                cam->ForceThirdPerson();
            } else {
                g_wasFirstPerson = false;
            }

            if (cam->currentState && (cam->currentState->id == RE::CameraState::kThirdPerson || cam->currentState->id == RE::CameraState::kAutoVanity)) {
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
                    g_lockedZoomOffset = 0.45f;
                    g_hasLockedZoom.store(true, std::memory_order_release);
                }
            }
        }
        if (player) {
            cam->cameraTarget = player->GetHandle();
        }
    }
}

void RestoreStudioActors() {
    auto player = RE::PlayerCharacter::GetSingleton();

    for (const auto& [actorID, transform] : g_originalFollowerTransforms) {
        auto* form = RE::TESForm::LookupByID(actorID);
        auto* actor = form ? form->As<RE::Actor>() : nullptr;
        if (actor && !actor->IsPlayerRef() && actor->GetFormID() != 0x00000007 && actor->GetFormID() != 0x00000014) {
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
            actor->NotifyAnimationGraph("IdleStop");
        }
    }

    g_originalFollowerTransforms.clear();
    g_currentStudioActorID = 0x00000007;

    {
        std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
        g_manager.activeTargetActorID = 0x00000007;
    }

    if (player && g_hasSavedCameraState) {
        SetActorRestrainedAndDontMove(player, false);
        player->SetLifeState(RE::ACTOR_LIFE_STATE::kAlive);
        player->GetActorRuntimeData().boolFlags.reset(RE::Actor::BOOL_FLAGS::kMovementBlocked);
        player->NotifyAnimationGraph("IdleStop");
        player->NotifyAnimationGraph("IdleStop_Loose");
        player->EvaluatePackage(true, false);
    }

    auto cam = RE::PlayerCamera::GetSingleton();
    if (cam && g_hasSavedCameraState) {
        if (player) {
            cam->cameraTarget = player->GetHandle();
        }
        if (g_wasFirstPerson) {
            cam->ForceFirstPerson();
            g_wasFirstPerson = false;
        } else if (cam->currentState && (cam->currentState->id == RE::CameraState::kThirdPerson || cam->currentState->id == RE::CameraState::kAutoVanity)) {
            auto* tpState = static_cast<RE::ThirdPersonState*>(cam->currentState.get());
            if (tpState) {
                tpState->freeRotation.x = g_savedFreeRotationX;
                tpState->freeRotation.y = g_savedFreeRotationY;
                tpState->targetZoomOffset = g_savedZoomOffset;
                g_lockedZoomOffset = g_savedZoomOffset;
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
                        cam->ForceFirstPerson();
                        cam->ForceThirdPerson();
                        auto* aState = player->AsActorState();
                        cam->UpdateThirdPerson(aState ? aState->IsWeaponDrawn() : false);
                        if (cam->currentState && (cam->currentState->id == RE::CameraState::kThirdPerson || cam->currentState->id == RE::CameraState::kAutoVanity)) {
                            auto* tpState = static_cast<RE::ThirdPersonState*>(cam->currentState.get());
                            if (tpState) {
                                tpState->freeRotation.x = 3.14159265f;
                                tpState->freeRotation.y = 0.0f;
                                tpState->targetZoomOffset = 0.45f;
                                g_lockedZoomOffset = 0.45f;
                                g_hasLockedZoom.store(true, std::memory_order_release);
                            }
                        }
                    }
                    g_currentStudioActorID = 0x00000007;
                    SetActorRestrainedAndDontMove(player, false);
                    player->SetLifeState(RE::ACTOR_LIFE_STATE::kAlive);
                    player->GetActorRuntimeData().boolFlags.reset(RE::Actor::BOOL_FLAGS::kMovementBlocked);
                    player->NotifyAnimationGraph("IdleForceDefaultState");
                    player->NotifyAnimationGraph("IdleStop");
                    player->NotifyAnimationGraph("IdleStop_Loose");
                    player->NotifyAnimationGraph("CameraPitchWalkRunStop");
                    player->EvaluatePackage(true, false);
                } else {
                    auto* form = RE::TESForm::LookupByID(targetID);
                    auto* follower = form ? form->As<RE::Actor>() : nullptr;

                    if (follower && !follower->IsPlayerRef() && follower->GetFormID() != 0x00000007 && follower->GetFormID() != 0x00000014) {
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
                                    g_lockedZoomOffset = 0.45f;
                                    g_hasLockedZoom.store(true, std::memory_order_release);
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
        if (req.contains("themeAccentColor")) g_themeAccentColor = req.value("themeAccentColor", "#38bdf8");
        if (req.contains("themeBgColor")) g_themeBgColor = req.value("themeBgColor", "#070a0f");
        if (req.contains("themeTextColor")) g_themeTextColor = req.value("themeTextColor", "#f8fafc");
        if (req.contains("themeDepth")) g_themeDepth = req.value("themeDepth", 0.60f);
        if (req.contains("themePreset")) g_themePreset = req.value("themePreset", "sapphire");
        if (req.contains("showItemPreviews")) g_showItemPreviews.store(req.value("showItemPreviews", true), std::memory_order_relaxed);

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
        json req = json::parse(data ? data : "{}");
        bool typing = req.value("typing", req.value("isTyping", false));
        g_isTyping.store(typing, std::memory_order_release);
        SKSE::GetTaskInterface()->AddTask([typing]() {
            auto controlMap = RE::ControlMap::GetSingleton();
            if (controlMap) {
                if (typing) {
                    controlMap->ToggleControls(RE::ControlMap::UEFlag::kAll, false);
                } else if (!isMenuOpen.load(std::memory_order_acquire)) {
                    controlMap->ToggleControls(RE::ControlMap::UEFlag::kAll, true);
                }
            }
        });
    } catch (...) {}
}

void SortOutfitsAlphabeticallyListener(const char*) {
    SKSE::GetTaskInterface()->AddTask([]() {
        g_manager.SortOutfitsAlphabetically();
        if (PrismaUI && view != 0) {
            bool isEdit = g_isEditModeOpen.load(std::memory_order_acquire);
            std::string jsCmd = GetWheelJsonInternal(isEdit);
            PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());
        }
    });
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

void SetSlotColorListener(const char* data) {
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
        std::string color = req.value("color", "");

        SKSE::GetTaskInterface()->AddTask([wheelIdx, slotIdx, color]() {
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
                wheels[wIdx].slots[slotIdx].outfit->color = color;
            } else {
                Outfit o;
                o.name = "Outfit " + std::to_string(slotIdx + 1);
                o.color = color;
                wheels[wIdx].slots[slotIdx].outfit = o;
            }

            SaveDataAsync();
        });
    } catch (...) {}
}

void SetWheelColorListener(const char* data) {
    try {
        json req = json::parse(data);
        size_t wheelIdx = 0;
        if (req.contains("wheelIndex")) {
            if (req["wheelIndex"].is_number()) wheelIdx = req["wheelIndex"].get<size_t>();
            else if (req["wheelIndex"].is_string()) { try { wheelIdx = std::stoull(req["wheelIndex"].get<std::string>()); } catch (...) {} }
        }
        std::string color = req.value("color", "");

        SKSE::GetTaskInterface()->AddTask([wheelIdx, color]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            auto& wheels = g_manager.GetActiveWheels();
            if (wheels.empty()) {
                Wheel defaultWheel;
                defaultWheel.name = "Wheel 1";
                wheels.push_back(defaultWheel);
            }
            size_t wIdx = wheelIdx;
            if (wIdx >= wheels.size()) wIdx = 0;
            wheels[wIdx].color = color;

            SaveDataAsync();
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

        SKSE::GetTaskInterface()->AddTask([delta]() {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            auto& wheels = g_manager.GetActiveWheels();
            if (wheels.empty()) return;
            int current = static_cast<int>(g_manager.GetActiveWheelIndex());
            int total = static_cast<int>(wheels.size());
            if (total <= 0) return;
            current = (current + delta) % total;
            if (current < 0) current += total;
            g_manager.GetActiveWheelIndex() = static_cast<size_t>(current);

            if (PrismaUI && view != 0) {
                json updateData;
                updateData["wheelIndex"] = static_cast<size_t>(current);
                updateData["wheelName"] = SanitizeUtf8(wheels[current].name);
                updateData["wheelColor"] = wheels[current].color;
                updateData["totalWheels"] = wheels.size();
                json slotsJson = json::array();
                size_t slotIdx = 0;
                for (const auto& slot : wheels[current].slots) {
                    json sJson;
                    sJson["index"] = slotIdx;
                    bool hasRealOutfit = slot.outfit.has_value() && (!slot.outfit->items.empty() || !slot.outfit->itemIDs.empty() || !slot.outfit->name.empty());
                    sJson["hasOutfit"] = hasRealOutfit;
                    sJson["name"] = slot.outfit.has_value() ? SanitizeUtf8(slot.outfit->name) : ("Outfit " + std::to_string(slotIdx + 1));
                    sJson["count"] = slot.outfit.has_value() ? (!slot.outfit->items.empty() ? slot.outfit->items.size() : slot.outfit->itemIDs.size()) : 0;
                    sJson["icon"] = slot.outfit.has_value() ? slot.outfit->icon : "";
                    sJson["color"] = slot.outfit.has_value() ? slot.outfit->color : "";
                    sJson["contextTag"] = slot.outfit.has_value() ? SanitizeUtf8(slot.outfit->contextTag) : "";
                    sJson["preferenceStars"] = slot.outfit.has_value() ? slot.outfit->preferenceStars : 3;
                    sJson["controlState"] = slot.outfit.has_value() ? static_cast<uint8_t>(slot.outfit->controlState) : 0;
                    sJson["isAddon"] = slot.outfit.has_value() ? slot.outfit->isAddon : false;

                    json itemObjects = json::array();
                    if (slot.outfit.has_value()) {
                        if (!slot.outfit->items.empty()) {
                            for (const auto& itm : slot.outfit->items) {
                                auto* f = RE::TESForm::LookupByID(itm.formID);
                                if (f && IsPureApparel(f)) {
                                    auto* arm = f->As<RE::TESObjectARMO>();
                                    std::string tStr = GetApparelTypeString(arm);
                                    std::string dName = (!itm.customName.empty() && itm.customName != "Apparel") ? itm.customName : (f->GetName() ? f->GetName() : "Apparel");
                                    std::string cName = (itm.customName == "Apparel") ? "" : itm.customName;
                                    std::string thumbPath = "";
                                    if (auto* bound = f->As<RE::TESBoundObject>()) {
                                        auto* bFile = bound->GetFile(0);
                                        std::string bMod = bFile ? std::string(bFile->GetFilename()) : "Skyrim.esm";
                                        uint32_t bLoc = bound->GetLocalFormID();
                                        if (OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().HasThumbnail(bMod, bLoc)) {
                                            thumbPath = OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().GetRelativePath(bMod, bLoc);
                                        }
                                    }
                                    itemObjects.push_back(json{
                                        {"id", FormatFormID(itm.formID)},
                                        {"rawID", static_cast<uint32_t>(itm.formID)},
                                        {"uniqueID", itm.uniqueID},
                                        {"enchantmentID", static_cast<uint32_t>(itm.enchantmentID)},
                                        {"customName", SanitizeUtf8(cName)},
                                        {"name", SanitizeUtf8(dName)},
                                        {"type", tStr},
                                        {"thumbnail", thumbPath}
                                    });
                                }
                            }
                        } else {
                            for (auto fID : slot.outfit->itemIDs) {
                                auto* f = RE::TESForm::LookupByID(fID);
                                if (f && IsPureApparel(f)) {
                                    auto* arm = f->As<RE::TESObjectARMO>();
                                    std::string tStr = GetApparelTypeString(arm);
                                    std::string thumbPath = "";
                                    if (auto* bound = f->As<RE::TESBoundObject>()) {
                                        auto* bFile = bound->GetFile(0);
                                        std::string bMod = bFile ? std::string(bFile->GetFilename()) : "Skyrim.esm";
                                        uint32_t bLoc = bound->GetLocalFormID();
                                        if (OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().HasThumbnail(bMod, bLoc)) {
                                            thumbPath = OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().GetRelativePath(bMod, bLoc);
                                        }
                                    }
                                    itemObjects.push_back(json{
                                        {"id", FormatFormID(fID)},
                                        {"rawID", static_cast<uint32_t>(fID)},
                                        {"name", SanitizeUtf8(f->GetName() ? f->GetName() : "Apparel")},
                                        {"type", tStr},
                                        {"thumbnail", thumbPath}
                                    });
                                }
                            }
                        }
                    }
                    sJson["items"] = itemObjects;
                    slotsJson.push_back(sJson);
                    slotIdx++;
                }
                updateData["slots"] = slotsJson;

                std::string jsCmd = updateData.dump(-1, ' ', false, json::error_handler_t::replace);
                PrismaUI->InteropCall(view, "updateActiveWheel", jsCmd.c_str());
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
            if (!initialContextTag.empty() && initialContextTag != "ALL") {
                Outfit o;
                o.name = "Outfit " + std::to_string(wheels[activeIdx].slots.size() + 1);
                o.contextTag = initialContextTag;
                s.outfit = o;
            }
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
        std::string reqSource = req.value("source", "");

        SKSE::GetTaskInterface()->AddTask([rawID, reqUniqueID, reqEnchID, reqCustomName, reqSource]() {
            RE::Actor* targetActor = nullptr;
            if (!reqSource.empty() && reqSource != "0x00000007" && reqSource != "0X00000007" && reqSource != "PLAYER" && reqSource != "CLOUD" && reqSource != "MODEX") {
                try {
                    RE::FormID sID = 0;
                    if (reqSource.rfind("0x", 0) == 0 || reqSource.rfind("0X", 0) == 0) {
                        sID = static_cast<RE::FormID>(std::stoul(reqSource.substr(2), nullptr, 16));
                    } else {
                        sID = static_cast<RE::FormID>(std::stoul(reqSource, nullptr, 10));
                    }
                    if (sID != 0) {
                        auto* f = RE::TESForm::LookupByID(sID);
                        if (f) targetActor = f->As<RE::Actor>();
                    }
                } catch (...) {}
            }
            if (!targetActor) {
                targetActor = ResolveActiveTargetActor();
            }
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

            bool finalEquipped = false;
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
                    if (!targetExtra && entryData && entryData->extraLists) {
                        for (auto* xList : *entryData->extraLists) {
                            if (xList && (xList->HasType(RE::ExtraDataType::kWorn) || xList->HasType(RE::ExtraDataType::kWornLeft))) {
                                targetExtra = xList;
                                break;
                            }
                        }
                    }
                    equipManager->UnequipObject(targetActor, bound, targetExtra, 1, nullptr, false, false, true, true);
                    finalEquipped = false;
                    RE::DebugNotification(("Unequipped " + std::string(bound->GetName() ? bound->GetName() : "item")).c_str());
                } else {
                    targetExtra = FindBestMatchingExtraList(entryData, itm, bound);
                    if (itm.enchantmentID != 0 && !targetExtra) {
                        RE::DebugNotification("Matching enchanted item not found in inventory!");
                        return;
                    }
                    equipManager->EquipObject(targetActor, bound, targetExtra, 1, nullptr, false, false, true, true);
                    finalEquipped = true;
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
                    finalEquipped = false;
                    RE::DebugNotification(("Unequipped " + std::string(bound->GetName() ? bound->GetName() : "item")).c_str());
                } else {
                    auto* wardrobe = GetWardrobeContainer();
                    bool transferred = false;

                    if (wardrobe) {
                        auto wInv = wardrobe->GetInventory();
                        auto wIt = wInv.find(bound);
                        if (wIt != wInv.end() && wIt->second.first > 0) {
                            RE::ExtraDataList* wExtra = FindBestMatchingExtraList(wIt->second.second.get(), itm, bound);
                            wardrobe->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, wExtra, targetActor);
                            SanitizeWardrobeContainer();

                            auto tInv = targetActor->GetInventory();
                            auto tIt = tInv.find(bound);
                            RE::ExtraDataList* targetExtra = nullptr;
                            if (tIt != tInv.end() && tIt->second.second) {
                                targetExtra = FindBestMatchingExtraList(tIt->second.second.get(), itm, bound);
                            }
                            equipManager->EquipObject(targetActor, bound, targetExtra, 1, nullptr, false, false, true, true);
                            finalEquipped = true;
                            RE::DebugNotification(("Transferred from Wardrobe Chest & Equipped " + std::string(bound->GetName() ? bound->GetName() : "item")).c_str());
                            transferred = true;
                        }
                    }

                    if (!transferred && targetActor != player && player) {
                        auto pInv = player->GetInventory();
                        auto pIt = pInv.find(bound);
                        if (pIt != pInv.end() && pIt->second.first > 0) {
                            RE::ExtraDataList* pExtra = FindBestMatchingExtraList(pIt->second.second.get(), itm, bound);
                            player->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, pExtra, targetActor);

                            auto tInv = targetActor->GetInventory();
                            auto tIt = tInv.find(bound);
                            RE::ExtraDataList* targetExtra = nullptr;
                            if (tIt != tInv.end() && tIt->second.second) {
                                targetExtra = FindBestMatchingExtraList(tIt->second.second.get(), itm, bound);
                            }
                            equipManager->EquipObject(targetActor, bound, targetExtra, 1, nullptr, false, false, true, true);
                            finalEquipped = true;
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
                                        RE::ExtraDataList* oExtra = FindBestMatchingExtraList(oIt->second.second.get(), itm, bound);
                                        otherActor->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, oExtra, targetActor);

                                        auto tInv = targetActor->GetInventory();
                                        auto tIt = tInv.find(bound);
                                        RE::ExtraDataList* targetExtra = nullptr;
                                        if (tIt != tInv.end() && tIt->second.second) {
                                            targetExtra = FindBestMatchingExtraList(tIt->second.second.get(), itm, bound);
                                        }
                                        equipManager->EquipObject(targetActor, bound, targetExtra, 1, nullptr, false, false, true, true);
                                        finalEquipped = true;
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
                json res;
                res["rawID"] = static_cast<uint32_t>(rawID);
                res["id"] = FormatFormID(rawID);
                res["equipped"] = finalEquipped;
                PrismaUI->InteropCall(view, "onItemEquipToggled", res.dump().c_str());
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

        uint16_t reqUniqueID = req.value("uniqueID", static_cast<uint16_t>(0));
        RE::FormID reqEnchID = req.value("enchantmentID", static_cast<RE::FormID>(0));
        std::string reqCustomName = req.value("customName", "");

        std::string fromSource = req.value("fromSource", "");
        std::string toTarget = req.value("toTarget", "");

        SKSE::GetTaskInterface()->AddTask([rawID, reqUniqueID, reqEnchID, reqCustomName, fromSource, toTarget]() {
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
                    OutfitItem itm;
                    itm.formID = rawID;
                    itm.uniqueID = reqUniqueID;
                    itm.enchantmentID = reqEnchID;
                    itm.customName = reqCustomName;
                    RE::ExtraDataList* xList = FindBestMatchingExtraList(it->second.second.get(), itm, bound);
                    srcContainer->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, xList, dstContainer);
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
                            int32_t remaining = data.first;
                            if (data.second && data.second->extraLists) {
                                for (auto* xList : *data.second->extraLists) {
                                    if (remaining <= 0) break;
                                    srcContainer->RemoveItem(bound, 1, RE::ITEM_REMOVE_REASON::kStoreInContainer, xList, dstContainer);
                                    totalCount++;
                                    remaining--;
                                }
                            }
                            if (remaining > 0) {
                                srcContainer->RemoveItem(bound, remaining, RE::ITEM_REMOVE_REASON::kStoreInContainer, nullptr, dstContainer);
                                totalCount += remaining;
                            }
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
        std::string loadedSource = "";
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
                    std::string reqPath = req["path"].get<std::string>();
                    std::ifstream in(reqPath);
                    if (in.is_open()) {
                        std::stringstream ss;
                        ss << in.rdbuf();
                        jsonPayload = ss.str();
                        loadedSource = reqPath;
                    }
                } else if (req.is_object() && (req.contains("actors") || req.contains("wheels"))) {
                    jsonPayload = req.dump();
                }
            } catch (...) {
                jsonPayload = data;
            }
        }

        // If no payload was loaded yet, auto-scan Data/SKSE/Plugins/backups for the newest .json backup
        if (jsonPayload.empty()) {
            std::filesystem::path backupsDir = "Data/SKSE/Plugins/backups";
            std::error_code ec;
            if (std::filesystem::exists(backupsDir, ec) && std::filesystem::is_directory(backupsDir, ec)) {
                std::filesystem::path newestFile;
                std::filesystem::file_time_type newestTime{};
                for (const auto& entry : std::filesystem::directory_iterator(backupsDir, ec)) {
                    if (entry.is_regular_file(ec) && entry.path().extension() == ".json") {
                        auto ftime = entry.last_write_time(ec);
                        if (newestFile.empty() || ftime > newestTime) {
                            newestTime = ftime;
                            newestFile = entry.path();
                        }
                    }
                }
                if (!newestFile.empty()) {
                    std::ifstream in(newestFile);
                    if (in.is_open()) {
                        std::stringstream ss;
                        ss << in.rdbuf();
                        jsonPayload = ss.str();
                        loadedSource = newestFile.string();
                    }
                }
            }
        }

        // Fallback to .bak files
        if (jsonPayload.empty()) {
            const char* bakFiles[] = {
                "Data/SKSE/Plugins/OutfitWheelerData.json.bak",
                "Data/SKSE/Plugins/PrismaOutfitWheel.json.bak",
                "Data/SKSE/Plugins/OutfitWheelerData.json"
            };
            for (const char* bf : bakFiles) {
                std::ifstream in(bf);
                if (in.is_open()) {
                    std::stringstream ss;
                    ss << in.rdbuf();
                    jsonPayload = ss.str();
                    loadedSource = bf;
                    break;
                }
            }
        }

        SKSE::GetTaskInterface()->AddTask([jsonPayload, loadedSource]() {
            bool success = ImportConfigFromString(jsonPayload);
            if (success) {
                RE::DebugNotification(("Config restored from: " + (loadedSource.empty() ? "backup" : loadedSource)).c_str());
            } else {
                RE::DebugNotification("Failed to restore configuration.");
            }

            if (PrismaUI && view != 0) {
                std::string jsCmd = GetWheelJsonInternal(true);
                PrismaUI->InteropCall(view, "openWheel", jsCmd.c_str());

                json res;
                res["success"] = success;
                res["source"] = loadedSource;
                PrismaUI->InteropCall(view, "onConfigImported", res.dump().c_str());
            }
        });
    } catch (...) {}
}

void GetBackupsListListener(const char* data) {
    (void)data;
    try {
        json res = json::array();
        std::filesystem::path backupsDir = "Data/SKSE/Plugins/backups";
        std::error_code ec;
        if (std::filesystem::exists(backupsDir, ec) && std::filesystem::is_directory(backupsDir, ec)) {
            struct BackupEntry {
                std::string filename;
                std::string path;
                std::filesystem::file_time_type ftime;
                uintmax_t size;
            };
            std::vector<BackupEntry> entries;
            for (const auto& entry : std::filesystem::directory_iterator(backupsDir, ec)) {
                if (entry.is_regular_file(ec) && entry.path().extension() == ".json") {
                    entries.push_back({
                        entry.path().filename().string(),
                        entry.path().string(),
                        entry.last_write_time(ec),
                        entry.file_size(ec)
                    });
                }
            }
            std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
                return a.ftime > b.ftime;
            });
            for (const auto& be : entries) {
                json item;
                item["filename"] = be.filename;
                item["path"] = be.path;
                item["size"] = be.size;
                res.push_back(item);
            }
        }
        if (PrismaUI && view != 0) {
            PrismaUI->InteropCall(view, "onBackupsListReceived", res.dump().c_str());
        }
    } catch (...) {}
}

void ClearThumbnailCacheListener(const char* data) {
    (void)data;
    try {
        std::filesystem::path thumbDir = "Data/PrismaUI/views/PrismaOutfitWheel/thumbnails";
        std::error_code ec;
        uint32_t removed = 0;
        if (std::filesystem::exists(thumbDir, ec) && std::filesystem::is_directory(thumbDir, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(thumbDir, ec)) {
                if (entry.is_regular_file(ec) && entry.path().extension() == ".png") {
                    std::filesystem::remove(entry.path(), ec);
                    ++removed;
                }
            }
        }
        OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().RescanCache();
        if (PrismaUI && view != 0) {
            json res;
            res["success"] = true;
            res["removed"] = removed;
            PrismaUI->InteropCall(view, "onThumbnailCacheCleared", res.dump().c_str());
        }
        RE::DebugNotification(("Thumbnail cache cleared: " + std::to_string(removed) + " files removed").c_str());
    } catch (...) {}
}

void RequestThumbnailListener(const char* data) {
    if (!g_showItemPreviews.load(std::memory_order_acquire)) return;
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

void NotifyThumbnailFailed(uint32_t a_formID) {
    if (!PrismaUI || view == 0) return;
    try {
        PrismaUI->InteropCall(view, "onThumbnailFailed", std::to_string(a_formID).c_str());
    } catch (...) {}
}

void NotifyThumbnailsBatchReady(const std::vector<std::pair<RE::FormID, std::string>>& a_items) {
    if (!PrismaUI || view == 0 || a_items.empty()) return;
    try {
        json arr = json::array();
        for (const auto& [fID, rPath] : a_items) {
            arr.push_back({
                { "formID", fID },
                { "rawID", fID },
                { "thumbnail", rPath }
            });
        }
        PrismaUI->InteropCall(view, "onThumbnailReady", arr.dump().c_str());
    } catch (...) {}
}

void RequestModCatalogSummaryListener(const char*) {
    if (!PrismaUI || view == 0) return;
    SKSE::GetTaskInterface()->AddTask([]() {
        if (!PrismaUI || view == 0) return;
        try {
            json summaries = ModCatalogManager::GetSingleton()->GetModSummaries();
            PrismaUI->InteropCall(view, "onModCatalogSummaryReady", summaries.dump().c_str());
        } catch (...) {}
    });
}

void RequestModCatalogItemsListener(const char* data) {
    if (!PrismaUI || view == 0) return;
    try {
        json req = json::parse(data ? data : "{}");
        std::string modName = req.value("modName", "");
        if (modName.empty()) return;
        SKSE::GetTaskInterface()->AddTask([modName]() {
            if (!PrismaUI || view == 0) return;
            try {
                json items = ModCatalogManager::GetSingleton()->GetModItems(modName);
                json res = {
                    {"modName", modName},
                    {"items", items}
                };
                PrismaUI->InteropCall(view, "onModCatalogItemsReady", res.dump().c_str());
            } catch (...) {}
        });
    } catch (...) {}
}

void SpawnModItemListener(const char* data) {
    try {
        json req = json::parse(data ? data : "{}");
        uint32_t rawID = 0;
        if (req.contains("rawID") && req["rawID"].is_number()) {
            rawID = req["rawID"].get<uint32_t>();
        } else if (req.contains("id") && req["id"].is_string()) {
            try { rawID = static_cast<uint32_t>(std::stoul(req["id"].get<std::string>(), nullptr, 16)); } catch (...) {}
        }
        if (rawID == 0) return;
        std::string targetActorId = req.value("targetActorId", "0x00000007");
        bool autoEquip = req.value("autoEquip", false);
        std::string destination = req.value("destination", "INVENTORY");

        SKSE::GetTaskInterface()->AddTask([rawID, targetActorId, autoEquip, destination]() {
            bool success = ModCatalogManager::GetSingleton()->SpawnItem(rawID, targetActorId, autoEquip, destination);
            if (success && autoEquip && PrismaUI && view != 0) {
                json res;
                res["rawID"] = rawID;
                res["id"] = FormatFormID(rawID);
                res["equipped"] = true;
                PrismaUI->InteropCall(view, "onItemEquipToggled", res.dump().c_str());
            }
        });
    } catch (...) {}
}

void SetStudioPoseListener(const char* data) {
    try {
        json req = json::parse(data ? data : "{}");
        std::string actorIDStr = req.value("actorID", "");
        std::string eventName = req.value("eventName", "IdleForceDefaultState");
        uint32_t actorID = 0;
        if (!actorIDStr.empty()) {
            try { actorID = static_cast<uint32_t>(std::stoul(actorIDStr, nullptr, 16)); } catch (...) {}
        }
        if (actorID == 0) {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            actorID = (g_manager.activeTargetActorID != 0) ? g_manager.activeTargetActorID : 0x00000007;
        }

        SKSE::GetTaskInterface()->AddTask([actorID, eventName]() {
#if __has_include(<RE/Skyrim.h>)
            RE::Actor* actor = nullptr;
            if (actorID == 0x00000007 || actorID == 0x00000014 || actorID == 0) {
                actor = RE::PlayerCharacter::GetSingleton();
            } else {
                auto* form = RE::TESForm::LookupByID(actorID);
                actor = form ? form->As<RE::Actor>() : nullptr;
                if (!actor) {
                    actor = RE::PlayerCharacter::GetSingleton();
                }
            }
            if (!actor) return;

            if (actor->IsPlayerRef()) {
                auto* playerCam = RE::PlayerCamera::GetSingleton();
                if (playerCam && playerCam->IsInFirstPerson()) {
                    playerCam->ForceThirdPerson();
                }
            }

            if (eventName == "IdleForceDefaultState" || eventName == "IdleStop") {
                actor->NotifyAnimationGraph("IdleStop");
                actor->NotifyAnimationGraph("IdleStop_Loose");
                actor->NotifyAnimationGraph("IdleForceDefaultState");
                return;
            }

            bool played = false;
            auto* currentProc = actor->GetActorRuntimeData().currentProcess;
            if (currentProc) {
                if (eventName.rfind("0x", 0) == 0 || eventName.rfind("0X", 0) == 0) {
                    try {
                        uint32_t fId = static_cast<uint32_t>(std::stoul(eventName, nullptr, 16));
                        auto* f = RE::TESForm::LookupByID(fId);
                        if (f) {
                            auto* idl = f->As<RE::TESIdleForm>();
                            if (idl) played = currentProc->PlayIdle(actor, idl, actor);
                        }
                    } catch (...) {}
                }
                if (!played) {
                    auto* idle = RE::TESForm::LookupByEditorID<RE::TESIdleForm>(eventName);
                    if (idle) {
                        played = currentProc->PlayIdle(actor, idle, actor);
                    }
                }
                if (!played) {
                    auto* handler = RE::TESDataHandler::GetSingleton();
                    if (handler) {
                        const auto& idles = handler->GetFormArray<RE::TESIdleForm>();
                        for (auto* idl : idles) {
                            if (!idl) continue;
                            const char* eid = idl->GetFormEditorID();
                            if (eid && _stricmp(eid, eventName.c_str()) == 0) {
                                played = currentProc->PlayIdle(actor, idl, actor);
                                break;
                            }
                        }
                    }
                }
            }
            if (!played) {
                actor->NotifyAnimationGraph(eventName);
            }
#endif
        });
    } catch (...) {}
}

void RequestPosePacksListener(const char*) {
    if (!PrismaUI || view == 0) return;
    try {
        json packs = GetDiscoveredPosePacksJson();
        PrismaUI->InteropCall(view, "onPosePacksReady", packs.dump().c_str());
    } catch (...) {}
}

void RegisterJSListeners(PrismaView v) {
    if (!PrismaUI || !v) return;

    PrismaUI->RegisterJSListener(v, "SaveSettings", SaveSettingsListener);
    PrismaUI->RegisterJSListener(v, "SetTypingMode", SetTypingModeListener);
    PrismaUI->RegisterJSListener(v, "SetTypingState", SetTypingModeListener);
    PrismaUI->RegisterJSListener(v, "SortOutfitsAlphabetically", SortOutfitsAlphabeticallyListener);
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
    PrismaUI->RegisterJSListener(v, "RequestModCatalogSummary", RequestModCatalogSummaryListener);
    PrismaUI->RegisterJSListener(v, "RequestModCatalogItems", RequestModCatalogItemsListener);
    PrismaUI->RegisterJSListener(v, "SpawnModItem", SpawnModItemListener);
    PrismaUI->RegisterJSListener(v, "SetStudioPose", SetStudioPoseListener);
    PrismaUI->RegisterJSListener(v, "RequestPosePacks", RequestPosePacksListener);
    PrismaUI->RegisterJSListener(v, "SetSlotColor", SetSlotColorListener);
    PrismaUI->RegisterJSListener(v, "SetWheelColor", SetWheelColorListener);
    PrismaUI->RegisterJSListener(v, "GetBackupsList", GetBackupsListListener);
    PrismaUI->RegisterJSListener(v, "ClearThumbnailCache", ClearThumbnailCacheListener);
}