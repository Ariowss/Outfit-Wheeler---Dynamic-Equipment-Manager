#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

using json = nlohmann::json;

class ModCatalogManager {
public:
    static ModCatalogManager* GetSingleton();

    json GetModSummaries();
    json GetModItems(const std::string& modName);
    bool SpawnItem(uint32_t rawID, const std::string& targetActorId, bool autoEquip, const std::string& destination);

    void SetMockData(const json& summaries, const json& items);
    void EnsureCatalogIndexed();

private:
    ModCatalogManager() = default;
    json m_mockSummaries = json::array();
    json m_mockItems = json::array();

    std::mutex m_catalogMutex;
    bool m_catalogIndexed{ false };
    json m_cachedSummaries = json::array();
    std::unordered_map<std::string, json> m_cachedModItems;
    std::unordered_map<std::string, std::vector<uint32_t>> m_modFormIDsMap;
};
