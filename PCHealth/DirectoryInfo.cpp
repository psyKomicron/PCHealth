#include "pch.h"
#include "DirectoryInfo.h"

#include <DirectorySizeCalculator.h>

#include <ppl.h>
#include <atomic>
#include <vector>
#include <Shlobj.h>
#include <shobjidl_core.h>
#include <comip.h>
#include <comdef.h>
#include <shlwapi.h>

#include <winrt/Windows.Storage.h>

#define MIMIC_DELETE

namespace pchealth::filesystem::ComPtrs
{
    _COM_SMARTPTR_TYPEDEF(IFileOperation, __uuidof(IFileOperation));
    _COM_SMARTPTR_TYPEDEF(IShellItem, __uuidof(IShellItem));
}

namespace pchealth::filesystem
{
    DirectoryInfo::DirectoryInfo(const std::wstring& directoryPath)
    {
        path = directoryPath;
    }

    DirectoryInfo DirectoryInfo::CreateWithSize(const std::wstring& directoryPath)
    {
        return DirectoryInfo(directoryPath);
    }

    uint64_t DirectoryInfo::GetSize(const std::vector<std::wstring>* directories)
    {
        std::atomic_uint_fast64_t atomicDirSize = std::atomic_uint_fast64_t();
        concurrency::parallel_for_each(
            std::begin(*directories), 
            std::end(*directories), 
            [&atomicDirSize](const std::wstring& path)
        {
            // TODO: Reduce allocation.
            int64_t size = DirectoryInfo(path).GetSize();
            atomicDirSize.fetch_add(size);
        });
        return atomicDirSize.load();
    }

    std::wstring DirectoryInfo::Path()
    {
        return path;
    }

    uint64_t DirectoryInfo::Size()
    {
        return size;
    }

    bool DirectoryInfo::IsSizeComputed()
    {
        return isSizeComputed;
    }

    uint64_t DirectoryInfo::GetSize()
    {
        if (!isSizeComputed && !path.empty())
        {
            DirectorySizeCalculator calculator{};
            size = calculator.GetSize(path, true);
        }
        return size;
    }

    winrt::Windows::Foundation::IAsyncAction DirectoryInfo::EmptyDirectoryAsync()
    {
        auto&& folder = co_await winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(path);
        auto&& files = co_await folder.GetFilesAsync();
        for (auto&& file : files)
        {
            co_await file.DeleteAsync();
        }
    }

    void DirectoryInfo::EmptyDirectory(const bool& recycle, const bool& parallelize)
    {
        const unsigned int ALTERNATE_MAX_PATH = 2048;
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
            std::vector<std::wstring> directoryPathes = std::vector<std::wstring>();
            do
            {
                std::wstring filePath = std::wstring(findData.cFileName);

                // 1- Queue the directories to be removed once they all are empty.
                // 2- Remove every file in the current directory.
                // 3- Delete the now empty directories.
                if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 1)
                {
                    std::wstring filePath = std::wstring(findData.cFileName);
                    directoryPathes.push_back(filePath);
                    
                }
                else if (filePath != L"." && filePath != L"..")
                {
                    // Remove the file.
                    WCHAR combinedPath[ALTERNATE_MAX_PATH]{};
                    PathCombine(combinedPath, path.c_str(), filePath.c_str());
#ifdef MIMIC_DELETE
                    OutputDebugString(L"MIMIC_DELETE enabled, not deleting file.");
#else
                    if (!DeleteFileItem(recycle, std::wstring(combinedPath))) // TODO: If it fails to delete, an exception might be thrown.
                    {
                        OutputDebugString(L"Failed to delete a file.");
                }
#endif
                }
            } while (FindNextFile(findHandle, &findData));
            FindClose(findHandle);

            if (parallelize && directoryPathes.size() > 1)
            {
                // Ask for the directories to empty themselves.
                concurrency::parallel_for_each(begin(directoryPathes), end(directoryPathes), [this, wstr = path, recycle](const std::wstring& dir)
                {
                    WCHAR combinedPath[ALTERNATE_MAX_PATH]{};
                    PathCombine(combinedPath, wstr.c_str(), dir.c_str());
                    std::wstring deepPath = std::wstring(combinedPath) + L"\\";

                    DirectoryInfo dirInfo{ deepPath };
                    dirInfo.EmptyDirectory(recycle, true);
                });
            }
            else
            {
                for (size_t i = 0; i < directoryPathes.size(); i++)
                {
                    WCHAR combinedPath[ALTERNATE_MAX_PATH];
                    PathCombine(combinedPath, path.c_str(), directoryPathes[i].c_str());
                    std::wstring deepPath = std::wstring(combinedPath) + L"\\";

                    DirectoryInfo dirInfo{ deepPath };
                    dirInfo.EmptyDirectory(recycle, parallelize);
                }
            }
        }
    }

    bool DirectoryInfo::DeleteFileItem(const bool& recycle, const std::wstring& filePath)
    {
        HRESULT hr{};
        winrt::com_ptr<IFileOperation> fileOp{ nullptr };
        DWORD opFlags = FOF_NO_UI | (FOF_ALLOWUNDO & (recycle ? 0 : 1));

        if (SUCCEEDED(hr = CoCreateInstance(__uuidof(FileOperation), NULL, CLSCTX_ALL, __uuidof(IFileOperation), reinterpret_cast<void**>(fileOp.get())))
            || SUCCEEDED(hr = fileOp->SetOperationFlags(FOF_NO_UI | FOF_ALLOWUNDO)))
        {
            winrt::com_ptr<IShellItem> item{ nullptr };
            if (SUCCEEDED(hr = SHCreateItemFromParsingName(filePath.c_str(), nullptr, __uuidof(IShellItem), reinterpret_cast<void**>(item.get())))
                && SUCCEEDED(hr = fileOp->DeleteItem(item.get(), nullptr))
                && SUCCEEDED(hr = fileOp->PerformOperations()))
            {
                return true;
            }
            else
            {
                throw std::system_error(std::error_code(static_cast<int>(hr), std::generic_category()), "Could not delete file");
            }
        }
        else
        {
            throw std::system_error(std::error_code(static_cast<int>(hr), std::generic_category()), "Could not delete file");
        }
        return false;
    }
}
