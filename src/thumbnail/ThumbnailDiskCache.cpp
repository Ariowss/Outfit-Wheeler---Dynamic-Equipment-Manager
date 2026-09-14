#include "ThumbnailDiskCache.h"

#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

#pragma comment(lib, "windowscodecs.lib")

namespace OutfitWheeler::Thumbnail
{
    using Microsoft::WRL::ComPtr;

    ThumbnailDiskCache& ThumbnailDiskCache::GetSingleton()
    {
        static ThumbnailDiskCache instance;
        return instance;
    }

    ThumbnailDiskCache::ThumbnailDiskCache()
    {
        m_baseDir = std::filesystem::path("Data") / "PrismaUI" / "views" / "PrismaOutfitWheel" / "thumbnails";
        std::error_code ec;
        std::filesystem::create_directories(m_baseDir, ec);
        RescanCache();
    }

    void ThumbnailDiskCache::SetBaseDirectory(const std::filesystem::path& path)
    {
        m_baseDir = path;
        std::error_code ec;
        std::filesystem::create_directories(m_baseDir, ec);
        RescanCache();
    }

    void ThumbnailDiskCache::RescanCache()
    {
        std::unique_lock<std::shared_mutex> lock(m_cacheMutex);
        m_cachedFileNames.clear();
        std::error_code ec;

        if (std::filesystem::exists(m_baseDir, ec) && std::filesystem::is_directory(m_baseDir, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(m_baseDir, ec)) {
                if (entry.is_regular_file(ec) && entry.file_size(ec) > 0) {
                    m_cachedFileNames.insert(entry.path().filename().string());
                }
            }
        }
    }

    const std::filesystem::path& ThumbnailDiskCache::GetBaseDirectory() const
    {
        return m_baseDir;
    }

    std::string ThumbnailDiskCache::GenerateFileName(const std::string& modName, uint32_t localFormID) const
    {
        std::string cleanMod = modName.empty() ? "Skyrim.esm" : modName;
        for (char& c : cleanMod) {
            if (c == ' ' || c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
                c = '_';
            }
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "_%08X.png", localFormID);
        return cleanMod + buf;
    }

    std::filesystem::path ThumbnailDiskCache::GetThumbnailPath(const std::string& modName, uint32_t localFormID) const
    {
        return m_baseDir / GenerateFileName(modName, localFormID);
    }

    std::string ThumbnailDiskCache::GetRelativePath(const std::string& modName, uint32_t localFormID) const
    {
        return "thumbnails/" + GenerateFileName(modName, localFormID);
    }

    bool ThumbnailDiskCache::HasThumbnail(const std::string& modName, uint32_t localFormID) const
    {
        std::string fileName = GenerateFileName(modName, localFormID);
        std::shared_lock<std::shared_mutex> lock(m_cacheMutex);
        return m_cachedFileNames.contains(fileName);
    }

    struct ScopedCOM {
        HRESULT hr;
        ScopedCOM() : hr(::CoInitializeEx(nullptr, COINIT_MULTITHREADED)) {}
        ~ScopedCOM() {
            if (SUCCEEDED(hr)) {
                ::CoUninitialize();
            }
        }
    };

    bool ThumbnailDiskCache::SaveRgbaToPng(const std::filesystem::path& destPath, const uint8_t* rgbaPixels, uint32_t width, uint32_t height)
    {
        if (!rgbaPixels || width == 0 || height == 0) return false;

        std::error_code ec;
        if (destPath.has_parent_path()) {
            std::filesystem::create_directories(destPath.parent_path(), ec);
        }

        ScopedCOM scopedCom;

        ComPtr<IWICImagingFactory> factory;
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory));

        if (FAILED(hr) || !factory) return false;

        ComPtr<IWICStream> stream;
        hr = factory->CreateStream(&stream);
        if (FAILED(hr) || !stream) return false;

        hr = stream->InitializeFromFilename(destPath.wstring().c_str(), GENERIC_WRITE);
        if (FAILED(hr)) return false;

        ComPtr<IWICBitmapEncoder> encoder;
        hr = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
        if (FAILED(hr) || !encoder) return false;

        hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
        if (FAILED(hr)) return false;

        ComPtr<IWICBitmapFrameEncode> frame;
        ComPtr<IPropertyBag2> props;
        hr = encoder->CreateNewFrame(&frame, &props);
        if (FAILED(hr) || !frame) return false;

        hr = frame->Initialize(props.Get());
        if (FAILED(hr)) return false;

        hr = frame->SetSize(width, height);
        if (FAILED(hr)) return false;

        WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat32bppRGBA;
        hr = frame->SetPixelFormat(&pixelFormat);
        if (FAILED(hr)) return false;

        const UINT stride = width * 4;
        const UINT bufferSize = stride * height;

        const uint8_t* writePtr = rgbaPixels;
        std::vector<uint8_t> convBuffer;

        if (pixelFormat == GUID_WICPixelFormat32bppBGRA) {
            convBuffer.resize(bufferSize);
            for (size_t i = 0; i < bufferSize; i += 4) {
                convBuffer[i + 0] = rgbaPixels[i + 2];
                convBuffer[i + 1] = rgbaPixels[i + 1];
                convBuffer[i + 2] = rgbaPixels[i + 0];
                convBuffer[i + 3] = rgbaPixels[i + 3];
            }
            writePtr = convBuffer.data();
        }

        hr = frame->WritePixels(height, stride, bufferSize, const_cast<BYTE*>(writePtr));
        if (FAILED(hr)) return false;

        hr = frame->Commit();
        if (FAILED(hr)) return false;

        hr = encoder->Commit();
        if (!SUCCEEDED(hr)) return false;

        {
            std::unique_lock<std::shared_mutex> lock(m_cacheMutex);
            m_cachedFileNames.insert(destPath.filename().string());
        }

        return true;
    }
}
