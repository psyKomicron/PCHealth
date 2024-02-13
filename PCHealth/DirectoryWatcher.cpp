#include "pch.h"
#include "DirectoryWatcher.h"

#define PARALLEL_FOR_EACH

#include "System.h"
#include "utilities.h"

#include <ppl.h>

#include <filesystem>
#include <chrono>


namespace pchealth::filesystem
{
    DirectoryWatcher::DirectoryWatcher(const std::wstring& path, const std::function<void(std::vector<pchealth::filesystem::Win32FileInformation>&)>& callback)
    {
        _path = path;
        callbackFunc = callback;
    }

    DirectoryWatcher::~DirectoryWatcher()
    {
        if (directoryHandle != INVALID_HANDLE_VALUE)
        {
            threadRunning.exchange(false);
            CancelIoEx(directoryHandle, nullptr);
            threadExitFlag.wait(true);
            if (notificationThread.joinable())
            {
                notificationThread.join();
            }
            CloseHandle(directoryHandle);
        }
    }

    std::wstring DirectoryWatcher::path() const
    {
        return _path;
    }

    void DirectoryWatcher::startWatching()
    {
        if (!threadRunning.load())
        {
            enumerateDirectory(oldEntries);

            directoryHandle = CreateFile(_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
            if (directoryHandle != INVALID_HANDLE_VALUE)
            {
                notificationThread = std::thread(&DirectoryWatcher::_threadFunc, this);
                threadRunning.wait(true);
                OutputDebug(L"Started watcher.");
            }
            else
            {
                winrt::check_hresult(HRESULT_FROM_WIN32(GetLastError()));
            }
        }
    }

    void DirectoryWatcher::stopWatching()
    {
        // TODO: Use RAII to stop watching or function and RAII ?
    }


    std::vector<Win32FileInformation> DirectoryWatcher::detectDirectoryChanges
#ifdef TEST
    (const std::array<FILE_NOTIFY_EXTENDED_INFORMATION, 100>& fileInformations, const uint64_t& count)
#else
    (const std::array<FILE_NOTIFY_INFORMATION, 100>& fileInformations, const uint64_t& count)
#endif
    {
        std::vector<pchealth::filesystem::Win32FileInformation> changes{};
        if (count > 0)
        {
            OutputDebug(L"Received notification for " + std::to_wstring(count) + L" changes.");
            for (size_t i = 0; i < count; i++)
            {
                auto&& fileInfo = fileInformations[i];
                auto&& fileName = std::wstring(fileInfo.FileName);
                try
                {
                    auto&& win32FileInformation = pchealth::filesystem::Win32FileInformation(fileInfo, directoryHandle);

                    switch (fileInfo.Action)
                    {
                        case FILE_ACTION_ADDED:
                            OutputDebug(std::format(L"'{}' file added, +{} bytes to directory size.", win32FileInformation.path(), win32FileInformation.size()));
                            break;
                        case FILE_ACTION_REMOVED:
                            OutputDebug(std::format(L"'{}' file removed, -{} bytes to directory size.", win32FileInformation.path(), win32FileInformation.size()));
                            break;
                        case FILE_ACTION_MODIFIED:
                            OutputDebug(std::format(L"'{}' file modified, new size: {}.", win32FileInformation.path(), win32FileInformation.size()));
                            break;
                    }

                    changes.push_back(win32FileInformation);
                }
                catch (const winrt::hresult_error& err)
                {
                    OutputDebug(std::format(L"Failed to create pchealth::filesystem::Win32FileInformation from FILE_NOTIFY_EXTENDED_INFORMATION : '{}'.", err.message()));
                }
                catch (...)
                {
                    OutputDebug(L"Failed to create pchealth::filesystem::Win32FileInformation from FILE_NOTIFY_EXTENDED_INFORMATION : unknown error.");
                }
            }
        }
        else
        {
            concurrency::concurrent_vector<Win32FileInformation> entries{};
            enumerateDirectory(entries);

            if (!entries.empty())
            {
                for (size_t i = 0; i < entries.size(); i++)
                {
                    if (!entries[i].directory())
                    {
                        bool newFile = true;
                        for (size_t ii = 0; ii < oldEntries.size(); ii++)
                        {
                            if (entries[i].path() == oldEntries[ii].path())
                            {
                                newFile = false;
                                if (entries[i].size() != oldEntries[ii].size())
                                {
                                    changes.push_back(entries[i]);

                                    OutputDebug(
                                        std::format(L"Change detected ({}{} bytes) : '{}'\n", 
                                            (oldEntries[ii].size() < entries[i].size() ? L"+" : L""),
                                            static_cast<int64_t>(entries[i].size()) - oldEntries[ii].size(),
                                            entries[i].name()));
                                }
                                break;
                            }
                        }

                        if (newFile)
                        {
                            changes.push_back(entries[i]);
                            OutputDebug(std::format(L"New entry detected : '{}'\n", entries[i].name()));
                        }
                    }
                }

                for (size_t i = 0; i < oldEntries.size(); i++)
                {
                    bool exists = false;
                    for (size_t ii = 0; ii < entries.size(); ii++)
                    {
                        if (oldEntries[i].path() == entries[ii].path())
                        {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists)
                    {
                        OutputDebug(std::format(L"Entry removed : '{}', -{} bytes.\n", oldEntries[i].name(), oldEntries[i].size()));
                        changes.push_back(oldEntries[i]);
                    }
                }

                oldEntries = entries;
            }    
        }

        return changes;
    }

    void DirectoryWatcher::enumerateDirectory(concurrency::concurrent_vector<Win32FileInformation>& entries)
    {
#ifdef ENABLE_DEBUG_OUTPUT
        auto clock = std::chrono::high_resolution_clock::now();
#endif

        if (!_path.ends_with(L"\\"))
        {
            _path += L"\\";
        }
        flattenDirectory(_path, entries);

#ifdef ENABLE_DEBUG_OUTPUT
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - clock);
        OutputDebug(std::format(L"(watcher '{}') Elapsed {}µs for {} entries.", _path, duration.count(), entries.size()));
#endif
    }

    void DirectoryWatcher::flattenDirectory(const std::wstring& path, concurrency::concurrent_vector<Win32FileInformation>& entries)
    {
        WIN32_FIND_DATA findData{};
        HANDLE findHandle = nullptr;
        findHandle = FindFirstFile(std::format(L"{}*", path).c_str(), &findData);

        std::vector<std::wstring> directories{};
        if (findHandle != INVALID_HANDLE_VALUE)
        {
            try
            {
                do
                {
                    auto fileInfo = Win32FileInformation(findData, path);
                    if (!fileInfo.isNavigator())
                    {
                        if (!fileInfo.directory())
                        {
                            entries.push_back(std::move(fileInfo));
                        }
                        else
                        { 
                            std::filesystem::path parentDirectory{ path };
                            parentDirectory /= fileInfo.name();
                            parentDirectory /= L"";
                            directories.push_back(parentDirectory.wstring());
                        }
                    }
                } while (FindNextFile(findHandle, &findData));
            }
            catch (...) { }
            FindClose(findHandle);

            concurrency::parallel_for_each(std::begin(directories), std::end(directories), [this, &entries] (const std::wstring& directory)
            {
                flattenDirectory(directory, entries);
            });
        }
    }

    void DirectoryWatcher::fireCallback(std::vector<pchealth::filesystem::Win32FileInformation>& info)
    {
        callbackFunc(info);
    }

    /**
     * @brief Background thread function, do not call directly.
    */
    void DirectoryWatcher::_threadFunc()
    {
        OutputDebug(std::format(L"(watcher '{}') Notification thread started.", _path));

        threadRunning.exchange(true);

        while (true)
        {
            unsigned long bytesReturned = 0;
            const uint32_t flags = FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_FILE_NAME;
#ifdef TEST
            FILE_NOTIFY_EXTENDED_INFORMATION extendedChangesBuffer[100]{};

            if (ReadDirectoryChangesExW(directoryHandle, extendedChangesBuffer, sizeof(extendedChangesBuffer), true, flags, &bytesReturned, nullptr, nullptr, READ_DIRECTORY_NOTIFY_INFORMATION_CLASS::ReadDirectoryNotifyExtendedInformation))
            {
                auto&& changes = detectDirectoryChanges(std::to_array(extendedChangesBuffer), bytesReturned / sizeof(FILE_NOTIFY_EXTENDED_INFORMATION));
                fireCallback(changes);
            }
#else
            FILE_NOTIFY_INFORMATION changesBuffer[100]{};
            if (ReadDirectoryChangesW(directoryHandle, changesBuffer, sizeof(changesBuffer), true, flags, &bytesReturned, nullptr, nullptr))
            {
                detectDirectoryChanges(std::to_array(changesBuffer), bytesReturned / sizeof(FILE_NOTIFY_INFORMATION));
            }
#endif // TEST
            else
            {
                /*if (threadRunning.load())
                {
                    winrt::check_hresult(HRESULT_FROM_WIN32(GetLastError()));
                }*/
                break;
            }
        }

        OutputDebug(std::format(L"(watcher '{}') Notification thread exited.", _path));
        //threadRunning.exchange(false);
        threadExitFlag.notify_one();
    }
}
