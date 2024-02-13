#pragma once

#include "Win32FileInformation.h"

#include <concurrent_vector.h>
#include <functional>

#define TEST

namespace pchealth::filesystem
{
    class DirectoryWatcher
    {
    public:
        DirectoryWatcher(const std::wstring& path, const std::function<void(std::vector<pchealth::filesystem::Win32FileInformation>&)>& callback);
        ~DirectoryWatcher();

        std::wstring path() const;
        void startWatching();
        void stopWatching();

    private:
        std::wstring _path{};
        std::function<void(std::vector<pchealth::filesystem::Win32FileInformation>&)> callbackFunc{};
        std::thread notificationThread{};
        std::atomic_bool threadRunning = false;
        std::atomic_flag threadExitFlag{};
        HANDLE directoryHandle = INVALID_HANDLE_VALUE;
        concurrency::concurrent_vector<Win32FileInformation> oldEntries{};

        std::vector<Win32FileInformation> detectDirectoryChanges
#ifdef TEST
        (const std::array<FILE_NOTIFY_EXTENDED_INFORMATION, 100>& fileInformations, const uint64_t& count);
#else
        (const std::array<FILE_NOTIFY_INFORMATION, 100>& fileInformations, const uint64_t& count);
#endif
        void enumerateDirectory(concurrency::concurrent_vector<Win32FileInformation>& entries);
        void flattenDirectory(const std::wstring& path, concurrency::concurrent_vector<Win32FileInformation>& entries);
        void fireCallback(std::vector<pchealth::filesystem::Win32FileInformation>& info);
        void _threadFunc();
    };
}