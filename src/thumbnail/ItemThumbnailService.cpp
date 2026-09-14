#include "pch.h"
#include "ItemThumbnailService.h"
#include "ThumbnailDiskCache.h"
#include "ItemPreviewRenderer.h"
#include "../JSInterop.h"
#include <atomic>

extern std::atomic<bool> g_gameFullyLoaded;

namespace OutfitWheeler::Thumbnail
{
    ItemThumbnailService& ItemThumbnailService::GetSingleton()
    {
        static ItemThumbnailService instance;
        return instance;
    }

    ItemThumbnailService::ItemThumbnailService()
    {
        m_saveWorkerThread = std::thread(&ItemThumbnailService::WorkerLoop, this);
    }

    ItemThumbnailService::~ItemThumbnailService()
    {
        Shutdown();
    }

    void ItemThumbnailService::Shutdown()
    {
        if (m_stopWorker.exchange(true)) return;
        m_saveCv.notify_all();
        if (m_saveWorkerThread.joinable()) {
            m_saveWorkerThread.join();
        }
    }

    bool ItemThumbnailService::Initialize()
    {
        if (m_initialized) return true;

        auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
        if (!renderer) {
            logger::warn("[Thumbnail] Failed to get BSGraphics::Renderer singleton");
            return false;
        }

        auto* device = RE::BSGraphics::Renderer::GetDevice();
        if (!device) {
            logger::warn("[Thumbnail] Failed to get D3D11 device from Renderer");
            return false;
        }

        m_initialized = ItemPreviewRenderer::GetSingleton().Initialize(reinterpret_cast<ID3D11Device*>(device));
        if (m_initialized) {
            logger::info("[Thumbnail] ItemPreviewRenderer successfully initialized with D3D11 device");
        } else {
            logger::warn("[Thumbnail] ItemPreviewRenderer failed to initialize");
        }
        return m_initialized;
    }

    void ItemThumbnailService::WorkerLoop()
    {
        while (!m_stopWorker.load(std::memory_order_acquire)) {
            PendingSaveTask task;
            {
                std::unique_lock<std::mutex> lock(m_saveMutex);
                m_saveCv.wait(lock, [this]() {
                    return m_stopWorker.load(std::memory_order_acquire) || !m_saveQueue.empty();
                });
                if (m_stopWorker.load(std::memory_order_acquire) && m_saveQueue.empty()) {
                    break;
                }
                if (!m_saveQueue.empty()) {
                    task = std::move(m_saveQueue.front());
                    m_saveQueue.pop();
                }
            }

            if (!task.pixels.empty() && task.width > 0 && task.height > 0) {
                auto& cache = ThumbnailDiskCache::GetSingleton();
                auto destPath = cache.GetThumbnailPath(task.modName, task.localFormID);
                if (cache.SaveRgbaToPng(destPath, task.pixels.data(), task.width, task.height)) {
                    logger::info("[Thumbnail] Saved thumbnail PNG -> {}", destPath.string());
                    std::string relPath = cache.GetRelativePath(task.modName, task.localFormID);
                    if (task.formID != 0) {
                        auto* taskInterface = SKSE::GetTaskInterface();
                        if (taskInterface) {
                            taskInterface->AddTask([fID = task.formID, rPath = std::move(relPath)]() {
                                NotifyThumbnailReady(fID, rPath);
                            });
                        }
                    }
                }
            }
        }
    }

    void ItemThumbnailService::EnqueueSaveTask(const std::string& modName, uint32_t localFormID, uint32_t formID, std::vector<uint8_t> pixels, uint32_t w, uint32_t h)
    {
        {
            std::lock_guard<std::mutex> lock(m_saveMutex);
            m_saveQueue.push(PendingSaveTask{ modName, localFormID, formID, std::move(pixels), w, h });
        }
        m_saveCv.notify_one();
    }

    std::string ItemThumbnailService::GetOrGenerateThumbnail(RE::TESBoundObject* a_item)
    {
        if (!a_item) return "";

        auto* file = a_item->GetFile(0);
        std::string modName = file ? std::string(file->GetFilename()) : "Skyrim.esm";
        uint32_t localFormID = a_item->GetLocalFormID();

        auto& cache = ThumbnailDiskCache::GetSingleton();
        if (cache.HasThumbnail(modName, localFormID)) {
            return cache.GetRelativePath(modName, localFormID);
        }

        if (!m_initialized) {
            Initialize();
        }

        if (!m_initialized) {
            return "";
        }

        std::vector<uint8_t> pixels;
        uint32_t w = 0, h = 0;
        bool rendered = ItemPreviewRenderer::GetSingleton().RenderItemToBuffer(a_item, pixels, w, h);
        if (!rendered || pixels.empty()) {
            return "";
        }

        auto destPath = cache.GetThumbnailPath(modName, localFormID);
        if (cache.SaveRgbaToPng(destPath, pixels.data(), w, h)) {
            return cache.GetRelativePath(modName, localFormID);
        }

        return "";
    }

    void ItemThumbnailService::StartProcessing()
    {
        ScheduleProcessing();
    }

    bool ItemThumbnailService::HasPendingItems() const
    {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_queueMutex));
        return !m_highPriorityQueue.empty() || !m_normalPriorityQueue.empty();
    }

    static int SafeRenderItemSEH(RE::TESBoundObject* a_item, std::vector<uint8_t>& a_pixels, uint32_t& a_w, uint32_t& a_h)
    {
        __try {
            return ItemPreviewRenderer::GetSingleton().RenderItemToBuffer(a_item, a_pixels, a_w, a_h) ? 1 : 0;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return -1;
        }
    }

    void ItemThumbnailService::QueueItem(RE::TESBoundObject* a_item, bool a_highPriority)
    {
        if (!a_item) return;

        auto* file = a_item->GetFile(0);
        std::string modName = file ? std::string(file->GetFilename()) : "Skyrim.esm";
        uint32_t localFormID = a_item->GetLocalFormID();
        if (ThumbnailDiskCache::GetSingleton().HasThumbnail(modName, localFormID)) {
            std::string relPath = ThumbnailDiskCache::GetSingleton().GetRelativePath(modName, localFormID);
            NotifyThumbnailReady(a_item->GetFormID(), relPath);
            return;
        }

        auto formID = a_item->GetFormID();
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (a_highPriority) {
                m_failedFormIDs.erase(formID);
                if (m_highPriorityFormIDs.contains(formID)) {
                    return;
                }
                m_highPriorityFormIDs.insert(formID);
                m_highPriorityQueue.push_back(a_item);
            } else {
                if (m_failedFormIDs.contains(formID)) {
                    return;
                }
                if (m_queuedFormIDs.contains(formID) || m_highPriorityFormIDs.contains(formID)) {
                    return;
                }
                m_queuedFormIDs.insert(formID);
                m_normalPriorityQueue.push_back(a_item);
            }
        }
        ScheduleProcessing();
    }

    void ItemThumbnailService::QueueItems(const std::vector<RE::TESBoundObject*>& a_items, bool a_highPriority)
    {
        if (a_items.empty()) return;

        std::vector<std::pair<RE::FormID, std::string>> readyItems;
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            auto& cache = ThumbnailDiskCache::GetSingleton();

            for (auto* item : a_items) {
                if (!item) continue;
                auto* file = item->GetFile(0);
                std::string modName = file ? std::string(file->GetFilename()) : "Skyrim.esm";
                uint32_t localFormID = item->GetLocalFormID();
                if (cache.HasThumbnail(modName, localFormID)) {
                    if (a_highPriority) {
                        readyItems.push_back({ item->GetFormID(), cache.GetRelativePath(modName, localFormID) });
                    }
                    continue;
                }

                auto formID = item->GetFormID();
                if (a_highPriority) {
                    m_failedFormIDs.erase(formID);
                    if (!m_highPriorityFormIDs.contains(formID)) {
                        m_highPriorityFormIDs.insert(formID);
                        m_highPriorityQueue.push_back(item);
                    }
                } else {
                    if (m_failedFormIDs.contains(formID)) continue;
                    if (!m_queuedFormIDs.contains(formID) && !m_highPriorityFormIDs.contains(formID)) {
                        m_queuedFormIDs.insert(formID);
                        m_normalPriorityQueue.push_back(item);
                    }
                }
            }
        }

        if (a_highPriority) {
            for (const auto& [fID, rPath] : readyItems) {
                NotifyThumbnailReady(fID, rPath);
            }
        }

        ScheduleProcessing();
    }

    void ItemThumbnailService::ScheduleProcessing()
    {
        if (!g_gameFullyLoaded.load(std::memory_order_acquire)) return;
        if (m_processingScheduled.exchange(true)) return;

        auto* taskInterface = SKSE::GetTaskInterface();
        if (!taskInterface) {
            m_processingScheduled.store(false);
            return;
        }

        taskInterface->AddTask([this]() {
            m_processingScheduled.store(false);
            try {
                ProcessQueue(1);
            } catch (...) {}
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                if ((!m_highPriorityQueue.empty() || !m_normalPriorityQueue.empty()) && g_gameFullyLoaded.load(std::memory_order_acquire)) {
                    ScheduleProcessing();
                }
            }
        });
    }

    void ItemThumbnailService::ProcessQueue(uint32_t maxItemsPerFrame)
    {
        uint32_t budget = maxItemsPerFrame;
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (!m_highPriorityQueue.empty()) {
                budget = (budget > 2u) ? budget : 2u;
            }
        }

        uint32_t processed = 0;
        uint32_t skipped = 0;
        while (processed < budget && skipped < 100) {
            RE::TESBoundObject* item = nullptr;
            bool isHighPriority = false;
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                if (!m_highPriorityQueue.empty()) {
                    item = m_highPriorityQueue.front();
                    m_highPriorityQueue.pop_front();
                    isHighPriority = true;
                    if (item) {
                        m_highPriorityFormIDs.erase(item->GetFormID());
                        m_queuedFormIDs.erase(item->GetFormID());
                    }
                } else if (!m_normalPriorityQueue.empty()) {
                    item = m_normalPriorityQueue.front();
                    m_normalPriorityQueue.pop_front();
                    if (item) {
                        m_queuedFormIDs.erase(item->GetFormID());
                    }
                } else {
                    break;
                }
            }

            if (item) {
                auto* file = item->GetFile(0);
                std::string modName = file ? std::string(file->GetFilename()) : "Skyrim.esm";
                uint32_t localFormID = item->GetLocalFormID();

                if (!ThumbnailDiskCache::GetSingleton().HasThumbnail(modName, localFormID)) {
                    if (!m_initialized) {
                        Initialize();
                    }
                    if (m_initialized) {
                        try {
                            std::vector<uint8_t> pixels;
                            uint32_t w = 0, h = 0;
                            int res = SafeRenderItemSEH(item, pixels, w, h);
                            if (res == 1 && !pixels.empty()) {
                                EnqueueSaveTask(modName, localFormID, item->GetFormID(), std::move(pixels), w, h);
                                logger::info("[Thumbnail] Rendered 3D thumbnail for [{}] 0x{:08X}", modName, localFormID);
                            } else if (res == -1) {
                                std::lock_guard<std::mutex> lock(m_queueMutex);
                                m_failedFormIDs.insert(item->GetFormID());
                                logger::warn("[Thumbnail] Hardware fault rendering [{}] 0x{:08X}", modName, localFormID);
                            } else {
                                std::lock_guard<std::mutex> lock(m_queueMutex);
                                m_failedFormIDs.insert(item->GetFormID());
                                logger::warn("[Thumbnail] Failed to render thumbnail for [{}] 0x{:08X}", modName, localFormID);
                            }
                        } catch (const std::exception& e) {
                            std::lock_guard<std::mutex> lock(m_queueMutex);
                            m_failedFormIDs.insert(item->GetFormID());
                            logger::warn("[Thumbnail] Exception rendering [{}] 0x{:08X}: {}", modName, localFormID, e.what());
                        } catch (...) {
                            std::lock_guard<std::mutex> lock(m_queueMutex);
                            m_failedFormIDs.insert(item->GetFormID());
                            logger::warn("[Thumbnail] Unknown exception rendering [{}] 0x{:08X}", modName, localFormID);
                        }
                    }
                    processed++;
                } else {
                    if (isHighPriority) {
                        std::string relPath = ThumbnailDiskCache::GetSingleton().GetRelativePath(modName, localFormID);
                        NotifyThumbnailReady(item->GetFormID(), relPath);
                    }
                    skipped++;
                }
            }
        }
    }
}

extern "C" __declspec(dllexport) bool OutfitWheeler_GenerateThumbnail(uint32_t a_formID)
{
    auto* form = RE::TESForm::LookupByID(a_formID);
    auto* bound = form ? form->As<RE::TESBoundObject>() : nullptr;
    if (!bound) {
        return false;
    }

    std::string path = OutfitWheeler::Thumbnail::ItemThumbnailService::GetSingleton().GetOrGenerateThumbnail(bound);
    return !path.empty();
}

extern "C" __declspec(dllexport) bool OutfitWheeler_HasThumbnail(uint32_t a_formID)
{
    auto* form = RE::TESForm::LookupByID(a_formID);
    auto* bound = form ? form->As<RE::TESBoundObject>() : nullptr;
    if (!bound) return false;

    auto* file = bound->GetFile(0);
    std::string modName = file ? std::string(file->GetFilename()) : "Skyrim.esm";
    uint32_t localFormID = bound->GetLocalFormID();

    return OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().HasThumbnail(modName, localFormID);
}
