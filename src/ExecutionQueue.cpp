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
#include "InputHandler.h"
#include "Settings.h"
#include <SKSE/SKSE.h>

std::atomic<bool> g_gameFullyLoaded{ false };
std::atomic<uint64_t> g_gameSessionGeneration{ 0 };
std::atomic<uint64_t> g_outfitTaskGeneration{ 0 };
static std::atomic<bool> g_eventsRegistered{ false };
static std::atomic<bool> g_skipInitialLoadEval{ true };
static std::atomic<uint64_t> g_lastEvalTick{ 0 };

static RE::BGSKeyword* g_locTypePlayerHouse = nullptr;
static RE::BGSKeyword* g_locTypeCity = nullptr;
static RE::BGSKeyword* g_locTypeTown = nullptr;
static RE::BGSKeyword* g_locTypeDungeon = nullptr;
static RE::BGSKeyword* g_locTypeInn = nullptr;
static RE::BGSKeyword* g_locTypeStore = nullptr;
static RE::BGSKeyword* g_locTypeGuild = nullptr;
static RE::BGSKeyword* g_locTypeCastle = nullptr;
static RE::BGSKeyword* g_locTypeTemple = nullptr;
static RE::BGSKeyword* g_locTypeFarm = nullptr;
static RE::BGSKeyword* g_locTypeJail = nullptr;
static RE::BGSKeyword* g_locTypeMilitary = nullptr;

static void CacheKeywordsAndForms() {
    if (!g_locTypePlayerHouse) g_locTypePlayerHouse = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypePlayerHouse");
    if (!g_locTypeCity) g_locTypeCity = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeCity");
    if (!g_locTypeTown) g_locTypeTown = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeTown");
    if (!g_locTypeDungeon) g_locTypeDungeon = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeDungeon");
    if (!g_locTypeInn) g_locTypeInn = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeInn");
    if (!g_locTypeStore) g_locTypeStore = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeStore");
    if (!g_locTypeGuild) g_locTypeGuild = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeGuild");
    if (!g_locTypeCastle) g_locTypeCastle = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeCastle");
    if (!g_locTypeTemple) g_locTypeTemple = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeTemple");
    if (!g_locTypeFarm) g_locTypeFarm = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeFarm");
    if (!g_locTypeJail) g_locTypeJail = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeJail");
    if (!g_locTypeMilitary) g_locTypeMilitary = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("LocTypeMilitaryCamp");
}

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

    CacheKeywordsAndForms();

    scriptSource->AddEventSink<RE::TESCombatEvent>(this);
    scriptSource->AddEventSink<RE::TESActorLocationChangeEvent>(this);
    scriptSource->AddEventSink<RE::TESCellFullyLoadedEvent>(this);
    ui->AddEventSink<RE::MenuOpenCloseEvent>(this);

    auto* cellEventSource = player->AsBGSActorCellEventSource();
    if (cellEventSource) {
        cellEventSource->AddEventSink(this);
    }

    g_eventsRegistered.store(true, std::memory_order_release);
}

void ExecutionQueue::RegisterEvents() {
    SKSE::GetTaskInterface()->AddTask([this]() {
        RegisterEventsInternal();
    });
}

void ExecutionQueue::ResetEnvironmentState() {
    lastActorContexts.clear();
}

ContextFlags ExecutionQueue::ResolveLocationContext(RE::BGSLocation* location, RE::TESObjectCELL* cell) {
    CacheKeywordsAndForms();
    ContextFlags flags = ContextFlags::None;

    if (location) {
        auto* curLoc = location;
        while (curLoc) {
            if (g_locTypePlayerHouse && curLoc->HasKeyword(g_locTypePlayerHouse)) return ContextFlags::PlayerHome;
            if (g_locTypeDungeon && curLoc->HasKeyword(g_locTypeDungeon)) return ContextFlags::Dungeon;
            if (g_locTypeInn && curLoc->HasKeyword(g_locTypeInn)) return ContextFlags::Inn;
            if (g_locTypeStore && curLoc->HasKeyword(g_locTypeStore)) return ContextFlags::Store;
            if (g_locTypeGuild && curLoc->HasKeyword(g_locTypeGuild)) return ContextFlags::GuildHall;
            if (g_locTypeCastle && curLoc->HasKeyword(g_locTypeCastle)) return ContextFlags::Castle;
            if (g_locTypeTemple && curLoc->HasKeyword(g_locTypeTemple)) return ContextFlags::Temple;
            if (g_locTypeFarm && curLoc->HasKeyword(g_locTypeFarm)) return ContextFlags::Farm;
            if (g_locTypeJail && curLoc->HasKeyword(g_locTypeJail)) return ContextFlags::Jail;
            if (g_locTypeMilitary && curLoc->HasKeyword(g_locTypeMilitary)) return ContextFlags::Military;

            if (g_locTypeCity && curLoc->HasKeyword(g_locTypeCity)) {
                flags |= ContextFlags::City;
            }
            if (g_locTypeTown && curLoc->HasKeyword(g_locTypeTown)) {
                flags |= ContextFlags::Town;
            }
            curLoc = curLoc->parentLoc;
        }
    }

    if (cell) {
        auto* cellLoc = cell->GetLocation();
        if (cellLoc) {
            auto* curLoc = cellLoc;
            while (curLoc) {
                if (g_locTypePlayerHouse && curLoc->HasKeyword(g_locTypePlayerHouse)) return ContextFlags::PlayerHome;
                if (g_locTypeDungeon && curLoc->HasKeyword(g_locTypeDungeon)) return ContextFlags::Dungeon;
                if (g_locTypeInn && curLoc->HasKeyword(g_locTypeInn)) return ContextFlags::Inn;
                if (g_locTypeStore && curLoc->HasKeyword(g_locTypeStore)) return ContextFlags::Store;
                if (g_locTypeGuild && curLoc->HasKeyword(g_locTypeGuild)) return ContextFlags::GuildHall;
                if (g_locTypeCastle && curLoc->HasKeyword(g_locTypeCastle)) return ContextFlags::Castle;
                if (g_locTypeTemple && curLoc->HasKeyword(g_locTypeTemple)) return ContextFlags::Temple;
                if (g_locTypeFarm && curLoc->HasKeyword(g_locTypeFarm)) return ContextFlags::Farm;
                if (g_locTypeJail && curLoc->HasKeyword(g_locTypeJail)) return ContextFlags::Jail;
                if (g_locTypeMilitary && curLoc->HasKeyword(g_locTypeMilitary)) return ContextFlags::Military;

                if (g_locTypeCity && curLoc->HasKeyword(g_locTypeCity)) {
                    flags |= ContextFlags::City;
                }
                if (g_locTypeTown && curLoc->HasKeyword(g_locTypeTown)) {
                    flags |= ContextFlags::Town;
                }
                curLoc = curLoc->parentLoc;
            }
        }

        if (cell->IsInteriorCell()) {
            flags |= ContextFlags::Interior;
            auto owner = cell->GetOwner();
            if (owner) {
                if (owner->Is(RE::FormType::NPC) && (owner->GetFormID() == 0x00000007 || owner->GetFormID() == 0x00000014)) {
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
    }

    if (!HasContext(flags, ContextFlags::City) && !HasContext(flags, ContextFlags::Town)) {
        flags |= ContextFlags::World;
    }

    return flags;
}

ContextFlags ExecutionQueue::GatherActorContext(RE::Actor* a_actor, bool a_isPlayer) {
    if (!a_actor || !g_autoSystemEnabled) return ContextFlags::Default;

    auto actorState = a_actor->AsActorState();
    if (actorState) {
        auto sitSleep = actorState->GetSitSleepState();
        if (sitSleep == RE::SIT_SLEEP_STATE::kIsSleeping || sitSleep == RE::SIT_SLEEP_STATE::kWantToSleep) {
            return ContextFlags::Sleep;
        }
    }

    if (a_actor->IsOnMount()) {
        return ContextFlags::Mount;
    }

    auto player = RE::PlayerCharacter::GetSingleton();
    bool isInCombat = a_actor->IsInCombat() || (a_isPlayer ? false : (player && player->IsInCombat()));
    if (isInCombat && g_autoCombatEnabled) {
        return ContextFlags::Combat;
    }

    float waterHeight = a_actor->GetWaterHeight();
    float posZ = a_actor->GetPositionZ();
    if (actorState && actorState->IsSwimming()) {
        return ContextFlags::Swim;
    }
    if (waterHeight > posZ + 15.0f && waterHeight < posZ + 80.0f) {
        return ContextFlags::InWater;
    }

    auto parentCell = a_actor->GetParentCell();
    auto loc = a_actor->GetCurrentLocation();
    ContextFlags locFlags = ResolveLocationContext(loc, parentCell);

    auto* cal = RE::Calendar::GetSingleton();
    float hour = cal ? cal->GetHour() : 12.0f;
    if (hour < 6.0f || hour >= 20.0f) {
        locFlags |= ContextFlags::Night;
    }

    auto* sky = RE::Sky::GetSingleton();
    if (sky && sky->currentWeather) {
        auto flags = sky->currentWeather->data.flags;
        if (flags.any(RE::TESWeather::WeatherDataFlag::kRainy)) {
            locFlags |= ContextFlags::Rain;
        } else if (flags.any(RE::TESWeather::WeatherDataFlag::kSnow)) {
            locFlags |= ContextFlags::Snow;
        }
    }

    return locFlags;
}

ContextFlags ExecutionQueue::GatherCurrentContext() {
    auto player = RE::PlayerCharacter::GetSingleton();
    return GatherActorContext(player, true);
}

RE::BSEventNotifyControl ExecutionQueue::ProcessEvent(const RE::TESCombatEvent* a_event, RE::BSTEventSource<RE::TESCombatEvent>*) {
    if (!a_event) return RE::BSEventNotifyControl::kContinue;

    bool involvesPlayerOrTeammate = false;
    if (a_event->actor) {
        auto* act = a_event->actor->As<RE::Actor>();
        if (act && (act->IsPlayerRef() || act->IsPlayerTeammate())) {
            involvesPlayerOrTeammate = true;
        }
    }
    if (a_event->targetActor) {
        auto* target = a_event->targetActor->As<RE::Actor>();
        if (target && (target->IsPlayerRef() || target->IsPlayerTeammate())) {
            involvesPlayerOrTeammate = true;
        }
    }

    if (!involvesPlayerOrTeammate) {
        return RE::BSEventNotifyControl::kContinue;
    }

    if (a_event->newState.get() == RE::ACTOR_COMBAT_STATE::kCombat) {
        EvaluateAndQueueChange(true);
    } else {
        EvaluateAndQueueChange(false);
    }
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl ExecutionQueue::ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*) {
    if (!a_event) return RE::BSEventNotifyControl::kContinue;
    EvaluateAndQueueChange(false);
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl ExecutionQueue::ProcessEvent(const RE::TESActorLocationChangeEvent* a_event, RE::BSTEventSource<RE::TESActorLocationChangeEvent>*) {
    if (!a_event || !a_event->actor || !a_event->actor->IsPlayerRef()) {
        return RE::BSEventNotifyControl::kContinue;
    }
    EvaluateAndQueueChange(false);
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl ExecutionQueue::ProcessEvent(const RE::TESCellFullyLoadedEvent* a_event, RE::BSTEventSource<RE::TESCellFullyLoadedEvent>*) {
    if (!a_event) return RE::BSEventNotifyControl::kContinue;
    EvaluateAndQueueChange(false);
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl ExecutionQueue::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) {
    if (!a_event) return RE::BSEventNotifyControl::kContinue;

    if (!a_event->opening && (a_event->menuName == RE::LoadingMenu::MENU_NAME || a_event->menuName == RE::FaderMenu::MENU_NAME)) {
        EvaluateAndQueueChange(false);
    }
    return RE::BSEventNotifyControl::kContinue;
}

void ExecutionQueue::EvaluateAndQueueChange(bool isEmergency) {
    if (!g_autoSystemEnabled) return;

    if (isMenuOpen.load(std::memory_order_acquire)) return;

    if (g_skipInitialLoadEval.exchange(false, std::memory_order_acq_rel)) {
        lastActorContexts[0x00000007] = GatherCurrentContext();
        return;
    }

    uint64_t currentTick = GetTickCount64();
    uint64_t lastTick = g_lastEvalTick.load(std::memory_order_acquire);
    if (!isEmergency && (currentTick - lastTick < 600)) {
        return;
    }
    g_lastEvalTick.store(currentTick, std::memory_order_release);

    if (!g_gameFullyLoaded.load(std::memory_order_acquire)) return;

    auto uiRef = RE::UI::GetSingleton();
    if (uiRef && (uiRef->IsMenuOpen(RE::MainMenu::MENU_NAME) || uiRef->IsMenuOpen(RE::LoadingMenu::MENU_NAME) || uiRef->IsMenuOpen(RE::FaderMenu::MENU_NAME))) {
        return;
    }

    auto playerRef = RE::PlayerCharacter::GetSingleton();
    if (!playerRef || !playerRef->Is3DLoaded() || playerRef->IsDead()) return;

    std::vector<RE::Actor*> targetActors;
    targetActors.push_back(playerRef);

    auto processLists = RE::ProcessLists::GetSingleton();
    if (processLists) {
        for (auto& handle : processLists->highActorHandles) {
            auto actor = handle.get();
            if (actor && actor.get() != playerRef && actor->IsPlayerTeammate() && !actor->IsDead() && actor->Is3DLoaded()) {
                targetActors.push_back(actor.get());
            }
        }
    }

    auto manager = AdaptiveWardrobeManager::GetSingleton();

    for (auto* actor : targetActors) {
        if (!actor || !actor->Is3DLoaded() || actor->IsDead()) continue;

        bool isPlayer = actor->IsPlayerRef() || (actor == playerRef);
        RE::FormID actorID = isPlayer ? 0x00000007 : actor->GetFormID();

        ContextFlags currentContext = GatherActorContext(actor, isPlayer);

        auto lastCtxIt = lastActorContexts.find(actorID);
        ContextFlags lastCtx = (lastCtxIt != lastActorContexts.end()) ? lastCtxIt->second : ContextFlags::None;

        if (!isEmergency && currentContext == lastCtx) continue;
        lastActorContexts[actorID] = currentContext;

        auto selectionOpt = manager->SelectOptimalOutfit(actorID, currentContext, isEmergency);
        if (!selectionOpt.has_value()) continue;

        auto selection = selectionOpt.value();
        if (selection.selectedOutfit.id == manager->GetCurrentOutfitID(actorID)) continue;

        std::string targetID = selection.selectedOutfit.id;
        std::string targetName = selection.selectedOutfit.name;
        std::vector<RE::FormID> targetItems = selection.selectedOutfit.itemIDs;
        RE::ActorHandle aHandle = actor->GetHandle();

        SKSE::GetTaskInterface()->AddTask([manager, actorID, aHandle, targetID, targetName, targetItems]() {
            auto targetActorPtr = aHandle.get();
            auto equipManager = RE::ActorEquipManager::GetSingleton();
            if (!targetActorPtr || !equipManager || !targetActorPtr->Is3DLoaded() || targetActorPtr->IsDead()) {
                return;
            }

            PerformOutfitSwap(targetActorPtr.get(), equipManager, targetItems);
            manager->RecordOutfitEquipped(actorID, targetID, 0);
        });
    }
}