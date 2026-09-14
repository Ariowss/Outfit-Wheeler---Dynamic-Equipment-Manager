#pragma once

#include <RE/Skyrim.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace OutfitWheeler::Thumbnail
{
    class ItemPreviewRenderer
    {
    public:
        static ItemPreviewRenderer& GetSingleton();

        bool Initialize(ID3D11Device* a_device);
        [[nodiscard]] bool IsInitialized() const { return m_initialized; }

        bool RenderItemToBuffer(RE::TESBoundObject* a_item,
                                std::vector<uint8_t>& outRgbaPixels,
                                uint32_t& outWidth,
                                uint32_t& outHeight);

        ID3D11ShaderResourceView* GetOrCreateTexture(const std::string& a_texturePath);

    private:
        ItemPreviewRenderer() = default;
        ~ItemPreviewRenderer() = default;

        ItemPreviewRenderer(const ItemPreviewRenderer&) = delete;
        ItemPreviewRenderer& operator=(const ItemPreviewRenderer&) = delete;

        bool m_initialized{ false };
        uint32_t m_width{ 512 };
        uint32_t m_height{ 512 };

        Microsoft::WRL::ComPtr<ID3D11Device> m_device;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_renderTargetTexture;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_depthStencilTexture;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_stagingTexture;

        Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
        Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilState;
        Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_whiteTextureSRV;
        Microsoft::WRL::ComPtr<ID3D11BlendState> m_blendState;
        std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_textureCache;
    };
}
