#include "pch.h"
#include "Win32FileInformation.h"

#include "utilities.h"

#include <filesystem>

namespace pchealth::filesystem
{
    Win32FileInformation::Win32FileInformation(const WIN32_FIND_DATA& findData, const std::wstring& parent)
    {
        _name = std::wstring(findData.cFileName);
        _size = pchealth::utilities::convert(findData.nFileSizeHigh, findData.nFileSizeLow);
        _directory = findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        _path = (std::filesystem::path(parent) /= _name).wstring();
    }

    Win32FileInformation::Win32FileInformation(const FILE_NOTIFY_EXTENDED_INFORMATION& fileNotifyExInfo, const HANDLE& handle)
    {
        _name = fileNotifyExInfo.FileName;
        _size = fileNotifyExInfo.FileSize.QuadPart;
        _directory = fileNotifyExInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY;

        HANDLE parentFileHandle = getHandleForFileId(fileNotifyExInfo.ParentFileId, handle);
        if (parentFileHandle != INVALID_HANDLE_VALUE)
        {
            _path = std::filesystem::path(getPathForHandle(parentFileHandle)) /= _name;
            CloseHandle(parentFileHandle);
        }
    }

    std::wstring Win32FileInformation::name() const
    {
        return _name;
    }

    std::wstring Win32FileInformation::path() const
    {
        return _path;
    }

    uint64_t Win32FileInformation::size() const
    {
        return _size;
    }

    bool Win32FileInformation::directory() const
    {
        return _directory;
    }

    bool Win32FileInformation::isNavigator() const
    {
        return _name == L"." || _name == L"..";
    }

    bool Win32FileInformation::operator==(const Win32FileInformation& right)
    {
        return directory() && right.directory() 
            && size() == right.size() 
            && (path() == right.path());
    }


    HANDLE Win32FileInformation::getHandleForFileId(const LARGE_INTEGER& fileId, const HANDLE& directoryHandle) const
    {
        FILE_ID_DESCRIPTOR desc{};
        desc.dwSize = sizeof(FILE_ID_DESCRIPTOR);
        desc.Type = FILE_ID_TYPE::FileIdType;
        desc.FileId = fileId;

        HANDLE fileHandle = OpenFileById(directoryHandle, &desc, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, nullptr, FILE_FLAG_RANDOM_ACCESS);
        return fileHandle;
    }

    std::wstring Win32FileInformation::getPathForHandle(const HANDLE& handle) const
    {
        uint32_t flags = FILE_NAME_NORMALIZED | VOLUME_NAME_DOS;
        uint32_t pathBufferSize = 0;
        uint32_t getFinalPathRes = GetFinalPathNameByHandleW(handle, nullptr, 0, flags);
        if (getFinalPathRes == ERROR_PATH_NOT_FOUND)
        {
            //TODO: Throw error.
            return L"";
        }

        pathBufferSize = getFinalPathRes + 1;
        std::unique_ptr<WCHAR[]> path = std::make_unique<WCHAR[]>(pathBufferSize);
        getFinalPathRes = GetFinalPathNameByHandleW(handle, path.get(), pathBufferSize, flags);

        if (getFinalPathRes < pathBufferSize)
        {
            const std::wstring pathPrefix = L"\\\\?\\";
            return std::wstring(path.get()).substr(pathPrefix.size());
        }
        else
        {
            CloseHandle(handle);
            winrt::throw_last_error();
        }

        return L"";
    }
}

