#pragma once
#include <string>
#include "DriveTechnology.h"
#include "LibraryPathes.h"

namespace Common::Filesystem
{
    class DriveInfo
    {
    public:
        DriveInfo(const std::wstring& driveName);

        static std::vector<Common::Filesystem::DriveInfo> GetDrives();

        int64_t capacity();
        int64_t totalUsedSpace() const;
        std::wstring_view name();
        DriveTechnology technology();
        bool isMainDrive();

        uint64_t getRecycleBinSize();
        LibraryPathes getLibraries();
        void getExtensionsStats(std::map<std::wstring, uint64_t>* extensions);

    private:
        int64_t _capacity;
        int64_t _totalUsedSpace;
        std::wstring driveName;
        DriveTechnology driveTech;
        //std::timed_mutex statsMutex;

        void getDirStats(std::map<std::wstring, uint64_t>* extensions, const std::wstring& path, std::timed_mutex* timedMutex, const bool& listAll);
    };
}

