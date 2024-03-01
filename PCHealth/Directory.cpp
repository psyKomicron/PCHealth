#include "pch.h"
#include "Directory.h"

#include <functional>

using HandlePtr = std::unique_ptr<void, std::function<bool(HANDLE)>>;

namespace pchealth::filesystem
{
    Directory::Directory(const std::wstring& path)
    {
        dirPath = std::filesystem::path(path);
    }

    std::vector<std::pair<std::filesystem::path, bool>> Directory::enumerate(const std::filesystem::path& subFolder)
    {
        auto&& entries = std::vector<std::pair<std::filesystem::path, bool>>();

        std::filesystem::path path{ dirPath };
        if (!subFolder.empty())
        {
            path /= subFolder;
        }

        WIN32_FIND_DATA findData{};
        HANDLE findHandle = FindFirstFile((path / L"*").c_str(), &findData);
        if (findHandle != INVALID_HANDLE_VALUE)
        {
            HandlePtr findHandlePtr{ findHandle, FindClose };
            do
            {
                std::wstring fileName = std::wstring(findData.cFileName);
                if (fileName != L"." && fileName != L"..")
                {
                    std::filesystem::path filePath{ path };
                    filePath /= fileName;
                    bool isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
                    auto&& pair = std::make_pair(filePath, isDirectory);
                    entries.push_back(pair);
                }
            } while (FindNextFile(findHandle, &findData));
        }

        return entries;
    }

    std::vector<std::pair<std::filesystem::path, bool>> Directory::find(const std::wregex& query, const std::filesystem::path& subFolder)
    {
        std::vector<std::pair<std::filesystem::path, bool>> results{};

        std::filesystem::path path{ dirPath };
        if (!subFolder.empty())
        {
            path /= subFolder;
        }

        WIN32_FIND_DATA findData{};
        HANDLE findHandle = FindFirstFile((path / L"*").c_str(), &findData);
        if (findHandle != INVALID_HANDLE_VALUE)
        {
            HandlePtr findHandlePtr{ findHandle, FindClose };
            do
            {
                std::wstring fileName = std::wstring(findData.cFileName);
                if (fileName != L"." && fileName != L".." && std::regex_search(fileName, query))
                {
                    std::filesystem::path filePath = path / fileName;
                    bool isDirectory = findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
                    results.push_back(std::make_pair(filePath, isDirectory));
                }
            } while (FindNextFile(findHandle, &findData));
        }

        return results;
    }
}
