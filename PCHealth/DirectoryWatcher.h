#pragma once

#include "Win32FileInformation.h"

#include <functional>

namespace pchealth::filesystem
{
    class DirectoryWatcher
    {
    public:
        DirectoryWatcher(const std::wstring& path, const std::function<void(std::wstring)>& callback);
        ~DirectoryWatcher();

        std::wstring path() const;
        void startWatching();
        void stopWatching();

    private:
        std::wstring _path{};
        std::function<void(std::wstring)> callbackFunc{};
        std::thread notificationThread{};
        std::atomic_bool shutdownFlag = false;
        std::atomic_bool threadStarted = false;
        HANDLE directoryHandle = INVALID_HANDLE_VALUE;
        std::vector<Win32FileInformation> oldEntries{};

        void _threadFunc();
        void fireCallback(FILE_NOTIFY_INFORMATION* info);
        std::vector<std::wstring> detectDirectoryChanges(const std::optional<FILE_NOTIFY_INFORMATION*> fileInformation);
        std::vector<Win32FileInformation> enumerateDirectory();
    };
}