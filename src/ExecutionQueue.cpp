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
#include <mutex>
#include <unordered_set>
#include <thread>
#include <chrono>

std::atomic<bool> g_gameFullyLoaded{ false };
std::atomic<uint64_t> g_gameSessionGeneration{ 0 };
std::atomic<uint64_t> g_outfitTaskGeneration{ 0 };
static std::atomic<bool> g_eventsRegistered{ false };
static std::atomic<bool> g_skipInitialLoadEval{ true };
static std::atomic<uint64_t> g_lastEvalTick{ 0 };
static std::atomic<uint64_t> g_lastPeriodicTick{ 0 };
static std::mutex g_contextMutex;

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
static RE::BGSKeyword* g_actorTypeNPC = nullptr;

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
    if (!g_actorTypeNPC) g_actorTypeNPC = RE::TESForm::LookupByEditorID<RE::BGSKeyword>("ActorTypeNPC");
}

static bool IsHumanoidNPC(RE::Actor* a_actor) noexcept {
    if (!a_actor) return false;
    auto* race = a_actor->GetRace();
    if (!race) return false;
    CacheKeywordsAndForms();
    if (g_actorTypeNPC && !race->HasKeyword(g_actorTypeNPC)) {
        return false;
    }
    return true;
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

    static std::atomic<bool> s_timerStarted{ false };
    if (!s_timerStarted.exchange(true)) {
        std::thread([]() {
            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                if (g_gameFullyLoaded.load(std::memory_order_acquire)) {
                    SKSE::GetTaskInterface()->AddTask([]() {
                        ExecutionQueue::GetSingleton()->Update();
                    });
                }
            }
        }).detach();
    }
}

void ExecutionQueue::RegisterEvents() {
    SKSE::GetTaskInterface()->AddTask([this]() {
        RegisterEventsInternal();
    });
}

void ExecutionQueue::ResetEnvironmentState() {
    std::lock_guard<std::mutex> lock(g_contextMutex);
    lastActorContexts.clear();
    g_skipInitialLoadEval.store(true, std::memory_order_release);
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

    bool isSleeping = false;
    auto actorState = a_actor->AsActorState();
    if (actorState) {
        auto sitSleep = actorState->GetSitSleepState();
        if (sitSleep == RE::SIT_SLEEP_STATE::kWantToSleep ||
            sitSleep == RE::SIT_SLEEP_STATE::kWaitingForSleepAnim ||
            sitSleep == RE::SIT_SLEEP_STATE::kIsSleeping ||
            sitSleep == RE::SIT_SLEEP_STATE::kWantToWake) {
            isSleeping = true;
        }
    }

    if (!isSleeping) {
        bool graphSleeping = false;
        if (a_actor->GetGraphVariableBool("bIsSleeping", graphSleeping) && graphSleeping) {
            isSleeping = true;
        }
    }

    if (isSleeping) {
        return ContextFlags::Sleep;
    }

    bool isMounted = a_actor->IsOnMount();
    if (!isMounted) {
        bool bRiding = false;
        if (a_actor->GetGraphVariableBool("bIsRiding", bRiding) && bRiding) {
            isMounted = true;
        }
    }
    if (!isMounted && a_isPlayer) {
        auto cam = RE::PlayerCamera::GetSingleton();
        if (cam && cam->currentState && cam->currentState->id == RE::CameraState::kMount) {
            isMounted = true;
        }
    }

    if (isMounted) {
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

    static const RE::BSFixedString MENU_SLEEP_WAIT("Sleep/Wait Menu");

    if (a_event->menuName == MENU_SLEEP_WAIT) {
        EvaluateAndQueueChange(true);
    } else if (!a_event->opening && (a_event->menuName == RE::LoadingMenu::MENU_NAME || a_event->menuName == RE::FaderMenu::MENU_NAME || a_event->menuName == RE::TweenMenu::MENU_NAME || a_event->menuName == RE::InventoryMenu::MENU_NAME)) {
        EvaluateAndQueueChange(false);
    }
    return RE::BSEventNotifyControl::kContinue;
}

void ExecutionQueue::Update() {
    if (!g_gameFullyLoaded.load(std::memory_order_acquire)) return;

    auto ui = RE::UI::GetSingleton();
    if (!ui || ui->IsMenuOpen(RE::MainMenu::MENU_NAME) || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME) || ui->IsMenuOpen(RE::FaderMenu::MENU_NAME)) {
        return;
    }

    uint64_t currentTick = GetTickCount64();
    uint64_t lastTick = g_lastPeriodicTick.load(std::memory_order_acquire);
    if (currentTick - lastTick < 400) {
        return;
    }
    g_lastPeriodicTick.store(currentTick, std::memory_order_release);

    EvaluateAndQueueChange(false);
}

void ExecutionQueue::EvaluateAndQueueChange(bool isEmergency) {
    if (!g_autoSystemEnabled) return;
    if (!g_gameFullyLoaded.load(std::memory_order_acquire)) return;

    auto uiRef = RE::UI::GetSingleton();
    if (!uiRef || uiRef->IsMenuOpen(RE::MainMenu::MENU_NAME) || uiRef->IsMenuOpen(RE::LoadingMenu::MENU_NAME) || uiRef->IsMenuOpen(RE::FaderMenu::MENU_NAME)) {
        return;
    }

    if (isMenuOpen.load(std::memory_order_acquire)) return;

    auto playerRef = RE::PlayerCharacter::GetSingleton();
    if (!playerRef || !playerRef->Is3DLoaded() || playerRef->IsDead()) return;

    if (g_skipInitialLoadEval.load(std::memory_order_acquire)) {
        std::lock_guard<std::mutex> lock(g_contextMutex);
        lastActorContexts[0x00000007] = GatherActorContext(playerRef, true);

        auto manager = AdaptiveWardrobeManager::GetSingleton();
        auto wardrobe = manager->GetWardrobe(0x00000007);
        auto worn = GetEquippedInventoryEntries(playerRef);

        for (const auto& outfit : wardrobe) {
            if (outfit.items.empty() && outfit.itemIDs.empty()) continue;
            size_t matchCount = 0;
            if (!outfit.items.empty()) {
                for (const auto& itm : outfit.items) {
                    for (const auto& w : worn) {
                        if (MatchesWornEntry(w, itm)) {
                            matchCount++;
                            break;
                        }
                    }
                }
                if (matchCount == outfit.items.size() && matchCount == worn.size()) {
                    manager->SetCurrentOutfitID(0x00000007, outfit.id);
                    break;
                }
            } else {
                std::unordered_set<RE::FormID> wornSet;
                for (const auto& w : worn) {
                    if (w.object) wornSet.insert(w.object->GetFormID());
                }
                for (auto fID : outfit.itemIDs) {
                    if (wornSet.find(fID) != wornSet.end()) matchCount++;
                }
                if (matchCount == outfit.itemIDs.size() && matchCount == wornSet.size()) {
                    manager->SetCurrentOutfitID(0x00000007, outfit.id);
                    break;
                }
            }
        }

        auto processLists = RE::ProcessLists::GetSingleton();
        if (processLists) {
            for (auto& handle : processLists->highActorHandles) {
                auto actor = handle.get();
                if (actor && actor.get() != playerRef && actor->IsPlayerTeammate() && !actor->IsDead() && actor->Is3DLoaded() && IsHumanoidNPC(actor.get())) {
                    RE::FormID aID = actor->GetFormID();
                    auto fWardrobe = manager->GetWardrobe(aID);
                    if (fWardrobe.empty()) continue;

                    lastActorContexts[aID] = GatherActorContext(actor.get(), false);
                    auto fWorn = GetEquippedInventoryEntries(actor.get());

                    for (const auto& outfit : fWardrobe) {
                        if (outfit.items.empty() && outfit.itemIDs.empty()) continue;
                        size_t matchCount = 0;
                        if (!outfit.items.empty()) {
                            for (const auto& itm : outfit.items) {
                                for (const auto& w : fWorn) {
                                    if (MatchesWornEntry(w, itm)) {
                                        matchCount++;
                                        break;
                                    }
                                }
                            }
                            if (matchCount == outfit.items.size() && matchCount == fWorn.size()) {
                                manager->SetCurrentOutfitID(aID, outfit.id);
                                break;
                            }
                        } else {
                            std::unordered_set<RE::FormID> fWornSet;
                            for (const auto& w : fWorn) {
                                if (w.object) fWornSet.insert(w.object->GetFormID());
                            }
                            for (auto fID : outfit.itemIDs) {
                                if (fWornSet.find(fID) != fWornSet.end()) matchCount++;
                            }
                            if (matchCount == outfit.itemIDs.size() && matchCount == fWornSet.size()) {
                                manager->SetCurrentOutfitID(aID, outfit.id);
                                break;
                            }
                        }
                    }
                }
            }
        }

        AutoStashInactiveSlotItems(playerRef);

        if (processLists) {
            for (auto& handle : processLists->highActorHandles) {
                auto actor = handle.get();
                if (actor && actor.get() != playerRef && actor->IsPlayerTeammate() && !actor->IsDead() && actor->Is3DLoaded() && IsHumanoidNPC(actor.get())) {
                    AutoStashInactiveSlotItems(actor.get());
                }
            }
        }

        g_skipInitialLoadEval.store(false, std::memory_order_release);
        return;
    }

    uint64_t currentTick = GetTickCount64();
    uint64_t lastTick = g_lastEvalTick.load(std::memory_order_acquire);
    if (!isEmergency && (currentTick - lastTick < 400)) {
        return;
    }
    g_lastEvalTick.store(currentTick, std::memory_order_release);

    auto manager = AdaptiveWardrobeManager::GetSingleton();
    std::vector<RE::Actor*> targetActors;
    targetActors.push_back(playerRef);

    auto processLists = RE::ProcessLists::GetSingleton();
    if (processLists) {
        for (auto& handle : processLists->highActorHandles) {
            auto actor = handle.get();
            if (actor && actor.get() != playerRef && actor->IsPlayerTeammate() && !actor->IsDead() && actor->Is3DLoaded() && IsHumanoidNPC(actor.get())) {
                RE::FormID aID = actor->GetFormID();
                if (!manager->GetWardrobe(aID).empty()) {
                    targetActors.push_back(actor.get());
                }
            }
        }
    }

    for (auto* actor : targetActors) {
        if (!actor || !actor->Is3DLoaded() || actor->IsDead()) continue;

        bool isPlayer = actor->IsPlayerRef() || (actor == playerRef);
        RE::FormID actorID = isPlayer ? 0x00000007 : actor->GetFormID();

        ContextFlags currentContext = GatherActorContext(actor, isPlayer);

        {
            std::lock_guard<std::mutex> lock(g_contextMutex);
            auto lastCtxIt = lastActorContexts.find(actorID);
            ContextFlags lastCtx = (lastCtxIt != lastActorContexts.end()) ? lastCtxIt->second : ContextFlags::None;

            if (!isEmergency && currentContext == lastCtx) continue;
            lastActorContexts[actorID] = currentContext;
        }

        auto selectionOpt = manager->SelectOptimalOutfit(actorID, currentContext, isEmergency);
        if (!selectionOpt.has_value()) continue;

        auto selection = selectionOpt.value();
        if (selection.selectedOutfit.id == manager->GetCurrentOutfitID(actorID)) continue;

        std::string targetID = selection.selectedOutfit.id;
        std::string targetName = selection.selectedOutfit.name;

        std::vector<OutfitItem> targetItems = selection.selectedOutfit.items;
        if (targetItems.empty()) {
            for (auto fID : selection.selectedOutfit.itemIDs) {
                OutfitItem itm;
                itm.formID = fID;
                targetItems.push_back(itm);
            }
        }

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