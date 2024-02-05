#include "pch.h"
#include "DirectoryWatcher.h"

#include "System.h"

namespace pchealth::filesystem
{
    DirectoryWatcher::DirectoryWatcher(const std::wstring& path, const std::function<void(std::wstring)>& callback)
    {
        _path = path;
    }

    DirectoryWatcher::~DirectoryWatcher()
    {
        if (directoryHandle != INVALID_HANDLE_VALUE)
        {
            CancelIoEx(directoryHandle, nullptr);
        }

        if (threadStarted.load())
        {
            shutdownFlag.wait(true);
            CloseHandle(directoryHandle);
            notificationThread.join();
        }
    }

    std::wstring DirectoryWatcher::path() const
    {
        return _path;
    }

    void DirectoryWatcher::startWatching()
    {
        if (!threadStarted.load())
        {
            oldEntries = enumerateDirectory();

            directoryHandle = CreateFile(_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
            if (directoryHandle != INVALID_HANDLE_VALUE)
            {
                notificationThread = std::thread(&DirectoryWatcher::_threadFunc, this);
                OutputDebugString(L"Started watcher.\n");
            }
            else
            {
                winrt::check_hresult(HRESULT_FROM_WIN32(GetLastError()));
            }
        }
    }

    void DirectoryWatcher::stopWatching()
    {
        if (threadStarted.load())
        {
            // TODO: Use RAII to stop watching or function and RAII ?
        }
    }


    void DirectoryWatcher::fireCallback(FILE_NOTIFY_INFORMATION* info)
    {
        // TODO: Call callback and give formatted info the consumer.
    }

    std::vector<std::wstring> DirectoryWatcher::detectDirectoryChanges(const std::optional<FILE_NOTIFY_INFORMATION*> fileInformation)
    {
        if (fileInformation.has_value())
        {
            auto&& fileInfo = fileInformation.value();
            switch (fileInfo->Action)
            {
                case FILE_ACTION_ADDED:
                    OutputDebugString(L"FILE ACTION ADDED\n");
                    break;
                case FILE_ACTION_REMOVED:
                    OutputDebugString(L"FILE ACTION REMOVED\n");
                    break;
                case FILE_ACTION_MODIFIED:
                    OutputDebugString(L"FILE ACTION MODIFIED\n");
                    break;
            }
            return {};
        }
        else
        {
            auto entries = enumerateDirectory();
            if (entries.empty())
            {
                return std::vector<std::wstring>
                {
                    L"*"
                };
            }
            else
            {
                auto changes = std::vector<std::wstring>();
                for (size_t i = 0; i < entries.size(); i++)
                {
                    bool entryRemoved = true;
                    if (!entries[i].directory())
                    {
                        bool newFile = true;
                        for (size_t ii = 0; ii < oldEntries.size(); ii++)
                        {
                            if (entries[i].name() == oldEntries[ii].name())
                            {
                                newFile = false;
                                entryRemoved = false;
                                if (entries[i].size() != oldEntries[ii].size())
                                {
                                    changes.push_back(entries[i].name());

                                    OutputDebugString(
                                        std::format(L"Change detected ({}{} bytes) : '{}'\n", 
                                            (oldEntries[ii].size() < entries[i].size() ? L"+" : L""),
                                            static_cast<int64_t>(entries[i].size()) - oldEntries[ii].size(),
                                            entries[i].name())
                                        .c_str());
                                }
                                break;
                            }
                        }

                        if (newFile)
                        {
                            changes.push_back(entries[i].name());
                            OutputDebugString(std::format(L"New entry detected : '{}'\n", entries[i].name()).c_str());
                        }
                    }
                }

                for (size_t i = 0; i < oldEntries.size(); i++)
                {
                    bool exists = false;
                    for (size_t ii = 0; ii < entries.size(); ii++)
                    {
                        if (oldEntries[i].name() == entries[ii].name())
                        {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists)
                    {
                        OutputDebugString(std::format(L"Entry removed : '{}', -{} bytes.\n", oldEntries[i].name(), oldEntries[i].size()).c_str());
                    }
                }

                oldEntries = entries;
                return changes;
            }
        }
    }

    std::vector<Win32FileInformation> DirectoryWatcher::enumerateDirectory()
    {
        auto entries = std::vector<Win32FileInformation>();
        WIN32_FIND_DATA findData{};
        HANDLE findHandle = nullptr;
        if (!_path.ends_with(L"\\"))
        {
            findHandle = FindFirstFile((_path + L"\\*").c_str(), &findData);
        }
        else
        {
            findHandle = FindFirstFile((_path + L"*").c_str(), &findData);
        }

        if (findHandle != INVALID_HANDLE_VALUE)
        {
            try
            {
                do
                {
                    std::wstring filePath = std::wstring(findData.cFileName);
                    if (filePath != L"." && filePath != L"..")
                    {
                        entries.push_back(Win32FileInformation(&findData));
                    }
                } while (FindNextFile(findHandle, &findData));
            }
            catch (...) { }
            FindClose(findHandle);
        }

        return entries;
    }

    /**
     * @brief Background thread function, do not call directly.
    */
    void DirectoryWatcher::_threadFunc()
    {
        OutputDebugString(L"Notification thread started.\n");
        unsigned long bytesReturned = 0;
        while (!shutdownFlag.load())
        {
            if (ReadDirectoryChangesW(directoryHandle, nullptr, 0, false, FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_FILE_NAME, &bytesReturned, nullptr, nullptr))
            {
                OutputDebugString(std::format(L"'{}' change detected.\n", _path).c_str());
                detectDirectoryChanges({});
            }
            else
            {
                //winrt::check_hresult(HRESULT_FROM_WIN32(GetLastError()));
                break;
            }
        }

        OutputDebugString(L"Notification thread exited.\n");
        threadStarted.exchange(true);
        threadStarted.notify_one();
    }
}
