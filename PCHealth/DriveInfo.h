#pragma once
#include "DriveTechnology.h"
#include "LibraryPathes.h"

#include <string>
#include <mutex>

namespace pchealth::filesystem
{
    class DriveInfo
    {
    public:
        DriveInfo() = default;
        DriveInfo(const std::wstring& driveName);

        static std::vector<DriveInfo> GetDrives();

        int64_t capacity() const;
        int64_t totalUsedSpace() const;
        std::wstring name() const;
        DriveTechnology technology() const;
        bool isMainDrive() const;

        /**
         * @brief Returns the size in bit of the recycle bin for this drive.
        */
        uint64_t getRecycleBinSize() const;
        /**
         * @brief Tries to find the windows libraries for this drive or user created ones.
        */
        LibraryPathes getLibraries() const;
        /**
         * @brief Computes the size of the files matching the appropriate regex in the map.
         * @param extensionsRegices 
        */
        std::unordered_map<std::wstring, uint64_t> getExtensionsStats(std::unordered_map<std::wstring, uint64_t> extensionsRegices);

    private:
        int64_t _capacity =  0;
        int64_t _totalUsedSpace = 0;
        std::wstring driveName{};
        DriveTechnology driveTech = DriveTechnology::Unknown;

        void getDirStats(std::unordered_map<std::wstring, uint64_t>& extensions, const std::wstring& path, std::timed_mutex* timedMutex, const bool& listAll);
    };
}

