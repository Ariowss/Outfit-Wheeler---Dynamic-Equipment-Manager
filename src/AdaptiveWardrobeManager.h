#ifndef ADAPTIVE_WARDROBE_MANAGER_H
#define ADAPTIVE_WARDROBE_MANAGER_H

#include "AdaptiveWardrobeTypes.h"
#include <mutex>
#include <optional>
#include <random>
#include <deque>
#include <string>
#include <vector>

struct SelectionResult {
    AdaptiveOutfit selectedOutfit;
    std::string reasonCode;
    float calculatedWeight = 0.0f;
};

class AdaptiveWardrobeManager {
private:
    std::vector<AdaptiveOutfit> wardrobe;
    std::string currentOutfitID;
    std::deque<std::string> recentHistory;
    mutable std::mutex wardrobeMutex;
    std::mt19937 randomEngine;

    AdaptiveWardrobeManager();

    float CalculatePreferenceWeight(uint8_t stars) const;
    float CalculateRepetitionPenalty(const std::string& id) const;

public:
    static AdaptiveWardrobeManager* GetSingleton();

    void LoadWardrobe(const std::vector<AdaptiveOutfit>& outfits);
    std::vector<AdaptiveOutfit> GetWardrobe();

    ContextFlags ResolveContextHierarchy(ContextFlags activeContext) const;
    std::optional<SelectionResult> SelectOptimalOutfit(ContextFlags activeContext, bool isEmergency);

    void RecordOutfitEquipped(const std::string& outfitID, uint32_t currentTimestamp);

    void SetCurrentOutfitID(const std::string& id);
    std::string GetCurrentOutfitID();
};

#endif