#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <RE/Skyrim.h>
#include <atomic>

extern std::atomic<bool> isMenuOpen;
extern std::atomic<bool> g_isTyping;
extern bool g_inputRegistered;

void ShowWheel(bool isEditMode);

class WheelInputHandler : public RE::BSTEventSink<RE::InputEvent*> {
public:
    static WheelInputHandler* GetSingleton();
    static void Register();
    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override;
};

#endif