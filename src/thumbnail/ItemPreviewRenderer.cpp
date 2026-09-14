#include "pch.h"
#include <windows.h>
#include "ItemPreviewRenderer.h"
#include "ItemMeshExtractor.h"
#include <RE/B/BSResourceNiBinaryStream.h>

#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <thread>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace OutfitWheeler::Thumbnail
{
    using namespace DirectX;
    using Microsoft::WRL::ComPtr;

    namespace
    {
        struct ShaderConstants
        {
            XMFLOAT4X4 viewProj;
            XMFLOAT3 lightDir;
            float ambient{ 0.22f };
            XMFLOAT3 eyePos;
            float specularStrength{ 0.18f };
            uint32_t hasAlphaTest{ 0 };
            uint32_t isHair{ 0 };
            float specularPower{ 32.0f };
            float emissiveMultiple{ 0.0f };
            XMFLOAT4 tintColor{ 1.0f, 1.0f, 1.0f, 1.0f };
            XMFLOAT4 emissiveColor{ 0.0f, 0.0f, 0.0f, 0.0f };
        };
        static_assert(sizeof(ShaderConstants) % 16 == 0, "ShaderConstants size must be multiple of 16");

        const char kShaderSource[] = R"(
cbuffer Params : register(b0)
{
    float4x4 ViewProj;
    float3 LightDir;
    float Ambient;
    float3 EyePos;
    float SpecularStrength;
    uint HasAlphaTest;
    uint IsHair;
    float SpecularPower;
    float EmissiveMultiple;
    float4 TintColor;
    float4 EmissiveColor;
};

Texture2D g_tex : register(t0);
SamplerState g_sampler : register(s0);

struct VS_IN
{
    float3 Pos : POSITION;
    float2 UV : TEXCOORD0;
    float3 Normal : NORMAL;
};

struct VS_OUT
{
    float4 Pos : SV_Position;
    float2 UV : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float3 WorldPos : TEXCOORD2;
};

VS_OUT VSMain(VS_IN input)
{
    VS_OUT o;
    o.Pos = mul(float4(input.Pos, 1.0), ViewProj);
    o.UV = input.UV;
    o.Normal = normalize(input.Normal);
    o.WorldPos = input.Pos;
    return o;
}

float4 PSMain(VS_OUT input, bool isFrontFace : SV_IsFrontFace) : SV_Target
{
    float4 texColor = g_tex.Sample(g_sampler, input.UV);
    if (HasAlphaTest != 0 && texColor.a < 0.333) discard;

    if (IsHair != 0) {
        float maxC = max(texColor.r, max(texColor.g, texColor.b));
        float minC = min(texColor.r, min(texColor.g, texColor.b));
        float sat = (maxC > 1e-4) ? ((maxC - minC) / maxC) : 0.0;
        float3 effectiveTint = max(TintColor.rgb, float3(0.14, 0.14, 0.14));
        float tintFactor = saturate(1.0 - sat * 3.5);
        float3 tintedColor = texColor.rgb * effectiveTint;
        texColor.rgb = lerp(texColor.rgb, tintedColor, tintFactor);
    }

    float3 normal = normalize(input.Normal) * (isFrontFace ? 1.0 : -1.0);
    float3 keyDir = normalize(LightDir);
    float3 viewDir = normalize(EyePos - input.WorldPos);
    float3 fillDir = normalize(float3(-keyDir.x * 0.75, keyDir.y * 0.25, keyDir.z * 0.55 + 0.25));

    float NdotL = saturate(dot(normal, keyDir));
    float wrappedKey = saturate(dot(normal, keyDir) * 0.70 + 0.30) * 0.50;
    float fillLight = saturate(dot(normal, fillDir)) * 0.18;
    float rimLight = pow(1.0 - saturate(dot(normal, viewDir)), 3.0) * 0.08;
    float lighting = Ambient + wrappedKey + fillLight + rimLight;

    float3 halfDir = normalize(keyDir + viewDir);
    float specAngle = saturate(dot(normal, halfDir));
    float spec = pow(specAngle, max(SpecularPower, 32.0)) * (SpecularStrength * 0.25) * saturate(NdotL * 2.0);

    float3 emissive = EmissiveColor.rgb * min(EmissiveMultiple, 3.0);
    float3 lit = texColor.rgb * lighting + spec + emissive;

    float3 compressedColor = lit / (1.0 + lit * 0.18);
    float outAlpha = (HasAlphaTest != 0) ? saturate(texColor.a * 1.35 + 0.10) : 1.0;
    return float4(saturate(compressedColor), outAlpha);
}
)";
    }

    ItemPreviewRenderer& ItemPreviewRenderer::GetSingleton()
    {
        static ItemPreviewRenderer instance;
        return instance;
    }

    bool ItemPreviewRenderer::Initialize(ID3D11Device* a_device)
    {
        if (m_initialized) return true;
        if (!a_device) return false;

        uint32_t screenW = 0;
        uint32_t screenH = 0;
        auto sz = RE::BSGraphics::Renderer::GetScreenSize();
        screenW = sz.width;
        screenH = sz.height;

        if (screenW < 3840 && screenH < 2160) {
            DEVMODEA dm{};
            dm.dmSize = sizeof(dm);
            if (EnumDisplaySettingsA(nullptr, ENUM_CURRENT_SETTINGS, &dm)) {
                if (dm.dmPelsWidth > screenW) screenW = dm.dmPelsWidth;
                if (dm.dmPelsHeight > screenH) screenH = dm.dmPelsHeight;
            }
        }

        if (screenW >= 3840 || screenH >= 2160) {
            m_width = 1024;
            m_height = 1024;
        } else {
            m_width = 512;
            m_height = 512;
        }

        logger::info("[Thumbnail] Screen resolution: {}x{}, set thumbnail dimensions to {}x{}", screenW, screenH, m_width, m_height);

        m_device = a_device;

        D3D11_TEXTURE2D_DESC rtDesc{};
        rtDesc.Width = m_width;
        rtDesc.Height = m_height;
        rtDesc.MipLevels = 1;
        rtDesc.ArraySize = 1;
        rtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtDesc.SampleDesc.Count = 1;
        rtDesc.Usage = D3D11_USAGE_DEFAULT;
        rtDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

        HRESULT hr = m_device->CreateTexture2D(&rtDesc, nullptr, &m_renderTargetTexture);
        if (FAILED(hr)) return false;

        hr = m_device->CreateRenderTargetView(m_renderTargetTexture.Get(), nullptr, &m_renderTargetView);
        if (FAILED(hr)) return false;

        D3D11_TEXTURE2D_DESC dsDesc{};
        dsDesc.Width = m_width;
        dsDesc.Height = m_height;
        dsDesc.MipLevels = 1;
        dsDesc.ArraySize = 1;
        dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        dsDesc.SampleDesc.Count = 1;
        dsDesc.Usage = D3D11_USAGE_DEFAULT;
        dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

        hr = m_device->CreateTexture2D(&dsDesc, nullptr, &m_depthStencilTexture);
        if (FAILED(hr)) return false;

        hr = m_device->CreateDepthStencilView(m_depthStencilTexture.Get(), nullptr, &m_depthStencilView);
        if (FAILED(hr)) return false;

        D3D11_TEXTURE2D_DESC stDesc = rtDesc;
        stDesc.BindFlags = 0;
        stDesc.Usage = D3D11_USAGE_STAGING;
        stDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        hr = m_device->CreateTexture2D(&stDesc, nullptr, &m_stagingTexture);
        if (FAILED(hr)) return false;

        ComPtr<ID3DBlob> vsBlob;
        ComPtr<ID3DBlob> psBlob;
        ComPtr<ID3DBlob> errBlob;

        hr = D3DCompile(kShaderSource, sizeof(kShaderSource), nullptr, nullptr, nullptr,
                        "VSMain", "vs_5_0", 0, 0, &vsBlob, &errBlob);
        if (FAILED(hr) || !vsBlob) return false;

        hr = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
        if (FAILED(hr)) return false;

        hr = D3DCompile(kShaderSource, sizeof(kShaderSource), nullptr, nullptr, nullptr,
                        "PSMain", "ps_5_0", 0, 0, &psBlob, &errBlob);
        if (FAILED(hr) || !psBlob) return false;

        hr = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);
        if (FAILED(hr)) return false;

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        hr = m_device->CreateInputLayout(layout, 3, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &m_inputLayout);
        if (FAILED(hr)) return false;

        D3D11_BUFFER_DESC cbDesc{};
        cbDesc.ByteWidth = sizeof(ShaderConstants);
        cbDesc.Usage = D3D11_USAGE_DYNAMIC;
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        hr = m_device->CreateBuffer(&cbDesc, nullptr, &m_constantBuffer);
        if (FAILED(hr)) return false;

        D3D11_RASTERIZER_DESC rsDesc{};
        rsDesc.FillMode = D3D11_FILL_SOLID;
        rsDesc.CullMode = D3D11_CULL_NONE;
        rsDesc.DepthClipEnable = TRUE;
        m_device->CreateRasterizerState(&rsDesc, &m_rasterizerState);

        D3D11_DEPTH_STENCIL_DESC dssDesc{};
        dssDesc.DepthEnable = TRUE;
        dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dssDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        m_device->CreateDepthStencilState(&dssDesc, &m_depthStencilState);

        D3D11_SAMPLER_DESC sampDesc{};
        sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sampDesc.MinLOD = 0.0f;
        sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
        m_device->CreateSamplerState(&sampDesc, &m_samplerState);

        D3D11_TEXTURE2D_DESC whiteTexDesc{};
        whiteTexDesc.Width = 1;
        whiteTexDesc.Height = 1;
        whiteTexDesc.MipLevels = 1;
        whiteTexDesc.ArraySize = 1;
        whiteTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        whiteTexDesc.SampleDesc.Count = 1;
        whiteTexDesc.Usage = D3D11_USAGE_IMMUTABLE;
        whiteTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        uint32_t whitePixel = 0xFFFFFFFF;
        D3D11_SUBRESOURCE_DATA whiteInitData{};
        whiteInitData.pSysMem = &whitePixel;
        whiteInitData.SysMemPitch = sizeof(uint32_t);

        ComPtr<ID3D11Texture2D> whiteTex;
        if (SUCCEEDED(m_device->CreateTexture2D(&whiteTexDesc, &whiteInitData, &whiteTex))) {
            m_device->CreateShaderResourceView(whiteTex.Get(), nullptr, &m_whiteTextureSRV);
        }

        D3D11_BLEND_DESC blendDesc{};
        blendDesc.RenderTarget[0].BlendEnable = TRUE;
        blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        m_device->CreateBlendState(&blendDesc, &m_blendState);

        m_initialized = true;
        return true;
    }

    ID3D11ShaderResourceView* ItemPreviewRenderer::GetOrCreateTexture(const std::string& a_texturePath)
    {
        if (a_texturePath.empty() || !m_device) return nullptr;

        std::string cacheKey = a_texturePath;
        for (char& c : cacheKey) {
            if (c == '/') c = '\\';
            c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        }

        auto it = m_textureCache.find(cacheKey);
        if (it != m_textureCache.end()) {
            return it->second.Get();
        }

        std::string norm = a_texturePath;
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

        std::vector<std::string> candidatePaths;
        std::string withTex = "textures\\" + norm;
        candidatePaths.push_back(withTex);
        candidatePaths.push_back(norm);

        std::string withTexFwd = withTex;
        std::replace(withTexFwd.begin(), withTexFwd.end(), '\\', '/');
        candidatePaths.push_back(withTexFwd);

        std::string normFwd = norm;
        std::replace(normFwd.begin(), normFwd.end(), '\\', '/');
        candidatePaths.push_back(normFwd);

        std::string withTexLower = withTex;
        std::transform(withTexLower.begin(), withTexLower.end(), withTexLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        candidatePaths.push_back(withTexLower);

        std::string normLower = norm;
        std::transform(normLower.begin(), normLower.end(), normLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        candidatePaths.push_back(normLower);

        std::vector<uint8_t> buffer;
        for (const auto& path : candidatePaths) {
            RE::BSResourceNiBinaryStream stream(path.c_str());
            if (stream.good() && stream.stream) {
                RE::BSResource::Info info{};
                if (stream.stream->DoGetInfo(info) == RE::BSResource::ErrorCode::kNone && info.fileSize >= 128 && info.fileSize < 100000000) {
                    buffer.resize(static_cast<size_t>(info.fileSize));
                    uint64_t bytesRead = 0;
                    auto err = stream.stream->DoRead(buffer.data(), info.fileSize, bytesRead);
                    if (err == RE::BSResource::ErrorCode::kNone && bytesRead >= 128) {
                        buffer.resize(static_cast<size_t>(bytesRead));
                        break;
                    }
                    buffer.clear();
                }

                uint8_t chunk[65536];
                while (buffer.size() < 30 * 1024 * 1024) {
                    uint64_t chunkRead = 0;
                    auto err = stream.stream->DoRead(chunk, sizeof(chunk), chunkRead);
                    if (chunkRead > 0) {
                        buffer.insert(buffer.end(), chunk, chunk + chunkRead);
                    }
                    if (err != RE::BSResource::ErrorCode::kNone || chunkRead == 0) {
                        break;
                    }
                }
                if (buffer.size() >= 128) {
                    break;
                }
                buffer.clear();
            }
        }

        if (buffer.size() < 128) {
            for (const auto& path : candidatePaths) {
                std::string testPaths[2] = { "Data\\" + path, path };
                for (const auto& tp : testPaths) {
                    std::ifstream file(tp, std::ios::binary | std::ios::ate);
                    if (file.is_open()) {
                        auto sz = file.tellg();
                        if (sz >= 128 && sz < 100000000) {
                            buffer.resize(static_cast<size_t>(sz));
                            file.seekg(0, std::ios::beg);
                            if (file.read(reinterpret_cast<char*>(buffer.data()), sz)) {
                                break;
                            }
                        }
                        buffer.clear();
                    }
                }
                if (buffer.size() >= 128) {
                    break;
                }
            }
        }

        if (buffer.size() < 128) {
            logger::warn("[Thumbnail] Failed to read DDS data for texture: {}", a_texturePath);
            m_textureCache[cacheKey] = nullptr;
            return nullptr;
        }

        uint32_t magic = *reinterpret_cast<const uint32_t*>(buffer.data());
        if (magic != 0x20534444) {
            logger::warn("[Thumbnail] Invalid DDS magic 0x{:08X} for {}", magic, a_texturePath);
            m_textureCache[cacheKey] = nullptr;
            return nullptr;
        }

        const uint32_t* h = reinterpret_cast<const uint32_t*>(buffer.data() + 4);
        uint32_t height = h[2];
        uint32_t width = h[3];
        uint32_t pfFlags = h[19];
        uint32_t fourCC = h[20];
        uint32_t rgbBits = h[21];
        uint32_t rMask = h[22];

        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        uint32_t blockSize = 0;
        bool isBC = false;
        size_t dataOffset = 128;

        if (pfFlags & 0x00000004) {
            if (fourCC == 0x31545844) {
                format = DXGI_FORMAT_BC1_UNORM;
                blockSize = 8;
                isBC = true;
            } else if (fourCC == 0x32545844 || fourCC == 0x33545844) {
                format = DXGI_FORMAT_BC2_UNORM;
                blockSize = 16;
                isBC = true;
            } else if (fourCC == 0x34545844 || fourCC == 0x35545844) {
                format = DXGI_FORMAT_BC3_UNORM;
                blockSize = 16;
                isBC = true;
            } else if (fourCC == 0x55344342 || fourCC == 0x53344342 || fourCC == 0x31495441) {
                format = DXGI_FORMAT_BC4_UNORM;
                blockSize = 8;
                isBC = true;
            } else if (fourCC == 0x55354342 || fourCC == 0x53354342 || fourCC == 0x32495441) {
                format = DXGI_FORMAT_BC5_UNORM;
                blockSize = 16;
                isBC = true;
            } else if (fourCC == 0x00374342 || fourCC == 0x37434220 || fourCC == 0x55374342) {
                format = DXGI_FORMAT_BC7_UNORM;
                blockSize = 16;
                isBC = true;
            } else if (fourCC == 0x30315844) {
                if (buffer.size() < 148) {
                    m_textureCache[cacheKey] = nullptr;
                    return nullptr;
                }
                dataOffset = 148;
                uint32_t dxgi = *reinterpret_cast<const uint32_t*>(buffer.data() + 128);
                format = static_cast<DXGI_FORMAT>(dxgi);
                if (format == DXGI_FORMAT_BC1_UNORM || format == DXGI_FORMAT_BC1_UNORM_SRGB) {
                    format = DXGI_FORMAT_BC1_UNORM;
                    blockSize = 8; isBC = true;
                } else if (format == DXGI_FORMAT_BC2_UNORM || format == DXGI_FORMAT_BC2_UNORM_SRGB) {
                    format = DXGI_FORMAT_BC2_UNORM;
                    blockSize = 16; isBC = true;
                } else if (format == DXGI_FORMAT_BC3_UNORM || format == DXGI_FORMAT_BC3_UNORM_SRGB) {
                    format = DXGI_FORMAT_BC3_UNORM;
                    blockSize = 16; isBC = true;
                } else if (format == DXGI_FORMAT_BC4_UNORM || format == DXGI_FORMAT_BC4_SNORM) {
                    format = DXGI_FORMAT_BC4_UNORM;
                    blockSize = 8; isBC = true;
                } else if (format == DXGI_FORMAT_BC5_UNORM || format == DXGI_FORMAT_BC5_SNORM) {
                    format = DXGI_FORMAT_BC5_UNORM;
                    blockSize = 16; isBC = true;
                } else if (format == DXGI_FORMAT_BC7_UNORM || format == DXGI_FORMAT_BC7_UNORM_SRGB) {
                    format = DXGI_FORMAT_BC7_UNORM;
                    blockSize = 16; isBC = true;
                } else if (format == DXGI_FORMAT_R8G8B8A8_UNORM || format == DXGI_FORMAT_B8G8R8A8_UNORM) {
                    blockSize = 4; isBC = false;
                }
            }
        }
        if (format == DXGI_FORMAT_UNKNOWN && (pfFlags & 0x00000040)) {
            if (rgbBits == 32) {
                format = (rMask == 0x00FF0000) ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
                blockSize = 4;
                isBC = false;
            } else if (rgbBits == 24 && buffer.size() >= dataOffset + width * height * 3) {
                std::vector<uint8_t> rgba(width * height * 4);
                const uint8_t* src = buffer.data() + dataOffset;
                for (uint32_t i = 0; i < width * height; ++i) {
                    rgba[i * 4 + 0] = (rMask == 0x00FF0000) ? src[i * 3 + 2] : src[i * 3 + 0];
                    rgba[i * 4 + 1] = src[i * 3 + 1];
                    rgba[i * 4 + 2] = (rMask == 0x00FF0000) ? src[i * 3 + 0] : src[i * 3 + 2];
                    rgba[i * 4 + 3] = 255;
                }
                buffer = std::move(rgba);
                dataOffset = 0;
                format = DXGI_FORMAT_R8G8B8A8_UNORM;
                blockSize = 4;
                isBC = false;
            }
        }

        if (format == DXGI_FORMAT_UNKNOWN || width == 0 || height == 0) {
            logger::warn("[Thumbnail] Unsupported DDS format for {}: fourCC 0x{:08X}, pfFlags 0x{:08X}", a_texturePath, fourCC, pfFlags);
            m_textureCache[cacheKey] = nullptr;
            return nullptr;
        }

        uint32_t pitch = 0;
        uint32_t mipBytes = 0;
        if (isBC) {
            uint32_t numBlocksWide = (std::max)(1u, (width + 3) / 4);
            uint32_t numBlocksHigh = (std::max)(1u, (height + 3) / 4);
            pitch = numBlocksWide * blockSize;
            mipBytes = pitch * numBlocksHigh;
        } else {
            pitch = width * blockSize;
            mipBytes = pitch * height;
        }

        if (buffer.size() < dataOffset + mipBytes) {
            logger::warn("[Thumbnail] DDS data too small for {}: buffer {} < required {}", a_texturePath, buffer.size(), dataOffset + mipBytes);
            m_textureCache[cacheKey] = nullptr;
            return nullptr;
        }

        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem = buffer.data() + dataOffset;
        initData.SysMemPitch = pitch;

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = format;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        ComPtr<ID3D11Texture2D> tex2D;
        HRESULT hr = m_device->CreateTexture2D(&desc, &initData, &tex2D);
        if (FAILED(hr) || !tex2D) {
            logger::warn("[Thumbnail] CreateTexture2D failed (0x{:08X}) for {}", static_cast<uint32_t>(hr), a_texturePath);
            m_textureCache[cacheKey] = nullptr;
            return nullptr;
        }

        ComPtr<ID3D11ShaderResourceView> srv;
        hr = m_device->CreateShaderResourceView(tex2D.Get(), nullptr, &srv);
        if (FAILED(hr) || !srv) {
            logger::warn("[Thumbnail] CreateShaderResourceView failed (0x{:08X}) for {}", static_cast<uint32_t>(hr), a_texturePath);
            m_textureCache[cacheKey] = nullptr;
            return nullptr;
        }

        logger::info("[Thumbnail] Successfully loaded DDS texture: {} ({}x{}, format {})", a_texturePath, width, height, static_cast<uint32_t>(format));
        m_textureCache[cacheKey] = srv;
        return srv.Get();
    }

    bool ItemPreviewRenderer::RenderItemToBuffer(RE::TESBoundObject* a_item,
                                                std::vector<uint8_t>& outRgbaPixels,
                                                uint32_t& outWidth,
                                                uint32_t& outHeight)
    {
        if (!m_initialized || !a_item) return false;

        auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
        if (!renderer) return false;

        auto* rendererData = RE::BSGraphics::Renderer::GetRendererDataSingleton();
        if (!rendererData || !rendererData->context) return false;

        ID3D11DeviceContext* context = reinterpret_cast<ID3D11DeviceContext*>(rendererData->context);

        ItemModelData modelData = ItemMeshExtractor::ExtractItemModel(a_item);
        if (!modelData.valid || modelData.submeshes.empty()) return false;

        ID3D11RenderTargetView* prevRTV = nullptr;
        ID3D11DepthStencilView* prevDSV = nullptr;
        context->OMGetRenderTargets(1, &prevRTV, &prevDSV);

        ID3D11BlendState* prevBlendState = nullptr;
        float prevBlendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        UINT prevSampleMask = 0xFFFFFFFF;
        context->OMGetBlendState(&prevBlendState, prevBlendFactor, &prevSampleMask);

        ID3D11DepthStencilState* prevDSS = nullptr;
        UINT prevStencilRef = 0;
        context->OMGetDepthStencilState(&prevDSS, &prevStencilRef);

        ID3D11RasterizerState* prevRS = nullptr;
        context->RSGetState(&prevRS);

        ID3D11VertexShader* prevVS = nullptr;
        ID3D11ClassInstance* prevVSClassInstances[1] = { nullptr };
        UINT prevVSNumInstances = 0;
        context->VSGetShader(&prevVS, prevVSClassInstances, &prevVSNumInstances);

        ID3D11PixelShader* prevPS = nullptr;
        ID3D11ClassInstance* prevPSClassInstances[1] = { nullptr };
        UINT prevPSNumInstances = 0;
        context->PSGetShader(&prevPS, prevPSClassInstances, &prevPSNumInstances);

        ID3D11InputLayout* prevLayout = nullptr;
        context->IAGetInputLayout(&prevLayout);

        D3D11_PRIMITIVE_TOPOLOGY prevTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        context->IAGetPrimitiveTopology(&prevTopology);

        ID3D11Buffer* prevVSCB = nullptr;
        context->VSGetConstantBuffers(0, 1, &prevVSCB);

        ID3D11Buffer* prevPSCB = nullptr;
        context->PSGetConstantBuffers(0, 1, &prevPSCB);

        ID3D11SamplerState* prevSampler = nullptr;
        context->PSGetSamplers(0, 1, &prevSampler);

        D3D11_VIEWPORT prevViewport{};
        UINT numViewports = 1;
        context->RSGetViewports(&numViewports, &prevViewport);

        D3D11_VIEWPORT vp{};
        vp.Width = static_cast<float>(m_width);
        vp.Height = static_cast<float>(m_height);
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        context->RSSetViewports(1, &vp);

        ID3D11RenderTargetView* rtvs[1] = { m_renderTargetView.Get() };
        context->OMSetRenderTargets(1, rtvs, m_depthStencilView.Get());

        float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        context->OMSetBlendState(m_blendState.Get(), blendFactor, 0xFFFFFFFF);

        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
        context->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        float cx = modelData.boundingCenter[0];
        float cy = modelData.boundingCenter[1];
        float cz = modelData.boundingCenter[2];
        float radius = modelData.boundingRadius > 0.001f ? modelData.boundingRadius : (modelData.maxExtent * 0.65f);
        if (radius < 0.1f) radius = 5.0f;

        float fov = XMConvertToRadians(24.0f);
        float dist = (radius / std::tan(fov * 0.5f)) * 1.25f;

        XMVECTOR targetPos = XMVectorSet(cx, cy, cz, 1.0f);
        XMVECTOR eyeDir = XMVector3Normalize(XMVectorSet(-0.25f, 0.92f, 0.22f, 0.0f));
        XMVECTOR eyePos = XMVectorAdd(targetPos, XMVectorScale(eyeDir, dist));
        XMVECTOR upVec = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

        XMMATRIX viewMat = XMMatrixLookAtLH(eyePos, targetPos, upVec);
        XMMATRIX projMat = XMMatrixPerspectiveFovLH(fov, 1.0f, 0.1f, dist * 5.0f);
        XMMATRIX viewProjMat = XMMatrixMultiply(viewMat, projMat);

        context->IASetInputLayout(m_inputLayout.Get());
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
        context->PSSetShader(m_pixelShader.Get(), nullptr, 0);

        ID3D11Buffer* cbs[1] = { m_constantBuffer.Get() };
        context->VSSetConstantBuffers(0, 1, cbs);
        context->PSSetConstantBuffers(0, 1, cbs);

        ID3D11SamplerState* samplers[1] = { m_samplerState.Get() };
        context->PSSetSamplers(0, 1, samplers);

        context->RSSetState(m_rasterizerState.Get());
        context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

        for (const auto& submesh : modelData.submeshes) {
            if (submesh.vertices.empty() || submesh.indices.empty()) continue;

            D3D11_MAPPED_SUBRESOURCE cbMapped;
            if (SUCCEEDED(context->Map(m_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &cbMapped))) {
                auto* pData = static_cast<ShaderConstants*>(cbMapped.pData);
                XMStoreFloat4x4(&pData->viewProj, XMMatrixTranspose(viewProjMat));
                pData->lightDir = XMFLOAT3(-0.35f, 0.75f, 0.55f);
                pData->ambient = 0.22f;
                XMStoreFloat3(&pData->eyePos, eyePos);
                pData->specularStrength = submesh.specularStrength;
                pData->hasAlphaTest = submesh.hasAlphaTest ? 1 : 0;
                pData->isHair = submesh.isHair ? 1 : 0;
                pData->specularPower = submesh.specularPower;
                pData->emissiveMultiple = submesh.emissiveMultiple;
                pData->tintColor = XMFLOAT4(submesh.tintColor[0], submesh.tintColor[1], submesh.tintColor[2], submesh.tintColor[3]);
                pData->emissiveColor = XMFLOAT4(submesh.emissiveColor[0], submesh.emissiveColor[1], submesh.emissiveColor[2], 1.0f);
                context->Unmap(m_constantBuffer.Get(), 0);
            }

            D3D11_BUFFER_DESC vbDesc{};
            vbDesc.ByteWidth = static_cast<UINT>(submesh.vertices.size() * sizeof(ItemVertex));
            vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
            vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA vbData{};
            vbData.pSysMem = submesh.vertices.data();

            ComPtr<ID3D11Buffer> vb;
            if (FAILED(m_device->CreateBuffer(&vbDesc, &vbData, &vb))) continue;

            D3D11_BUFFER_DESC ibDesc{};
            ibDesc.ByteWidth = static_cast<UINT>(submesh.indices.size() * sizeof(uint32_t));
            ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
            ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

            D3D11_SUBRESOURCE_DATA ibData{};
            ibData.pSysMem = submesh.indices.data();

            ComPtr<ID3D11Buffer> ib;
            if (FAILED(m_device->CreateBuffer(&ibDesc, &ibData, &ib))) continue;

            UINT stride = sizeof(ItemVertex);
            UINT offset = 0;
            ID3D11Buffer* vbs[1] = { vb.Get() };
            context->IASetVertexBuffers(0, 1, vbs, &stride, &offset);
            context->IASetIndexBuffer(ib.Get(), DXGI_FORMAT_R32_UINT, 0);

            ID3D11ShaderResourceView* srv = nullptr;
            if (!submesh.diffuseTexture.empty()) {
                srv = GetOrCreateTexture(submesh.diffuseTexture);
            }
            if (!srv && submesh.textureSRV) {
                srv = submesh.textureSRV;
            }
            if (!srv) {
                srv = m_whiteTextureSRV.Get();
            }
            context->PSSetShaderResources(0, 1, &srv);

            context->DrawIndexed(static_cast<UINT>(submesh.indices.size()), 0, 0);
        }

        context->CopyResource(m_stagingTexture.Get(), m_renderTargetTexture.Get());
        context->Flush();

        D3D11_MAPPED_SUBRESOURCE mapped{};
        bool readSuccess = false;
        HRESULT mapHr = E_FAIL;
        for (int retry = 0; retry < 5; ++retry) {
            mapHr = context->Map(m_stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
            if (SUCCEEDED(mapHr)) break;
            if (mapHr != DXGI_ERROR_WAS_STILL_DRAWING) break;
            context->Flush();
            std::this_thread::yield();
        }
        if (SUCCEEDED(mapHr)) {
            outWidth = m_width;
            outHeight = m_height;
            outRgbaPixels.resize(m_width * m_height * 4);

            for (uint32_t y = 0; y < m_height; ++y) {
                std::memcpy(outRgbaPixels.data() + y * m_width * 4,
                            static_cast<const uint8_t*>(mapped.pData) + y * mapped.RowPitch,
                            m_width * 4);
            }
            context->Unmap(m_stagingTexture.Get(), 0);
            readSuccess = true;
        }

        ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
        context->PSSetShaderResources(0, 1, nullSRV);

        context->OMSetRenderTargets(1, &prevRTV, prevDSV);
        if (prevRTV) prevRTV->Release();
        if (prevDSV) prevDSV->Release();

        context->OMSetBlendState(prevBlendState, prevBlendFactor, prevSampleMask);
        if (prevBlendState) prevBlendState->Release();

        context->OMSetDepthStencilState(prevDSS, prevStencilRef);
        if (prevDSS) prevDSS->Release();

        context->RSSetState(prevRS);
        if (prevRS) prevRS->Release();

        context->VSSetShader(prevVS, nullptr, 0);
        if (prevVS) prevVS->Release();

        context->PSSetShader(prevPS, nullptr, 0);
        if (prevPS) prevPS->Release();

        context->IASetInputLayout(prevLayout);
        if (prevLayout) prevLayout->Release();

        if (prevTopology != D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED) {
            context->IASetPrimitiveTopology(prevTopology);
        }

        context->VSSetConstantBuffers(0, 1, &prevVSCB);
        if (prevVSCB) prevVSCB->Release();

        context->PSSetConstantBuffers(0, 1, &prevPSCB);
        if (prevPSCB) prevPSCB->Release();

        context->PSSetSamplers(0, 1, &prevSampler);
        if (prevSampler) prevSampler->Release();

        if (numViewports > 0) context->RSSetViewports(numViewports, &prevViewport);

        return readSuccess;
    }
}
