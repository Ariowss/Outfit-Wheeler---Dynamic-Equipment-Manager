#pragma once

#include <RE/Skyrim.h>
#include <vector>
#include <string>
#include <cstdint>

struct ID3D11ShaderResourceView;

namespace OutfitWheeler::Thumbnail
{
    struct ItemVertex
    {
        float x{ 0.0f }, y{ 0.0f }, z{ 0.0f };
        float u{ 0.0f }, v{ 0.0f };
        float nx{ 0.0f }, ny{ 0.0f }, nz{ 1.0f };
    };

    struct ExtractedSubmesh
    {
        std::vector<ItemVertex> vertices;
        std::vector<uint32_t> indices;
        std::string diffuseTexture;
        ID3D11ShaderResourceView* textureSRV{ nullptr };
        bool hasAlphaTest{ false };
        bool isHair{ false };
        float tintColor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
        float specularPower{ 24.0f };
        float specularStrength{ 0.20f };
        float emissiveColor[3]{ 0.0f, 0.0f, 0.0f };
        float emissiveMultiple{ 0.0f };
    };

    struct ItemModelData
    {
        std::vector<ExtractedSubmesh> submeshes;
        float boundingCenter[3]{ 0.0f, 0.0f, 0.0f };
        float boundingRadius{ 10.0f };
        float aabbMin[3]{ 0.0f, 0.0f, 0.0f };
        float aabbMax[3]{ 0.0f, 0.0f, 0.0f };
        float maxExtent{ 10.0f };
        bool valid{ false };
    };

    class ItemMeshExtractor
    {
    public:
        static ItemModelData ExtractItemModel(RE::TESBoundObject* a_item);
        static std::string ResolveModelPath(RE::TESBoundObject* a_item);
    };
}
