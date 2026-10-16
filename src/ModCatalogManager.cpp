#include "ModCatalogManager.h"
#include <algorithm>
#include <map>
#include <cctype>

#if __has_include(<RE/Skyrim.h>)
#include <RE/Skyrim.h>
#include "EquipmentManager.h"
#include "thumbnail/ThumbnailDiskCache.h"

static std::string FormatCatalogFormID(uint32_t a_id) {
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%08X", a_id);
    return std::string(buf);
}

static std::string GetApparelTypeFromMask(uint32_t slotMask) {
    using Slot = RE::BGSBipedObjectForm::BipedObjectSlot;
    if (slotMask & (static_cast<uint32_t>(Slot::kHead) | static_cast<uint32_t>(Slot::kHair) | static_cast<uint32_t>(Slot::kCirclet))) {
        return "Helmet";
    }
    if (slotMask & static_cast<uint32_t>(Slot::kAmulet)) return "Amulet";
    if (slotMask & static_cast<uint32_t>(Slot::kRing)) return "Ring";
    if (slotMask & static_cast<uint32_t>(Slot::kHands)) return "Gauntlets";
    if (slotMask & static_cast<uint32_t>(Slot::kFeet)) return "Boots";
    if (slotMask & static_cast<uint32_t>(Slot::kShield)) return "Shield";
    if (slotMask & static_cast<uint32_t>(Slot::kBody)) return "Cuirass";
    return "Armor";
}

static int SafeProbeArmorRecord(RE::TESObjectARMO* a_armor, const char** a_outName, uint32_t* a_outFormID) {
    __try {
        if (!a_armor) return 0;
        if (a_armor->GetFormType() != RE::FormType::Armor) return 0;
        if ((a_armor->formFlags & 0x00000024) != 0) return 0;
        if (!a_armor->GetPlayable()) return 0;

        const char* name = a_armor->GetName();
        if (!name || name[0] == '\0') return 0;

        uint32_t fID = a_armor->GetFormID();
        if (fID == 0) return 0;

        *a_outName = name;
        *a_outFormID = fID;
        return 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

static int SafeGetArmorStats(RE::TESObjectARMO* a_armor, float* a_outRating, float* a_outWeight, int* a_outValue, uint32_t* a_outSlotMask) {
    __try {
        if (!a_armor) return 0;
        *a_outRating = static_cast<float>(a_armor->GetArmorRating());
        *a_outWeight = a_armor->GetWeight();
        *a_outValue = a_armor->GetGoldValue();
        *a_outSlotMask = static_cast<uint32_t>(a_armor->GetSlotMask());
        return 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

static const char* SafeGetArmorFirstFileName(RE::TESObjectARMO* a_armor) {
    __try {
        if (!a_armor) return nullptr;
        if (auto* f = a_armor->GetFile(0)) {
            if (f && f->fileName && f->fileName[0] != '\0') {
                return f->fileName;
            }
        }
        return nullptr;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return nullptr;
    }
}

#endif

ModCatalogManager* ModCatalogManager::GetSingleton() {
    static ModCatalogManager instance;
    return &instance;
}

void ModCatalogManager::SetMockData(const json& summaries, const json& items) {
    std::lock_guard<std::mutex> lock(m_catalogMutex);
    m_mockSummaries = summaries;
    m_mockItems = items;
    m_catalogIndexed = false;
    m_cachedSummaries.clear();
    m_cachedModItems.clear();
    m_modFormIDsMap.clear();
}

void ModCatalogManager::EnsureCatalogIndexed() {
    std::lock_guard<std::mutex> lock(m_catalogMutex);
    if (m_catalogIndexed) return;

#if __has_include(<RE/Skyrim.h>)
    auto* dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) return;

    struct CachedModInfo {
        std::string name;
        std::string lower;
    };

    std::vector<CachedModInfo> regularMods(256);
    std::vector<CachedModInfo> lightMods(4096);

    for (uint32_t i = 0; i < 256; ++i) {
        const auto* file = dataHandler->LookupLoadedModByIndex(static_cast<uint8_t>(i));
        if (file && file->fileName && file->fileName[0] != '\0') {
            regularMods[i].name = file->fileName;
            std::string l = file->fileName;
            std::transform(l.begin(), l.end(), l.begin(), ::tolower);
            regularMods[i].lower = std::move(l);
        }
    }

    for (uint32_t i = 0; i < 4096; ++i) {
        const auto* file = dataHandler->LookupLoadedLightModByIndex(static_cast<uint16_t>(i));
        if (file && file->fileName && file->fileName[0] != '\0') {
            lightMods[i].name = file->fileName;
            std::string l = file->fileName;
            std::transform(l.begin(), l.end(), l.begin(), ::tolower);
            lightMods[i].lower = std::move(l);
        }
    }

    static const CachedModInfo s_defaultMod{ "Skyrim.esm", "skyrim.esm" };

    std::map<std::string, uint32_t> counts;
    m_modFormIDsMap.clear();

    try {
        const auto& armors = dataHandler->GetFormArray<RE::TESObjectARMO>();
        for (auto* armor : armors) {
            const char* name = nullptr;
            uint32_t formID = 0;
            if (SafeProbeArmorRecord(armor, &name, &formID) == 1) {
                const CachedModInfo* pMod = nullptr;
                uint8_t modIndex = static_cast<uint8_t>((formID >> 24) & 0xFF);
                if (modIndex == 0xFE) {
                    uint16_t lightIndex = static_cast<uint16_t>((formID >> 12) & 0x0FFF);
                    if (lightIndex < lightMods.size() && !lightMods[lightIndex].name.empty()) {
                        pMod = &lightMods[lightIndex];
                    }
                } else {
                    if (modIndex < regularMods.size() && !regularMods[modIndex].name.empty()) {
                        pMod = &regularMods[modIndex];
                    }
                }

                if (!pMod && armor) {
                    if (const char* fName = SafeGetArmorFirstFileName(armor)) {
                        std::string modStr(fName);
                        std::string modLower = modStr;
                        std::transform(modLower.begin(), modLower.end(), modLower.begin(), ::tolower);
                        counts[modStr]++;
                        m_modFormIDsMap[modLower].push_back(formID);
                        continue;
                    }
                }

                if (!pMod) pMod = &s_defaultMod;

                counts[pMod->name]++;
                m_modFormIDsMap[pMod->lower].push_back(formID);
            }
        }
    } catch (...) {}

    m_cachedSummaries = json::array();
    for (const auto& [mName, cnt] : counts) {
        if (cnt > 0) {
            m_cachedSummaries.push_back({
                {"modName", mName},
                {"count", cnt}
            });
        }
    }
    m_catalogIndexed = true;
#endif
}

json ModCatalogManager::GetModSummaries() {
    {
        std::lock_guard<std::mutex> lock(m_catalogMutex);
        if (!m_mockSummaries.empty()) {
            return m_mockSummaries;
        }
        if (m_catalogIndexed) {
            return m_cachedSummaries;
        }
    }
    EnsureCatalogIndexed();
    std::lock_guard<std::mutex> lock(m_catalogMutex);
    return m_cachedSummaries;
}

json ModCatalogManager::GetModItems(const std::string& modName) {
    if (modName.empty()) return json::array();

    std::string targetModLower = modName;
    std::transform(targetModLower.begin(), targetModLower.end(), targetModLower.begin(), ::tolower);

    {
        std::lock_guard<std::mutex> lock(m_catalogMutex);
        if (!m_mockItems.empty()) {
            json arr = json::array();
            for (const auto& it : m_mockItems) {
                std::string itMod = it.value("modName", "");
                std::transform(itMod.begin(), itMod.end(), itMod.begin(), ::tolower);
                if (itMod == targetModLower) {
                    arr.push_back(it);
                }
            }
            return arr;
        }

        auto it = m_cachedModItems.find(targetModLower);
        if (it != m_cachedModItems.end()) {
            return it->second;
        }
    }

#if __has_include(<RE/Skyrim.h>)
    EnsureCatalogIndexed();

    std::vector<uint32_t> formIDs;
    {
        std::lock_guard<std::mutex> lock(m_catalogMutex);
        auto it = m_modFormIDsMap.find(targetModLower);
        if (it != m_modFormIDsMap.end()) {
            formIDs = it->second;
        }
    }

    if (formIDs.empty()) return json::array();

    json items = json::array();
    try {
        for (uint32_t formID : formIDs) {
            auto* form = RE::TESForm::LookupByID(formID);
            if (!form) continue;
            auto* armor = form->As<RE::TESObjectARMO>();
            if (!armor) continue;

            const char* name = nullptr;
            uint32_t probedID = 0;
            if (SafeProbeArmorRecord(armor, &name, &probedID) != 1) continue;

            float armRating = 0.0f;
            float weight = 0.0f;
            int goldVal = 0;
            uint32_t slotMask = 0;
            SafeGetArmorStats(armor, &armRating, &weight, &goldVal, &slotMask);

            std::string typeStr = GetApparelTypeFromMask(slotMask);
            std::string catStr = (typeStr == "Ring" || typeStr == "Amulet") ? "JEWELRY" : "ARMORS";

            uint32_t bLoc = armor->GetLocalFormID();
            std::string thumbPath = "";
            if (OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().HasThumbnail(modName, bLoc)) {
                thumbPath = OutfitWheeler::Thumbnail::ThumbnailDiskCache::GetSingleton().GetRelativePath(modName, bLoc);
            }

            items.push_back({
                {"id", FormatCatalogFormID(formID)},
                {"rawID", formID},
                {"name", std::string(name ? name : "Apparel")},
                {"type", typeStr},
                {"category", catStr},
                {"armor", armRating},
                {"weight", weight},
                {"value", goldVal},
                {"modName", modName},
                {"thumbnail", thumbPath},
                {"source", "MODEX"}
            });
        }
    } catch (...) {}

    {
        std::lock_guard<std::mutex> lock(m_catalogMutex);
        m_cachedModItems[targetModLower] = items;
    }
    return items;
#else
    return json::array();
#endif
}

bool ModCatalogManager::SpawnItem(uint32_t rawID, const std::string& targetActorId, bool autoEquip, const std::string& destination) {
    if (rawID == 0) return false;

#if __has_include(<RE/Skyrim.h>)
    auto* form = RE::TESForm::LookupByID(rawID);
    if (!form) return false;

    auto* bound = form->As<RE::TESBoundObject>();
    if (!bound) return false;

    const char* bName = bound->GetName();
    std::string itemName = (bName && bName[0] != '\0') ? std::string(bName) : "Apparel";

    if (destination == "CLOUD") {
        auto* wardrobe = GetWardrobeContainer();
        if (wardrobe) {
            wardrobe->InitInventoryIfRequired();
            wardrobe->AddObjectToContainer(bound, nullptr, 1, nullptr);
            SanitizeWardrobeContainer();
            std::string msg = "Stored in Wardrobe: " + itemName;
            RE::DebugNotification(msg.c_str());
            return true;
        }
        return false;
    }

    RE::Actor* targetActor = nullptr;
    if (!targetActorId.empty() && targetActorId != "0x00000007") {
        try {
            RE::FormID fID = static_cast<RE::FormID>(std::stoul(targetActorId, nullptr, 16));
            auto* aForm = RE::TESForm::LookupByID(fID);
            if (aForm) targetActor = aForm->As<RE::Actor>();
        } catch (...) {}
    }
    if (!targetActor) {
        targetActor = RE::PlayerCharacter::GetSingleton();
    }
    if (!targetActor) return false;

    targetActor->AddObjectToContainer(bound, nullptr, 1, nullptr);
    if (autoEquip) {
        auto* equipMgr = RE::ActorEquipManager::GetSingleton();
        if (equipMgr) {
            equipMgr->EquipObject(targetActor, bound, nullptr, 1, nullptr, false, true, true, true);
        }
    }
    std::string msg = (autoEquip ? "Equipped: " : "Obtained: ") + itemName;
    RE::DebugNotification(msg.c_str());
    return true;
#else
    (void)targetActorId;
    (void)autoEquip;
    (void)destination;
    return true;
#endif
}
