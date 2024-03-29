#include "pch.h"
#include "DirectoryInfo.h"

#include "UserLibraries.h"

#include "System.h"
#include <Windows.h>
#include <shlwapi.h>

namespace pchealth::filesystem
{
    UserLibraries UserLibraries::getUserLibraries(const wchar_t& driveLetter)
    {
        std::wstring drivePath = std::format(L"{}:\\", driveLetter);
        if (!pchealth::windows::System::pathExists(drivePath))
        {
            throw std::invalid_argument("Drive doesn't exists.");
        }
        
        if (isFolderLibraryRoot(drivePath))
        {

        }
        else
        {
#ifdef _DEBUG
            throw winrt::hresult_not_implemented();
#else
            std::vector<DirectoryInfo> rootDirs/* = pchealth::windows::System::EnumerateDirectories(drivePath)*/{};
            for (auto&& dirInfo : rootDirs)
            {
                std::vector<DirectoryInfo> depth1Folders/* = pchealth::windows::System::EnumerateDirectories(drivePath)*/{};
                for (auto&& subDirInfo : depth1Folders)
                {
                    if (isFolderLibraryRoot(subDirInfo.Path()))
                    {
                        UserLibraries libs{};
                        std::wstring subDirPath = subDirInfo.Path();
                        if (!subDirPath.ends_with(L'\\'))
                        {
                            subDirPath += L'\\';
                        }
                        libs.documentsFolderPath = subDirPath + L"Documents";
                        libs.downloadsFolderPath = subDirPath + L"Downloads";
                        libs.musicFolderPath = subDirPath + L"Music";
                        libs.picturesFolderPath = subDirPath + L"Pictures";
                        libs.videosFolderPath = subDirPath + L"Videos";
                    }
                }
            }
#endif
        }

        return UserLibraries();
    }


    bool UserLibraries::isFolderLibraryRoot(const std::wstring path)
    {
        //I18N: Potential improvements.
        if (path.ends_with(L'\\'))
        {
            return PathFileExists((path + L"Documents").c_str())
                && PathFileExists((path + L"Music").c_str())
                && PathFileExists((path + L"Pictures").c_str())
                && PathFileExists((path + L"Videos").c_str())
                && PathFileExists((path + L"Downloads").c_str());
        }
        else
        {
            return PathFileExists((path + L"\\Documents").c_str())
                && PathFileExists((path + L"\\Music").c_str())
                && PathFileExists((path + L"\\Pictures").c_str())
                && PathFileExists((path + L"\\Videos").c_str())
                && PathFileExists((path + L"\\Downloads").c_str());
        }
    }
}
