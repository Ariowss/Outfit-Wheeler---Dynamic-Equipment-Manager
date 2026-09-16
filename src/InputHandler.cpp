#include "pch.h"
#include "InputHandler.h"
#include "Settings.h"
#include "OutfitManager.h"
#include "EquipmentManager.h"
#include "PrismaUI_API.h"
#include "JSInterop.h"
#include "ExecutionQueue.h"
#include "InGameTestRunner.h"
#include <RE/C/CrosshairPickData.h>
#include <RE/P/PlayerControls.h>
#include <RE/C/ControlMap.h>
#include <SKSE/SKSE.h>
#include <windows.h>
#include <Xinput.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <string>

#pragma comment(lib, "Xinput.lib")

#define OUTFITWHEELER_EXPORTS
#include "OutfitWheelerAPI.h"

extern PRISMA_UI_API::IVPrismaUI2* PrismaUI;
extern PrismaView view;

std::atomic<bool> isMenuOpen{ false };
std::atomic<bool> g_isTyping{ false };
std::atomic<bool> g_isEditModeOpen{ false };
std::atomic<bool> g_inputRegistered{ false };
std::atomic<bool> g_hotkeysEnabled{ true };

static std::chrono::steady_clock::time_point g_lastToggleTime{};
static std::chrono::steady_clock::time_point g_lastFilterTime{};
static std::chrono::steady_clock::time_point g_lastWheelTime{};
static uint32_t g_currentGamepadMask = 0;
static float g_lastSentAngle = -999.0f;
static float g_lockedZoomOffset = 1.0f;
static std::atomic<bool> g_hasLockedZoom{ false };

static const RE::BSFixedString MENU_CONSOLE("Console");
static const RE::BSFixedString MENU_RACEMENU("RaceMenu");
static const RE::BSFixedString MENU_INVENTORY("InventoryMenu");
static const RE::BSFixedString MENU_MAGIC("MagicMenu");
static const RE::BSFixedString MENU_CONTAINER("ContainerMenu");
static const RE::BSFixedString MENU_BARTER("BarterMenu");
static const RE::BSFixedString MENU_BOOK("Book Menu");
static const RE::BSFixedString MENU_JOURNAL("Journal Menu");
static const RE::BSFixedString MENU_MAP("MapMenu");
static const RE::BSFixedString MENU_LOCKPICKING("Lockpicking Menu");
static const RE::BSFixedString MENU_CRAFTING("Crafting Menu");
static const RE::BSFixedString MENU_DIALOGUE("Dialogue Menu");
static const RE::BSFixedString MENU_FADER("Fader Menu");
static const RE::BSFixedString MENU_LOADING("Loading Menu");

static inline bool IsModifierMatch(uint32_t modKey, bool shift, bool ctrl, bool alt) {
    if (modKey == 0) return !shift && !ctrl && !alt;
    if ((modKey == 0x2A || modKey == 0x36 || modKey == 42 || modKey == 54) && shift) return true;
    if ((modKey == 0x1D || modKey == 0x9D || modKey == 29 || modKey == 157) && ctrl) return true;
    if ((modKey == 0x38 || modKey == 0xB8 || modKey == 56 || modKey == 184) && alt) return true;
    return false;
}

static bool GetActiveGamepadState(XINPUT_STATE* outState) {
    for (DWORD i = 0; i < 4; ++i) {
        ZeroMemory(outState, sizeof(XINPUT_STATE));
        if (XInputGetState(i, outState) == ERROR_SUCCESS) {
            return true;
        }
    }
    return false;
}

void OpenWheel(bool isEditMode) {
    if (!PrismaUI || view == 0) return;

    auto pc = RE::PlayerControls::GetSingleton();
    if (pc) pc->blockPlayerInput = true;
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

    auto cam = RE::PlayerCamera::GetSingleton();
    if (cam && cam->currentState && (cam->currentState->id == RE::CameraState::kThirdPerson || cam->currentState->id == RE::CameraState::kAutoVanity)) {
        auto* tpState = static_cast<RE::ThirdPersonState*>(cam->currentState.get());
        if (tpState) {
            g_lockedZoomOffset = tpState->targetZoomOffset;
            g_hasLockedZoom.store(true, std::memory_order_release);
        }
    }

    SKSE::GetTaskInterface()->AddTask([isEditMode]() {
        if (!PrismaUI || view == 0) return;

        RE::FormID targetID = 0x00000007;
        if (!isEditMode) {
            auto* crosshair = RE::CrosshairPickData::GetSingleton();
            if (crosshair && crosshair->target) {
                auto targetRef = crosshair->target->get();
                if (targetRef) {
                    auto* actor = targetRef->As<RE::Actor>();
                    if (actor && actor->IsPlayerTeammate() && !actor->IsDead()) {
                        targetID = actor->GetFormID();
                    }
                }
            }
        } else {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            targetID = g_manager.activeTargetActorID;
        }

        {
            std::lock_guard<std::recursive_mutex> lock(g_dataMutex);
            g_manager.activeTargetActorID = targetID;
        }

        bool wasEditMode = g_isEditModeOpen.load(std::memory_order_acquire);
        if (wasEditMode && !isEditMode) {
            SetupStudioCameraAndControls(false);
        }

        if (isEditMode && g_editUiMode == 1) {
            InitializeStudioStage();
            SetupStudioCameraAndControls(true);
        }

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

        auto cam = RE::PlayerCamera::GetSingleton();
        if (cam && cam->currentState && (cam->currentState->id == RE::CameraState::kThirdPerson || cam->currentState->id == RE::CameraState::kAutoVanity)) {
            auto* tpState = static_cast<RE::ThirdPersonState*>(cam->currentState.get());
            if (tpState) {
                g_lockedZoomOffset = tpState->targetZoomOffset;
                g_hasLockedZoom.store(true, std::memory_order_release);
            }
        }

        PrismaUI->Show(view);
        PrismaUI->Focus(view, false, false);
        std::string wheelDataJson = GetWheelJson(isEditMode);
        PrismaUI->InteropCall(view, "openWheel", wheelDataJson.c_str());

        auto now = std::chrono::steady_clock::now();
        g_lastWheelTime = now + std::chrono::milliseconds(400);
        g_lastFilterTime = now + std::chrono::milliseconds(400);
        g_lastSentAngle = -999.0f;

        g_isEditModeOpen.store(isEditMode, std::memory_order_release);
        isMenuOpen.store(true, std::memory_order_release);
    });
}

void ShowWheel(bool isEditMode) {
    OpenWheel(isEditMode);
}

void HideWheel() {
    if (!PrismaUI || view == 0) return;

    auto pc = RE::PlayerControls::GetSingleton();
    if (pc) pc->blockPlayerInput = false;
    auto controlMap = RE::ControlMap::GetSingleton();
    if (controlMap) {
        controlMap->ToggleControls(RE::ControlMap::UEFlag::kAll, true);
    }
    SetCameraInputHandlingEnabled(true);
    g_hasLockedZoom.store(false, std::memory_order_release);

    SKSE::GetTaskInterface()->AddTask([]() {
        if (!PrismaUI || view == 0) return;

        RestoreStudioActors();
        SetupStudioCameraAndControls(false);

        PrismaUI->Unfocus(view);
        PrismaUI->Hide(view);

        g_lastSentAngle = -999.0f;
        g_isTyping.store(false, std::memory_order_release);
        g_hasLockedZoom.store(false, std::memory_order_release);
        auto controlMap = RE::ControlMap::GetSingleton();
        if (controlMap) {
            controlMap->ToggleControls(RE::ControlMap::UEFlag::kAll, true);
        }
        SetCameraInputHandlingEnabled(true);
        g_isEditModeOpen.store(false, std::memory_order_release);
        isMenuOpen.store(false, std::memory_order_release);
    });
}

WheelInputHandler* WheelInputHandler::GetSingleton() {
    static WheelInputHandler singleton;
    return &singleton;
}

void WheelInputHandler::OpenWheel(bool isEditMode) {
    ::OpenWheel(isEditMode);
}

void WheelInputHandler::HideWheel() {
    ::HideWheel();
}

void WheelInputHandler::Register() {
    if (g_inputRegistered.load(std::memory_order_acquire)) return;
    auto inputDevice = RE::BSInputDeviceManager::GetSingleton();
    if (inputDevice) {
        inputDevice->PrependEventSink(GetSingleton());
        g_inputRegistered.store(true, std::memory_order_release);
    }
}

RE::BSEventNotifyControl WheelInputHandler::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) {
    if (!a_event || !*a_event) return RE::BSEventNotifyControl::kContinue;
    if (!g_gameFullyLoaded.load(std::memory_order_acquire)) return RE::BSEventNotifyControl::kContinue;

    bool currentlyOpen = isMenuOpen.load(std::memory_order_acquire);
    bool isEditMode = g_isEditModeOpen.load(std::memory_order_acquire);
    bool isTypingActive = g_isTyping.load(std::memory_order_acquire);

    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->Is3DLoaded() || player->IsDead()) return RE::BSEventNotifyControl::kContinue;

    auto ui = RE::UI::GetSingleton();
    if (!ui) return RE::BSEventNotifyControl::kContinue;

    if (!currentlyOpen) {
        if (isTypingActive) return RE::BSEventNotifyControl::kContinue;

        bool isInventoryOpen = ui->IsMenuOpen(MENU_INVENTORY);
        bool hasBlockingMenu = ui->IsMenuOpen(MENU_CONSOLE) || ui->IsMenuOpen(MENU_RACEMENU) ||
                              ui->IsMenuOpen(MENU_MAGIC) || ui->IsMenuOpen(MENU_CONTAINER) ||
                              ui->IsMenuOpen(MENU_BARTER) || ui->IsMenuOpen(MENU_BOOK) ||
                              ui->IsMenuOpen(MENU_JOURNAL) || ui->IsMenuOpen(MENU_MAP) ||
                              ui->IsMenuOpen(MENU_LOCKPICKING) || ui->IsMenuOpen(MENU_CRAFTING) ||
                              ui->IsMenuOpen(MENU_DIALOGUE) || ui->IsMenuOpen(MENU_FADER) ||
                              ui->IsMenuOpen(MENU_LOADING);

        if (hasBlockingMenu || (ui->GameIsPaused() && !isInventoryOpen)) {
            return RE::BSEventNotifyControl::kContinue;
        }

        static uint64_t lastProcessedSession = 0;
        uint64_t currentSession = g_gameSessionGeneration.load(std::memory_order_acquire);
        if (lastProcessedSession != currentSession) {
            lastProcessedSession = currentSession;
            isMenuOpen.store(false, std::memory_order_release);
            g_isEditModeOpen.store(false, std::memory_order_release);
            g_isTyping.store(false, std::memory_order_release);
        }
    } else {
        if (!isEditMode) {
            XINPUT_STATE xState;
            if (GetActiveGamepadState(&xState)) {
                float rx = static_cast<float>(xState.Gamepad.sThumbRX) / 32767.0f;
                float ry = static_cast<float>(xState.Gamepad.sThumbRY) / 32767.0f;
                float rMag = std::sqrt(rx * rx + ry * ry);

                float lx = static_cast<float>(xState.Gamepad.sThumbLX) / 32767.0f;
                float ly = static_cast<float>(xState.Gamepad.sThumbLY) / 32767.0f;
                float lMag = std::sqrt(lx * lx + ly * ly);

                float useX = 0.0f;
                float useY = 0.0f;
                float useMag = 0.0f;

                if (rMag >= g_thumbstickDeadzone) {
                    useX = rx;
                    useY = ry;
                    useMag = rMag;
                } else if (lMag >= g_thumbstickDeadzone) {
                    useX = lx;
                    useY = ly;
                    useMag = lMag;
                }

                if (useMag >= g_thumbstickDeadzone) {
                    float phi = std::atan2(useX, useY);
                    if (phi < 0.0f) phi += 6.2831853f;

                    if (std::abs(phi - g_lastSentAngle) > 0.03f) {
                        g_lastSentAngle = phi;
                        SKSE::GetTaskInterface()->AddTask([phi]() {
                            if (PrismaUI && view != 0) {
                                PrismaUI->InteropCall(view, "setGamepadAngle", std::to_string(phi).c_str());
                            }
                        });
                    }
                }
            }
        }
    }

    bool shift = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) || ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0) || ((GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0);
    bool ctrl = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) || ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) || ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0);
    bool alt = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) || ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0) || ((GetAsyncKeyState(VK_RMENU) & 0x8000) != 0);

    auto now = std::chrono::steady_clock::now();

    if (currentlyOpen) {
        auto playerControls = RE::PlayerControls::GetSingleton();
        if (playerControls && !playerControls->blockPlayerInput) {
            playerControls->blockPlayerInput = true;
        }
        if (g_hasLockedZoom.load(std::memory_order_acquire)) {
            auto cam = RE::PlayerCamera::GetSingleton();
            if (cam && cam->currentState && (cam->currentState->id == RE::CameraState::kThirdPerson || cam->currentState->id == RE::CameraState::kAutoVanity)) {
                auto* tpState = static_cast<RE::ThirdPersonState*>(cam->currentState.get());
                if (tpState) {
                    tpState->targetZoomOffset = g_lockedZoomOffset;
                    tpState->currentZoomOffset = g_lockedZoomOffset;
                }
            }
        }
    }

    for (auto event = *a_event; event; event = event->next) {
        if (event->eventType == RE::INPUT_EVENT_TYPE::kButton) {
            auto buttonEvent = event->AsButtonEvent();
            if (!buttonEvent) continue;

            auto device = buttonEvent->GetDevice();
            uint32_t keyCode = buttonEvent->GetIDCode();

            if (device == RE::INPUT_DEVICE::kKeyboard) {
                if (buttonEvent->IsUp()) {
                    if (keyCode == g_editKey || keyCode == g_playKey) {
                        buttonEvent->GetRuntimeData().heldDownSecs = 0.0f;
                        buttonEvent->GetRuntimeData().value = 0.0f;
                        return RE::BSEventNotifyControl::kStop;
                    }
                }

                if (buttonEvent->IsPressed()) {
                    if (!isTypingActive) {
                        if (keyCode == 0x57 && shift) {
                            RunInGameLiveDiagnostics();
                            return RE::BSEventNotifyControl::kContinue;
                        }

                        if (keyCode == 0x01 && currentlyOpen) {
                            HideWheel();
                            buttonEvent->GetRuntimeData().heldDownSecs = 0.0f;
                            buttonEvent->GetRuntimeData().value = 0.0f;
                            return RE::BSEventNotifyControl::kStop;
                        }

                        bool hotkeysEnabled = g_hotkeysEnabled.load(std::memory_order_acquire);
                        bool editModMatch = IsModifierMatch(g_editModifierKey, shift, ctrl, alt);
                        bool playModMatch = IsModifierMatch(g_playModifierKey, shift, ctrl, alt);

                        bool triggerEdit = hotkeysEnabled && (keyCode == g_editKey && editModMatch);
                        bool triggerPlay = hotkeysEnabled && (keyCode == g_playKey && playModMatch);

                        if (triggerEdit || triggerPlay) {
                            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastToggleTime).count() < 200) {
                                buttonEvent->GetRuntimeData().heldDownSecs = 0.0f;
                                buttonEvent->GetRuntimeData().value = 0.0f;
                                return RE::BSEventNotifyControl::kStop;
                            }
                            g_lastToggleTime = now;

                            bool targetMode = triggerEdit;
                            if (!isMenuOpen.load(std::memory_order_acquire)) OpenWheel(targetMode);
                            else HideWheel();

                            buttonEvent->GetRuntimeData().heldDownSecs = 0.0f;
                            buttonEvent->GetRuntimeData().value = 0.0f;
                            return RE::BSEventNotifyControl::kStop;
                        }
                    }
                }
            } else if (device == RE::INPUT_DEVICE::kGamepad) {
                if (currentlyOpen && isEditMode) {
                    return RE::BSEventNotifyControl::kContinue;
                }

                if (buttonEvent->IsDown()) g_currentGamepadMask |= keyCode;
                else if (buttonEvent->IsUp()) g_currentGamepadMask &= ~keyCode;

                if (buttonEvent->IsPressed()) {
                    if (currentlyOpen && !isEditMode) {
                        SKSE::GetTaskInterface()->AddTask([keyCode]() {
                            if (PrismaUI && view != 0) {
                                PrismaUI->InteropCall(view, "onGamepadButtonPress", std::to_string(keyCode).c_str());
                            }
                        });

                        if (keyCode == 0x2000 || keyCode == 0x0020 || keyCode == 0x0010) {
                            if (!isTypingActive) {
                                HideWheel();
                            }
                            return RE::BSEventNotifyControl::kContinue;
                        }

                        if (keyCode == 0x1000) {
                            SKSE::GetTaskInterface()->AddTask([]() {
                                if (PrismaUI && view != 0) PrismaUI->InteropCall(view, "gamepadConfirm", "");
                            });
                            return RE::BSEventNotifyControl::kContinue;
                        }

                        if (keyCode == 0x0100 || keyCode == 0x0200) {
                            if (now >= g_lastWheelTime) {
                                g_lastWheelTime = now + std::chrono::milliseconds(220);
                                int dir = (keyCode == 0x0100) ? -1 : 1;
                                SKSE::GetTaskInterface()->AddTask([dir]() {
                                    if (PrismaUI && view != 0) PrismaUI->InteropCall(view, "gamepadCycleWheel", std::to_string(dir).c_str());
                                });
                            }
                            return RE::BSEventNotifyControl::kContinue;
                        }

                        if (keyCode == 0x0009 || keyCode == 0x000A) {
                            if (now >= g_lastFilterTime) {
                                g_lastFilterTime = now + std::chrono::milliseconds(220);
                                int dir = (keyCode == 0x0009) ? -1 : 1;
                                SKSE::GetTaskInterface()->AddTask([dir]() {
                                    if (PrismaUI && view != 0) PrismaUI->InteropCall(view, "gamepadCycleFilter", std::to_string(dir).c_str());
                                });
                            }
                            return RE::BSEventNotifyControl::kContinue;
                        }
                    } else if (!currentlyOpen) {
                        if (!isTypingActive && g_hotkeysEnabled.load(std::memory_order_acquire)) {
                            bool gpPlayModMatch = (g_gamepadPlayModifier == 0) || ((g_currentGamepadMask & g_gamepadPlayModifier) != 0);
                            bool gpTriggerPlay = (keyCode == g_gamepadPlayKey && gpPlayModMatch);

                            if (gpTriggerPlay) {
                                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastToggleTime).count() < 200) {
                                    return RE::BSEventNotifyControl::kContinue;
                                }
                                g_lastToggleTime = now;

                                OpenWheel(false);
                                return RE::BSEventNotifyControl::kContinue;
                            }
                        }
                    }
                }
            }
        }
    }
    return RE::BSEventNotifyControl::kContinue;
}

static bool CanOpenWheelFromAPI() {
    if (!PrismaUI || view == 0) return false;
    if (!g_gameFullyLoaded.load(std::memory_order_acquire)) return false;

    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->Is3DLoaded() || player->IsDead()) return false;

    auto ui = RE::UI::GetSingleton();
    if (!ui) return false;

    bool isInventoryOpen = ui->IsMenuOpen(MENU_INVENTORY);
    bool hasBlockingMenu = ui->IsMenuOpen(MENU_CONSOLE) || ui->IsMenuOpen(MENU_RACEMENU) ||
                          ui->IsMenuOpen(MENU_MAGIC) || ui->IsMenuOpen(MENU_CONTAINER) ||
                          ui->IsMenuOpen(MENU_BARTER) || ui->IsMenuOpen(MENU_BOOK) ||
                          ui->IsMenuOpen(MENU_JOURNAL) || ui->IsMenuOpen(MENU_MAP) ||
                          ui->IsMenuOpen(MENU_LOCKPICKING) || ui->IsMenuOpen(MENU_CRAFTING) ||
                          ui->IsMenuOpen(MENU_DIALOGUE) || ui->IsMenuOpen(MENU_FADER) ||
                          ui->IsMenuOpen(MENU_LOADING);

    if (hasBlockingMenu || (ui->GameIsPaused() && !isInventoryOpen)) {
        return false;
    }

    return true;
}

OUTFITWHEELER_API bool OutfitWheeler_Open() {
    if (isMenuOpen.load(std::memory_order_acquire)) return true;
    if (!CanOpenWheelFromAPI()) return false;
    OpenWheel(false);
    return true;
}

OUTFITWHEELER_API bool OutfitWheeler_OpenEdit() {
    if (isMenuOpen.load(std::memory_order_acquire)) return true;
    if (!CanOpenWheelFromAPI()) return false;
    OpenWheel(true);
    return true;
}

OUTFITWHEELER_API void OutfitWheeler_Close() {
    if (isMenuOpen.load(std::memory_order_acquire)) {
        HideWheel();
    }
}

OUTFITWHEELER_API bool OutfitWheeler_IsMenuOpen() {
    return isMenuOpen.load(std::memory_order_acquire);
}

OUTFITWHEELER_API void OutfitWheeler_SetHotkeyEnabled(bool a_enabled) {
    g_hotkeysEnabled.store(a_enabled, std::memory_order_release);
}

OUTFITWHEELER_API bool OutfitWheeler_IsHotkeyEnabled() {
    return g_hotkeysEnabled.load(std::memory_order_acquire);
}

OUTFITWHEELER_API bool OpenOutfitWheeler() { return OutfitWheeler_Open(); }
OUTFITWHEELER_API void CloseOutfitWheeler() { OutfitWheeler_Close(); }
OUTFITWHEELER_API bool IsOutfitWheelerOpen() { return OutfitWheeler_IsMenuOpen(); }
OUTFITWHEELER_API void SetOutfitWheelerHotkeyEnabled(bool a_enabled) { OutfitWheeler_SetHotkeyEnabled(a_enabled); }
OUTFITWHEELER_API bool IsOutfitWheelerHotkeyEnabled() { return OutfitWheeler_IsHotkeyEnabled(); }

OUTFITWHEELER_API bool YourMod_Open() { return OutfitWheeler_Open(); }
OUTFITWHEELER_API void YourMod_Close() { OutfitWheeler_Close(); }
OUTFITWHEELER_API bool YourMod_IsMenuOpen() { return OutfitWheeler_IsMenuOpen(); }
OUTFITWHEELER_API void YourMod_SetHotkeyEnabled(bool a_enabled) { OutfitWheeler_SetHotkeyEnabled(a_enabled); }