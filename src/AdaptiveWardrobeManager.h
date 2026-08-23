#ifndef ADAPTIVE_WARDROBE_MANAGER_H
#define ADAPTIVE_WARDROBE_MANAGER_H

#include "AdaptiveWardrobeTypes.h"
#include <mutex>
#include <optional>
#include <random>
#include <deque>
#include <string>
#include <vector>
#include <unordered_map>

struct SelectionResult {
    AdaptiveOutfit selectedOutfit;
    std::string reasonCode;
    float calculatedWeight = 0.0f;
};

struct ActorWardrobeData {
    std::vector<AdaptiveOutfit> wardrobe;
    std::string currentOutfitID;
    std::deque<std::string> recentHistory;
};

class AdaptiveWardrobeManager {
private:
    std::unordered_map<RE::FormID, ActorWardrobeData> actorWardrobes;
    mutable std::mutex wardrobeMutex;
    std::mt19937 randomEngine;

    AdaptiveWardrobeManager();

    float CalculatePreferenceWeight(uint8_t stars) const noexcept;
    float CalculateRepetitionPenalty(const std::deque<std::string>& history, const std::string& id) const noexcept;
    ActorWardrobeData& GetOrCreateWardrobeData(RE::FormID a_actorID);

public:
    static AdaptiveWardrobeManager* GetSingleton();

    void LoadWardrobe(RE::FormID a_actorID, const std::vector<AdaptiveOutfit>& outfits);
    std::vector<AdaptiveOutfit> GetWardrobe(RE::FormID a_actorID);

    ContextFlags ResolveContextHierarchy(ContextFlags activeContext) const noexcept;
    std::optional<SelectionResult> SelectOptimalOutfit(RE::FormID a_actorID, ContextFlags activeContext, bool isEmergency);

    void RecordOutfitEquipped(RE::FormID a_actorID, const std::string& outfitID, uint32_t currentTimestamp);

    void SetCurrentOutfitID(RE::FormID a_actorID, const std::string& id);
    std::string GetCurrentOutfitID(RE::FormID a_actorID);
};

#endif