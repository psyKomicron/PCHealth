#include "pch.h"

#include "DirectorySizeCalculator.h"
#include "DriveInfo.h"
#include "System.h"

#include <Windows.h>
#include <shlwapi.h>
#include <ppl.h>
#include <filesystem>
#include <regex>

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

    int64_t DriveInfo::capacity()
    {
        return _capacity;
    }

    int64_t DriveInfo::totalUsedSpace() const
    {
        return _totalUsedSpace;
    }

    std::wstring_view DriveInfo::name()
    {
        return driveName;
    }

    DriveTechnology DriveInfo::technology()
    {
        return DriveTechnology();
    }

    bool DriveInfo::isMainDrive()
    {
        return driveName == L"C:\\";
    }

    uint64_t DriveInfo::getRecycleBinSize()
    {
        SHQUERYRBINFO queryBinInfo{ sizeof(SHQUERYRBINFO) };
        SHQueryRecycleBin(driveName.c_str(), &queryBinInfo);
        return queryBinInfo.i64Size;
    }

    LibraryPathes DriveInfo::getLibraries()
    {
        return {};
    }

    void DriveInfo::getExtensionsStats(std::map<std::wstring, uint64_t>* extensions)
    {
        std::timed_mutex timedMutex{};
        getDirStats(extensions, driveName, &timedMutex, extensions->empty());
    }

    void DriveInfo::getDirStats(std::map<std::wstring, uint64_t>* extensions, const std::wstring& path, std::timed_mutex* timedMutex, const bool& listAll)
    {
        if (path.starts_with(L"\\")) // Prevents infinite looping when the directory name is invalid (often buffer too small)
        {
            return;
        }

        WIN32_FIND_DATA findData{};
        HANDLE findHandle = nullptr;
        if (!path.ends_with(L"\\"))
        {
            findHandle = FindFirstFile((path + L"\\*").c_str(), &findData);
        }
        else
        {
            findHandle = FindFirstFile((path + L"*").c_str(), &findData);
        }

        if (findHandle != INVALID_HANDLE_VALUE)
        {
            std::vector<std::wstring> pathes = std::vector<std::wstring>();
            do
            {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    uint64_t fileSize = (static_cast<int64_t>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow;
                    //std::wstring fileName{ findData.cFileName };
                    std::filesystem::path filePath{ findData.cFileName };
                    if (filePath.has_extension())
                    {
                        std::wstring extension = filePath.extension();
                        std::wregex extRegex{ extension };
                        //{ L"^.[A-z]{2,}" };
                        bool hasMatch = false;
                        if (!listAll)
                        {

                        }

                        if (listAll || hasMatch)
                        {
                            using namespace std::chrono_literals;
                            std::unique_lock<std::timed_mutex> lock{ *timedMutex, std::defer_lock };

                            if (lock.try_lock_for(1s))
                            {
                                extensions->insert_or_assign(extension, (extensions->contains(extension) ? extensions->at(extension) : 0) + fileSize);
                            }
#ifdef _DEBUG
                            else
                            {
                                std::filesystem::path root{ path };
                                root /= filePath;
                                OutputDebugString(std::format(L"Failed to take mutex (1 sec) for '{}'.\n", root.generic_wstring()).c_str());
                            }
#endif
                        }
#ifdef _DEBUG
                        else
                        {
                            OutputDebugString(std::format(L"Extension not valid '{}'.\n", extension).c_str());
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

            std::atomic_uint_fast64_t atomicDirSize{};
            if (pathes.size() > 1)
            {
                concurrency::parallel_for_each(begin(pathes), end(pathes), [this, wstr = path, timedMutex, extensions, listAll](const std::wstring& dir)
                {
                    WCHAR combinedPath[2048]{};
                    PathCombine(combinedPath, wstr.c_str(), dir.c_str());
                    std::wstring deepPath = std::wstring(combinedPath) + L"\\";
                    getDirStats(extensions, deepPath, timedMutex, listAll);
                    //TODO: Add the computed sizes to the main map.
                });
            }
            else
            {
                size_t max = pathes.size();
                for (size_t i = 0; i < max; i++)
                {
                    WCHAR combinedPath[2048];
                    PathCombine(combinedPath, path.c_str(), pathes[i].c_str());
                    getDirStats(extensions, std::wstring(combinedPath) + L"\\", timedMutex, listAll);
                }
            }
        }
    }
}