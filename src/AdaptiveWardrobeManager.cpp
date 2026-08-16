#include "pch.h"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include "AdaptiveWardrobeManager.h"
#include <SKSE/SKSE.h>
#include <array>

AdaptiveWardrobeManager::AdaptiveWardrobeManager() {
    std::random_device rd;
    randomEngine = std::mt19937(rd());
}

AdaptiveWardrobeManager* AdaptiveWardrobeManager::GetSingleton() {
    static AdaptiveWardrobeManager instance;
    return &instance;
}

void AdaptiveWardrobeManager::LoadWardrobe(const std::vector<AdaptiveOutfit>& outfits) {
    std::lock_guard<std::mutex> lock(wardrobeMutex);
    wardrobe = outfits;
}

std::vector<AdaptiveOutfit> AdaptiveWardrobeManager::GetWardrobe() {
    std::lock_guard<std::mutex> lock(wardrobeMutex);
    return wardrobe;
}

ContextFlags AdaptiveWardrobeManager::ResolveContextHierarchy(ContextFlags activeContext) const noexcept {
    if (HasContext(activeContext, ContextFlags::Combat)) return ContextFlags::Combat;
    if (HasContext(activeContext, ContextFlags::Swim)) return ContextFlags::Swim;
    if (HasContext(activeContext, ContextFlags::PlayerHome)) return ContextFlags::PlayerHome;
    if (HasContext(activeContext, ContextFlags::CityTown)) return ContextFlags::CityTown;
    return ContextFlags::None;
}

float AdaptiveWardrobeManager::CalculatePreferenceWeight(uint8_t stars) const noexcept {
    static constexpr float kWeightTable[] = { 4.0f, 1.0f, 2.0f, 4.0f, 7.0f, 11.0f };
    if (stars <= 5) return kWeightTable[stars];
    return 4.0f;
}

float AdaptiveWardrobeManager::CalculateRepetitionPenalty(const std::string& id) const noexcept {
    size_t count = 0;
    for (const auto& historyID : recentHistory) {
        if (historyID == id) {
            count++;
        }
    }
    if (count == 0) return 1.00f;
    if (count == 1) return 0.70f;
    if (count == 2) return 0.40f;
    if (count == 3) return 0.20f;
    return 0.10f;
}

std::optional<SelectionResult> AdaptiveWardrobeManager::SelectOptimalOutfit(ContextFlags activeContext, [[maybe_unused]] bool isEmergency) {
    std::lock_guard<std::mutex> lock(wardrobeMutex);
    if (wardrobe.empty()) return std::nullopt;

    ContextFlags targetContext = ResolveContextHierarchy(activeContext);
    if (targetContext == ContextFlags::None) return std::nullopt;

    if (!currentOutfitID.empty()) {
        for (const auto& outfit : wardrobe) {
            if (outfit.id == currentOutfitID && outfit.controlState != ControlState::Excluded) {
                for (const auto& rule : outfit.suitabilities) {
                    if (HasContext(rule.contextMask, targetContext)) {
                        SelectionResult res;
                        res.selectedOutfit = outfit;
                        res.reasonCode = "Current Outfit Already Suitable";
                        res.calculatedWeight = 100.0f;
                        return res;
                    }
                }
            }
        }
    }

    std::array<const AdaptiveOutfit*, 64> candidatesBuffer;
    size_t candidateCount = 0;

    for (const auto& outfit : wardrobe) {
        if (outfit.controlState == ControlState::Excluded) continue;
        for (const auto& rule : outfit.suitabilities) {
            if (HasContext(rule.contextMask, targetContext)) {
                if (candidateCount < candidatesBuffer.size()) {
                    candidatesBuffer[candidateCount++] = &outfit;
                }
                break;
            }
        }
    }

    if (candidateCount == 0) return std::nullopt;

    for (size_t i = 0; i < candidateCount; ++i) {
        if (candidatesBuffer[i]->controlState == ControlState::Forced) {
            SelectionResult res;
            res.selectedOutfit = *candidatesBuffer[i];
            res.reasonCode = "Forced Control State Priority";
            res.calculatedWeight = 99999.0f;
            return res;
        }
    }

    std::array<float, 64> weightsBuffer;
    float totalWeight = 0.0f;

    for (size_t i = 0; i < candidateCount; ++i) {
        const auto* candidate = candidatesBuffer[i];
        float prefWeight = CalculatePreferenceWeight(candidate->preferenceStars);
        float repPenalty = CalculateRepetitionPenalty(candidate->id);
        float ctrlMultiplier = (candidate->controlState == ControlState::Favorite) ? 1.5f : 1.0f;

        float weight = prefWeight * repPenalty * ctrlMultiplier;
        weightsBuffer[i] = weight;
        totalWeight += weight;

        SKSE::log::info("SelectOptimalOutfit: Candidate '{}' ({}) | Stars: {} | RepPenalty: {:.2f} | FinalWeight: {:.2f}",
                        candidate->name, candidate->id, candidate->preferenceStars, repPenalty, weight);
    }

    if (totalWeight <= 0.0f) return std::nullopt;

    std::uniform_real_distribution<float> dist(0.0f, totalWeight);
    float roll = dist(randomEngine);
    float currentSum = 0.0f;
    size_t selectedIndex = 0;

    for (size_t i = 0; i < candidateCount; ++i) {
        currentSum += weightsBuffer[i];
        if (roll <= currentSum) {
            selectedIndex = i;
            break;
        }
    }

    SelectionResult result;
    result.selectedOutfit = *candidatesBuffer[selectedIndex];
    result.reasonCode = "Weighted Random Selection";
    result.calculatedWeight = weightsBuffer[selectedIndex];

    SKSE::log::info("SelectOptimalOutfit: WINNER Selected '{}' ({}) | Weight: {:.2f} / Total: {:.2f} (Prob: {:.1f}%)",
                    result.selectedOutfit.name, result.selectedOutfit.id, result.calculatedWeight, totalWeight,
                    (result.calculatedWeight / totalWeight) * 100.0f);

    return result;
}

void AdaptiveWardrobeManager::RecordOutfitEquipped(const std::string& outfitID, uint32_t) {
    std::lock_guard<std::mutex> lock(wardrobeMutex);
    currentOutfitID = outfitID;

    recentHistory.push_front(outfitID);
    if (recentHistory.size() > 5) {
        recentHistory.pop_back();
    }
}

void AdaptiveWardrobeManager::SetCurrentOutfitID(const std::string& id) {
    std::lock_guard<std::mutex> lock(wardrobeMutex);
    currentOutfitID = id;
}

std::string AdaptiveWardrobeManager::GetCurrentOutfitID() {
    std::lock_guard<std::mutex> lock(wardrobeMutex);
    return currentOutfitID;
}