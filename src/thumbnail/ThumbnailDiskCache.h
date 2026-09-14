#pragma once

#include <string>
#include <filesystem>
#include <cstdint>
#include <vector>
#include <unordered_set>
#include <shared_mutex>

namespace OutfitWheeler::Thumbnail
{
    class ThumbnailDiskCache
    {
    public:
        static ThumbnailDiskCache& GetSingleton();

        void SetBaseDirectory(const std::filesystem::path& path);
        [[nodiscard]] const std::filesystem::path& GetBaseDirectory() const;

        [[nodiscard]] std::string GenerateFileName(const std::string& modName, uint32_t localFormID) const;

        [[nodiscard]] std::filesystem::path GetThumbnailPath(const std::string& modName, uint32_t localFormID) const;

        [[nodiscard]] std::string GetRelativePath(const std::string& modName, uint32_t localFormID) const;

        [[nodiscard]] bool HasThumbnail(const std::string& modName, uint32_t localFormID) const;

        bool SaveRgbaToPng(const std::filesystem::path& destPath, const uint8_t* rgbaPixels, uint32_t width, uint32_t height);

        void RescanCache();

    private:
        ThumbnailDiskCache();
        ~ThumbnailDiskCache() = default;

        ThumbnailDiskCache(const ThumbnailDiskCache&) = delete;
        ThumbnailDiskCache& operator=(const ThumbnailDiskCache&) = delete;

        std::filesystem::path m_baseDir;
        mutable std::shared_mutex m_cacheMutex;
        mutable std::unordered_set<std::string> m_cachedFileNames;
    };
}
