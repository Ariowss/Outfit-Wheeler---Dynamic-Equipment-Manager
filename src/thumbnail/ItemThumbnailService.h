#pragma once

#include <RE/Skyrim.h>
#include <string>
#include <queue>
#include <deque>
#include <mutex>
#include <unordered_set>
#include <cstdint>
#include <vector>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <chrono>

namespace OutfitWheeler::Thumbnail
{
    struct PendingSaveTask
    {
        std::string modName;
        uint32_t localFormID{ 0 };
        uint32_t formID{ 0 };
        std::vector<uint8_t> pixels;
        uint32_t width{ 0 };
        uint32_t height{ 0 };
    };

    class ItemThumbnailService
    {
    public:
        static ItemThumbnailService& GetSingleton();

        bool Initialize();
        [[nodiscard]] bool IsInitialized() const { return m_initialized; }

        std::string GetOrGenerateThumbnail(RE::TESBoundObject* a_item);

        void QueueItem(RE::TESBoundObject* a_item, bool a_highPriority = false);
        void QueueItems(const std::vector<RE::TESBoundObject*>& a_items, bool a_highPriority = false);

        void ProcessQueue(uint32_t maxItemsPerFrame = 1);

        void StartProcessing();
        [[nodiscard]] bool HasPendingItems() const;

        void Shutdown();

    private:
        ItemThumbnailService();
        ~ItemThumbnailService();

        ItemThumbnailService(const ItemThumbnailService&) = delete;
        ItemThumbnailService& operator=(const ItemThumbnailService&) = delete;

        bool m_initialized{ false };
        std::deque<RE::TESBoundObject*> m_highPriorityQueue;
        std::deque<RE::TESBoundObject*> m_normalPriorityQueue;
        std::unordered_set<RE::FormID> m_highPriorityFormIDs;
        std::unordered_set<RE::FormID> m_queuedFormIDs;
        std::unordered_set<RE::FormID> m_failedFormIDs;
        std::mutex m_queueMutex;
        std::atomic<bool> m_processingScheduled{ false };
        std::chrono::steady_clock::time_point m_lastRenderTime{};

        std::queue<PendingSaveTask> m_saveQueue;
        std::mutex m_saveMutex;
        std::condition_variable m_saveCv;
        std::thread m_saveWorkerThread;
        std::atomic<bool> m_stopWorker{ false };

        void ScheduleProcessing();
        void WorkerLoop();
        void EnqueueSaveTask(const std::string& modName, uint32_t localFormID, uint32_t formID, std::vector<uint8_t> pixels, uint32_t w, uint32_t h);
    };
}

extern "C" __declspec(dllexport) bool OutfitWheeler_GenerateThumbnail(uint32_t a_formID);
extern "C" __declspec(dllexport) bool OutfitWheeler_HasThumbnail(uint32_t a_formID);
