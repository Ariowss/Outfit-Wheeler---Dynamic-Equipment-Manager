#include "InputHandler.h"
#include "Settings.h"
#include "OutfitManager.h"
#include "PrismaUI_API.h"
#include <SKSE/SKSE.h>
#include <windows.h>
#include <Xinput.h>
#include <atomic>
#include <chrono>
#include <cmath>
#include <string>

#pragma comment(lib, "Xinput.lib")

extern PRISMA_UI_API::IVPrismaUI2* PrismaUI;
extern PrismaView view;

std::atomic<bool> isMenuOpen{ false };
std::atomic<bool> g_isTyping{ false };
bool g_inputRegistered = false;

static std::chrono::steady_clock::time_point g_lastToggleTime{};
static std::chrono::steady_clock::time_point g_lastFilterTime{};
static std::chrono::steady_clock::time_point g_lastWheelTime{};
static uint32_t g_currentGamepadMask = 0;
static float g_lastSentAngle = -999.0f;

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

static inline bool IsModifierMatch(uint32_t modKey, bool shift, bool ctrl, bool alt) {
    if (modKey == 0) return true;
    if ((modKey == 0x2A || modKey == 0x36 || modKey == 42) && shift) return true;
    if ((modKey == 0x1D || modKey == 0x9D || modKey == 29) && ctrl) return true;
    if ((modKey == 0x38 || modKey == 0xB8 || modKey == 56) && alt) return true;
    return false;
}

void ShowWheel(bool isEditMode) {
    if (!PrismaUI || view == 0) return;
    std::string wheelDataJson = GetWheelJson(isEditMode);
    PrismaUI->Show(view);
    PrismaUI->Focus(view, true, false);
    PrismaUI->InteropCall(view, "openWheel", wheelDataJson.c_str());

    auto now = std::chrono::steady_clock::now();
    g_lastWheelTime = now + std::chrono::milliseconds(400);
    g_lastFilterTime = now + std::chrono::milliseconds(400);
    g_lastSentAngle = -999.0f;

    isMenuOpen.store(true, std::memory_order_release);
}

void HideWheel() {
    if (PrismaUI && view != 0) {
        PrismaUI->Unfocus(view);
        PrismaUI->Hide(view);
    }
    g_lastSentAngle = -999.0f;
    isMenuOpen.store(false, std::memory_order_release);
}

WheelInputHandler* WheelInputHandler::GetSingleton() {
    static WheelInputHandler singleton;
    return &singleton;
}

void WheelInputHandler::Register() {
    if (g_inputRegistered) return;
    auto inputDevice = RE::BSInputDeviceManager::GetSingleton();
    if (inputDevice) {
        inputDevice->AddEventSink(GetSingleton());
        g_inputRegistered = true;
    }
}

RE::BSEventNotifyControl WheelInputHandler::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) {
    if (!a_event || !*a_event) return RE::BSEventNotifyControl::kContinue;

    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->Is3DLoaded()) return RE::BSEventNotifyControl::kContinue;

    auto ui = RE::UI::GetSingleton();
    if (!ui) return RE::BSEventNotifyControl::kContinue;

    bool currentlyOpen = isMenuOpen.load(std::memory_order_acquire);
    bool isTypingActive = g_isTyping.load(std::memory_order_acquire);

    if (!currentlyOpen) {
        if (isTypingActive) return RE::BSEventNotifyControl::kContinue;

        bool isInventoryOpen = ui->IsMenuOpen(MENU_INVENTORY);
        bool hasBlockingMenu = ui->IsMenuOpen(MENU_CONSOLE) || ui->IsMenuOpen(MENU_RACEMENU) ||
                              ui->IsMenuOpen(MENU_MAGIC) || ui->IsMenuOpen(MENU_CONTAINER) ||
                              ui->IsMenuOpen(MENU_BARTER) || ui->IsMenuOpen(MENU_BOOK) ||
                              ui->IsMenuOpen(MENU_JOURNAL) || ui->IsMenuOpen(MENU_MAP) ||
                              ui->IsMenuOpen(MENU_LOCKPICKING) || ui->IsMenuOpen(MENU_CRAFTING) ||
                              ui->IsMenuOpen(MENU_DIALOGUE) || ui->IsMenuOpen(MENU_FADER);

        if (hasBlockingMenu || (ui->GameIsPaused() && !isInventoryOpen)) {
            return RE::BSEventNotifyControl::kContinue;
        }
    } else {
        XINPUT_STATE xState;
        ZeroMemory(&xState, sizeof(XINPUT_STATE));
        if (XInputGetState(0, &xState) == ERROR_SUCCESS) {
            float lx = static_cast<float>(xState.Gamepad.sThumbLX);
            float ly = static_cast<float>(xState.Gamepad.sThumbLY);

            float normX = lx / 32767.0f;
            float normY = ly / 32767.0f;
            float mag = std::sqrt(normX * normX + normY * normY);

            if (mag >= g_thumbstickDeadzone) {
                float phi = std::atan2(normX, normY);
                if (phi < 0.0f) phi += 6.2831853f;

                if (std::abs(phi - g_lastSentAngle) > 0.05f) {
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

    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0 || (GetKeyState(VK_LSHIFT) & 0x8000) != 0 || (GetKeyState(VK_RSHIFT) & 0x8000) != 0;
    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0 || (GetKeyState(VK_LCONTROL) & 0x8000) != 0 || (GetKeyState(VK_RCONTROL) & 0x8000) != 0;
    bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0 || (GetKeyState(VK_LMENU) & 0x8000) != 0 || (GetKeyState(VK_RMENU) & 0x8000) != 0;

    for (auto event = *a_event; event; event = event->next) {
        if (event->eventType == RE::INPUT_EVENT_TYPE::kButton) {
            auto buttonEvent = event->AsButtonEvent();
            if (!buttonEvent) continue;

            auto device = buttonEvent->GetDevice();
            uint32_t keyCode = buttonEvent->GetIDCode();

            if (device == RE::INPUT_DEVICE::kKeyboard && buttonEvent->IsPressed()) {
                if (keyCode == 0x01 && currentlyOpen) {
                    SKSE::GetTaskInterface()->AddTask([]() { HideWheel(); });
                    return RE::BSEventNotifyControl::kContinue;
                }

                if (!isTypingActive) {
                    bool editModMatch = IsModifierMatch(g_editModifierKey, shift, ctrl, alt);
                    bool playModMatch = IsModifierMatch(g_playModifierKey, shift, ctrl, alt);

                    bool triggerEdit = (keyCode == g_editKey && editModMatch);
                    bool triggerPlay = (keyCode == g_playKey && playModMatch);

                    if (triggerEdit || triggerPlay) {
                        auto now = std::chrono::steady_clock::now();
                        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastToggleTime).count() < 200) {
                            return RE::BSEventNotifyControl::kContinue;
                        }
                        g_lastToggleTime = now;

                        bool targetMode = triggerEdit;
                        SKSE::GetTaskInterface()->AddTask([targetMode]() {
                            if (!isMenuOpen.load(std::memory_order_acquire)) ShowWheel(targetMode);
                            else HideWheel();
                        });
                    }
                }
            } else if (device == RE::INPUT_DEVICE::kGamepad) {
                if (buttonEvent->IsDown()) g_currentGamepadMask |= keyCode;
                else if (buttonEvent->IsUp()) g_currentGamepadMask &= ~keyCode;

                if (buttonEvent->IsPressed()) {
                    auto now = std::chrono::steady_clock::now();

                    if (currentlyOpen) {
                        SKSE::GetTaskInterface()->AddTask([keyCode]() {
                            if (PrismaUI && view != 0) {
                                PrismaUI->InteropCall(view, "onGamepadButtonPress", std::to_string(keyCode).c_str());
                            }
                        });

                        if (keyCode == 0x2000 || keyCode == 0x0020 || keyCode == 0x0010) {
                            SKSE::GetTaskInterface()->AddTask([]() { HideWheel(); });
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
                                g_lastWheelTime = now + std::chrono::milliseconds(250);
                                int dir = (keyCode == 0x0100) ? -1 : 1;
                                SKSE::GetTaskInterface()->AddTask([dir]() {
                                    if (PrismaUI && view != 0) PrismaUI->InteropCall(view, "gamepadCycleWheel", std::to_string(dir).c_str());
                                });
                            }
                            return RE::BSEventNotifyControl::kContinue;
                        }
                        if (keyCode == 0x0009 || keyCode == 0x000A) {
                            if (now >= g_lastFilterTime) {
                                g_lastFilterTime = now + std::chrono::milliseconds(250);
                                int dir = (keyCode == 0x0009) ? -1 : 1;
                                SKSE::GetTaskInterface()->AddTask([dir]() {
                                    if (PrismaUI && view != 0) PrismaUI->InteropCall(view, "gamepadCycleFilter", std::to_string(dir).c_str());
                                });
                            }
                            return RE::BSEventNotifyControl::kContinue;
                        }
                    } else {
                        if (!isTypingActive) {
                            bool gpPlayModMatch = (g_gamepadPlayModifier == 0) || ((g_currentGamepadMask & g_gamepadPlayModifier) != 0);
                            bool gpTriggerPlay = (keyCode == g_gamepadPlayKey && gpPlayModMatch);

                            if (gpTriggerPlay) {
                                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastToggleTime).count() < 200) {
                                    return RE::BSEventNotifyControl::kContinue;
                                }
                                g_lastToggleTime = now;

                                SKSE::GetTaskInterface()->AddTask([]() { ShowWheel(false); });
                            }
                        }
                    }
                }
            }
        }
    }
    return RE::BSEventNotifyControl::kContinue;
}