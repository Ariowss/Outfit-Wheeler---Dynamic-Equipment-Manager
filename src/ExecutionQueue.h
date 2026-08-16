#ifndef EXECUTION_QUEUE_H
#define EXECUTION_QUEUE_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "AdaptiveWardrobeTypes.h"
#include <RE/Skyrim.h>
#include <atomic>

extern std::atomic<bool> g_gameFullyLoaded;
extern std::atomic<uint64_t> g_gameSessionGeneration;
extern std::atomic<uint64_t> g_outfitTaskGeneration;

class ExecutionQueue : public RE::BSTEventSink<RE::TESCombatEvent>,
                       public RE::BSTEventSink<RE::BGSActorCellEvent>,
                       public RE::BSTEventSink<RE::TESActorLocationChangeEvent>,
                       public RE::BSTEventSink<RE::TESCellFullyLoadedEvent>,
                       public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
private:
    ContextFlags lastEnvironmentContext = ContextFlags::None;
    ExecutionQueue() = default;

    void RegisterEventsInternal();
    ContextFlags ResolveLocationContext(RE::BGSLocation* location, RE::TESObjectCELL* cell);

public:
    static ExecutionQueue* GetSingleton();
    ContextFlags GatherCurrentContext();

    void RegisterEvents();
    void ResetEnvironmentState();
    void EvaluateAndQueueChange(bool isEmergency);

    RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* a_event, RE::BSTEventSource<RE::TESCombatEvent>*) override;
    RE::BSEventNotifyControl ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*) override;
    RE::BSEventNotifyControl ProcessEvent(const RE::TESActorLocationChangeEvent* a_event, RE::BSTEventSource<RE::TESActorLocationChangeEvent>*) override;
    RE::BSEventNotifyControl ProcessEvent(const RE::TESCellFullyLoadedEvent* a_event, RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) override;
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;
};

#endif