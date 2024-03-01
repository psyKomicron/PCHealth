#pragma once

#include <filesystem>
#include <vector>
#include <regex>

namespace pchealth::filesystem
{
    class Directory
    {
    public:
        Directory(const std::wstring& path);

        std::wstring name();
        std::filesystem::path path();
        uint64_t size();

        std::vector<std::pair<std::filesystem::path, bool>> enumerate(const std::filesystem::path& subFolder = {});
        std::vector<std::pair<std::filesystem::path, bool>> find(const std::wregex& query, const std::filesystem::path& subFolder = {});

    private:
        std::filesystem::path dirPath;
    };
}
