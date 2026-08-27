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

ActorWardrobeData& AdaptiveWardrobeManager::GetOrCreateWardrobeData(RE::FormID a_actorID) {
    RE::FormID canonicalID = (a_actorID == 0x00000014 || a_actorID == 0) ? 0x00000007 : a_actorID;
    return actorWardrobes[canonicalID];
}

void AdaptiveWardrobeManager::LoadWardrobe(RE::FormID a_actorID, const std::vector<AdaptiveOutfit>& outfits) {
    std::lock_guard<std::mutex> lock(wardrobeMutex);
    auto& data = GetOrCreateWardrobeData(a_actorID);
    data.wardrobe = outfits;
}

std::vector<AdaptiveOutfit> AdaptiveWardrobeManager::GetWardrobe(RE::FormID a_actorID) {
    std::lock_guard<std::mutex> lock(wardrobeMutex);
    auto& data = GetOrCreateWardrobeData(a_actorID);
    return data.wardrobe;
}

ContextFlags AdaptiveWardrobeManager::ResolveContextHierarchy(ContextFlags activeContext) const noexcept {
    if (HasContext(activeContext, ContextFlags::Sleep)) return ContextFlags::Sleep;
    if (HasContext(activeContext, ContextFlags::Combat)) return ContextFlags::Combat;
    if (HasContext(activeContext, ContextFlags::Swim)) return ContextFlags::Swim;
    if (HasContext(activeContext, ContextFlags::InWater)) return ContextFlags::InWater;
    if (HasContext(activeContext, ContextFlags::Mount)) return ContextFlags::Mount;

    if (HasContext(activeContext, ContextFlags::Dungeon)) return ContextFlags::Dungeon;
    if (HasContext(activeContext, ContextFlags::Jail)) return ContextFlags::Jail;
    if (HasContext(activeContext, ContextFlags::Castle)) return ContextFlags::Castle;
    if (HasContext(activeContext, ContextFlags::GuildHall)) return ContextFlags::GuildHall;
    if (HasContext(activeContext, ContextFlags::Temple)) return ContextFlags::Temple;
    if (HasContext(activeContext, ContextFlags::Store)) return ContextFlags::Store;
    if (HasContext(activeContext, ContextFlags::Inn)) return ContextFlags::Inn;
    if (HasContext(activeContext, ContextFlags::Farm)) return ContextFlags::Farm;
    if (HasContext(activeContext, ContextFlags::Military)) return ContextFlags::Military;
    if (HasContext(activeContext, ContextFlags::PlayerHome)) return ContextFlags::PlayerHome;

    if (HasContext(activeContext, ContextFlags::City)) {
        if (HasContext(activeContext, ContextFlags::Interior)) return ContextFlags::City | ContextFlags::Interior;
        if (HasContext(activeContext, ContextFlags::Rain)) return ContextFlags::City | ContextFlags::Rain;
        if (HasContext(activeContext, ContextFlags::Snow)) return ContextFlags::City | ContextFlags::Snow;
        if (HasContext(activeContext, ContextFlags::Night)) return ContextFlags::City | ContextFlags::Night;
        return ContextFlags::City;
    }

    if (HasContext(activeContext, ContextFlags::Town)) {
        if (HasContext(activeContext, ContextFlags::Interior)) return ContextFlags::Town | ContextFlags::Interior;
        if (HasContext(activeContext, ContextFlags::Rain)) return ContextFlags::Town | ContextFlags::Rain;
        if (HasContext(activeContext, ContextFlags::Snow)) return ContextFlags::Town | ContextFlags::Snow;
        if (HasContext(activeContext, ContextFlags::Night)) return ContextFlags::Town | ContextFlags::Night;
        return ContextFlags::Town;
    }

    if (HasContext(activeContext, ContextFlags::World)) {
        if (HasContext(activeContext, ContextFlags::Interior)) return ContextFlags::World | ContextFlags::Interior;
        if (HasContext(activeContext, ContextFlags::Rain)) return ContextFlags::World | ContextFlags::Rain;
        if (HasContext(activeContext, ContextFlags::Snow)) return ContextFlags::World | ContextFlags::Snow;
        if (HasContext(activeContext, ContextFlags::Night)) return ContextFlags::World | ContextFlags::Night;
        return ContextFlags::World;
    }

    return ContextFlags::Default;
}

float AdaptiveWardrobeManager::CalculatePreferenceWeight(uint8_t stars) const noexcept {
    static constexpr float kWeightTable[] = { 4.0f, 1.0f, 2.0f, 4.0f, 7.0f, 11.0f };
    if (stars <= 5) return kWeightTable[stars];
    return 4.0f;
}

float AdaptiveWardrobeManager::CalculateRepetitionPenalty(const std::deque<std::string>& history, const std::string& id) const noexcept {
    size_t count = 0;
    for (const auto& historyID : history) {
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

std::optional<SelectionResult> AdaptiveWardrobeManager::SelectOptimalOutfit(RE::FormID a_actorID, ContextFlags activeContext, bool isEmergency) {
    std::lock_guard<std::mutex> lock(wardrobeMutex);
    auto& data = GetOrCreateWardrobeData(a_actorID);
    if (data.wardrobe.empty()) return std::nullopt;

    ContextFlags targetContext = ResolveContextHierarchy(activeContext);

    if (!isEmergency && !data.currentOutfitID.empty()) {
        for (const auto& outfit : data.wardrobe) {
            if (outfit.id == data.currentOutfitID && outfit.controlState != ControlState::Excluded) {
                for (const auto& rule : outfit.suitabilities) {
                    if (HasContext(rule.contextMask, targetContext) || rule.contextMask == targetContext) {
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
    candidates.reserve(data.wardrobe.size());

    for (const auto& outfit : data.wardrobe) {
        if (outfit.controlState == ControlState::Excluded) continue;
        for (const auto& rule : outfit.suitabilities) {
            if (HasContext(rule.contextMask, targetContext) || (rule.contextMask & targetContext) != ContextFlags::None) {
                candidates.push_back(&outfit);
                break;
            }
        }
    }

    if (candidates.empty() && HasContext(targetContext, ContextFlags::City)) {
        for (const auto& outfit : data.wardrobe) {
            if (outfit.controlState == ControlState::Excluded) continue;
            for (const auto& rule : outfit.suitabilities) {
                if (HasContext(rule.contextMask, ContextFlags::City)) {
                    candidates.push_back(&outfit);
                    break;
                }
            }
        }
    }

    if (candidates.empty() && HasContext(targetContext, ContextFlags::Town)) {
        for (const auto& outfit : data.wardrobe) {
            if (outfit.controlState == ControlState::Excluded) continue;
            for (const auto& rule : outfit.suitabilities) {
                if (HasContext(rule.contextMask, ContextFlags::Town)) {
                    candidates.push_back(&outfit);
                    break;
                }
            }
        }
    }

    if (candidates.empty() && HasContext(targetContext, ContextFlags::World)) {
        for (const auto& outfit : data.wardrobe) {
            if (outfit.controlState == ControlState::Excluded) continue;
            for (const auto& rule : outfit.suitabilities) {
                if (HasContext(rule.contextMask, ContextFlags::World)) {
                    candidates.push_back(&outfit);
                    break;
                }
            }
        }
    }

    if (candidates.empty()) {
        for (const auto& outfit : data.wardrobe) {
            if (outfit.controlState != ControlState::Excluded) {
                candidates.push_back(&outfit);
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
        float repPenalty = CalculateRepetitionPenalty(data.recentHistory, candidate->id);
        float ctrlMultiplier = (candidate->controlState == ControlState::Favorite) ? 1.5f : 1.0f;

        float weight = prefWeight * repPenalty * ctrlMultiplier;
        weights.push_back(weight);
        totalWeight += weight;
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
    result.reasonCode = "Weighted Selection";
    result.calculatedWeight = weights[selectedIndex];
    return result;
}

void AdaptiveWardrobeManager::RecordOutfitEquipped(RE::FormID a_actorID, const std::string& outfitID, uint32_t) {
    std::lock_guard<std::mutex> lock(wardrobeMutex);
    auto& data = GetOrCreateWardrobeData(a_actorID);
    data.currentOutfitID = outfitID;

    data.recentHistory.push_front(outfitID);
    if (data.recentHistory.size() > 5) {
        data.recentHistory.pop_back();
    }
}

void AdaptiveWardrobeManager::SetCurrentOutfitID(RE::FormID a_actorID, const std::string& id) {
    std::lock_guard<std::mutex> lock(wardrobeMutex);
    auto& data = GetOrCreateWardrobeData(a_actorID);
    data.currentOutfitID = id;
}

std::string AdaptiveWardrobeManager::GetCurrentOutfitID(RE::FormID a_actorID) {
    std::lock_guard<std::mutex> lock(wardrobeMutex);
    auto& data = GetOrCreateWardrobeData(a_actorID);
    return data.currentOutfitID;
}