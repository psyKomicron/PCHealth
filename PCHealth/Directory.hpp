#pragma once

#include <filesystem>
#include <vector>
#include <regex>

namespace pchealth::filesystem
{
    class Directory
    {
    public:
        static std::optional<Directory> tryCreateDirectory(const std::wstring& wstring);

        static uint64_t enumerateDirectory(const std::filesystem::path& path, std::vector<std::pair<std::wstring, bool>>& entries);

        Directory() = default;

        Directory(const std::wstring& path);

        std::filesystem::path path() const;

        bool exists() const;

        std::vector<std::pair<std::filesystem::path, bool>> enumerate(const std::filesystem::path& subFolder = {});
        
        std::vector<std::pair<std::filesystem::path, bool>> find(const std::wregex& query, const std::filesystem::path& subFolder = {});

    private:
        std::filesystem::path _path;

        static bool pathFileExists(const std::filesystem::path& path);
    };
}
