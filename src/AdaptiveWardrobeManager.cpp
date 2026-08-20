#include "pch.h"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include "AdaptiveWardrobeManager.h"
#include <SKSE/SKSE.h>
#include <chrono>

AdaptiveWardrobeManager::AdaptiveWardrobeManager() {
    auto seed = static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    randomEngine = std::mt19937(seed);
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

    std::vector<const AdaptiveOutfit*> candidates;
    candidates.reserve(wardrobe.size());

    for (const auto& outfit : wardrobe) {
        if (outfit.controlState == ControlState::Excluded) continue;
        for (const auto& rule : outfit.suitabilities) {
            if (HasContext(rule.contextMask, targetContext)) {
                candidates.push_back(&outfit);
                break;
            }
        }
    }

    if (candidates.empty()) return std::nullopt;

    for (const auto* cand : candidates) {
        if (cand->controlState == ControlState::Forced) {
            SelectionResult res;
            res.selectedOutfit = *cand;
            res.reasonCode = "Forced Control State Priority";
            res.calculatedWeight = 99999.0f;
            return res;
        }
    }

    std::vector<float> weights;
    weights.reserve(candidates.size());
    float totalWeight = 0.0f;

    for (const auto* candidate : candidates) {
        float prefWeight = CalculatePreferenceWeight(candidate->preferenceStars);
        float repPenalty = CalculateRepetitionPenalty(candidate->id);
        float ctrlMultiplier = (candidate->controlState == ControlState::Favorite) ? 1.5f : 1.0f;

        float weight = prefWeight * repPenalty * ctrlMultiplier;
        weights.push_back(weight);
        totalWeight += weight;

        SKSE::log::info("SelectOptimalOutfit: Candidate '{}' ({}) | Stars: {} | RepPenalty: {:.2f} | FinalWeight: {:.2f}",
                        candidate->name, candidate->id, candidate->preferenceStars, repPenalty, weight);
    }

    if (totalWeight <= 0.0f) return std::nullopt;

    std::uniform_real_distribution<float> dist(0.0f, totalWeight);
    float roll = dist(randomEngine);
    float currentSum = 0.0f;
    size_t selectedIndex = 0;

    for (size_t i = 0; i < candidates.size(); ++i) {
        currentSum += weights[i];
        if (roll <= currentSum) {
            selectedIndex = i;
            break;
        }
    }

    SelectionResult result;
    result.selectedOutfit = *candidates[selectedIndex];
    result.reasonCode = "Weighted Random Selection";
    result.calculatedWeight = weights[selectedIndex];

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