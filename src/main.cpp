#include "pch.h"
#include "PrismaUI_API.h"
#include "Settings.h"
#include "OutfitManager.h"
#include "EquipmentManager.h"
#include "InputHandler.h"
#include "JSInterop.h"
#include "ExecutionQueue.h"
#include "thumbnail/ItemThumbnailService.h"
#include "ModCatalogManager.h"
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <spdlog/sinks/basic_file_sink.h>

PRISMA_UI_API::IVPrismaUI2* PrismaUI = nullptr;
PrismaView view = 0;
bool g_hasFullV2API = false;

static void InitLog() {
    auto path = SKSE::log::log_directory();
    if (!path) return;
    *path /= "PrismaOutfitWheel.log";
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
    auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));
    log->set_level(spdlog::level::trace);
    log->flush_on(spdlog::level::trace);
    spdlog::set_default_logger(std::move(log));
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
}

void EnsurePrismaView() {
    if (!PrismaUI) {
        SKSE::log::info("Requesting PrismaUI API Interface v2");
        PrismaUI = PRISMA_UI_API::RequestPluginAPI<PRISMA_UI_API::IVPrismaUI2>();
        if (PrismaUI) {
            g_hasFullV2API = true;
        } else {
            SKSE::log::warn("IVPrismaUI2 request failed, trying IVPrismaUI1");
            PrismaUI = static_cast<PRISMA_UI_API::IVPrismaUI2*>(
                PRISMA_UI_API::RequestPluginAPI(PRISMA_UI_API::InterfaceVersion::V1)
            );
        }
    }
    if (PrismaUI && view == 0) {
        const char* viewPath = "PrismaOutfitWheel/index.html";
        view = PrismaUI->CreateView(viewPath, OnDomReadyCallbackFunc);
        if (view != 0) {
            PrismaUI->Hide(view);
            PrismaUI->Unfocus(view);
            isMenuOpen.store(false, std::memory_order_release);
            g_isEditModeOpen.store(false, std::memory_order_release);
            if (g_hasFullV2API) PrismaUI->RegisterConsoleCallback(view, OnConsoleMessageCallbackFunc);
            PrismaUI->SetScrollingPixelSize(view, 40);
            RegisterJSListeners(view);
        }
    }
}

static void SKSEMessageHandler(SKSE::MessagingInterface::Message* message) {
    if (!message) return;
    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        LoadINI();
        LoadData();
        OutfitWheeler::Thumbnail::ItemThumbnailService::GetSingleton().Initialize();
        ModCatalogManager::GetSingleton()->EnsureCatalogIndexed();
        EnsurePrismaView();
        WheelInputHandler::Register();
    } else if (message->type == SKSE::MessagingInterface::kPreLoadGame) {
        g_gameFullyLoaded.store(false, std::memory_order_release);
        g_gameSessionGeneration.fetch_add(1, std::memory_order_acq_rel);
        ExecutionQueue::GetSingleton()->ResetEnvironmentState();
        isMenuOpen.store(false, std::memory_order_release);
        g_isEditModeOpen.store(false, std::memory_order_release);
        g_isTyping.store(false, std::memory_order_release);
        SKSE::GetTaskInterface()->AddTask([]() {
            if (PrismaUI && view != 0) {
                PrismaUI->Unfocus(view);
                PrismaUI->Hide(view);
            }
            RestoreStudioActors();
            auto player = RE::PlayerCharacter::GetSingleton();
            auto cam = RE::PlayerCamera::GetSingleton();
            if (cam && player) {
                cam->cameraTarget = player->GetHandle();
            }
            auto controlMap = RE::ControlMap::GetSingleton();
            if (controlMap) {
                controlMap->ToggleControls(RE::ControlMap::UEFlag::kAll, true);
            }
            auto pc = RE::PlayerControls::GetSingleton();
            if (pc) {
                pc->blockPlayerInput = false;
                if (pc->attackBlockHandler) pc->attackBlockHandler->SetInputEventHandlingEnabled(true);
                if (pc->readyWeaponHandler) pc->readyWeaponHandler->SetInputEventHandlingEnabled(true);
            }
            SetCameraInputHandlingEnabled(true);
        });
    } else if (message->type == SKSE::MessagingInterface::kPostLoadGame || message->type == SKSE::MessagingInterface::kNewGame) {
        LoadINI();
        LoadData();
        SanitizeWardrobeContainer();
        OutfitWheeler::Thumbnail::ItemThumbnailService::GetSingleton().Initialize();
        ExecutionQueue::GetSingleton()->ResetEnvironmentState();
        ExecutionQueue::GetSingleton()->RegisterEvents();
        isMenuOpen.store(false, std::memory_order_release);
        g_isEditModeOpen.store(false, std::memory_order_release);
        g_isTyping.store(false, std::memory_order_release);
        SKSE::GetTaskInterface()->AddTask([]() {
            g_gameFullyLoaded.store(true, std::memory_order_release);
            if (PrismaUI && view != 0) {
                PrismaUI->Unfocus(view);
                PrismaUI->Hide(view);
            }
            RestoreStudioActors();
            auto player = RE::PlayerCharacter::GetSingleton();
            auto cam = RE::PlayerCamera::GetSingleton();
            if (cam && player) {
                cam->cameraTarget = player->GetHandle();
            }
            if (player) {
                SetActorRestrainedAndDontMove(player, false);
                player->SetLifeState(RE::ACTOR_LIFE_STATE::kAlive);
                player->GetActorRuntimeData().boolFlags.reset(RE::Actor::BOOL_FLAGS::kMovementBlocked);
            }
            auto controlMap = RE::ControlMap::GetSingleton();
            if (controlMap) {
                controlMap->ToggleControls(RE::ControlMap::UEFlag::kAll, true);
            }
            auto pc = RE::PlayerControls::GetSingleton();
            if (pc) {
                pc->blockPlayerInput = false;
                if (pc->attackBlockHandler) pc->attackBlockHandler->SetInputEventHandlingEnabled(true);
                if (pc->readyWeaponHandler) pc->readyWeaponHandler->SetInputEventHandlingEnabled(true);
            }
            SetCameraInputHandlingEnabled(true);
        });
    } else if (message->type == SKSE::MessagingInterface::kSaveGame) {
        SanitizeWardrobeContainer();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    SKSE::Init(a_skse);
    InitLog();
    auto messaging = SKSE::GetMessagingInterface();
    if (!messaging) return false;
    messaging->RegisterListener(SKSEMessageHandler);

    auto serialization = SKSE::GetSerializationInterface();
    if (serialization) {
        serialization->SetUniqueID('OFWL');
        serialization->SetSaveCallback([](SKSE::SerializationInterface*) {
            SanitizeWardrobeContainer();
        });
        serialization->SetLoadCallback([](SKSE::SerializationInterface*) {
            SanitizeWardrobeContainer();
        });
        serialization->SetRevertCallback([](SKSE::SerializationInterface*) {
            SanitizeWardrobeContainer();
        });
    }

    return true;
}