#include "pch.h"
#include "ItemMeshExtractor.h"
#include "RE/B/BSLightingShaderMaterialBase.h"
#include "RE/B/BSTextureSet.h"
#include "RE/B/BSTriShape.h"
#include "RE/B/BSEffectShaderProperty.h"
#include "RE/B/BSEffectShaderMaterial.h"
#include "RE/B/BSDismemberSkinInstance.h"
#include "RE/B/BGSBipedObjectForm.h"
#include "RE/B/BSResourceNiBinaryStream.h"
#include "RE/N/NiSkinPartition.h"
#include "RE/N/NiSkinInstance.h"
#include "RE/N/NiSourceTexture.h"
#include "RE/N/NiAlphaProperty.h"
#include "RE/B/BSLightingShaderMaterialHairTint.h"
#include "RE/B/BSLightingShaderMaterialEnvmap.h"
#include "RE/B/BGSColorForm.h"
#include "RE/P/PlayerCharacter.h"
#include "RE/T/TESNPC.h"
#include "RE/T/TESObjectARMA.h"
#include "RE/T/TESModelTextureSwap.h"
#include <DirectXMath.h>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <cctype>

namespace OutfitWheeler::Thumbnail
{
    std::string ItemMeshExtractor::ResolveModelPath(RE::TESBoundObject* a_item)
    {
        if (!a_item) return "";

        if (auto* armor = a_item->As<RE::TESObjectARMO>()) {
            auto isClutterModel = [](const char* m) {
                if (!m || m[0] == '\0') return true;
                std::string s = m;
                std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                return s.find("clutter\\") != std::string::npos ||
                       s.find("clutter/") != std::string::npos ||
                       s.find("wolfpelt") != std::string::npos ||
                       s.find("foxpelt") != std::string::npos ||
                       s.find("pelts\\") != std::string::npos ||
                       s.find("pelts/") != std::string::npos;
            };

            const char* femaleWorld = armor->worldModels[RE::TESBipedModelForm::Sexes::kFemale].GetModel();
            if (femaleWorld && femaleWorld[0] != '\0' && !isClutterModel(femaleWorld)) {
                return femaleWorld;
            }

            const char* maleWorld = armor->worldModels[RE::TESBipedModelForm::Sexes::kMale].GetModel();
            if (maleWorld && maleWorld[0] != '\0' && !isClutterModel(maleWorld)) {
                return maleWorld;
            }

            for (auto* addon : armor->armorAddons) {
                if (addon) {
                    const char* femaleModel = addon->bipedModels[RE::SEXES::kFemale].GetModel();
                    if (femaleModel && femaleModel[0] != '\0' && !isClutterModel(femaleModel)) return femaleModel;
                    const char* maleModel = addon->bipedModels[RE::SEXES::kMale].GetModel();
                    if (maleModel && maleModel[0] != '\0' && !isClutterModel(maleModel)) return maleModel;
                }
            }

            for (auto* addon : armor->armorAddons) {
                if (addon) {
                    const char* femaleModel = addon->bipedModels[RE::SEXES::kFemale].GetModel();
                    if (femaleModel && femaleModel[0] != '\0') return femaleModel;
                    const char* maleModel = addon->bipedModels[RE::SEXES::kMale].GetModel();
                    if (maleModel && maleModel[0] != '\0') return maleModel;
                }
            }

            if (femaleWorld && femaleWorld[0] != '\0') return femaleWorld;
            if (maleWorld && maleWorld[0] != '\0') return maleWorld;
        }

        if (auto* weapon = a_item->As<RE::TESObjectWEAP>()) {
            const char* weaponModel = weapon->GetModel();
            if (weaponModel && weaponModel[0] != '\0') {
                return weaponModel;
            }
        }

        if (auto* modelObj = a_item->As<RE::TESModel>()) {
            const char* m = modelObj->GetModel();
            if (m && m[0] != '\0') {
                return m;
            }
        }

        return "";
    }

    struct TextureSwapEntry
    {
        uint32_t geomIndex{ 0xFFFFFFFFu };
        std::string name3D;
        std::string diffusePath;
    };

    static bool PathsEqual(const char* a, const char* b)
    {
        if (!a || !b) return false;
        while (*a && *b) {
            char ca = *a++;
            char cb = *b++;
            if (ca == '/') ca = '\\';
            if (cb == '/') cb = '\\';
            if (tolower(static_cast<unsigned char>(ca)) != tolower(static_cast<unsigned char>(cb))) return false;
        }
        return *a == '\0' && *b == '\0';
    }

    static std::string NormalizeTexturePath(const std::string& a_path)
    {
        std::string norm = a_path;
        for (char& c : norm) {
            if (c == '/') c = '\\';
        }
        while (!norm.empty() && (norm.front() == '\\' || norm.front() == ' ' || norm.front() == '/')) {
            norm.erase(0, 1);
        }
        if (_strnicmp(norm.c_str(), "data\\", 5) == 0) {
            norm.erase(0, 5);
        }
        while (!norm.empty() && (norm.front() == '\\' || norm.front() == ' ' || norm.front() == '/')) {
            norm.erase(0, 1);
        }
        if (_strnicmp(norm.c_str(), "textures\\", 9) == 0) {
            norm.erase(0, 9);
        }
        while (!norm.empty() && (norm.front() == '\\' || norm.front() == ' ' || norm.front() == '/')) {
            norm.erase(0, 1);
        }
        return norm;
    }

    static std::string ResolveTexturePath(const std::string& a_rawTex, const std::string& a_modelPath)
    {
        if (a_rawTex.empty()) return "";
        std::string norm = NormalizeTexturePath(a_rawTex);
        if (norm.empty()) return "";

        static std::unordered_map<std::string, std::string> s_resolveCache;
        std::string cacheKey = a_modelPath + "|" + norm;
        auto it = s_resolveCache.find(cacheKey);
        if (it != s_resolveCache.end()) {
            return it->second;
        }

        auto checkStream = [](const std::string& p) -> bool {
            RE::BSResourceNiBinaryStream stream(p.c_str());
            return stream.good() && stream.stream != nullptr;
        };

        if (checkStream("textures\\" + norm)) {
            s_resolveCache[cacheKey] = norm;
            return norm;
        }
        if (checkStream(norm)) {
            s_resolveCache[cacheKey] = norm;
            return norm;
        }

        std::string normModel = a_modelPath;
        for (char& c : normModel) {
            if (c == '/') c = '\\';
        }
        while (!normModel.empty() && (normModel.front() == '\\' || normModel.front() == ' ' || normModel.front() == '/')) {
            normModel.erase(0, 1);
        }
        if (_strnicmp(normModel.c_str(), "data\\", 5) == 0) normModel.erase(0, 5);
        while (!normModel.empty() && (normModel.front() == '\\' || normModel.front() == ' ' || normModel.front() == '/')) {
            normModel.erase(0, 1);
        }
        if (_strnicmp(normModel.c_str(), "meshes\\", 7) == 0) normModel.erase(0, 7);
        while (!normModel.empty() && (normModel.front() == '\\' || normModel.front() == ' ' || normModel.front() == '/')) {
            normModel.erase(0, 1);
        }

        size_t lastSlash = normModel.find_last_of('\\');
        std::string modelDir = (lastSlash != std::string::npos) ? normModel.substr(0, lastSlash) : "";

        std::string currDir = modelDir;
        while (!currDir.empty()) {
            std::string cand1 = "textures\\" + currDir + "\\" + norm;
            if (checkStream(cand1)) {
                std::string res = currDir + "\\" + norm;
                s_resolveCache[cacheKey] = res;
                return res;
            }
            std::string cand2 = currDir + "\\" + norm;
            if (checkStream(cand2)) {
                std::string res = currDir + "\\" + norm;
                s_resolveCache[cacheKey] = res;
                return res;
            }

            size_t slashInNorm = norm.find_first_of('\\');
            std::string firstNormFolder = (slashInNorm != std::string::npos) ? norm.substr(0, slashInNorm) : norm;
            std::string currLower = currDir;
            std::transform(currLower.begin(), currLower.end(), currLower.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            std::string firstLower = firstNormFolder;
            std::transform(firstLower.begin(), firstLower.end(), firstLower.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

            size_t matchPos = currLower.find(firstLower);
            if (matchPos != std::string::npos) {
                std::string prefix = currDir.substr(0, matchPos);
                while (!prefix.empty() && prefix.back() == '\\') prefix.pop_back();
                if (!prefix.empty()) {
                    std::string cand3 = "textures\\" + prefix + "\\" + norm;
                    if (checkStream(cand3)) {
                        std::string res = prefix + "\\" + norm;
                        s_resolveCache[cacheKey] = res;
                        return res;
                    }
                }
            }

            size_t upSlash = currDir.find_last_of('\\');
            if (upSlash != std::string::npos) {
                currDir = currDir.substr(0, upSlash);
            } else {
                break;
            }
        }

        s_resolveCache[cacheKey] = norm;
        return norm;
    }

    static ID3D11ShaderResourceView* TextureSrv(RE::NiTexture* a_texture)
    {
        auto* src = netimmerse_cast<RE::NiSourceTexture*>(a_texture);
        if (!src || !src->rendererTexture) {
            return nullptr;
        }
        auto* rd = reinterpret_cast<RE::NiTexture::RendererData*>(src->rendererTexture);
        return rd ? reinterpret_cast<ID3D11ShaderResourceView*>(rd->resourceView) : nullptr;
    }

    static DirectX::XMFLOAT4 GetPlayerHairColor()
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (player) {
            auto* base = player->GetActorBase();
            if (base) {
                auto* headData = base->headRelatedData;
                if (headData && headData->hairColor) {
                    float r = (std::max)(static_cast<float>(headData->hairColor->color.red) / 255.0f, 0.12f);
                    float g = (std::max)(static_cast<float>(headData->hairColor->color.green) / 255.0f, 0.12f);
                    float b = (std::max)(static_cast<float>(headData->hairColor->color.blue) / 255.0f, 0.12f);
                    return DirectX::XMFLOAT4(r, g, b, 1.0f);
                }
            }
        }
        return DirectX::XMFLOAT4(0.35f, 0.22f, 0.14f, 1.0f);
    }

    static std::vector<TextureSwapEntry> CollectTextureSwaps(RE::TESBoundObject* a_item, const std::string& a_modelPath)
    {
        std::vector<TextureSwapEntry> swaps;
        if (!a_item) return swaps;

        auto collectFromSwapModel = [&](const RE::TESModelTextureSwap& modelSwap) {
            if (!modelSwap.alternateTextures || modelSwap.numAlternateTextures == 0) return;
            for (uint32_t i = 0; i < modelSwap.numAlternateTextures; ++i) {
                const auto& alt = modelSwap.alternateTextures[i];
                if (!alt.textureSet) continue;
                const char* diff = alt.textureSet->GetTexturePath(RE::BSTextureSet::Texture::kDiffuse);
                if (!diff || diff[0] == '\0') continue;
                TextureSwapEntry e;
                e.geomIndex = alt.index3D;
                if (!alt.name3D.empty()) {
                    e.name3D = alt.name3D.c_str();
                }
                e.diffusePath = ResolveTexturePath(diff, a_modelPath);
                swaps.push_back(std::move(e));
            }
        };

        if (auto* armor = a_item->As<RE::TESObjectARMO>()) {
            for (auto* addon : armor->armorAddons) {
                if (!addon) continue;
                bool matchedModel = false;
                for (int sex = 0; sex < RE::SEXES::kTotal; ++sex) {
                    const char* m = addon->bipedModels[sex].GetModel();
                    if (m && PathsEqual(m, a_modelPath.c_str())) {
                        collectFromSwapModel(addon->bipedModels[sex]);
                        matchedModel = true;
                    }
                }
                if (!matchedModel && armor->armorAddons.size() == 1) {
                    for (int sex = 0; sex < RE::SEXES::kTotal; ++sex) {
                        collectFromSwapModel(addon->bipedModels[sex]);
                    }
                }
            }
            for (int sex = 0; sex < RE::TESBipedModelForm::Sexes::kTotal; ++sex) {
                const char* m = armor->worldModels[sex].GetModel();
                if (m && PathsEqual(m, a_modelPath.c_str())) {
                    collectFromSwapModel(armor->worldModels[sex]);
                }
            }
        }

        if (auto* weapon = a_item->As<RE::TESObjectWEAP>()) {
            if (auto* modelSwap = weapon->As<RE::TESModelTextureSwap>()) {
                collectFromSwapModel(*modelSwap);
            }
        }

        if (auto* modelSwap = a_item->As<RE::TESModelTextureSwap>()) {
            collectFromSwapModel(*modelSwap);
        }

        return swaps;
    }

    struct GeometryEntry
    {
        RE::BSGeometry* geom{ nullptr };
        RE::NiTransform transform;
        uint32_t depthIndex{ 0 };
    };

    static void WalkGeometryNodes(RE::NiAVObject* a_node, const RE::NiTransform& a_parentXf, std::vector<GeometryEntry>& a_outGeoms, uint32_t& a_runningIdx)
    {
        if (!a_node) return;
        RE::NiTransform currentXf = a_parentXf * a_node->local;
        if (auto* geom = a_node->AsGeometry()) {
            GeometryEntry entry;
            entry.geom = geom;
            entry.transform = currentXf;
            entry.depthIndex = a_runningIdx++;
            a_outGeoms.push_back(entry);
            return;
        }
        if (auto* parentNode = a_node->AsNode()) {
            for (auto& child : parentNode->GetChildren()) {
                if (child) {
                    WalkGeometryNodes(child.get(), currentXf, a_outGeoms, a_runningIdx);
                }
            }
        }
    }

    static float HalfToFloat(uint16_t h)
    {
        uint32_t sign = (h >> 15) & 0x0001;
        uint32_t exp = (h >> 10) & 0x001f;
        uint32_t mant = h & 0x03ff;
        if (exp == 0) {
            if (mant == 0) return sign ? -0.0f : 0.0f;
            while (!(mant & 0x0400)) { mant <<= 1; exp--; }
            exp++; mant &= ~0x0400;
        } else if (exp == 31) {
            return (mant == 0) ? (sign ? -INFINITY : INFINITY) : NAN;
        }
        exp = exp + (127 - 15);
        mant = mant << 13;
        uint32_t val = (sign << 31) | (exp << 23) | mant;
        float f = 0.0f;
        std::memcpy(&f, &val, sizeof(f));
        return f;
    }

    static bool AttributePresent(RE::BSGraphics::VertexDesc a_desc, RE::BSGraphics::Vertex::Attribute a_attr)
    {
        using V = RE::BSGraphics::Vertex;
        switch (a_attr) {
            case V::VA_POSITION:  return a_desc.HasFlag(V::VF_VERTEX);
            case V::VA_TEXCOORD0: return a_desc.HasFlag(V::VF_UV);
            case V::VA_TEXCOORD1: return a_desc.HasFlag(V::VF_UV_2);
            case V::VA_NORMAL:    return a_desc.HasFlag(V::VF_NORMAL);
            case V::VA_BINORMAL:  return a_desc.HasFlag(V::VF_NORMAL) && a_desc.HasFlag(V::VF_TANGENT);
            case V::VA_COLOR:     return a_desc.HasFlag(V::VF_COLORS);
            case V::VA_SKINNING:  return a_desc.HasFlag(V::VF_SKINNED);
            case V::VA_LANDDATA:  return a_desc.HasFlag(V::VF_LANDDATA);
            case V::VA_EYEDATA:   return a_desc.HasFlag(V::VF_EYEDATA);
            default:              return false;
        }
    }

    static bool PositionIsFullFloat(RE::BSGraphics::VertexDesc a_desc, uint32_t a_posOffset)
    {
        const uint32_t stride = a_desc.GetSize();
        uint32_t next = stride;
        const RE::BSGraphics::Vertex::Attribute others[] = {
            RE::BSGraphics::Vertex::VA_TEXCOORD0,
            RE::BSGraphics::Vertex::VA_TEXCOORD1,
            RE::BSGraphics::Vertex::VA_NORMAL,
            RE::BSGraphics::Vertex::VA_BINORMAL,
            RE::BSGraphics::Vertex::VA_COLOR,
            RE::BSGraphics::Vertex::VA_SKINNING,
            RE::BSGraphics::Vertex::VA_LANDDATA,
            RE::BSGraphics::Vertex::VA_EYEDATA
        };
        for (const auto attr : others) {
            if (AttributePresent(a_desc, attr)) {
                const auto off = a_desc.GetAttributeOffset(attr);
                if (off > a_posOffset && off < next) {
                    next = off;
                }
            }
        }
        return (next - a_posOffset) >= 12;
    }

    static DirectX::XMFLOAT3 TransformNormal(const RE::NiTransform& a_xf, float a_x, float a_y, float a_z)
    {
        const auto& r = a_xf.rotate.entry;
        DirectX::XMFLOAT3 n{
            r[0][0] * a_x + r[0][1] * a_y + r[0][2] * a_z,
            r[1][0] * a_x + r[1][1] * a_y + r[1][2] * a_z,
            r[2][0] * a_x + r[2][1] * a_y + r[2][2] * a_z
        };
        float len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        if (len > 1.0e-6f) {
            n.x /= len; n.y /= len; n.z /= len;
        } else {
            n.z = 1.0f;
        }
        return n;
    }

    static DirectX::XMFLOAT3 TransformPoint(const RE::NiTransform& a_xf, float a_x, float a_y, float a_z)
    {
        const auto& r = a_xf.rotate.entry;
        float sx = a_xf.scale * a_x;
        float sy = a_xf.scale * a_y;
        float sz = a_xf.scale * a_z;
        return DirectX::XMFLOAT3{
            r[0][0] * sx + r[0][1] * sy + r[0][2] * sz + a_xf.translate.x,
            r[1][0] * sx + r[1][1] * sy + r[1][2] * sz + a_xf.translate.y,
            r[2][0] * sx + r[2][1] * sy + r[2][2] * sz + a_xf.translate.z
        };
    }

    static bool IsAbsoluteJunkString(const std::string& a_rawName)
    {
        if (a_rawName.empty()) return false;
        std::string name = a_rawName;
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (name.find("ground") != std::string::npos ||
            name.find("floor") != std::string::npos ||
            name.find("plane") != std::string::npos ||
            name.find("pedestal") != std::string::npos ||
            name.find("stage") != std::string::npos ||
            name.find("stand") != std::string::npos) return true;

        if (name.find("collision") != std::string::npos ||
            name.find("cphy") != std::string::npos ||
            name.find("cbox") != std::string::npos ||
            name.find("cage") != std::string::npos ||
            name.find("hitbox") != std::string::npos ||
            name.find("proxy") != std::string::npos ||
            name.find("dummy") != std::string::npos ||
            name.find("bhk") != std::string::npos ||
            name.find("boxhead") != std::string::npos ||
            name.find("colhead") != std::string::npos ||
            name.find("colwig") != std::string::npos ||
            name.find("box") != std::string::npos ||
            name.find("lcol") != std::string::npos ||
            name.find("col") != std::string::npos) return true;

        if (name.find("reference") != std::string::npos ||
            name.find("guide") != std::string::npos ||
            name.find("virtualbody") != std::string::npos ||
            name.find("virtualhair") != std::string::npos ||
            name.find("virtualground") != std::string::npos ||
            name.find("virtual") != std::string::npos ||
            name.find("mannequin") != std::string::npos ||
            name.find("manikin") != std::string::npos) return true;

        return false;
    }

    static bool IsJunkTexture(const std::string& a_rawTex)
    {
        if (a_rawTex.empty()) return false;
        std::string tex = a_rawTex;
        std::transform(tex.begin(), tex.end(), tex.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (tex.find("0alfa") != std::string::npos ||
            tex.find("dummy") != std::string::npos ||
            tex.find("null") != std::string::npos ||
            tex.find("invisible") != std::string::npos ||
            tex.find("empty") != std::string::npos) {
            return true;
        }
        return false;
    }

    static bool HasAbsoluteJunkNodeName(RE::BSGeometry* a_geom)
    {
        if (!a_geom) return false;
        if (a_geom->name.c_str() && IsAbsoluteJunkString(a_geom->name.c_str())) {
            return true;
        }
        for (auto* cur = a_geom->parent; cur != nullptr; cur = cur->parent) {
            if (cur->name.c_str() && IsAbsoluteJunkString(cur->name.c_str())) {
                return true;
            }
        }
        return false;
    }

    static bool IsBodyNodeName(RE::BSGeometry* a_geom, bool a_hasBody, bool a_hasHead, bool a_hasHands, bool a_hasFeet)
    {
        if (!a_geom) return false;
        const char* raw = a_geom->name.c_str();
        if (!raw || raw[0] == '\0') return false;
        std::string name = raw;
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        for (const char* prefix : { "cbbe_", "cbbe ", "3ba_", "3ba ", "3bbb_", "3bbb ", "unp_", "unp ", "bhunp_", "bhunp ", "uunp_", "uunp ", "7b_", "tbd_" }) {
            size_t plen = std::strlen(prefix);
            if (name.rfind(prefix, 0) == 0) {
                name = name.substr(plen);
                break;
            }
        }

        if (name.find("vagina") != std::string::npos || name.find("anus") != std::string::npos || name.find("nipple") != std::string::npos) return true;
        if (name.find("genitals") != std::string::npos || name.find("penis") != std::string::npos || name.find("clitoris") != std::string::npos) return true;
        if (name.find("areola") != std::string::npos || name.find("pubic") != std::string::npos) return true;

        if (name == "bd" || name == "body" || name == "basebody" || name == "base_body" || name == "bodymesh" || name == "body_mesh" || name == "baseshape" || name == "femalebody" || name == "malebody") {
            return !a_hasBody;
        }

        if (!a_hasHead && (name == "head" || name == "face" || name == "femalehead" || name == "malehead")) return true;
        if (!a_hasHands && (name == "hand" || name == "hands" || name == "femalehands" || name == "malehands")) return true;
        if (!a_hasFeet && (name == "foot" || name == "feet" || name == "femalefeet" || name == "malefeet" || name == "calf" || name == "calves")) return true;

        return false;
    }

    static bool IsBodyTexture(const std::string& a_texture)
    {
        if (a_texture.empty()) return false;
        std::string tex = a_texture;
        std::transform(tex.begin(), tex.end(), tex.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        for (char& c : tex) {
            if (c == '/') c = '\\';
        }

        if (tex.find("femalebody") != std::string::npos ||
            tex.find("malebody") != std::string::npos ||
            tex.find("femalehead") != std::string::npos ||
            tex.find("malehead") != std::string::npos ||
            tex.find("femalehands") != std::string::npos ||
            tex.find("malehands") != std::string::npos ||
            tex.find("femalefeet") != std::string::npos ||
            tex.find("malefeet") != std::string::npos ||
            tex.find("mouthhuman") != std::string::npos ||
            tex.find("eyesmale") != std::string::npos ||
            tex.find("eyesfemale") != std::string::npos ||
            tex.find("bodymale") != std::string::npos ||
            tex.find("bodyfemale") != std::string::npos ||
            tex.find("humanbody") != std::string::npos) {
            return true;
        }
        return false;
    }

    ItemModelData ItemMeshExtractor::ExtractItemModel(RE::TESBoundObject* a_item)
    {
        ItemModelData result;
        std::string modelPath = ResolveModelPath(a_item);
        if (modelPath.empty()) return result;

        std::string lowerPath = modelPath;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (lowerPath.rfind("meshes\\", 0) == 0 || lowerPath.rfind("meshes/", 0) == 0) {
            modelPath = modelPath.substr(7);
        } else if (lowerPath.rfind("\\meshes\\", 0) == 0 || lowerPath.rfind("/meshes/", 0) == 0) {
            modelPath = modelPath.substr(8);
        }

        RE::NiPointer<RE::NiNode> rootNode;
        RE::BSModelDB::DBTraits::ArgsType args;
        args.texLoadLevel = 3;
        args.postProcess = true;

        auto err = RE::BSModelDB::Demand(modelPath.c_str(), rootNode, args);
        if ((err != RE::BSResource::ErrorCode::kNone || !rootNode) && lowerPath.rfind("meshes\\", 0) != 0) {
            std::string withMeshes = "meshes\\" + modelPath;
            err = RE::BSModelDB::Demand(withMeshes.c_str(), rootNode, args);
        }
        if (err != RE::BSResource::ErrorCode::kNone || !rootNode) {
            return result;
        }

        std::vector<GeometryEntry> geometries;
        RE::NiTransform rootIdentity;
        uint32_t runningGeomCounter = 0;
        WalkGeometryNodes(rootNode.get(), rootIdentity, geometries, runningGeomCounter);
        if (geometries.empty()) return result;

        bool armorHasBody = false;
        bool armorHasHands = false;
        bool armorHasFeet = false;
        bool armorHasHead = false;
        bool isArmorItem = false;
        if (auto* armor = a_item->As<RE::TESObjectARMO>()) {
            isArmorItem = true;
            auto slots = static_cast<uint32_t>(armor->GetSlotMask());
            armorHasBody = (slots & static_cast<uint32_t>(RE::BGSBipedObjectForm::BipedObjectSlot::kBody)) != 0;
            armorHasHands = (slots & (static_cast<uint32_t>(RE::BGSBipedObjectForm::BipedObjectSlot::kHands) | static_cast<uint32_t>(RE::BGSBipedObjectForm::BipedObjectSlot::kForearms))) != 0;
            armorHasFeet = (slots & (static_cast<uint32_t>(RE::BGSBipedObjectForm::BipedObjectSlot::kFeet) | static_cast<uint32_t>(RE::BGSBipedObjectForm::BipedObjectSlot::kCalves))) != 0;
            armorHasHead = (slots & (static_cast<uint32_t>(RE::BGSBipedObjectForm::BipedObjectSlot::kHead) | static_cast<uint32_t>(RE::BGSBipedObjectForm::BipedObjectSlot::kHair) | static_cast<uint32_t>(RE::BGSBipedObjectForm::BipedObjectSlot::kCirclet))) != 0;
        }

        float minBound[3] = { 1e9f, 1e9f, 1e9f };
        float maxBound[3] = { -1e9f, -1e9f, -1e9f };
        bool haveBound = false;

        struct TempSubmeshEntry
        {
            ExtractedSubmesh submesh;
            bool isAbsoluteJunk{ false };
            bool isBody{ false };
            float minPt[3]{ 1e9f, 1e9f, 1e9f };
            float maxPt[3]{ -1e9f, -1e9f, -1e9f };
            bool hasBound{ false };
        };
        std::vector<TempSubmeshEntry> extractedEntries;
        auto textureSwaps = CollectTextureSwaps(a_item, modelPath);

        for (const auto& geomEntry : geometries) {
            auto* geom = geomEntry.geom;
            if (!geom) continue;

            uint32_t thisGeomIdx = geomEntry.depthIndex;
            ExtractedSubmesh submesh;
            bool hasShaderProp = false;

            const TextureSwapEntry* matchingSwap = nullptr;
            for (const auto& swap : textureSwaps) {
                if (swap.geomIndex == thisGeomIdx) {
                    matchingSwap = &swap;
                    break;
                }
                if (!swap.name3D.empty() && geom->name.c_str() && _stricmp(geom->name.c_str(), swap.name3D.c_str()) == 0) {
                    matchingSwap = &swap;
                    break;
                }
                if (swap.geomIndex == 0xFFFFFFFFu) {
                    matchingSwap = &swap;
                }
            }

            bool isHairSubmesh = false;

            DirectX::XMFLOAT4 hairTint = GetPlayerHairColor();
            float specPower = 32.0f;
            float specStrength = 0.18f;

            const auto& geomData = geom->GetGeometryRuntimeData();
            for (uint32_t p = 0; p < RE::BSGeometry::States::kTotal; ++p) {
                auto* prop = geomData.properties[p].get();
                if (!prop) continue;

                if (auto* shaderProp = netimmerse_cast<RE::BSLightingShaderProperty*>(prop)) {
                    hasShaderProp = true;
                    auto* matBase = shaderProp->material ? static_cast<RE::BSLightingShaderMaterialBase*>(shaderProp->material) : nullptr;
                    bool hasGlow = shaderProp->flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kGlowMap) ||
                        (matBase && matBase->GetFeature() == RE::BSShaderMaterial::Feature::kGlowMap);

                    if (hasGlow) {
                        if (shaderProp->emissiveColor) {
                            submesh.emissiveColor[0] = shaderProp->emissiveColor->red;
                            submesh.emissiveColor[1] = shaderProp->emissiveColor->green;
                            submesh.emissiveColor[2] = shaderProp->emissiveColor->blue;
                            submesh.emissiveMultiple = (shaderProp->emissiveMult > 0.0f) ? shaderProp->emissiveMult : 1.0f;
                        } else {
                            submesh.emissiveColor[0] = 1.0f;
                            submesh.emissiveColor[1] = 1.0f;
                            submesh.emissiveColor[2] = 1.0f;
                            submesh.emissiveMultiple = (shaderProp->emissiveMult > 0.0f) ? shaderProp->emissiveMult : 1.0f;
                        }
                    }

                    if (shaderProp->flags.any(RE::BSShaderProperty::EShaderPropertyFlag::kEnvMap)) {
                        specPower = 64.0f;
                        specStrength = 0.22f;
                    }

                    if (matBase) {
                        if (matBase->GetFeature() == RE::BSShaderMaterial::Feature::kHairTint) {
                            isHairSubmesh = true;
                            auto* hairMat = static_cast<RE::BSLightingShaderMaterialHairTint*>(matBase);
                            float dr = std::abs(hairMat->tintColor.red - hairMat->tintColor.green);
                            float dg = std::abs(hairMat->tintColor.green - hairMat->tintColor.blue);
                            if ((dr > 0.06f || dg > 0.06f) && (hairMat->tintColor.red > 0.05f || hairMat->tintColor.green > 0.05f || hairMat->tintColor.blue > 0.05f)) {
                                hairTint = DirectX::XMFLOAT4(hairMat->tintColor.red, hairMat->tintColor.green, hairMat->tintColor.blue, 1.0f);
                            }
                        } else if (matBase->GetFeature() == RE::BSShaderMaterial::Feature::kEnvironmentMap) {
                            specPower = 64.0f;
                            specStrength = 0.22f;
                        }

                        if (matBase->specularPower > 10.0f) {
                            specPower = matBase->specularPower;
                        }
                        if (matBase->specularColorScale > 0.0f) {
                            specStrength = std::clamp(specStrength * matBase->specularColorScale, 0.08f, 0.25f);
                        }

                        auto texSet = matBase->GetTextureSet();
                        if (!texSet) {
                            texSet = matBase->textureSet;
                        }
                        if (texSet) {
                            const char* diffuse = texSet->GetTexturePath(RE::BSTextureSet::Texture::kDiffuse);
                            if (diffuse && diffuse[0] != '\0') {
                                submesh.diffuseTexture = ResolveTexturePath(diffuse, modelPath);
                            }
                        }

                        if (!matchingSwap && matBase->diffuseTexture) {
                            submesh.textureSRV = TextureSrv(matBase->diffuseTexture.get());
                        }
                    }
                } else if (auto* effectProp = netimmerse_cast<RE::BSEffectShaderProperty*>(prop)) {
                    hasShaderProp = true;
                    if (effectProp->GetMaterial()) {
                        const char* effTex = effectProp->GetMaterial()->sourceTexturePath.c_str();
                        if (effTex && effTex[0] != '\0') {
                            submesh.diffuseTexture = ResolveTexturePath(effTex, modelPath);
                        }
                    }
                } else if (auto* alphaProp = netimmerse_cast<RE::NiAlphaProperty*>(prop)) {
                    if (alphaProp->GetAlphaTesting() || alphaProp->GetAlphaBlending()) {
                        submesh.hasAlphaTest = true;
                    }
                }
            }

            if (matchingSwap && !matchingSwap->diffusePath.empty()) {
                submesh.diffuseTexture = matchingSwap->diffusePath;
                submesh.textureSRV = nullptr;
            }

            submesh.isHair = isHairSubmesh;
            submesh.tintColor[0] = hairTint.x;
            submesh.tintColor[1] = hairTint.y;
            submesh.tintColor[2] = hairTint.z;
            submesh.tintColor[3] = 1.0f;
            submesh.specularPower = specPower;
            submesh.specularStrength = specStrength;

            auto* triShape = geom->AsTriShape();
            auto* rendererData = geomData.rendererData;
            if (triShape && rendererData && rendererData->rawVertexData && rendererData->rawIndexData) {
                const auto& triData = triShape->GetTrishapeRuntimeData();
                uint32_t numVerts = triData.vertexCount;
                uint32_t numTris = triData.triangleCount;
                uint32_t vertexSize = rendererData->vertexDesc.GetSize();
                uint32_t posOffset = rendererData->vertexDesc.GetAttributeOffset(RE::BSGraphics::Vertex::VA_POSITION);
                uint32_t uvOffset = rendererData->vertexDesc.GetAttributeOffset(RE::BSGraphics::Vertex::VA_TEXCOORD0);
                uint32_t nrmOffset = rendererData->vertexDesc.GetAttributeOffset(RE::BSGraphics::Vertex::VA_NORMAL);
                bool hasUV = rendererData->vertexDesc.HasFlag(RE::BSGraphics::Vertex::VF_UV);
                bool hasNormal = rendererData->vertexDesc.HasFlag(RE::BSGraphics::Vertex::VF_NORMAL);
                bool fullPrec = PositionIsFullFloat(rendererData->vertexDesc, posOffset);

                auto* dynamicShape = triShape->AsDynamicTriShape();
                const DirectX::XMFLOAT4* dynPositions = nullptr;
                if (dynamicShape) {
                    const auto& dyn = dynamicShape->GetDynamicTrishapeRuntimeData();
                    if (dyn.dynamicData && dyn.dataSize >= numVerts * sizeof(DirectX::XMFLOAT4)) {
                        dynPositions = static_cast<const DirectX::XMFLOAT4*>(dyn.dynamicData);
                    }
                }

                if (vertexSize > 0 && numVerts > 0 && numTris > 0) {
                    submesh.vertices.resize(numVerts);
                    for (uint32_t v = 0; v < numVerts; ++v) {
                        const uint8_t* vertBytes = rendererData->rawVertexData + (v * vertexSize);
                        float rx = 0.0f, ry = 0.0f, rz = 0.0f;
                        if (dynPositions) {
                            rx = dynPositions[v].x;
                            ry = dynPositions[v].y;
                            rz = dynPositions[v].z;
                        } else if (fullPrec) {
                            const float* pos = reinterpret_cast<const float*>(vertBytes + posOffset);
                            rx = pos[0]; ry = pos[1]; rz = pos[2];
                        } else {
                            const uint16_t* ph = reinterpret_cast<const uint16_t*>(vertBytes + posOffset);
                            rx = HalfToFloat(ph[0]);
                            ry = HalfToFloat(ph[1]);
                            rz = HalfToFloat(ph[2]);
                        }
                        auto tp = TransformPoint(geomEntry.transform, rx, ry, rz);
                        submesh.vertices[v].x = tp.x;
                        submesh.vertices[v].y = tp.y;
                        submesh.vertices[v].z = tp.z;

                        if (hasUV) {
                            const uint16_t* uvRaw = reinterpret_cast<const uint16_t*>(vertBytes + uvOffset);
                            submesh.vertices[v].u = HalfToFloat(uvRaw[0]);
                            submesh.vertices[v].v = HalfToFloat(uvRaw[1]);
                        } else {
                            submesh.vertices[v].u = 0.0f;
                            submesh.vertices[v].v = 0.0f;
                        }

                        if (hasNormal) {
                            const uint8_t* n = vertBytes + nrmOffset;
                            auto tn = TransformNormal(geomEntry.transform,
                                (static_cast<float>(n[0]) / 255.0f) * 2.0f - 1.0f,
                                (static_cast<float>(n[1]) / 255.0f) * 2.0f - 1.0f,
                                (static_cast<float>(n[2]) / 255.0f) * 2.0f - 1.0f);
                            submesh.vertices[v].nx = tn.x;
                            submesh.vertices[v].ny = tn.y;
                            submesh.vertices[v].nz = tn.z;
                        } else {
                            submesh.vertices[v].nx = 0.0f;
                            submesh.vertices[v].ny = 0.0f;
                            submesh.vertices[v].nz = 0.0f;
                        }
                    }

                    uint32_t numIndices = numTris * 3;
                    submesh.indices.reserve(numIndices);
                    for (uint32_t i = 0; i + 2 < numIndices; i += 3) {
                        uint32_t i0 = rendererData->rawIndexData[i];
                        uint32_t i1 = rendererData->rawIndexData[i + 1];
                        uint32_t i2 = rendererData->rawIndexData[i + 2];
                        if (i0 >= numVerts || i1 >= numVerts || i2 >= numVerts) continue;
                        if (i0 == i1 || i1 == i2 || i0 == i2) continue;
                        submesh.indices.push_back(i0);
                        submesh.indices.push_back(i1);
                        submesh.indices.push_back(i2);
                    }

                    if (!hasNormal) {
                        for (size_t i = 0; i + 2 < submesh.indices.size(); i += 3) {
                            uint32_t i0 = submesh.indices[i];
                            uint32_t i1 = submesh.indices[i + 1];
                            uint32_t i2 = submesh.indices[i + 2];
                            if (i0 < submesh.vertices.size() && i1 < submesh.vertices.size() && i2 < submesh.vertices.size()) {
                                auto& v0 = submesh.vertices[i0];
                                auto& v1 = submesh.vertices[i1];
                                auto& v2 = submesh.vertices[i2];
                                float ax = v1.x - v0.x, ay = v1.y - v0.y, az = v1.z - v0.z;
                                float bx = v2.x - v0.x, by = v2.y - v0.y, bz = v2.z - v0.z;
                                float nx = ay * bz - az * by;
                                float ny = az * bx - ax * bz;
                                float nz = ax * by - ay * bx;
                                v0.nx += nx; v0.ny += ny; v0.nz += nz;
                                v1.nx += nx; v1.ny += ny; v1.nz += nz;
                                v2.nx += nx; v2.ny += ny; v2.nz += nz;
                            }
                        }
                        for (auto& v : submesh.vertices) {
                            float len = std::sqrt(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz);
                            if (len > 1e-6f) {
                                v.nx /= len; v.ny /= len; v.nz /= len;
                            } else {
                                v.nz = 1.0f;
                            }
                        }
                    }
                }
            }

            if (submesh.vertices.empty()) {
                auto* skinInstance = geomData.skinInstance.get();
                if (skinInstance && skinInstance->skinPartition) {
                    auto* dynamicShape = geom->AsTriShape() ? geom->AsTriShape()->AsDynamicTriShape() : nullptr;
                    const DirectX::XMFLOAT4* dynPositions = nullptr;
                    uint32_t dynCount = 0;
                    if (dynamicShape) {
                        const auto& dyn = dynamicShape->GetDynamicTrishapeRuntimeData();
                        if (dyn.dynamicData) {
                            dynPositions = static_cast<const DirectX::XMFLOAT4*>(dyn.dynamicData);
                            dynCount = static_cast<uint32_t>(dyn.dataSize / sizeof(DirectX::XMFLOAT4));
                        }
                    }

                    auto* partitionObj = skinInstance->skinPartition.get();
                    RE::BSGraphics::TriShape* primaryBuff = nullptr;
                    for (const auto& part : partitionObj->partitions) {
                        if (part.buffData && part.buffData->rawVertexData) {
                            primaryBuff = part.buffData;
                            break;
                        }
                    }

                    bool isShared = (primaryBuff != nullptr);
                    for (const auto& part : partitionObj->partitions) {
                        if (part.buffData && part.buffData != primaryBuff) {
                            isShared = false;
                            break;
                        }
                    }

                    if (isShared && primaryBuff) {
                        uint32_t totalVertCount = partitionObj->vertexCount;
                        if (totalVertCount == 0) {
                            for (const auto& part : partitionObj->partitions) {
                                totalVertCount = (std::max)(totalVertCount, static_cast<uint32_t>(part.vertices));
                            }
                        }

                        uint32_t pVertexSize = primaryBuff->vertexDesc.GetSize();
                        uint32_t pPosOffset = primaryBuff->vertexDesc.GetAttributeOffset(RE::BSGraphics::Vertex::VA_POSITION);
                        uint32_t pUvOffset = primaryBuff->vertexDesc.GetAttributeOffset(RE::BSGraphics::Vertex::VA_TEXCOORD0);
                        uint32_t pNrmOffset = primaryBuff->vertexDesc.GetAttributeOffset(RE::BSGraphics::Vertex::VA_NORMAL);
                        bool pHasUV = primaryBuff->vertexDesc.HasFlag(RE::BSGraphics::Vertex::VF_UV);
                        bool pHasNormal = primaryBuff->vertexDesc.HasFlag(RE::BSGraphics::Vertex::VF_NORMAL);
                        bool pFullPrec = PositionIsFullFloat(primaryBuff->vertexDesc, pPosOffset);

                        if (pVertexSize > 0 && totalVertCount > 0) {
                            uint32_t baseVertex = static_cast<uint32_t>(submesh.vertices.size());
                            for (uint32_t pv = 0; pv < totalVertCount; ++pv) {
                                const uint8_t* pvb = primaryBuff->rawVertexData + (pv * pVertexSize);
                                float rx = 0.0f, ry = 0.0f, rz = 0.0f;
                                if (dynPositions && pv < dynCount && !std::isnan(dynPositions[pv].x) && !std::isinf(dynPositions[pv].x)) {
                                    rx = dynPositions[pv].x;
                                    ry = dynPositions[pv].y;
                                    rz = dynPositions[pv].z;
                                } else if (pFullPrec) {
                                    const float* pos = reinterpret_cast<const float*>(pvb + pPosOffset);
                                    if (!std::isnan(pos[0]) && !std::isinf(pos[0]) &&
                                        !std::isnan(pos[1]) && !std::isinf(pos[1]) &&
                                        !std::isnan(pos[2]) && !std::isinf(pos[2])) {
                                        rx = pos[0]; ry = pos[1]; rz = pos[2];
                                    }
                                } else {
                                    const uint16_t* ph = reinterpret_cast<const uint16_t*>(pvb + pPosOffset);
                                    rx = HalfToFloat(ph[0]);
                                    ry = HalfToFloat(ph[1]);
                                    rz = HalfToFloat(ph[2]);
                                }
                                auto tp = TransformPoint(geomEntry.transform, rx, ry, rz);
                                ItemVertex iv;
                                iv.x = tp.x; iv.y = tp.y; iv.z = tp.z;
                                if (pHasUV) {
                                    const uint16_t* uvRaw = reinterpret_cast<const uint16_t*>(pvb + pUvOffset);
                                    iv.u = HalfToFloat(uvRaw[0]);
                                    iv.v = HalfToFloat(uvRaw[1]);
                                } else {
                                    iv.u = 0.0f;
                                    iv.v = 0.0f;
                                }
                                if (pHasNormal) {
                                    const uint8_t* n = pvb + pNrmOffset;
                                    auto tn = TransformNormal(geomEntry.transform,
                                        (static_cast<float>(n[0]) / 255.0f) * 2.0f - 1.0f,
                                        (static_cast<float>(n[1]) / 255.0f) * 2.0f - 1.0f,
                                        (static_cast<float>(n[2]) / 255.0f) * 2.0f - 1.0f);
                                    iv.nx = tn.x; iv.ny = tn.y; iv.nz = tn.z;
                                } else {
                                    iv.nx = 0.0f; iv.ny = 0.0f; iv.nz = 0.0f;
                                }
                                submesh.vertices.push_back(iv);
                            }
                            for (const auto& part : partitionObj->partitions) {
                                if (part.triList && part.triangles > 0) {
                                    for (uint32_t t = 0; t < part.triangles; ++t) {
                                        uint32_t i0 = part.triList[t * 3];
                                        uint32_t i1 = part.triList[t * 3 + 1];
                                        uint32_t i2 = part.triList[t * 3 + 2];
                                        if (part.vertexMap) {
                                            if (i0 < part.vertices) i0 = part.vertexMap[i0];
                                            if (i1 < part.vertices) i1 = part.vertexMap[i1];
                                            if (i2 < part.vertices) i2 = part.vertexMap[i2];
                                        }
                                        if (i0 >= totalVertCount || i1 >= totalVertCount || i2 >= totalVertCount) {
                                            continue;
                                        }
                                        if (i0 == i1 || i1 == i2 || i0 == i2) {
                                            continue;
                                        }
                                        submesh.indices.push_back(baseVertex + i0);
                                        submesh.indices.push_back(baseVertex + i1);
                                        submesh.indices.push_back(baseVertex + i2);
                                    }
                                }
                            }
                        }
                    } else {
                        for (const auto& part : partitionObj->partitions) {
                            auto* buffData = part.buffData;
                            if (buffData && buffData->rawVertexData && part.triList) {
                                uint32_t pVertexCount = part.vertices;
                                uint32_t pTriCount = part.triangles;
                                uint32_t pVertexSize = buffData->vertexDesc.GetSize();
                                uint32_t pPosOffset = buffData->vertexDesc.GetAttributeOffset(RE::BSGraphics::Vertex::VA_POSITION);
                                uint32_t pUvOffset = buffData->vertexDesc.GetAttributeOffset(RE::BSGraphics::Vertex::VA_TEXCOORD0);
                                uint32_t pNrmOffset = buffData->vertexDesc.GetAttributeOffset(RE::BSGraphics::Vertex::VA_NORMAL);
                                bool pHasUV = buffData->vertexDesc.HasFlag(RE::BSGraphics::Vertex::VF_UV);
                                bool pHasNormal = buffData->vertexDesc.HasFlag(RE::BSGraphics::Vertex::VF_NORMAL);
                                bool pFullPrec = PositionIsFullFloat(buffData->vertexDesc, pPosOffset);

                                if (pVertexSize > 0 && pVertexCount > 0 && pTriCount > 0) {
                                    uint32_t baseVertex = static_cast<uint32_t>(submesh.vertices.size());
                                    for (uint32_t pv = 0; pv < pVertexCount; ++pv) {
                                        const uint8_t* pvb = buffData->rawVertexData + (pv * pVertexSize);
                                        float rx = 0.0f, ry = 0.0f, rz = 0.0f;
                                        if (dynPositions && pv < dynCount && !std::isnan(dynPositions[pv].x) && !std::isinf(dynPositions[pv].x)) {
                                            rx = dynPositions[pv].x;
                                            ry = dynPositions[pv].y;
                                            rz = dynPositions[pv].z;
                                        } else if (pFullPrec) {
                                            const float* pos = reinterpret_cast<const float*>(pvb + pPosOffset);
                                            if (!std::isnan(pos[0]) && !std::isinf(pos[0]) &&
                                                !std::isnan(pos[1]) && !std::isinf(pos[1]) &&
                                                !std::isnan(pos[2]) && !std::isinf(pos[2])) {
                                                rx = pos[0]; ry = pos[1]; rz = pos[2];
                                            }
                                        } else {
                                            const uint16_t* ph = reinterpret_cast<const uint16_t*>(pvb + pPosOffset);
                                            rx = HalfToFloat(ph[0]);
                                            ry = HalfToFloat(ph[1]);
                                            rz = HalfToFloat(ph[2]);
                                        }
                                        auto tp = TransformPoint(geomEntry.transform, rx, ry, rz);
                                        ItemVertex iv;
                                        iv.x = tp.x; iv.y = tp.y; iv.z = tp.z;
                                        if (pHasUV) {
                                            const uint16_t* uvRaw = reinterpret_cast<const uint16_t*>(pvb + pUvOffset);
                                            iv.u = HalfToFloat(uvRaw[0]);
                                            iv.v = HalfToFloat(uvRaw[1]);
                                        } else {
                                            iv.u = 0.0f;
                                            iv.v = 0.0f;
                                        }
                                        if (pHasNormal) {
                                            const uint8_t* n = pvb + pNrmOffset;
                                            auto tn = TransformNormal(geomEntry.transform,
                                                (static_cast<float>(n[0]) / 255.0f) * 2.0f - 1.0f,
                                                (static_cast<float>(n[1]) / 255.0f) * 2.0f - 1.0f,
                                                (static_cast<float>(n[2]) / 255.0f) * 2.0f - 1.0f);
                                            iv.nx = tn.x; iv.ny = tn.y; iv.nz = tn.z;
                                        } else {
                                            iv.nx = 0.0f; iv.ny = 0.0f; iv.nz = 0.0f;
                                        }
                                        submesh.vertices.push_back(iv);
                                    }
                                    for (uint32_t t = 0; t < pTriCount; ++t) {
                                        uint32_t i0 = part.triList[t * 3];
                                        uint32_t i1 = part.triList[t * 3 + 1];
                                        uint32_t i2 = part.triList[t * 3 + 2];
                                        if (i0 >= pVertexCount || i1 >= pVertexCount || i2 >= pVertexCount) {
                                            continue;
                                        }
                                        if (i0 == i1 || i1 == i2 || i0 == i2) {
                                            continue;
                                        }
                                        submesh.indices.push_back(baseVertex + i0);
                                        submesh.indices.push_back(baseVertex + i1);
                                        submesh.indices.push_back(baseVertex + i2);
                                    }
                                }
                            }
                        }
                    }
                    if (!submesh.indices.empty()) {
                        bool anyNormal = false;
                        for (const auto& v : submesh.vertices) {
                            if (std::abs(v.nx) > 1e-4f || std::abs(v.ny) > 1e-4f || std::abs(v.nz) > 1e-4f) {
                                anyNormal = true;
                                break;
                            }
                        }
                        if (!anyNormal) {
                            for (size_t i = 0; i + 2 < submesh.indices.size(); i += 3) {
                                uint32_t i0 = submesh.indices[i];
                                uint32_t i1 = submesh.indices[i + 1];
                                uint32_t i2 = submesh.indices[i + 2];
                                if (i0 < submesh.vertices.size() && i1 < submesh.vertices.size() && i2 < submesh.vertices.size()) {
                                    auto& v0 = submesh.vertices[i0];
                                    auto& v1 = submesh.vertices[i1];
                                    auto& v2 = submesh.vertices[i2];
                                    float ax = v1.x - v0.x, ay = v1.y - v0.y, az = v1.z - v0.z;
                                    float bx = v2.x - v0.x, by = v2.y - v0.y, bz = v2.z - v0.z;
                                    float nx = ay * bz - az * by;
                                    float ny = az * bx - ax * bz;
                                    float nz = ax * by - ay * bx;
                                    v0.nx += nx; v0.ny += ny; v0.nz += nz;
                                    v1.nx += nx; v1.ny += ny; v1.nz += nz;
                                    v2.nx += nx; v2.ny += ny; v2.nz += nz;
                                }
                            }
                            for (auto& v : submesh.vertices) {
                                float len = std::sqrt(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz);
                                if (len > 1e-6f) {
                                    v.nx /= len; v.ny /= len; v.nz /= len;
                                } else {
                                    v.nz = 1.0f;
                                }
                            }
                        }
                    }
                }
            }

            bool partitionMismatch = false;
            if (isArmorItem && geomData.skinInstance && (armorHasBody || armorHasHands || armorHasFeet || armorHasHead)) {
                if (auto* dismember = netimmerse_cast<RE::BSDismemberSkinInstance*>(geomData.skinInstance.get())) {
                    const auto& rtData = dismember->GetRuntimeData();
                    for (int32_t pi = 0; pi < rtData.numPartitions; ++pi) {
                        uint16_t slot = rtData.partitions[pi].slot;
                        if (slot == 32 && !armorHasBody && IsBodyTexture(submesh.diffuseTexture)) {
                            partitionMismatch = true;
                            break;
                        }
                        if (slot == 33 && !armorHasHands && IsBodyTexture(submesh.diffuseTexture)) {
                            partitionMismatch = true;
                            break;
                        }
                        if (slot == 37 && !armorHasFeet && IsBodyTexture(submesh.diffuseTexture)) {
                            partitionMismatch = true;
                            break;
                        }
                        if (slot == 30 && !armorHasHead && IsBodyTexture(submesh.diffuseTexture)) {
                            partitionMismatch = true;
                            break;
                        }
                    }
                }
            }

            TempSubmeshEntry entry;
            entry.isAbsoluteJunk = HasAbsoluteJunkNodeName(geom) || IsJunkTexture(submesh.diffuseTexture);
            entry.isBody = IsBodyNodeName(geom, armorHasBody, armorHasHead, armorHasHands, armorHasFeet) ||
                           (!armorHasBody && IsBodyTexture(submesh.diffuseTexture)) ||
                           partitionMismatch ||
                           !hasShaderProp;

            if (!submesh.vertices.empty()) {
                if (!submesh.indices.empty()) {
                    for (uint32_t idx : submesh.indices) {
                        if (idx < submesh.vertices.size()) {
                            const auto& v = submesh.vertices[idx];
                            if ((v.x * v.x + v.y * v.y + v.z * v.z < 0.25f) && submesh.vertices.size() > 10) {
                                continue;
                            }
                            entry.minPt[0] = (std::min)(entry.minPt[0], v.x);
                            entry.minPt[1] = (std::min)(entry.minPt[1], v.y);
                            entry.minPt[2] = (std::min)(entry.minPt[2], v.z);
                            entry.maxPt[0] = (std::max)(entry.maxPt[0], v.x);
                            entry.maxPt[1] = (std::max)(entry.maxPt[1], v.y);
                            entry.maxPt[2] = (std::max)(entry.maxPt[2], v.z);
                            entry.hasBound = true;
                        }
                    }
                }
                if (!entry.hasBound) {
                    for (const auto& v : submesh.vertices) {
                        entry.minPt[0] = (std::min)(entry.minPt[0], v.x);
                        entry.minPt[1] = (std::min)(entry.minPt[1], v.y);
                        entry.minPt[2] = (std::min)(entry.minPt[2], v.z);
                        entry.maxPt[0] = (std::max)(entry.maxPt[0], v.x);
                        entry.maxPt[1] = (std::max)(entry.maxPt[1], v.y);
                        entry.maxPt[2] = (std::max)(entry.maxPt[2], v.z);
                        entry.hasBound = true;
                    }
                }

                if (submesh.vertices.size() >= 3) {
                    float dz = entry.maxPt[2] - entry.minPt[2];
                    float dx = entry.maxPt[0] - entry.minPt[0];
                    float dy = entry.maxPt[1] - entry.minPt[1];
                    if (dz < 3.5f && dx > 25.0f && dy > 25.0f && entry.maxPt[2] < 15.0f) {
                        entry.isAbsoluteJunk = true;
                    }
                }
            } else {
                entry.hasBound = false;
            }

            entry.submesh = std::move(submesh);
            extractedEntries.push_back(std::move(entry));
        }

        size_t nonBodyCount = 0;
        for (const auto& entry : extractedEntries) {
            if (!entry.isAbsoluteJunk && !entry.isBody) {
                if (!entry.submesh.diffuseTexture.empty() || entry.submesh.textureSRV != nullptr) {
                    ++nonBodyCount;
                }
            }
        }

        if (nonBodyCount == 0) {
            for (const auto& entry : extractedEntries) {
                if (!entry.isAbsoluteJunk && !entry.isBody) {
                    ++nonBodyCount;
                }
            }
        }

        for (auto& entry : extractedEntries) {
            if (entry.isAbsoluteJunk) {
                continue;
            }
            if (nonBodyCount > 0 && entry.isBody) {
                continue;
            }
            if (entry.submesh.vertices.empty()) {
                continue;
            }

            if (entry.hasBound) {
                minBound[0] = (std::min)(minBound[0], entry.minPt[0]);
                minBound[1] = (std::min)(minBound[1], entry.minPt[1]);
                minBound[2] = (std::min)(minBound[2], entry.minPt[2]);
                maxBound[0] = (std::max)(maxBound[0], entry.maxPt[0]);
                maxBound[1] = (std::max)(maxBound[1], entry.maxPt[1]);
                maxBound[2] = (std::max)(maxBound[2], entry.maxPt[2]);
                haveBound = true;
            }

            result.submeshes.push_back(std::move(entry.submesh));
        }

        if (result.submeshes.empty()) {
            for (auto& entry : extractedEntries) {
                if (entry.isAbsoluteJunk || entry.submesh.vertices.empty()) {
                    continue;
                }
                if (entry.hasBound) {
                    minBound[0] = (std::min)(minBound[0], entry.minPt[0]);
                    minBound[1] = (std::min)(minBound[1], entry.minPt[1]);
                    minBound[2] = (std::min)(minBound[2], entry.minPt[2]);
                    maxBound[0] = (std::max)(maxBound[0], entry.maxPt[0]);
                    maxBound[1] = (std::max)(maxBound[1], entry.maxPt[1]);
                    maxBound[2] = (std::max)(maxBound[2], entry.maxPt[2]);
                    haveBound = true;
                }
                result.submeshes.push_back(std::move(entry.submesh));
            }
        }

        if (haveBound) {
            result.boundingCenter[0] = (minBound[0] + maxBound[0]) * 0.5f;
            result.boundingCenter[1] = (minBound[1] + maxBound[1]) * 0.5f;
            result.boundingCenter[2] = (minBound[2] + maxBound[2]) * 0.5f;

            result.aabbMin[0] = minBound[0];
            result.aabbMin[1] = minBound[1];
            result.aabbMin[2] = minBound[2];

            result.aabbMax[0] = maxBound[0];
            result.aabbMax[1] = maxBound[1];
            result.aabbMax[2] = maxBound[2];

            float dx = maxBound[0] - minBound[0];
            float dy = maxBound[1] - minBound[1];
            float dz = maxBound[2] - minBound[2];
            result.maxExtent = (std::max)({ dx, dy, dz });
            if (result.maxExtent < 0.1f) result.maxExtent = 5.0f;
            result.boundingRadius = std::sqrt(dx * dx + dy * dy + dz * dz) * 0.5f;
            if (result.boundingRadius < 0.1f) result.boundingRadius = 5.0f;
        }

        bool hasAnyVertices = false;
        for (const auto& sm : result.submeshes) {
            if (!sm.vertices.empty() && !sm.indices.empty()) {
                hasAnyVertices = true;
                break;
            }
        }

        if (!hasAnyVertices && haveBound) {
            ExtractedSubmesh fallbackMesh;
            float cx = result.boundingCenter[0];
            float cy = result.boundingCenter[1];
            float cz = result.boundingCenter[2];
            float r = result.boundingRadius * 0.7f;

            fallbackMesh.vertices = {
                { cx, cy, cz + r, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f },
                { cx - r, cy, cz, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f },
                { cx + r, cy, cz, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
                { cx, cy - r, cz, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f },
                { cx, cy + r, cz, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f },
                { cx, cy, cz - r, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f }
            };
            fallbackMesh.indices = {
                0, 1, 3,  0, 3, 2,  0, 2, 4,  0, 4, 1,
                5, 3, 1,  5, 2, 3,  5, 4, 2,  5, 1, 4
            };
            result.submeshes.push_back(std::move(fallbackMesh));
        }

        result.valid = !result.submeshes.empty();
        return result;
    }
}
