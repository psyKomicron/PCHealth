#include "pch.h"
#include "Directory.hpp"

#include "utilities.h"

#include "shlwapi.h"

#include <functional>

using HandlePtr = std::unique_ptr<void, std::function<bool(HANDLE)>>;

using namespace pchealth::filesystem;


std::optional<Directory> Directory::tryCreateDirectory(const std::wstring& wstring)
{
    auto path = std::filesystem::path{ wstring };
    if (wstring.size() >= 3)
    {
        if (!pathFileExists(path))
        {
            OUTPUT_DEBUG(L"[Directory] Path doesn't exists, parsing to find valid path.");

            path.remove_filename();
            while(!pathFileExists(path))
            {
                path = path.parent_path();
            }
        
            OUTPUT_DEBUG(std::format(L"[Directory] Valid path: {}", path.wstring()));
        }

        auto&& dir = Directory();
        dir._path = path;
        return dir;
    }

    return std::optional<Directory>();
}

uint64_t pchealth::filesystem::Directory::enumerateDirectory(const std::filesystem::path& path, std::vector<std::pair<std::wstring, bool>>& entries)
{
    uint64_t count = 0;

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
                std::filesystem::path filePath = path / fileName;
                entries.push_back(
                    std::make_pair(filePath, findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                );
                count++;
            }
        } while (FindNextFile(findHandle, &findData));
    }

    return count;
}


Directory::Directory(const std::wstring& path)
{
    _path = std::filesystem::path(path);
}

std::filesystem::path Directory::path() const
{
    return _path;
}

bool Directory::exists() const
{
    return pathFileExists(_path);
}

std::vector<std::pair<std::filesystem::path, bool>> Directory::enumerate(const std::filesystem::path& subFolder)
{
    auto&& entries = std::vector<std::pair<std::filesystem::path, bool>>();

    std::filesystem::path path{ _path };
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

    std::filesystem::path path{ _path };
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
        } 
        while (FindNextFile(findHandle, &findData));
    }

    return results;
}


bool pchealth::filesystem::Directory::pathFileExists(const std::filesystem::path& path)
{
    return PathFileExistsW(path.wstring().c_str());
}
