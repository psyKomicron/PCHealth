#pragma once

namespace pchealth::filesystem
{
    class UserLibraries
    {
    public:
        UserLibraries() = default;

        static UserLibraries getUserLibraries(const wchar_t& driveLetter);
        static UserLibraries getWindowsLibraries();

        DirectoryInfo downloads() const;
        DirectoryInfo documents() const;
        DirectoryInfo music() const;
        DirectoryInfo pictures() const;
        DirectoryInfo videos() const;

    private:
        bool useSystemLibraries{};
        std::wstring downloadsFolderPath;
        std::wstring documentsFolderPath;
        std::wstring musicFolderPath;
        std::wstring picturesFolderPath;
        std::wstring videosFolderPath;
        // following members are unused if we aren't using Window's libraries, which can have multiple folders in different locations per library.
        std::vector<std::wstring> documentsLibraryFolders{};
        std::vector<std::wstring> musicLibraryFolders{};
        std::vector<std::wstring> picturesLibraryFolders{};
        std::vector<std::wstring> videosLibraryFolders{};

        static bool isFolderLibraryRoot(const std::wstring path);
    };
}

