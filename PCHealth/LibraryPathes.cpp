#include "pch.h"
#include "LibraryPathes.h"

#include <winrt/Windows.Storage.h>

namespace pchealth::filesystem
{
    LibraryPathes::LibraryPathes(const std::wstring& downloadsFolder, const std::vector<std::wstring>& documentsFolders, const std::vector<std::wstring>& musicFolders, const std::vector<std::wstring>& picturesFolders, const std::vector<std::wstring>& videosFolders)
    {
        this->downloadsFolder = downloadsFolder;
        this->documentsFolders = documentsFolders;
        this->musicFolders = musicFolders;
        this->picturesFolders = picturesFolders;
        this->videosFolders = videosFolders;
    }

    LibraryPathes::LibraryPathes(const std::wstring& downloadsPath, const winrt::Windows::Storage::StorageFolder& documentsFolder, const winrt::Windows::Storage::StorageFolder& musicFolder, const winrt::Windows::Storage::StorageFolder& picturesFolder, const winrt::Windows::Storage::StorageFolder& videosFolder)
    {
        downloadsFolder = downloadsPath;
        documentsRoot = std::wstring(documentsFolder.Path());
        musicRoot = std::wstring(musicFolder.Path());
        picturesRoot = std::wstring(picturesFolder.Path());
        videosRoot = std::wstring(videosFolder.Path());

        GetSubFolders(documentsFolder, &documentsFolders);
        GetSubFolders(musicFolder, &musicFolders);
        GetSubFolders(picturesFolder, &picturesFolders);
        GetSubFolders(videosFolder, &videosFolders);
    }

    LibraryPathes::LibraryPathes(const std::wstring& downloadsPath, const std::wstring& documentsFolder, const std::wstring& musicFolder, const std::wstring& picturesFolder, const std::wstring& videosFolder)
    {
        documentsRoot = documentsFolder;
        musicRoot = musicFolder;
        picturesRoot = picturesFolder;
        videosRoot = videosFolder;
        downloadsFolder = downloadsPath;
    }

    std::wstring LibraryPathes::DownloadsFolder()
    {
        return downloadsFolder;
    }

    void LibraryPathes::DownloadsFolder(const std::wstring& value)
    {
        downloadsFolder = value;
    }

    std::wstring LibraryPathes::DocumentsLibraryPath()
    {
        return documentsRoot;
    }

    void LibraryPathes::DocumentsLibraryPath(const std::wstring& value)
    {
        documentsRoot = value;
    }

    std::wstring LibraryPathes::MusicLibraryPath()
    {
        return musicRoot;
    }

    void LibraryPathes::MusicLibraryPath(const std::wstring& value)
    {
        musicRoot = value;
    }

    std::wstring LibraryPathes::PicturesLibraryPath()
    {
        return picturesRoot;
    }

    void LibraryPathes::PicturesLibraryPath(const std::wstring& value)
    {
        picturesRoot = value;
    }

    std::wstring LibraryPathes::VideosLibraryPath()
    {
        return videosRoot;
    }

    void LibraryPathes::VideosLibraryPath(const std::wstring& value)
    {
        videosRoot = value;
    }

    std::vector<std::wstring>* LibraryPathes::DocumentsLibraryFolders()
    {
        return &documentsFolders;
    }

    std::vector<std::wstring>* LibraryPathes::MusicLibraryFolders()
    {
        return &musicFolders;
    }
    
    std::vector<std::wstring>* LibraryPathes::PicturesLibraryFolders()
    {
        return &picturesFolders;
    }

    std::vector<std::wstring>* LibraryPathes::VideosLibraryFolders()
    {
        return &videosFolders;
    }

    void LibraryPathes::GetSubFolders(const winrt::Windows::Storage::StorageFolder& folder, std::vector<std::wstring>* subfolders)
    {
        auto&& libFolders = folder.GetFoldersAsync().get();
        for (auto&& libfolder : libFolders)
        {
            subfolders->push_back(std::wstring(libfolder.Path()));
        }
    }
}
