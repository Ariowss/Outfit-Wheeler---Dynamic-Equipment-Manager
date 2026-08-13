#include "InputHandler.h"
#include "Settings.h"
#include "OutfitManager.h"
#include "PrismaUI_API.h"
#include <SKSE/SKSE.h>
#include <windows.h>
#include <atomic>
#include <chrono>

extern PRISMA_UI_API::IVPrismaUI2* PrismaUI;
extern PrismaView view;

std::atomic<bool> isMenuOpen{ false };
std::atomic<bool> g_isTyping{ false };
bool g_inputRegistered = false;

static std::chrono::steady_clock::time_point g_lastToggleTime{};

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
    if (!PrismaUI || view == 0) {
        SKSE::log::error("ShowWheel failed: PrismaUI API or View is null");
        return;
    }

    std::string wheelDataJson = GetWheelJson(isEditMode);

    SKSE::log::info("ShowWheel: Showing view");
    PrismaUI->Show(view);

    SKSE::log::info("ShowWheel: Focusing view");
    PrismaUI->Focus(view, true, false);

    SKSE::log::info("ShowWheel: Calling InteropCall openWheel");
    PrismaUI->InteropCall(view, "openWheel", wheelDataJson.c_str());

    isMenuOpen.store(true, std::memory_order_release);
}

void HideWheel() {
    if (PrismaUI && view != 0) {
        PrismaUI->Hide(view);
    }
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
        SKSE::log::info("WheelInputHandler event sink registered successfully");
    } else {
        SKSE::log::warn("BSInputDeviceManager not ready yet");
    }
}

RE::BSEventNotifyControl WheelInputHandler::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) {
    if (!a_event || !*a_event) return RE::BSEventNotifyControl::kContinue;

    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->Is3DLoaded()) {
        return RE::BSEventNotifyControl::kContinue;
    }

    auto ui = RE::UI::GetSingleton();
    if (!ui) return RE::BSEventNotifyControl::kContinue;

    bool currentlyOpen = isMenuOpen.load(std::memory_order_acquire);

    for (auto event = *a_event; event; event = event->next) {
        if (event->eventType == RE::INPUT_EVENT_TYPE::kButton) {
            auto buttonEvent = event->AsButtonEvent();
            if (buttonEvent && buttonEvent->IsPressed()) {
                auto device = buttonEvent->GetDevice();
                uint32_t keyCode = buttonEvent->GetIDCode();

                if (device == RE::INPUT_DEVICE::kKeyboard && keyCode == 0x01 && currentlyOpen) {
                    SKSE::GetTaskInterface()->AddTask([]() {
                        HideWheel();
                    });
                    return RE::BSEventNotifyControl::kContinue;
                }
            }
        }
    }

    if (g_isTyping.load(std::memory_order_acquire)) {
        return RE::BSEventNotifyControl::kContinue;
    }

    bool isInventoryOpen = ui->IsMenuOpen(MENU_INVENTORY);

    bool hasBlockingMenu = ui->IsMenuOpen(MENU_CONSOLE) ||
                          ui->IsMenuOpen(MENU_RACEMENU) ||
                          ui->IsMenuOpen(MENU_MAGIC) ||
                          ui->IsMenuOpen(MENU_CONTAINER) ||
                          ui->IsMenuOpen(MENU_BARTER) ||
                          ui->IsMenuOpen(MENU_BOOK) ||
                          ui->IsMenuOpen(MENU_JOURNAL) ||
                          ui->IsMenuOpen(MENU_MAP) ||
                          ui->IsMenuOpen(MENU_LOCKPICKING) ||
                          ui->IsMenuOpen(MENU_CRAFTING) ||
                          ui->IsMenuOpen(MENU_DIALOGUE) ||
                          ui->IsMenuOpen(MENU_FADER) ||
                          ui->IsApplicationMenuOpen() ||
                          ui->IsModalMenuOpen();

    if (hasBlockingMenu) {
        return RE::BSEventNotifyControl::kContinue;
    }

    if (ui->GameIsPaused() && !currentlyOpen && !isInventoryOpen) {
        return RE::BSEventNotifyControl::kContinue;
    }

    bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0 || (GetKeyState(VK_LSHIFT) & 0x8000) != 0 || (GetKeyState(VK_RSHIFT) & 0x8000) != 0;
    bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0 || (GetKeyState(VK_LCONTROL) & 0x8000) != 0 || (GetKeyState(VK_RCONTROL) & 0x8000) != 0;
    bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0 || (GetKeyState(VK_LMENU) & 0x8000) != 0 || (GetKeyState(VK_RMENU) & 0x8000) != 0;

    for (auto event = *a_event; event; event = event->next) {
        if (event->eventType == RE::INPUT_EVENT_TYPE::kButton) {
            auto buttonEvent = event->AsButtonEvent();
            if (buttonEvent && buttonEvent->IsPressed()) {
                auto device = buttonEvent->GetDevice();
                if (device != RE::INPUT_DEVICE::kKeyboard) {
                    continue;
                }

                uint32_t keyCode = buttonEvent->GetIDCode();

                bool editRequiresMod = (g_editModifierKey != 0);
                bool playRequiresMod = (g_playModifierKey != 0);

                bool editModMatch = IsModifierMatch(g_editModifierKey, shift, ctrl, alt);
                bool playModMatch = IsModifierMatch(g_playModifierKey, shift, ctrl, alt);

                bool triggerEdit = false;
                bool triggerPlay = false;

                if (keyCode == g_editKey && editModMatch && editRequiresMod) {
                    triggerEdit = true;
                } else if (keyCode == g_playKey && playModMatch && playRequiresMod) {
                    triggerPlay = true;
                } else if (keyCode == g_editKey && editModMatch && !editRequiresMod) {
                    triggerEdit = true;
                } else if (keyCode == g_playKey && playModMatch && !playRequiresMod) {
                    triggerPlay = true;
                }

                if (triggerEdit || triggerPlay) {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastToggleTime).count();

                    if (elapsedMs < 200) {
                        return RE::BSEventNotifyControl::kContinue;
                    }

                    g_lastToggleTime = now;
                    bool targetMode = triggerEdit;

                    SKSE::GetTaskInterface()->AddTask([targetMode]() {
                        if (!isMenuOpen.load(std::memory_order_acquire)) {
                            ShowWheel(targetMode);
                        } else {
                            HideWheel();
                        }
                    });
                }
            }
        }
    }
    return RE::BSEventNotifyControl::kContinue;
}