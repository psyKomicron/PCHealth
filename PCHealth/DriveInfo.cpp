#include "pch.h"
#include "DriveInfo.h"

#include "System.h"
#include "DirectorySizeCalculator.h"
#include "utilities.h"

#include <Windows.h>
#include <shlwapi.h>
#include <ppl.h>
#include <filesystem>
#include <regex>
#include <chrono>

namespace Common::Filesystem
{
    DriveInfo::DriveInfo(const std::wstring& driveName)
    {
        this->driveName = driveName;

        ULARGE_INTEGER totalNumbersOfFreeBytes{}, totalNumberOfBytes{};
        if (GetDiskFreeSpaceExW(
            driveName.c_str(),
            NULL,
            &totalNumberOfBytes,
            &totalNumbersOfFreeBytes))
        {
            _capacity = totalNumbersOfFreeBytes.QuadPart + totalNumberOfBytes.QuadPart;
            _totalUsedSpace = totalNumberOfBytes.QuadPart - totalNumbersOfFreeBytes.QuadPart;
        }
        else
        {
            //TODO: Log or do something.
#ifdef _DEBUG
            throw std::exception("Failed to get disk free space (GetDiskFreeSpaceExW)");
#endif
        }
    }

    std::vector<Common::Filesystem::DriveInfo> DriveInfo::GetDrives()
    {
        WCHAR drives[512]{};
        WCHAR* pointer = drives;
        std::vector<Common::Filesystem::DriveInfo> drivesList{};

        if (GetLogicalDriveStrings(512, drives))
        {
            int max = 256;
            while (max != 0)
            {
                max--;
                if (*pointer == NULL)
                {
                    break;
                }

                drivesList.push_back(Common::Filesystem::DriveInfo(std::wstring(pointer)));

                while (*pointer++);
            }
        }
        else
        {
            throw std::exception("Failed to get logical drives.");
        }

        return drivesList;
    }

    int64_t DriveInfo::capacity() const
    {
        return _capacity;
    }

    int64_t DriveInfo::totalUsedSpace() const
    {
        return _totalUsedSpace;
    }

    std::wstring_view DriveInfo::name() const
    {
        return driveName;
    }

    DriveTechnology DriveInfo::technology() const
    {
        return DriveTechnology();
    }

    bool DriveInfo::isMainDrive() const
    {
        return driveName == L"C:\\";
    }

    uint64_t DriveInfo::getRecycleBinSize() const
    {
        SHQUERYRBINFO queryBinInfo{ sizeof(SHQUERYRBINFO) };
        SHQueryRecycleBin(driveName.c_str(), &queryBinInfo);
        return queryBinInfo.i64Size;
    }

    LibraryPathes DriveInfo::getLibraries() const
    {
        return {};
    }

    std::unordered_map<std::wstring, uint64_t> DriveInfo::getExtensionsStats(std::unordered_map<std::wstring, uint64_t> extensionsRegices)
    {
        auto clock = std::chrono::high_resolution_clock::now();

        std::timed_mutex timedMutex{};
        getDirStats(extensionsRegices, driveName, &timedMutex, extensionsRegices.empty());

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - clock);
        OutputDebug(std::format(L"'{}' Extensions stats : elapsed {}ms.", driveName, duration.count()));

        return extensionsRegices;
    }

    void DriveInfo::getDirStats(std::unordered_map<std::wstring, uint64_t>& extensions, const std::wstring& path, std::timed_mutex* timedMutex, const bool& listAll)
    {
        WIN32_FIND_DATA findData{};
        HANDLE findHandle = nullptr;
        std::wstring searchPath = path;
        if (searchPath[searchPath.size() - 1] != L'\\')
        {
            searchPath.append(1, L'\\');
        }
        findHandle = FindFirstFile((searchPath + L"*").c_str(), &findData);

        if (findHandle != INVALID_HANDLE_VALUE)
        {
            std::vector<std::wstring> pathes = std::vector<std::wstring>();
            do
            {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    uint64_t fileSize = pchealth::utilities::convert(findData.nFileSizeHigh, findData.nFileSizeLow);
                    std::filesystem::path filePath{ findData.cFileName };
                    if (filePath.has_extension())
                    {
                        std::wstring extension = filePath.extension();
                        
                        using namespace std::chrono_literals;
                        std::unique_lock<std::timed_mutex> lock{ *timedMutex, std::defer_lock };
                        if (lock.try_lock_for(333ms))
                        {
                            for (const auto& pair : extensions)
                            {
                                std::wregex re{ pair.first, std::regex_constants::icase };
                                if (std::regex_search(extension, re))
                                {
                                    extensions[pair.first] = pair.second + fileSize;
                                    break;
                                }
                            }
                        }
#ifdef ENABLE_DEBUG_OUTPUT
                        else
                        {
                            std::filesystem::path root{ path };
                            root /= filePath;
                            OutputDebug(std::format(L"Failed to take mutex (333 ms) for '{}'.", root.wstring()));
                        }
#endif
                    }
                }
                else
                {
                    std::wstring directoryName = std::wstring(findData.cFileName);
                    if (directoryName != L"." && directoryName != L".." && directoryName != L"Windows")
                    {
                        pathes.push_back(directoryName);
                    }
                }
            } while (FindNextFile(findHandle, &findData));
            FindClose(findHandle);

            concurrency::parallel_for_each(begin(pathes), end(pathes), [this, wstr = path, timedMutex, &extensions, listAll](const std::wstring& dir)
            {
                std::filesystem::path combinedPath{ wstr };
                combinedPath /= dir;
                getDirStats(extensions, combinedPath, timedMutex, listAll);
            });
        }
    }
}