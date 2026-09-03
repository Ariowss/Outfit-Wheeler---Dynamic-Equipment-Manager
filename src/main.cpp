#include "pch.h"
#include "PrismaUI_API.h"
#include "Settings.h"
#include "OutfitManager.h"
#include "EquipmentManager.h"
#include "InputHandler.h"
#include "JSInterop.h"
#include "ExecutionQueue.h"
#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <spdlog/sinks/basic_file_sink.h>

PRISMA_UI_API::IVPrismaUI2* PrismaUI = nullptr;
PrismaView view = 0;

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

static void SKSEMessageHandler(SKSE::MessagingInterface::Message* message) {
    if (!message) return;

    if (message->type == SKSE::MessagingInterface::kDataLoaded) {
        SKSE::log::info("Processing kDataLoaded event");
        LoadINI();
        LoadData();

        if (!PrismaUI) {
            SKSE::log::info("Requesting PrismaUI API Interface v2 at kDataLoaded");
            PrismaUI = PRISMA_UI_API::RequestPluginAPI<PRISMA_UI_API::IVPrismaUI2>();
            if (!PrismaUI) {
                SKSE::log::warn("IVPrismaUI2 request failed, trying IVPrismaUI1");
                PrismaUI = static_cast<PRISMA_UI_API::IVPrismaUI2*>(
                    PRISMA_UI_API::RequestPluginAPI(PRISMA_UI_API::InterfaceVersion::V1)
                );
            }
            SKSE::log::info("PrismaUI API pointer: {}", static_cast<void*>(PrismaUI));
        }

        if (PrismaUI && view == 0) {
            const char* viewPath = "PrismaOutfitWheel/index.html";
            SKSE::log::info("Calling CreateView with path: '{}'", viewPath);

            view = PrismaUI->CreateView(viewPath, OnDomReadyCallbackFunc);
            SKSE::log::info("CreateView returned View ID: {}", view);

            if (view != 0) {
                PrismaUI->Hide(view);
                isMenuOpen.store(false, std::memory_order_release);

                PrismaUI->RegisterConsoleCallback(view, OnConsoleMessageCallbackFunc);
                RegisterJSListeners(view);
                SKSE::log::info("Registered all JS listeners successfully");
            }
        }

        WheelInputHandler::Register();
    } else if (message->type == SKSE::MessagingInterface::kPreLoadGame) {
        g_gameFullyLoaded.store(false, std::memory_order_release);
        g_gameSessionGeneration.fetch_add(1, std::memory_order_acq_rel);
        ExecutionQueue::GetSingleton()->ResetEnvironmentState();
    } else if (message->type == SKSE::MessagingInterface::kPostLoadGame || message->type == SKSE::MessagingInterface::kNewGame) {
        LoadData();
        ExecutionQueue::GetSingleton()->ResetEnvironmentState();
        g_gameFullyLoaded.store(true, std::memory_order_release);
        ExecutionQueue::GetSingleton()->RegisterEvents();
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    InitLog();
    SKSE::log::info("OutfitWheeler SKSEPluginLoad initialized");

    SKSE::Init(a_skse);

    auto messaging = SKSE::GetMessagingInterface();
    if (!messaging) {
        SKSE::log::error("Failed to acquire SKSE MessagingInterface");
        return false;
    }

    messaging->RegisterListener(SKSEMessageHandler);
    return true;
}