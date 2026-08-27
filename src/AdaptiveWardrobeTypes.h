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
    j.at("suitabilities").get_to(o.suitabilities);
    j.at("preferenceStars").get_to(o.preferenceStars);
    uint8_t cs = j.at("controlState").get<uint8_t>();
    o.controlState = static_cast<ControlState>(cs);
    j.at("dynamicVars").get_to(o.dynamicVars);
}

#endif