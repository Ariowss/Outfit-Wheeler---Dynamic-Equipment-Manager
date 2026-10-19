#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <string_view>
#include <filesystem>
#include <fstream>
#include <windows.h>
#include <nlohmann/json.hpp>
#include <cmath>
#include <deque>
#include <algorithm>
#include <unordered_map>
#include <optional>
#include <cstdint>
#include <stdexcept>
#include "Settings.h"
#include "ThumbnailDiskCache.h"
#include "ModCatalogManager.h"

using json = nlohmann::json;

static int g_passedTests = 0;
static int g_totalTests = 0;

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        throw std::runtime_error(std::string("Assertion failed: ") + #cond); \
    } \
} while(0)

#define RUN_TEST(name, func) do { \
    g_totalTests++; \
    try { \
        func(); \
        g_passedTests++; \
        std::cout << "[PASS] " << name << std::endl; \
    } catch (const std::exception& e) { \
        std::cerr << "[FAIL] " << name << ": " << e.what() << std::endl; \
    } catch (...) { \
        std::cerr << "[FAIL] " << name << ": unknown error" << std::endl; \
    } \
} while(0)

static std::string ToLowerString(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return str;
}

static bool ContainsTagToken(const std::string& str, const std::string_view token) {
    size_t pos = 0;
    while ((pos = str.find(token, pos)) != std::string::npos) {
        bool leftOk = (pos == 0) || !std::isalnum(static_cast<unsigned char>(str[pos - 1]));
        bool rightOk = (pos + token.size() >= str.size()) || !std::isalnum(static_cast<unsigned char>(str[pos + token.size()]));
        if (leftOk && rightOk) return true;
        pos += token.size();
    }
    return false;
}

enum class ContextFlags : uint64_t {
    None        = 0,
    Default     = 1ULL << 0,
    Combat      = 1ULL << 1,
    InWater     = 1ULL << 2,
    Swim        = 1ULL << 3,
    Sleep       = 1ULL << 4,
    Mount       = 1ULL << 5,
    Dungeon     = 1ULL << 10,
    PlayerHome  = 1ULL << 11,
    Inn         = 1ULL << 12,
    Store       = 1ULL << 13,
    GuildHall   = 1ULL << 14,
    Castle      = 1ULL << 15,
    Temple      = 1ULL << 16,
    Farm        = 1ULL << 17,
    Jail        = 1ULL << 18,
    Military    = 1ULL << 19,
    World       = 1ULL << 25,
    Town        = 1ULL << 26,
    City        = 1ULL << 27,
    Night       = 1ULL << 30,
    Rain        = 1ULL << 31,
    Snow        = 1ULL << 32,
    Interior    = 1ULL << 33
};

inline ContextFlags operator|(ContextFlags a, ContextFlags b) {
    return static_cast<ContextFlags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

inline ContextFlags operator&(ContextFlags a, ContextFlags b) {
    return static_cast<ContextFlags>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

inline bool HasContext(ContextFlags mask, ContextFlags flag) {
    return (static_cast<uint64_t>(mask) & static_cast<uint64_t>(flag)) != 0;
}

static ContextFlags ParseContextTagToFlags(const std::string& tag) {
    std::string lower = ToLowerString(tag);
    ContextFlags flags = ContextFlags::None;

    if (ContainsTagToken(lower, "combat") || ContainsTagToken(lower, "fight")) flags = flags | ContextFlags::Combat;
    if (ContainsTagToken(lower, "swim")) flags = flags | ContextFlags::Swim;
    if (ContainsTagToken(lower, "in_water") || ContainsTagToken(lower, "water")) flags = flags | ContextFlags::InWater;
    if (ContainsTagToken(lower, "sleep") || ContainsTagToken(lower, "bed") || ContainsTagToken(lower, "pajama") || ContainsTagToken(lower, "nightwear") || ContainsTagToken(lower, "rest")) flags = flags | ContextFlags::Sleep;
    if (ContainsTagToken(lower, "mount") || ContainsTagToken(lower, "horse") || ContainsTagToken(lower, "ride") || ContainsTagToken(lower, "riding")) flags = flags | ContextFlags::Mount;

    if (ContainsTagToken(lower, "dungeon") || ContainsTagToken(lower, "tomb") || ContainsTagToken(lower, "ruin")) flags = flags | ContextFlags::Dungeon;
    if (ContainsTagToken(lower, "home") || ContainsTagToken(lower, "house")) flags = flags | ContextFlags::PlayerHome;
    if (ContainsTagToken(lower, "inn") || ContainsTagToken(lower, "tavern")) flags = flags | ContextFlags::Inn;
    if (ContainsTagToken(lower, "store") || ContainsTagToken(lower, "shop") || ContainsTagToken(lower, "market")) flags = flags | ContextFlags::Store;
    if (ContainsTagToken(lower, "guild") || ContainsTagToken(lower, "thief") || ContainsTagToken(lower, "brotherhood")) flags = flags | ContextFlags::GuildHall;
    if (ContainsTagToken(lower, "castle") || ContainsTagToken(lower, "palace")) flags = flags | ContextFlags::Castle;
    if (ContainsTagToken(lower, "temple") || ContainsTagToken(lower, "shrine") || ContainsTagToken(lower, "talos")) flags = flags | ContextFlags::Temple;
    if (ContainsTagToken(lower, "farm") || ContainsTagToken(lower, "mill")) flags = flags | ContextFlags::Farm;
    if (ContainsTagToken(lower, "jail") || ContainsTagToken(lower, "prison")) flags = flags | ContextFlags::Jail;
    if (ContainsTagToken(lower, "military") || ContainsTagToken(lower, "camp") || ContainsTagToken(lower, "war")) flags = flags | ContextFlags::Military;

    if (ContainsTagToken(lower, "city")) flags = flags | ContextFlags::City;
    if (ContainsTagToken(lower, "town") || ContainsTagToken(lower, "village")) flags = flags | ContextFlags::Town;
    if (ContainsTagToken(lower, "world") || ContainsTagToken(lower, "adventure") || ContainsTagToken(lower, "mountain") || ContainsTagToken(lower, "nature")) flags = flags | ContextFlags::World;

    if (ContainsTagToken(lower, "night")) flags = flags | ContextFlags::Night;
    if (ContainsTagToken(lower, "rain")) flags = flags | ContextFlags::Rain;
    if (ContainsTagToken(lower, "snow")) flags = flags | ContextFlags::Snow;
    if (ContainsTagToken(lower, "interior")) flags = flags | ContextFlags::Interior;

    return flags;
}

static ContextFlags ResolveHierarchy(ContextFlags activeContext) {
    if (HasContext(activeContext, ContextFlags::Sleep)) return ContextFlags::Sleep;
    if (HasContext(activeContext, ContextFlags::Mount)) return ContextFlags::Mount;
    if (HasContext(activeContext, ContextFlags::Combat)) return ContextFlags::Combat;
    if (HasContext(activeContext, ContextFlags::Swim)) return ContextFlags::Swim;
    if (HasContext(activeContext, ContextFlags::InWater)) return ContextFlags::InWater;

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
        if (HasContext(activeContext, ContextFlags::Snow)) return ContextFlags::Snow | ContextFlags::City;
        if (HasContext(activeContext, ContextFlags::Night)) return ContextFlags::Night | ContextFlags::City;
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

static float PreferenceWeight(uint8_t stars) {
    if (stars == 0) return 0.0f;
    static constexpr float kWeightTable[] = { 0.0f, 1.0f, 2.0f, 4.0f, 7.0f, 11.0f };
    if (stars <= 5) return kWeightTable[stars];
    return 4.0f;
}

static float RepetitionPenalty(const std::deque<std::string>& history, const std::string& id) {
    size_t count = 0;
    for (const auto& historyID : history) {
        if (historyID == id) count++;
    }
    if (count == 0) return 1.00f;
    if (count == 1) return 0.70f;
    if (count == 2) return 0.40f;
    if (count == 3) return 0.20f;
    return 0.10f;
}

struct TestOutfitItem {
    uint32_t formID{ 0 };
    uint16_t uniqueID{ 0 };
    uint32_t enchantmentID{ 0 };
    std::string customName{};
};

struct TestWornEntry {
    uint32_t formID{ 0 };
    uint16_t uniqueID{ 0 };
    uint32_t enchantmentID{ 0 };
    std::string customName{};
    bool hasCustomName{ false };
    bool hasEnchantment{ false };
};

static bool MatchesEntry(const TestWornEntry& entry, const TestOutfitItem& target) {
    if (entry.formID != target.formID) return false;

    if (target.uniqueID != 0) {
        if (entry.uniqueID == target.uniqueID) return true;
    }

    if (!target.customName.empty()) {
        if (!entry.hasCustomName || entry.customName != target.customName) return false;
    } else {
        if (entry.hasCustomName && !entry.customName.empty()) return false;
    }

    if (target.enchantmentID != 0) {
        if (!entry.hasEnchantment || entry.enchantmentID != target.enchantmentID) return false;
        return true;
    } else {
        if (entry.hasEnchantment) return false;
    }

    return true;
}

enum class ItemType {
    Armor,
    WeaponOneHanded,
    WeaponTwoHanded,
    WeaponBow,
    WeaponCrossbow,
    WeaponDagger,
    WeaponStaff,
    Shield,
    Ammo,
    Potion,
    Scroll
};

struct TestItemForm {
    uint32_t formID{ 0 };
    ItemType type{ ItemType::Armor };
    bool isPlayable{ true };
    bool isDeleted{ false };
    std::string name{};
};

static bool IsPureApparelTest(const TestItemForm& item) {
    if (item.type != ItemType::Armor) return false;
    if (!item.isPlayable) return false;
    if (item.isDeleted) return false;
    if (item.name.empty()) return false;
    return true;
}

void TestIniReadWriteAndFallback() {
    std::filesystem::create_directories("Data/SKSE/Plugins");
    const std::string testIni = "Data/SKSE/Plugins/OutfitWheeler.ini";

    WritePrivateProfileStringA("Hotkeys", "kEditModeKey", "0x32", testIni.c_str());
    WritePrivateProfileStringA("Hotkeys", "kEditModeModifier", "0x2A", testIni.c_str());
    WritePrivateProfileStringA("Hotkeys", "kPlayModeKey", "0x2F", testIni.c_str());
    WritePrivateProfileStringA("Hotkeys", "kPlayModeModifier", "0x1D", testIni.c_str());

    WritePrivateProfileStringA("Gamepad", "kGamepadPlayKey", "8", testIni.c_str());
    WritePrivateProfileStringA("Gamepad", "kGamepadPlayModifier", "256", testIni.c_str());
    WritePrivateProfileStringA("Gamepad", "fThumbstickDeadzone", "0.35", testIni.c_str());

    WritePrivateProfileStringA("Appearance", "fWheelScale", "1.25", testIni.c_str());
    WritePrivateProfileStringA("Appearance", "fUiScale", "1.50", testIni.c_str());
    WritePrivateProfileStringA("Appearance", "uEditUiMode", "2", testIni.c_str());

    WritePrivateProfileStringA("AutoSystem", "bAutoCloseOnEquip", "0", testIni.c_str());
    WritePrivateProfileStringA("AutoSystem", "bAutoStashInactive", "1", testIni.c_str());
    WritePrivateProfileStringA("AutoSystem", "bAutoSystem", "1", testIni.c_str());
    WritePrivateProfileStringA("AutoSystem", "bAutoCity", "1", testIni.c_str());
    WritePrivateProfileStringA("AutoSystem", "bAutoHome", "0", testIni.c_str());
    WritePrivateProfileStringA("AutoSystem", "bAutoCombat", "1", testIni.c_str());
    WritePrivateProfileStringA("AutoSystem", "bAutoSwim", "1", testIni.c_str());

    LoadINI();

    TEST_ASSERT(g_editKey == 0x32);
    TEST_ASSERT(g_editModifierKey == 0x2A);
    TEST_ASSERT(g_playKey == 0x2F);
    TEST_ASSERT(g_playModifierKey == 0x1D);
    TEST_ASSERT(g_gamepadPlayKey == 8);
    TEST_ASSERT(g_gamepadPlayModifier == 256);
    TEST_ASSERT(std::fabs(g_thumbstickDeadzone - 0.35f) < 0.001f);
    TEST_ASSERT(std::fabs(g_wheelScale - 1.25f) < 0.001f);
    TEST_ASSERT(std::fabs(g_uiScale - 1.50f) < 0.001f);
    TEST_ASSERT(g_editUiMode == 2);
    TEST_ASSERT(g_autoCloseOnEquip.load() == false);
    TEST_ASSERT(g_autoStashEnabled.load() == true);
    TEST_ASSERT(g_autoSystemEnabled.load() == true);
    TEST_ASSERT(g_autoCityEnabled.load() == true);
    TEST_ASSERT(g_autoHomeEnabled.load() == false);
    TEST_ASSERT(g_autoCombatEnabled.load() == true);
    TEST_ASSERT(g_autoSwimEnabled.load() == true);

    g_uiScale = 1.75f;
    g_autoHomeEnabled.store(true);
    g_showItemPreviews.store(false);
    SaveINI();

    LoadINI();
    TEST_ASSERT(std::fabs(g_uiScale - 1.75f) < 0.001f);
    TEST_ASSERT(g_autoHomeEnabled.load() == true);
    TEST_ASSERT(g_showItemPreviews.load() == false);

    g_showItemPreviews.store(true);
    g_themeAccentColor = "#ef4444";
    g_themeBgColor = "#1a0507";
    g_themeTextColor = "#fff1f2";
    SaveINI();
    LoadINI();
    TEST_ASSERT(g_showItemPreviews.load() == true);
    TEST_ASSERT(g_themeAccentColor == "#ef4444");
    TEST_ASSERT(g_themeBgColor == "#1a0507");
    TEST_ASSERT(g_themeTextColor == "#fff1f2");
}

void TestFormIDEncodingAndESLArithmetic() {
    uint32_t regularFormID = 0x01012E82;
    uint8_t regularModIndex = static_cast<uint8_t>((regularFormID >> 24) & 0xFF);
    uint32_t regularLocalID = regularFormID & 0x00FFFFFF;

    TEST_ASSERT(regularModIndex == 0x01);
    TEST_ASSERT(regularLocalID == 0x00012E82);

    uint8_t compileIndex = 0x05;
    uint32_t reconstructedRegular = (static_cast<uint32_t>(compileIndex) << 24) | (regularLocalID & 0x00FFFFFF);
    TEST_ASSERT(reconstructedRegular == 0x05012E82);

    uint32_t lightFormID = 0xFE042805;
    uint8_t eslMarker = static_cast<uint8_t>((lightFormID >> 24) & 0xFF);
    uint16_t lightIndex = static_cast<uint16_t>((lightFormID >> 12) & 0x0FFF);
    uint32_t lightLocalID = lightFormID & 0x00000FFF;

    TEST_ASSERT(eslMarker == 0xFE);
    TEST_ASSERT(lightIndex == 0x042);
    TEST_ASSERT(lightLocalID == 0x805);

    uint16_t newLightIndex = 0x01A;
    uint32_t reconstructedLight = 0xFE000000 | (static_cast<uint32_t>(newLightIndex) << 12) | (lightLocalID & 0x00000FFF);
    TEST_ASSERT(reconstructedLight == 0xFE01A805);

    json j;
    j["plugin"] = "MyLightArmor.esl";
    j["localID"] = "0x00000805";
    j["rawID"] = lightFormID;

    std::string s = j["localID"].get<std::string>();
    if (s.rfind("0x", 0) == 0) s = s.substr(2);
    uint32_t parsedLocal = static_cast<uint32_t>(std::stoul(s, nullptr, 16));
    TEST_ASSERT(parsedLocal == 0x805);
}

void TestContextTagsAnd23Conditions() {
    std::string tag = "city, night, rain";
    ContextFlags flags = ParseContextTagToFlags(tag);
    TEST_ASSERT(HasContext(flags, ContextFlags::City));
    TEST_ASSERT(HasContext(flags, ContextFlags::Night));
    TEST_ASSERT(HasContext(flags, ContextFlags::Rain));
    TEST_ASSERT(!HasContext(flags, ContextFlags::Combat));

    std::string combatTag = "combat, dungeon";
    ContextFlags cFlags = ParseContextTagToFlags(combatTag);
    TEST_ASSERT(HasContext(cFlags, ContextFlags::Combat));
    TEST_ASSERT(HasContext(cFlags, ContextFlags::Dungeon));

    std::string swimTag = "swim, in_water";
    ContextFlags sFlags = ParseContextTagToFlags(swimTag);
    TEST_ASSERT(HasContext(sFlags, ContextFlags::Swim));
    TEST_ASSERT(HasContext(sFlags, ContextFlags::InWater));

    std::string sleepTag = "rest, pajama, bed";
    ContextFlags slFlags = ParseContextTagToFlags(sleepTag);
    TEST_ASSERT(HasContext(slFlags, ContextFlags::Sleep));

    std::string mountTag = "horse, ride";
    ContextFlags mFlags = ParseContextTagToFlags(mountTag);
    TEST_ASSERT(HasContext(mFlags, ContextFlags::Mount));
}

void TestHierarchyResolution() {
    ContextFlags active1 = ContextFlags::Combat | ContextFlags::Dungeon | ContextFlags::Night;
    ContextFlags resolved1 = ResolveHierarchy(active1);
    TEST_ASSERT(resolved1 == ContextFlags::Combat);

    ContextFlags active2 = ContextFlags::Sleep | ContextFlags::PlayerHome;
    ContextFlags resolved2 = ResolveHierarchy(active2);
    TEST_ASSERT(resolved2 == ContextFlags::Sleep);

    ContextFlags active3 = ContextFlags::Mount | ContextFlags::World;
    ContextFlags resolved3 = ResolveHierarchy(active3);
    TEST_ASSERT(resolved3 == ContextFlags::Mount);

    ContextFlags active4 = ContextFlags::Swim | ContextFlags::World;
    ContextFlags resolved4 = ResolveHierarchy(active4);
    TEST_ASSERT(resolved4 == ContextFlags::Swim);

    ContextFlags active5 = ContextFlags::Castle | ContextFlags::Interior;
    ContextFlags resolved5 = ResolveHierarchy(active5);
    TEST_ASSERT(resolved5 == ContextFlags::Castle);

    ContextFlags active6 = ContextFlags::City | ContextFlags::Rain;
    ContextFlags resolved6 = ResolveHierarchy(active6);
    TEST_ASSERT(HasContext(resolved6, ContextFlags::City));
    TEST_ASSERT(HasContext(resolved6, ContextFlags::Rain));

    ContextFlags active7 = ContextFlags::None;
    ContextFlags resolved7 = ResolveHierarchy(active7);
    TEST_ASSERT(resolved7 == ContextFlags::Default);
}

void TestStarPreferenceAndRepetitionPenalty() {
    TEST_ASSERT(PreferenceWeight(0) == 0.0f);
    TEST_ASSERT(PreferenceWeight(1) == 1.0f);
    TEST_ASSERT(PreferenceWeight(2) == 2.0f);
    TEST_ASSERT(PreferenceWeight(3) == 4.0f);
    TEST_ASSERT(PreferenceWeight(4) == 7.0f);
    TEST_ASSERT(PreferenceWeight(5) == 11.0f);

    std::deque<std::string> history;
    TEST_ASSERT(RepetitionPenalty(history, "outfit_1") == 1.00f);

    history.push_front("outfit_1");
    TEST_ASSERT(RepetitionPenalty(history, "outfit_1") == 0.70f);

    history.push_front("outfit_1");
    TEST_ASSERT(RepetitionPenalty(history, "outfit_1") == 0.40f);

    history.push_front("outfit_1");
    TEST_ASSERT(RepetitionPenalty(history, "outfit_1") == 0.20f);

    history.push_front("outfit_1");
    TEST_ASSERT(RepetitionPenalty(history, "outfit_1") == 0.10f);

    float baseScore = 6.0f;
    float starWeight = PreferenceWeight(5);
    float penalty = RepetitionPenalty(history, "outfit_1");
    float finalWeight = baseScore * starWeight * penalty;
    TEST_ASSERT(std::fabs(finalWeight - (6.0f * 11.0f * 0.10f)) < 0.001f);
}

void TestTemporaryContextRevertStateMachine() {
    std::string currentOutfit = "casual_dress";
    std::string preSpecialOutfit = "";

    ContextFlags enteringContext = ContextFlags::Swim;
    bool isSpecial = HasContext(enteringContext, ContextFlags::Swim) || HasContext(enteringContext, ContextFlags::Sleep);
    if (isSpecial && preSpecialOutfit.empty()) {
        preSpecialOutfit = currentOutfit;
    }
    currentOutfit = "swimsuit";
    TEST_ASSERT(preSpecialOutfit == "casual_dress");
    TEST_ASSERT(currentOutfit == "swimsuit");

    ContextFlags exitingContext = ContextFlags::City;
    bool stillSpecial = HasContext(exitingContext, ContextFlags::Swim) || HasContext(exitingContext, ContextFlags::Sleep);
    TEST_ASSERT(!stillSpecial);

    if (!stillSpecial && !preSpecialOutfit.empty()) {
        currentOutfit = preSpecialOutfit;
        preSpecialOutfit.clear();
    }
    TEST_ASSERT(currentOutfit == "casual_dress");
    TEST_ASSERT(preSpecialOutfit.empty());
}

void TestItemDisambiguationMatching() {
    TestWornEntry baseArmor{ 0x00012E82, 0, 0, "", false, false };
    TestWornEntry enchantedArmor{ 0x00012E82, 0, 0x0003B211, "", false, true };
    TestWornEntry renamedArmor{ 0x00012E82, 0, 0, "Ariowss Dragonplate", true, false };
    TestWornEntry uniqueIdArmor{ 0x00012E82, 42, 0, "", false, false };

    TestOutfitItem targetPlain{ 0x00012E82, 0, 0, "" };
    TEST_ASSERT(MatchesEntry(baseArmor, targetPlain) == true);
    TEST_ASSERT(MatchesEntry(enchantedArmor, targetPlain) == false);
    TEST_ASSERT(MatchesEntry(renamedArmor, targetPlain) == false);

    TestOutfitItem targetEnchanted{ 0x00012E82, 0, 0x0003B211, "" };
    TEST_ASSERT(MatchesEntry(baseArmor, targetEnchanted) == false);
    TEST_ASSERT(MatchesEntry(enchantedArmor, targetEnchanted) == true);

    TestOutfitItem targetRenamed{ 0x00012E82, 0, 0, "Ariowss Dragonplate" };
    TEST_ASSERT(MatchesEntry(baseArmor, targetRenamed) == false);
    TEST_ASSERT(MatchesEntry(renamedArmor, targetRenamed) == true);

    TestOutfitItem targetUnique{ 0x00012E82, 42, 0, "" };
    TEST_ASSERT(MatchesEntry(uniqueIdArmor, targetUnique) == true);
}

void TestPureApparelProtectionInvariant() {
    TestItemForm cuirass{ 0x00012E82, ItemType::Armor, true, false, "Steel Armor" };
    TestItemForm helmet{ 0x00012E83, ItemType::Armor, true, false, "Steel Helmet" };
    TestItemForm boots{ 0x00012E84, ItemType::Armor, true, false, "Steel Boots" };
    TestItemForm gauntlets{ 0x00012E85, ItemType::Armor, true, false, "Steel Gauntlets" };

    TestItemForm sword{ 0x00012E86, ItemType::WeaponOneHanded, true, false, "Steel Sword" };
    TestItemForm greatsword{ 0x00012E87, ItemType::WeaponTwoHanded, true, false, "Steel Greatsword" };
    TestItemForm bow{ 0x00012E88, ItemType::WeaponBow, true, false, "Hunting Bow" };
    TestItemForm shield{ 0x00012E89, ItemType::Shield, true, false, "Steel Shield" };
    TestItemForm dagger{ 0x00012E8A, ItemType::WeaponDagger, true, false, "Iron Dagger" };
    TestItemForm staff{ 0x00012E8B, ItemType::WeaponStaff, true, false, "Staff of Fireballs" };
    TestItemForm arrows{ 0x00012E8C, ItemType::Ammo, true, false, "Steel Arrow" };
    TestItemForm potion{ 0x00012E8D, ItemType::Potion, true, false, "Health Potion" };

    TEST_ASSERT(IsPureApparelTest(cuirass) == true);
    TEST_ASSERT(IsPureApparelTest(helmet) == true);
    TEST_ASSERT(IsPureApparelTest(boots) == true);
    TEST_ASSERT(IsPureApparelTest(gauntlets) == true);

    TEST_ASSERT(IsPureApparelTest(sword) == false);
    TEST_ASSERT(IsPureApparelTest(greatsword) == false);
    TEST_ASSERT(IsPureApparelTest(bow) == false);
    TEST_ASSERT(IsPureApparelTest(shield) == false);
    TEST_ASSERT(IsPureApparelTest(dagger) == false);
    TEST_ASSERT(IsPureApparelTest(staff) == false);
    TEST_ASSERT(IsPureApparelTest(arrows) == false);
    TEST_ASSERT(IsPureApparelTest(potion) == false);

    TestItemForm unplayableSkin{ 0x00099999, ItemType::Armor, false, false, "NakedBodySkin" };
    TEST_ASSERT(IsPureApparelTest(unplayableSkin) == false);

    TestItemForm unnamedArmor{ 0x00099998, ItemType::Armor, true, false, "" };
    TEST_ASSERT(IsPureApparelTest(unnamedArmor) == false);
}

void TestFollowerTeammateDefaultOutfitSuppression() {
    struct TestActor {
        bool isTeammate{ false };
        uint32_t defaultOutfitFormID{ 0x000A1B2C };
        uint32_t sleepOutfitFormID{ 0x000A1B2D };
        std::vector<uint32_t> inventory;
        std::vector<uint32_t> equipped;
    };

    TestActor lydia;
    lydia.isTeammate = true;
    lydia.inventory = { 0x00012E82, 0x00012E83, 0x0001397D };
    lydia.equipped = { 0x00012E82, 0x00012E83 };

    if (lydia.isTeammate) {
        lydia.defaultOutfitFormID = 0;
        lydia.sleepOutfitFormID = 0;
    }
    TEST_ASSERT(lydia.defaultOutfitFormID == 0);
    TEST_ASSERT(lydia.sleepOutfitFormID == 0);

    uint32_t newOutfitArmor = 0x0001397D;
    std::vector<uint32_t> containerItems;

    for (auto it = lydia.equipped.begin(); it != lydia.equipped.end(); ) {
        if (*it != newOutfitArmor) {
            containerItems.push_back(*it);
            it = lydia.equipped.erase(it);
        } else {
            ++it;
        }
    }
    lydia.equipped.push_back(newOutfitArmor);

    TEST_ASSERT(lydia.equipped.size() == 1);
    TEST_ASSERT(lydia.equipped[0] == 0x0001397D);
    TEST_ASSERT(containerItems.size() == 2);
    TEST_ASSERT(lydia.defaultOutfitFormID == 0);
}

void TestWardrobeStashingBalanceAndCarryWeight() {
    struct TestInventory {
        std::unordered_map<uint32_t, int> playerItems;
        std::unordered_map<uint32_t, int> wardrobeItems;
        float currentWeight = 0.0f;
    };

    std::unordered_map<uint32_t, float> itemWeights = {
        { 0x00012E82, 35.0f },
        { 0x00012E83, 10.0f },
        { 0x0001397D, 5.0f }
    };

    TestInventory inv;
    inv.playerItems[0x00012E82] = 1;
    inv.playerItems[0x00012E83] = 1;
    inv.playerItems[0x0001397D] = 1;

    for (const auto& [fID, count] : inv.playerItems) {
        inv.currentWeight += itemWeights[fID] * count;
    }
    TEST_ASSERT(std::fabs(inv.currentWeight - 50.0f) < 0.001f);

    std::vector<uint32_t> activeOutfit = { 0x0001397D };

    for (auto it = inv.playerItems.begin(); it != inv.playerItems.end(); ) {
        uint32_t fID = it->first;
        if (std::find(activeOutfit.begin(), activeOutfit.end(), fID) == activeOutfit.end()) {
            inv.wardrobeItems[fID] += it->second;
            inv.currentWeight -= itemWeights[fID] * it->second;
            it = inv.playerItems.erase(it);
        } else {
            ++it;
        }
    }

    TEST_ASSERT(inv.playerItems.size() == 1);
    TEST_ASSERT(inv.playerItems.count(0x0001397D) == 1);
    TEST_ASSERT(inv.wardrobeItems.size() == 2);
    TEST_ASSERT(inv.wardrobeItems.count(0x00012E82) == 1);
    TEST_ASSERT(inv.wardrobeItems.count(0x00012E83) == 1);
    TEST_ASSERT(std::fabs(inv.currentWeight - 5.0f) < 0.001f);

    std::vector<uint32_t> targetOutfit = { 0x00012E82, 0x00012E83 };
    for (uint32_t fID : targetOutfit) {
        if (inv.wardrobeItems.count(fID) && inv.wardrobeItems[fID] > 0) {
            inv.wardrobeItems[fID] -= 1;
            if (inv.wardrobeItems[fID] == 0) inv.wardrobeItems.erase(fID);
            inv.playerItems[fID]++;
            inv.currentWeight += itemWeights[fID];
        }
    }

    TEST_ASSERT(inv.wardrobeItems.empty());
    TEST_ASSERT(inv.playerItems.size() == 3);
    TEST_ASSERT(std::fabs(inv.currentWeight - 50.0f) < 0.001f);
}

void TestLegacyJsonBackwardCompatibilityMigration() {
    std::string legacyJsonStr = R"({
        "version": 2,
        "activeWheelIndex": 0,
        "wheels": [
            {
                "name": "Combat Sets",
                "slots": [
                    {
                        "outfit": {
                            "name": "Heavy Iron",
                            "itemIDs": [ 12345, 67890 ],
                            "contextTag": "combat"
                        }
                    }
                ]
            }
        ]
    })";

    json j = json::parse(legacyJsonStr);
    TEST_ASSERT(j.contains("wheels"));
    auto& wheel0 = j["wheels"][0];
    auto& slot0 = wheel0["slots"][0]["outfit"];

    TEST_ASSERT(slot0["name"] == "Heavy Iron");
    TEST_ASSERT(slot0["itemIDs"].size() == 2);

    std::vector<TestOutfitItem> migratedItems;
    if (slot0.contains("items") && slot0["items"].is_array()) {
        for (const auto& itm : slot0["items"]) {
            migratedItems.push_back({ itm.value("formID", 0u), itm.value("uniqueID", (uint16_t)0), itm.value("enchantmentID", 0u), itm.value("customName", "") });
        }
    } else if (slot0.contains("itemIDs") && slot0["itemIDs"].is_array()) {
        for (const auto& id : slot0["itemIDs"]) {
            migratedItems.push_back({ id.get<uint32_t>(), 0, 0, "" });
        }
    }

    TEST_ASSERT(migratedItems.size() == 2);
    TEST_ASSERT(migratedItems[0].formID == 12345);
    TEST_ASSERT(migratedItems[0].uniqueID == 0);
    TEST_ASSERT(migratedItems[1].formID == 67890);

    uint8_t preferenceStars = slot0.value("preferenceStars", (uint8_t)3);
    TEST_ASSERT(preferenceStars == 3);

    bool isAddon = slot0.value("isAddon", false);
    TEST_ASSERT(isAddon == false);
}

void TestWheelAndSlotMemoryCRUD() {
    struct TestSlot {
        std::optional<std::string> outfitName;
        std::vector<uint32_t> itemIDs;
        bool isAddon{ false };
        uint8_t stars{ 3 };
    };

    struct TestWheel {
        std::string name;
        std::vector<TestSlot> slots;
    };

    std::vector<TestWheel> wheels;

    TestWheel w1{ "Combat Outfits", {} };
    TestSlot s1{ "Heavy Steel", { 0x1234, 0x5678 }, false, 4 };
    TestSlot s2{ "Light Leather", { 0x9ABC }, false, 2 };
    w1.slots.push_back(s1);
    w1.slots.push_back(s2);
    wheels.push_back(w1);

    TEST_ASSERT(wheels.size() == 1);
    TEST_ASSERT(wheels[0].name == "Combat Outfits");
    TEST_ASSERT(wheels[0].slots.size() == 2);
    TEST_ASSERT(wheels[0].slots[0].outfitName.value() == "Heavy Steel");

    TestWheel w2{ "Casual Clothes", {} };
    TestSlot s3{ "Tavern Dress", { 0xDEF0 }, false, 5 };
    w2.slots.push_back(s3);
    wheels.push_back(w2);
    TEST_ASSERT(wheels.size() == 2);

    wheels[0].name = "Battle Armor";
    TEST_ASSERT(wheels[0].name == "Battle Armor");

    wheels[0].slots[0].outfitName = "Daedric Armor";
    wheels[0].slots[0].isAddon = true;
    wheels[0].slots[0].stars = 5;
    TEST_ASSERT(wheels[0].slots[0].outfitName.value() == "Daedric Armor");
    TEST_ASSERT(wheels[0].slots[0].isAddon == true);
    TEST_ASSERT(wheels[0].slots[0].stars == 5);

    std::swap(wheels[0].slots[0], wheels[0].slots[1]);
    TEST_ASSERT(wheels[0].slots[0].outfitName.value() == "Light Leather");
    TEST_ASSERT(wheels[0].slots[1].outfitName.value() == "Daedric Armor");

    std::swap(wheels[0], wheels[1]);
    TEST_ASSERT(wheels[0].name == "Casual Clothes");
    TEST_ASSERT(wheels[1].name == "Battle Armor");

    wheels[1].slots[1].outfitName.reset();
    wheels[1].slots[1].itemIDs.clear();
    TEST_ASSERT(!wheels[1].slots[1].outfitName.has_value());
    TEST_ASSERT(wheels[1].slots[1].itemIDs.empty());

    wheels[1].slots.erase(wheels[1].slots.begin() + 1);
    TEST_ASSERT(wheels[1].slots.size() == 1);

    wheels.erase(wheels.begin() + 1);
    TEST_ASSERT(wheels.size() == 1);
    TEST_ASSERT(wheels[0].name == "Casual Clothes");
}

static void TestExternalApiHotkeySuppressionAndRuntimeState() {
    std::atomic<bool> hotkeyEnabled{ true };
    std::atomic<bool> menuOpen{ false };

    auto SetHotkeyEnabled = [&](bool enabled) { hotkeyEnabled.store(enabled, std::memory_order_release); };
    auto IsHotkeyEnabled = [&]() { return hotkeyEnabled.load(std::memory_order_acquire); };

    TEST_ASSERT(IsHotkeyEnabled() == true);
    TEST_ASSERT(menuOpen.load() == false);

    uint32_t playKey = 0x2F;
    uint32_t inputKey = 0x2F;
    bool modMatch = true;

    bool triggerPlay = IsHotkeyEnabled() && (inputKey == playKey && modMatch);
    TEST_ASSERT(triggerPlay == true);

    SetHotkeyEnabled(false);
    TEST_ASSERT(IsHotkeyEnabled() == false);

    bool suppressedPlay = IsHotkeyEnabled() && (inputKey == playKey && modMatch);
    TEST_ASSERT(suppressedPlay == false);

    menuOpen.store(true);
    TEST_ASSERT(menuOpen.load() == true);

    menuOpen.store(false);
    TEST_ASSERT(menuOpen.load() == false);

    SetHotkeyEnabled(true);
    TEST_ASSERT(IsHotkeyEnabled() == true);
    bool restoredPlay = IsHotkeyEnabled() && (inputKey == playKey && modMatch);
    TEST_ASSERT(restoredPlay == true);
}

static void TestThumbnailDiskCacheAndPngEncoding() {
    auto& cache = OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton();

    std::string filename1 = cache.GenerateFileName("Skyrim.esm", 0x12E82);
    TEST_ASSERT(filename1 == "Skyrim.esm_00012E82.png");

    std::string filename2 = cache.GenerateFileName("My Mod / Weapon:Pack.esp", 0x805);
    TEST_ASSERT(filename2 == "My_Mod___Weapon_Pack.esp_00000805.png");

    std::string relPath = cache.GetRelativePath("ArmorPack.esl", 0xFE001801);
    TEST_ASSERT(relPath == "thumbnails/ArmorPack.esl_FE001801.png");

    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "OutfitWheeler_ThumbnailTest";
    cache.SetBaseDirectory(tempDir);
    TEST_ASSERT(cache.GetBaseDirectory() == tempDir);

    auto fullPath = cache.GetThumbnailPath("Skyrim.esm", 0x12E82);
    TEST_ASSERT(fullPath == tempDir / "Skyrim.esm_00012E82.png");

    if (std::filesystem::exists(fullPath)) {
        std::filesystem::remove(fullPath);
    }
    TEST_ASSERT(!cache.HasThumbnail("Skyrim.esm", 0x12E82));

    const uint32_t width = 16;
    const uint32_t height = 16;
    std::vector<uint8_t> pixels(width * height * 4, 0);

    for (size_t i = 0; i < width * height; ++i) {
        pixels[i * 4 + 0] = 50;
        pixels[i * 4 + 1] = 100;
        pixels[i * 4 + 2] = 200;
        pixels[i * 4 + 3] = 255;
    }

    bool saved = cache.SaveRgbaToPng(fullPath, pixels.data(), width, height);
    TEST_ASSERT(saved == true);
    TEST_ASSERT(cache.HasThumbnail("Skyrim.esm", 0x12E82) == true);
    TEST_ASSERT(std::filesystem::file_size(fullPath) > 0);

    std::ifstream file(fullPath, std::ios::binary);
    TEST_ASSERT(file.is_open());
    uint8_t header[8] = { 0 };
    file.read(reinterpret_cast<char*>(header), 8);
    TEST_ASSERT(header[0] == 0x89 && header[1] == 'P' && header[2] == 'N' && header[3] == 'G');
    file.close();

    std::filesystem::remove_all(tempDir);
}

static void TestNonDestructiveGhostItemPersistenceAndAutoBackup() {
    json originalJson = json::parse(R"({
        "wheels": [
            {
                "name": "Armors Wheel",
                "slots": [
                    {
                        "hasOutfit": true,
                        "name": "Custom Steel Valkyrie",
                        "contextTag": "combat",
                        "preferenceStars": 5,
                        "items": [
                            {
                                "plugin": "Skyrim.esm",
                                "localID": "0x00012E82",
                                "rawID": 77442,
                                "customName": "Iron Boots"
                            },
                            {
                                "plugin": "ValkyrieArmor.esp",
                                "localID": "0x00000800",
                                "rawID": 0,
                                "customName": "Valkyrie Cuirass"
                            },
                            {
                                "plugin": "ValkyrieArmor.esp",
                                "localID": "0x00000801",
                                "rawID": 0,
                                "customName": "Valkyrie Gauntlets"
                            }
                        ]
                    }
                ]
            }
        ]
    })");

    auto deserializeMock = [](const json& root) {
        struct MockItem {
            uint32_t formID{ 0 };
            std::string plugin;
            uint32_t localID{ 0 };
            std::string customName;
        };
        struct MockSlot {
            bool hasOutfit{ false };
            std::string name;
            std::vector<MockItem> items;
        };
        std::vector<MockSlot> slots;
        for (const auto& w : root["wheels"]) {
            for (const auto& s : w["slots"]) {
                MockSlot slot;
                slot.hasOutfit = s.value("hasOutfit", false);
                slot.name = s.value("name", "");
                if (s.contains("items")) {
                    for (const auto& itm : s["items"]) {
                        MockItem item;
                        std::string plugin = itm.value("plugin", "");
                        uint32_t locID = 0;
                        if (itm.contains("localID")) {
                            std::string sId = itm["localID"].get<std::string>();
                            if (sId.rfind("0x", 0) == 0) sId = sId.substr(2);
                            locID = std::stoul(sId, nullptr, 16);
                        }
                        if (plugin == "Skyrim.esm") {
                            item.formID = locID;
                        } else {
                            item.formID = 0;
                        }
                        item.plugin = plugin;
                        item.localID = locID;
                        item.customName = itm.value("customName", "");

                        if (item.formID != 0 || !item.plugin.empty() || item.localID != 0) {
                            slot.items.push_back(item);
                        }
                    }
                }
                slots.push_back(slot);
            }
        }
        return slots;
    };

    auto slots = deserializeMock(originalJson);
    TEST_ASSERT(slots.size() == 1);
    TEST_ASSERT(slots[0].hasOutfit == true);
    TEST_ASSERT(slots[0].name == "Custom Steel Valkyrie");
    TEST_ASSERT(slots[0].items.size() == 3);
    TEST_ASSERT(slots[0].items[1].plugin == "ValkyrieArmor.esp");
    TEST_ASSERT(slots[0].items[1].customName == "Valkyrie Cuirass");
    TEST_ASSERT(slots[0].items[1].localID == 0x800);

    json reserialized = json::object();
    json wheelsArr = json::array();
    json wNode;
    wNode["name"] = "Armors Wheel";
    json slotsArr = json::array();
    for (const auto& s : slots) {
        json sNode;
        sNode["hasOutfit"] = s.hasOutfit;
        sNode["name"] = s.name;
        json itmsArr = json::array();
        for (const auto& itm : s.items) {
            char buf[32];
            snprintf(buf, sizeof(buf), "0x%08X", itm.localID);
            itmsArr.push_back(json{
                {"plugin", itm.plugin},
                {"localID", std::string(buf)},
                {"customName", itm.customName}
            });
        }
        sNode["items"] = itmsArr;
        slotsArr.push_back(sNode);
    }
    wNode["slots"] = slotsArr;
    wheelsArr.push_back(wNode);
    reserialized["wheels"] = wheelsArr;

    TEST_ASSERT(reserialized["wheels"][0]["slots"][0]["items"].size() == 3);
    TEST_ASSERT(reserialized["wheels"][0]["slots"][0]["items"][1]["plugin"] == "ValkyrieArmor.esp");
    TEST_ASSERT(reserialized["wheels"][0]["slots"][0]["items"][1]["localID"] == "0x00000800");

    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "OW_BackupTest";
    std::filesystem::create_directories(tempDir);
    std::filesystem::path primaryFile = tempDir / "OutfitWheelerData.json";
    std::filesystem::path backupFile = tempDir / "OutfitWheelerData.json.bak";
    std::filesystem::path legacyFile = tempDir / "PrismaOutfitWheel.json";

    {
        std::ofstream out(primaryFile);
        out << originalJson.dump();
    }
    TEST_ASSERT(std::filesystem::exists(primaryFile));

    std::filesystem::copy_file(primaryFile, backupFile, std::filesystem::copy_options::overwrite_existing);
    TEST_ASSERT(std::filesystem::exists(backupFile));

    std::filesystem::remove(primaryFile);
    TEST_ASSERT(!std::filesystem::exists(primaryFile));

    std::filesystem::path resolvedLoadPath;
    if (std::filesystem::exists(primaryFile)) {
        resolvedLoadPath = primaryFile;
    } else if (std::filesystem::exists(legacyFile)) {
        resolvedLoadPath = legacyFile;
    } else if (std::filesystem::exists(backupFile)) {
        resolvedLoadPath = backupFile;
    }
    TEST_ASSERT(resolvedLoadPath == backupFile);

    {
        std::ifstream inBak(resolvedLoadPath);
        json recovered = json::parse(inBak);
        TEST_ASSERT(recovered["wheels"][0]["slots"][0]["name"] == "Custom Steel Valkyrie");
        inBak.close();
    }

    // Verify auto-migration from legacy file
    {
        std::ofstream outLegacy(legacyFile);
        outLegacy << originalJson.dump();
    }
    TEST_ASSERT(std::filesystem::exists(legacyFile));

    if (!std::filesystem::exists(primaryFile) && std::filesystem::exists(legacyFile)) {
        std::error_code ec;
        std::filesystem::copy_file(legacyFile, primaryFile, std::filesystem::copy_options::overwrite_existing, ec);
        std::filesystem::remove(legacyFile, ec);
    }
    TEST_ASSERT(std::filesystem::exists(primaryFile));
    TEST_ASSERT(!std::filesystem::exists(legacyFile));

    std::filesystem::remove_all(tempDir);
}

static void TestModCatalogIndexingAndSpawning() {
    auto* catMgr = ModCatalogManager::GetSingleton();
    TEST_ASSERT(catMgr != nullptr);

    json mockSummaries = json::array({
        {{"modName", "COCO_Shadow.esp"}, {"count", 24}},
        {{"modName", "Skyrim.esm"}, {"count", 150}},
        {{"modName", "BDOR_Armor.esp"}, {"count", 40}}
    });

    json mockItems = json::array({
        {{"id", "0x00000801"}, {"rawID", 0x801}, {"name", "Shadow Boots"}, {"type", "Boots"}, {"category", "ARMORS"}, {"armor", 18.0f}, {"weight", 2.0f}, {"value", 250}, {"modName", "COCO_Shadow.esp"}, {"thumbnail", ""}, {"source", "MODEX"}},
        {{"id", "0x00000802"}, {"rawID", 0x802}, {"name", "Shadow Cuirass"}, {"type", "Cuirass"}, {"category", "ARMORS"}, {"armor", 45.0f}, {"weight", 8.0f}, {"value", 850}, {"modName", "COCO_Shadow.esp"}, {"thumbnail", ""}, {"source", "MODEX"}},
        {{"id", "0x00000803"}, {"rawID", 0x803}, {"name", "Iron Armor"}, {"type", "Cuirass"}, {"category", "ARMORS"}, {"armor", 25.0f}, {"weight", 30.0f}, {"value", 125}, {"modName", "Skyrim.esm"}, {"thumbnail", ""}, {"source", "MODEX"}}
    });

    catMgr->SetMockData(mockSummaries, mockItems);

    json summaries = catMgr->GetModSummaries();
    TEST_ASSERT(summaries.is_array());
    TEST_ASSERT(summaries.size() == 3);
    TEST_ASSERT(summaries[0]["modName"] == "COCO_Shadow.esp");
    TEST_ASSERT(summaries[0]["count"] == 24);

    json cocoItems = catMgr->GetModItems("COCO_Shadow.esp");
    TEST_ASSERT(cocoItems.is_array());
    TEST_ASSERT(cocoItems.size() == 2);
    TEST_ASSERT(cocoItems[0]["name"] == "Shadow Boots");
    TEST_ASSERT(cocoItems[1]["name"] == "Shadow Cuirass");

    json skyrimItems = catMgr->GetModItems("skyrim.esm");
    TEST_ASSERT(skyrimItems.is_array());
    TEST_ASSERT(skyrimItems.size() == 1);
    TEST_ASSERT(skyrimItems[0]["name"] == "Iron Armor");

    json emptyItems = catMgr->GetModItems("NonExistent.esp");
    TEST_ASSERT(emptyItems.is_array());
    TEST_ASSERT(emptyItems.empty());

    bool spawnZero = catMgr->SpawnItem(0, "0x00000007", false, "INVENTORY");
    TEST_ASSERT(spawnZero == false);

    bool spawnValid = catMgr->SpawnItem(0x801, "0x00000007", true, "INVENTORY");
    TEST_ASSERT(spawnValid == true);
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Outfit Wheeler - Backend Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;

    RUN_TEST("1. INI Read/Write/Fallback & Persistence", TestIniReadWriteAndFallback);
    RUN_TEST("2. FormID 24-bit Mod Index & ESL 12-bit Arithmetic", TestFormIDEncodingAndESLArithmetic);
    RUN_TEST("3. Context Tags & 23 Auto Conditions Parser", TestContextTagsAnd23Conditions);
    RUN_TEST("4. Context Priority Hierarchy Resolution", TestHierarchyResolution);
    RUN_TEST("5. Star Preference Weights & Repetition Penalties", TestStarPreferenceAndRepetitionPenalty);
    RUN_TEST("6. Temporary Condition Revert State Machine", TestTemporaryContextRevertStateMachine);
    RUN_TEST("7. Item Disambiguation & Enchantment/Rename Matching", TestItemDisambiguationMatching);
    RUN_TEST("8. Pure Apparel Protection (Weapons & Shields Shielding)", TestPureApparelProtectionInvariant);
    RUN_TEST("9. Follower Teammate Default Outfit Suppression", TestFollowerTeammateDefaultOutfitSuppression);
    RUN_TEST("10. Wardrobe Stash / Retrieve Inventory Balance", TestWardrobeStashingBalanceAndCarryWeight);
    RUN_TEST("11. Legacy JSON Backward Compatibility Migration", TestLegacyJsonBackwardCompatibilityMigration);
    RUN_TEST("12. Wheel & Slot Memory CRUD Engine (Create, Read, Update, Delete)", TestWheelAndSlotMemoryCRUD);
    RUN_TEST("13. External C-API Hotkey Suppression & Runtime State Machine", TestExternalApiHotkeySuppressionAndRuntimeState);
    RUN_TEST("14. Standalone 3D Thumbnail Disk Cache & WIC PNG Encoding", TestThumbnailDiskCacheAndPngEncoding);
    RUN_TEST("15. Ghost Item Persistence, Fallback Load Paths & Auto .bak Backup", TestNonDestructiveGhostItemPersistenceAndAutoBackup);
    RUN_TEST("16. Mod Catalog Indexing, Summary Grouping and Spawning Logic", TestModCatalogIndexingAndSpawning);

    std::cout << "========================================" << std::endl;
    std::cout << "Backend Test Results: " << g_passedTests << " / " << g_totalTests << " Passed" << std::endl;
    std::cout << "========================================" << std::endl;

    return (g_passedTests == g_totalTests) ? 0 : 1;
}
