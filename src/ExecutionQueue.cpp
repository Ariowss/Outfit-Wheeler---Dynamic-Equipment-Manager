#include "pch.h"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include "ExecutionQueue.h"
#include "EquipmentManager.h"
#include "AdaptiveWardrobeManager.h"
#include <SKSE/SKSE.h>

std::atomic<bool> g_gameFullyLoaded{ false };
std::atomic<uint64_t> g_gameSessionGeneration{ 0 };
std::atomic<uint64_t> g_outfitTaskGeneration{ 0 };
static std::atomic<bool> g_eventsRegistered{ false };
static std::atomic<bool> g_skipInitialLoadEval{ true };
static std::atomic<uint32_t> g_lastEvalTick{ 0 };

ExecutionQueue* ExecutionQueue::GetSingleton() {
    static ExecutionQueue instance;
    return &instance;
}

void ExecutionQueue::RegisterEventsInternal() {
    if (g_eventsRegistered.load(std::memory_order_acquire)) return;

    auto scriptSource = RE::ScriptEventSourceHolder::GetSingleton();
    auto player = RE::PlayerCharacter::GetSingleton();
    auto ui = RE::UI::GetSingleton();

    if (!scriptSource || !player || !ui) return;

    scriptSource->AddEventSink<RE::TESCombatEvent>(this);
    scriptSource->AddEventSink<RE::TESActorLocationChangeEvent>(this);
    scriptSource->AddEventSink<RE::TESCellFullyLoadedEvent>(this);
    ui->AddEventSink<RE::MenuOpenCloseEvent>(this);

    auto* cellEventSource = player->AsBGSActorCellEventSource();
    if (cellEventSource) {
        cellEventSource->AddEventSink(this);
    }

    g_eventsRegistered.store(true, std::memory_order_release);
    SKSE::log::info("RegisterEvents: SUCCESS");
}

void ExecutionQueue::RegisterEvents() {
    SKSE::GetTaskInterface()->AddTask([this]() {
        RegisterEventsInternal();
    });
}

ContextFlags ExecutionQueue::ResolveLocationContext(RE::BGSLocation* location, RE::TESObjectCELL* cell) {
    if (location) {
        if (location->HasKeywordString("LocTypePlayerHouse") || (location->parentLoc && location->parentLoc->HasKeywordString("LocTypePlayerHouse"))) {
            return ContextFlags::PlayerHome;
        }
        if (location->HasKeywordString("LocTypeCity") || location->HasKeywordString("LocTypeTown") ||
            (location->parentLoc && (location->parentLoc->HasKeywordString("LocTypeCity") || location->parentLoc->HasKeywordString("LocTypeTown")))) {
            return ContextFlags::CityTown;
        }
    }

    if (cell && cell->IsInteriorCell()) {
        auto owner = cell->GetOwner();
        if (owner) {
            if (owner->Is(RE::FormType::NPC) && owner->GetFormID() == 0x00000007) {
                return ContextFlags::PlayerHome;
            }
            if (owner->Is(RE::FormType::Faction)) {
                uint32_t fID = owner->GetFormID();
                if (fID == 0x00013702 || fID == 0x000A2C9D) {
                    return ContextFlags::PlayerHome;
                }
            }
        }
    }

    return ContextFlags::None;
}

ContextFlags ExecutionQueue::GatherCurrentContext() {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) return ContextFlags::None;

    ContextFlags flags = ContextFlags::None;
    auto parentCell = player->GetParentCell();
    auto loc = player->GetCurrentLocation();

    flags |= ResolveLocationContext(loc, parentCell);

    if (player->IsInCombat()) {
        flags |= ContextFlags::Combat;
    }

    auto actorState = player->AsActorState();
    float waterHeight = player->GetWaterHeight();
    float playerZ = player->GetPositionZ();
    if ((actorState && actorState->IsSwimming()) || (waterHeight > playerZ + 20.0f)) {
        flags |= ContextFlags::Swim;
    }

    return flags;
}

RE::BSEventNotifyControl ExecutionQueue::ProcessEvent(const RE::TESCombatEvent* a_event, RE::BSTEventSource<RE::TESCombatEvent>*) {
    if (!a_event || !a_event->targetActor || !a_event->targetActor->IsPlayerRef()) {
        return RE::BSEventNotifyControl::kContinue;
    }
    SKSE::log::info("ProcessEvent: TESCombatEvent fired");
    AdaptiveWardrobeManager::GetSingleton()->ClearPause();
    EvaluateAndQueueChange(a_event->newState.get() == RE::ACTOR_COMBAT_STATE::kCombat);
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl ExecutionQueue::ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*) {
    if (!a_event) return RE::BSEventNotifyControl::kContinue;
    SKSE::log::info("ProcessEvent: BGSActorCellEvent fired");
    AdaptiveWardrobeManager::GetSingleton()->ClearPause();
    EvaluateAndQueueChange(false);
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl ExecutionQueue::ProcessEvent(const RE::TESActorLocationChangeEvent* a_event, RE::BSTEventSource<RE::TESActorLocationChangeEvent>*) {
    if (!a_event || !a_event->actor || !a_event->actor->IsPlayerRef()) {
        return RE::BSEventNotifyControl::kContinue;
    }
    SKSE::log::info("ProcessEvent: TESActorLocationChangeEvent fired");
    AdaptiveWardrobeManager::GetSingleton()->ClearPause();
    EvaluateAndQueueChange(false);
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl ExecutionQueue::ProcessEvent(const RE::TESCellFullyLoadedEvent* a_event, RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) {
    if (!a_event) return RE::BSEventNotifyControl::kContinue;
    SKSE::log::info("ProcessEvent: TESCellFullyLoadedEvent fired - cell fully ready");
    AdaptiveWardrobeManager::GetSingleton()->ClearPause();
    EvaluateAndQueueChange(false);
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl ExecutionQueue::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) {
    if (!a_event) return RE::BSEventNotifyControl::kContinue;

    if (!a_event->opening && (a_event->menuName == RE::LoadingMenu::MENU_NAME || a_event->menuName == RE::FaderMenu::MENU_NAME)) {
        SKSE::log::info("ProcessEvent: MenuOpenCloseEvent - '{}' CLOSED", a_event->menuName.c_str());
        AdaptiveWardrobeManager::GetSingleton()->ClearPause();
        EvaluateAndQueueChange(false);
    }
    return RE::BSEventNotifyControl::kContinue;
}

void ExecutionQueue::EvaluateAndQueueChange(bool isEmergency) {
    if (g_skipInitialLoadEval.exchange(false, std::memory_order_acq_rel)) {
        SKSE::log::info("EvaluateAndQueueChange: Skipped - Initial game load");
        return;
    }

    uint32_t currentTick = GetTickCount();
    uint32_t lastTick = g_lastEvalTick.load(std::memory_order_acquire);
    if (!isEmergency && (currentTick - lastTick < 1500)) {
        SKSE::log::info("EvaluateAndQueueChange: Coalesced duplicate cell/location event");
        return;
    }
    g_lastEvalTick.store(currentTick, std::memory_order_release);

    if (!g_gameFullyLoaded.load(std::memory_order_acquire)) {
        SKSE::log::info("EvaluateAndQueueChange: Skipped - Game not fully loaded");
        return;
    }

    auto uiRef = RE::UI::GetSingleton();
    if (uiRef && (uiRef->IsMenuOpen(RE::MainMenu::MENU_NAME) || uiRef->IsMenuOpen(RE::LoadingMenu::MENU_NAME) || uiRef->IsMenuOpen(RE::FaderMenu::MENU_NAME))) {
        SKSE::log::info("EvaluateAndQueueChange: Skipped - Loading/Fader/Main menu active");
        return;
    }

    auto playerRef = RE::PlayerCharacter::GetSingleton();
    if (!playerRef || !playerRef->Is3DLoaded()) {
        SKSE::log::info("EvaluateAndQueueChange: Skipped - Player or 3D not loaded");
        return;
    }

    auto manager = AdaptiveWardrobeManager::GetSingleton();
    if (manager->IsPaused()) {
        SKSE::log::info("EvaluateAndQueueChange: Skipped - System is paused");
        return;
    }

    ContextFlags currentContext = GatherCurrentContext();
    SKSE::log::info("EvaluateAndQueueChange: Context resolved to {:08X}", static_cast<uint32_t>(currentContext));

    auto selectionOpt = manager->SelectOptimalOutfit(currentContext, isEmergency);
    if (!selectionOpt.has_value()) {
        SKSE::log::info("EvaluateAndQueueChange: Skipped - No optimal outfit found for context {:08X}", static_cast<uint32_t>(currentContext));
        return;
    }

    auto selection = selectionOpt.value();
    SKSE::log::info("EvaluateAndQueueChange: Candidate selected outfit ID '{}' ('{}'), current outfit ID '{}'",
                    selection.selectedOutfit.id, selection.selectedOutfit.name, manager->GetCurrentOutfitID());

    if (selection.selectedOutfit.id == manager->GetCurrentOutfitID()) {
        SKSE::log::info("EvaluateAndQueueChange: Skipped - Selected outfit matches current outfit ID '{}'", manager->GetCurrentOutfitID());
        return;
    }

    std::string targetID = selection.selectedOutfit.id;
    std::string targetName = selection.selectedOutfit.name;
    std::vector<RE::FormID> targetItems = selection.selectedOutfit.itemIDs;

    SKSE::log::info("EvaluateAndQueueChange: Queuing SKSE task to swap to '{}' ({})", targetName, targetID);

    SKSE::GetTaskInterface()->AddTask([manager, targetID, targetName, targetItems]() {
        auto player = RE::PlayerCharacter::GetSingleton();
        auto equipManager = RE::ActorEquipManager::GetSingleton();
        if (!player || !equipManager || !player->Is3DLoaded()) {
            SKSE::log::info("TaskLambda: Skipped - Player/EquipManager null or 3D not loaded");
            return;
        }

        if (manager->GetCurrentOutfitID() == targetID) {
            SKSE::log::info("TaskLambda: Skipped - Outfit ID '{}' already equipped", targetID);
            return;
        }

        SKSE::log::info("TaskLambda: EXECUTING PerformOutfitSwap to '{}' ({})", targetName, targetID);
        PerformOutfitSwap(player, equipManager, targetItems);
        manager->RecordOutfitEquipped(targetID, 0);
        SKSE::log::info("TaskLambda: PerformOutfitSwap COMPLETE for '{}'", targetName);
    });
}