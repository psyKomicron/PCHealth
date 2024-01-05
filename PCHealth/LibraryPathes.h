#pragma once
namespace Common::Filesystem
{
    struct LibraryPathes
    {
    public:
        LibraryPathes() = default;
        LibraryPathes(
            const std::wstring& downloadsFolder, 
            const std::vector<std::wstring>& documentsFolders, 
            const std::vector<std::wstring>& musicFolders, 
            const std::vector<std::wstring>& picturesFolders, 
            const std::vector<std::wstring>& videosFolders);
        LibraryPathes(const std::wstring& downloadsPath,
            const winrt::Windows::Storage::StorageFolder& documentsFolder,
            const winrt::Windows::Storage::StorageFolder& musicFolder,
            const winrt::Windows::Storage::StorageFolder& picturesFolder,
            const winrt::Windows::Storage::StorageFolder& videosFolder);
        LibraryPathes(const std::wstring& downloadsPath,
            const std::wstring& documentsFolder,
            const std::wstring& musicFolder,
            const std::wstring& picturesFolder,
            const std::wstring& videosFolder);

        std::wstring DownloadsFolder();
        void DownloadsFolder(const std::wstring& value);
        std::wstring DocumentsLibraryPath();
        void DocumentsLibraryPath(const std::wstring& value);
        std::wstring MusicLibraryPath();
        void MusicLibraryPath(const std::wstring& value);
        std::wstring PicturesLibraryPath();
        void PicturesLibraryPath(const std::wstring& value);
        std::wstring VideosLibraryPath();
        void VideosLibraryPath(const std::wstring& value);
        std::vector<std::wstring>* DocumentsLibraryFolders();
        std::vector<std::wstring>* MusicLibraryFolders();
        std::vector<std::wstring>* PicturesLibraryFolders();
        std::vector<std::wstring>* VideosLibraryFolders();

    private:
        std::wstring downloadsFolder;
        std::wstring documentsRoot;
        std::wstring musicRoot;
        std::wstring picturesRoot;
        std::wstring videosRoot;
        std::vector<std::wstring> documentsFolders;
        std::vector<std::wstring> musicFolders;
        std::vector<std::wstring> picturesFolders;
        std::vector<std::wstring> videosFolders;

        void GetSubFolders(const winrt::Windows::Storage::StorageFolder& folder, std::vector<std::wstring>* subfolders);
    };
}

