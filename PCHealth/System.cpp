#include "pch.h"
#include "System.h"

#include <winrt/Windows.System.h>
#include <winrt/Windows.Storage.h>

#include <pplawait.h>
#include <shlwapi.h>

namespace Common
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

    //TODO: Use wstring_view.
    std::vector<Filesystem::DirectoryInfo> System::EnumerateDirectories(const std::wstring& path)
    {
        auto dirs = std::vector<Filesystem::DirectoryInfo>();

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
            do
            {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    std::wstring filePath = std::wstring(findData.cFileName);
                    if (filePath != L"." && filePath != L"..")
                    {
                        dirs.push_back(filePath);
                    }
                }
            } while (FindNextFile(findHandle, &findData));
            FindClose(findHandle);
        }

        return dirs;
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

    uint64_t System::GetFileSize(const std::wstring& filePath)
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

    bool System::PathExists(const std::wstring& path)
    {
        return PathFileExists(path.c_str());
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
            return std::move(foldersList);
        });
    }
}
