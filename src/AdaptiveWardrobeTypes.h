#ifndef ADAPTIVE_WARDROBE_TYPES_H
#define ADAPTIVE_WARDROBE_TYPES_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <RE/Skyrim.h>
#include <nlohmann/json.hpp>
#include <cstdint>
#include <string>
#include <vector>

using json = nlohmann::json;

struct OutfitItem {
    RE::FormID formID{ 0 };
    uint16_t uniqueID{ 0 };
    RE::FormID enchantmentID{ 0 };
    std::string customName{};
    std::string pluginName{};
    uint32_t localFormID{ 0 };
    bool isWornLeft{ false };
};

inline void to_json(json& j, const OutfitItem& itm) {
    j = json{
        {"formID", itm.formID},
        {"uniqueID", itm.uniqueID},
        {"enchantmentID", static_cast<uint32_t>(itm.enchantmentID)},
        {"customName", itm.customName},
        {"plugin", itm.pluginName},
        {"localID", itm.localFormID},
        {"wornLeft", itm.isWornLeft}
    };
}

inline void from_json(const json& j, OutfitItem& itm) {
    itm.formID = j.value("formID", static_cast<RE::FormID>(0));
    itm.uniqueID = j.value("uniqueID", static_cast<uint16_t>(0));
    itm.enchantmentID = static_cast<RE::FormID>(j.value("enchantmentID", static_cast<uint32_t>(0)));
    itm.customName = j.value("customName", "");
    itm.pluginName = j.value("plugin", "");
    itm.localFormID = j.value("localID", static_cast<uint32_t>(0));
    itm.isWornLeft = j.value("wornLeft", false);
}

enum class ContextFlags : uint64_t {
    None            = 0,
    Default         = 1ULL << 0,

    Combat          = 1ULL << 1,
    InWater         = 1ULL << 2,
    Swim            = 1ULL << 3,
    Sleep           = 1ULL << 4,
    Mount           = 1ULL << 5,

    Dungeon         = 1ULL << 10,
    PlayerHome      = 1ULL << 11,
    Inn             = 1ULL << 12,
    Store           = 1ULL << 13,
    GuildHall       = 1ULL << 14,
    Castle          = 1ULL << 15,
    Temple          = 1ULL << 16,
    Farm            = 1ULL << 17,
    Jail            = 1ULL << 18,
    Military        = 1ULL << 19,

    World           = 1ULL << 25,
    Town            = 1ULL << 26,
    City            = 1ULL << 27,

    Night           = 1ULL << 30,
    Rain            = 1ULL << 31,
    Snow            = 1ULL << 32,
    Interior        = 1ULL << 33
};

inline ContextFlags operator|(ContextFlags a, ContextFlags b) {
    return static_cast<ContextFlags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

inline ContextFlags operator&(ContextFlags a, ContextFlags b) {
    return static_cast<ContextFlags>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

inline ContextFlags& operator|=(ContextFlags& a, ContextFlags b) {
    a = a | b;
    return a;
}

inline bool HasContext(ContextFlags mask, ContextFlags flag) {
    return (static_cast<uint64_t>(mask) & static_cast<uint64_t>(flag)) != 0;
}

enum class ControlState : uint8_t {
    Normal = 0,
    Favorite,
    Forced,
    Excluded,
    Locked
};

struct SuitabilityRule {
    ContextFlags contextMask = ContextFlags::None;
    uint8_t score = 1;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SuitabilityRule, contextMask, score)

struct DynamicVariables {
    uint32_t usageCount = 0;
    float freshnessMultiplier = 1.5f;
    uint32_t lastWornTimestamp = 0;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DynamicVariables, usageCount, freshnessMultiplier, lastWornTimestamp)

struct AdaptiveOutfit {
    std::string id;
    std::string name;
    std::vector<RE::FormID> itemIDs;
    std::vector<OutfitItem> items;
    std::vector<SuitabilityRule> suitabilities;
    uint8_t preferenceStars = 3;
    ControlState controlState = ControlState::Normal;
    DynamicVariables dynamicVars;
};

inline void to_json(json& j, const AdaptiveOutfit& o) {
    j = json{
        {"id", o.id},
        {"name", o.name},
        {"itemIDs", o.itemIDs},
        {"items", o.items},
        {"suitabilities", o.suitabilities},
        {"preferenceStars", o.preferenceStars},
        {"controlState", static_cast<uint8_t>(o.controlState)},
        {"dynamicVars", o.dynamicVars}
    };
}

inline void from_json(const json& j, AdaptiveOutfit& o) {
    j.at("id").get_to(o.id);
    j.at("name").get_to(o.name);
    j.at("itemIDs").get_to(o.itemIDs);
    if (j.contains("items")) {
        j.at("items").get_to(o.items);
    }
    j.at("suitabilities").get_to(o.suitabilities);
    j.at("preferenceStars").get_to(o.preferenceStars);
    uint8_t cs = j.at("controlState").get<uint8_t>();
    o.controlState = static_cast<ControlState>(cs);
    j.at("dynamicVars").get_to(o.dynamicVars);
}

#endif