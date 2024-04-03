#include "pch.h"
#include "System.h"

#include "utilities.h"

#include <winrt/Windows.System.h>
#include <winrt/Windows.Storage.h>

#include <pplawait.h>
#include <shlwapi.h>
#include <processthreadsapi.h>
#include <windows.h>

#include <iostream>

namespace pchealth::windows
{
    int System::GetGeneralHealth()
    {
        return 0;
    }

    concurrency::task<pchealth::filesystem::LibraryPathes> System::GetLibraries()
    {
        return concurrency::create_task([]
        {
            winrt::Windows::System::User user = winrt::Windows::System::User::GetDefault();
            auto&& downloadsFolder = winrt::Windows::Storage::KnownFolders::GetFolderForUserAsync(user, winrt::Windows::Storage::KnownFolderId::DownloadsFolder).get();
            auto&& documentsFolders = winrt::Windows::Storage::KnownFolders::GetFolderForUserAsync(user, winrt::Windows::Storage::KnownFolderId::DocumentsLibrary).get();
            auto&& musicFolders = winrt::Windows::Storage::KnownFolders::GetFolderForUserAsync(user, winrt::Windows::Storage::KnownFolderId::MusicLibrary).get();
            auto&& picturesFolders = winrt::Windows::Storage::KnownFolders::GetFolderForUserAsync(user, winrt::Windows::Storage::KnownFolderId::PicturesLibrary).get();
            auto&& videosFolders = winrt::Windows::Storage::KnownFolders::GetFolderForUserAsync(user, winrt::Windows::Storage::KnownFolderId::VideosLibrary).get();
            auto&& appAllModsFolders = winrt::Windows::Storage::KnownFolders::GetFolderForUserAsync(user, winrt::Windows::Storage::KnownFolderId::AllAppMods).get();
            /*auto&& documentsFolders = GetLibraryFolders(&user, winrt::Windows::Storage::KnownFolderId::DocumentsLibrary).get();
            auto&& musicFolders = GetLibraryFolders(&user, winrt::Windows::Storage::KnownFolderId::MusicLibrary).get();
            auto&& picturesFolders = GetLibraryFolders(&user, winrt::Windows::Storage::KnownFolderId::PicturesLibrary).get();
            auto&& videosFolders = GetLibraryFolders(&user, winrt::Windows::Storage::KnownFolderId::VideosLibrary).get();
            auto&& appAllModsFolders = GetLibraryFolders(&user, winrt::Windows::Storage::KnownFolderId::AllAppMods).get();*/

            return pchealth::filesystem::LibraryPathes(
                std::wstring(downloadsFolder.Path()),
                documentsFolders,
                musicFolders,
                picturesFolders,
                videosFolders
            );
        });
    }

    std::wstring System::GetCurrentUserName()
    {
        DWORD bufferSize{};
        GetUserName(nullptr, &bufferSize);
        LPWSTR userNamePointer = new WCHAR[bufferSize]();
        winrt::check_bool(GetUserName(userNamePointer, &bufferSize));
        std::wstring userName{ userNamePointer };
        delete[] userNamePointer;
        return userName;
    }

    uint64_t System::getFileSize(const std::wstring& filePath)
    {
        WIN32_FIND_DATA findData{};
        HANDLE findHandle = nullptr;
        uint64_t fileSize = 0;

        findHandle = FindFirstFile(filePath.c_str(), &findData);
        if (findHandle != INVALID_HANDLE_VALUE)
        {
            fileSize = (static_cast<int64_t>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow;
            FindClose(findHandle);
        }

        return fileSize;
    }

    bool System::pathExists(const std::wstring& path)
    {
        return PathFileExists(path.c_str());
    }

    void System::openExplorer(std::wstring args)
    {
        STARTUPINFOW info{ sizeof(STARTUPINFOW) };
        PROCESS_INFORMATION processInfo{};
        std::wstring command = std::format(LR"(explorer /select,"{}")", args);
        if (CreateProcessW(nullptr, command.data(), nullptr, nullptr, false, DETACHED_PROCESS, nullptr, nullptr, &info, &processInfo))
        {
            CloseHandle(processInfo.hProcess);
            CloseHandle(processInfo.hThread);
        }
        else
        {
            winrt::throw_last_error();
        }
    }

    void System::launch(const std::wstring path)
    {
        OUTPUT_DEBUG(L"[System] WARNING! System::launch is not implemented.");
        return;
        STARTUPINFOW info{ sizeof(STARTUPINFOW) };
        PROCESS_INFORMATION processInfo{};
        std::wstring command = std::format(LR"(&"{}")", path);
        if (CreateProcessW(path.data(), nullptr, nullptr, nullptr, false, DETACHED_PROCESS, nullptr, nullptr, &info, &processInfo))
        {
            CloseHandle(processInfo.hProcess);
            CloseHandle(processInfo.hThread);
            OUTPUT_DEBUG(std::format(L"[System] Created process: {}", command));
        }
        else
        {
            winrt::throw_last_error();
        }
    }

    concurrency::task<std::vector<std::wstring>> System::GetLibraryFolders(winrt::Windows::System::User* user, const winrt::Windows::Storage::KnownFolderId& id)
    {
        return concurrency::create_task([user, id]
        {
            auto&& libFolder = winrt::Windows::Storage::KnownFolders::GetFolderForUserAsync(*user, id).get();
            auto&& libFolders = libFolder.GetFoldersAsync().get();

            auto&& foldersList = std::vector<std::wstring>();
            for (auto&& folder : libFolders)
            {
                //documentsFolder.Path()
                foldersList.push_back(std::wstring(folder.Path()));
            }
            return foldersList;
        });
    }
}
