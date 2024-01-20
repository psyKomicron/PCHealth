#pragma once
#include "DriveTechnology.h"
#include "LibraryPathes.h"
#include <string>

namespace Common::Filesystem
{
    class DriveInfo
    {
    public:
        DriveInfo() = default;
        DriveInfo(const std::wstring& driveName);

        static std::vector<Common::Filesystem::DriveInfo> GetDrives();

        int64_t capacity();
        int64_t totalUsedSpace() const;
        std::wstring_view name();
        DriveTechnology technology();
        bool isMainDrive();

        /**
         * @brief Returns the size in bit of the recycle bin for this drive.
        */
        uint64_t getRecycleBinSize();
        /**
         * @brief Tries to find the windows libraries for this drive or user created ones.
        */
        LibraryPathes getLibraries();
        /**
         * @brief Computes the size of the files matching the appropriate regex in the map.
         * @param extensionsRegices 
        */
        void getExtensionsStats(std::unordered_map<std::wstring, uint64_t>* extensionsRegices);

    private:
        int64_t _capacity =  0;
        int64_t _totalUsedSpace = 0;
        std::wstring driveName{};
        DriveTechnology driveTech = DriveTechnology::Unknown;

        void getDirStats(std::unordered_map<std::wstring, uint64_t>* extensions, const std::wstring& path, std::timed_mutex* timedMutex, const bool& listAll);
    };
}

