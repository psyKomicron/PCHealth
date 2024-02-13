#pragma once
namespace pchealth::filesystem
{
    class Win32FileInformation
    {
    public:
        Win32FileInformation() = default;
        Win32FileInformation(const WIN32_FIND_DATA& findData, const std::wstring& parent);
        Win32FileInformation(const FILE_NOTIFY_EXTENDED_INFORMATION& fileNotifyExInfo, const HANDLE& handle);

        std::wstring name() const;
        std::wstring path() const;
        uint64_t size() const;
        bool directory() const;
        bool isNavigator() const;

        bool operator==(const Win32FileInformation& right);

    private:
        std::wstring _name{};
        std::wstring _path{};
        uint64_t _size = 0;
        bool _directory = false;

        HANDLE getHandleForFileId(const LARGE_INTEGER& fileId, const HANDLE& directoryHandle) const;
        /**
         * @brief Gets the full path for the file pointed by the handle. If the function fails it will close the handle and throw the exception.
         * @param handle Handle to a file.
         * @return The fully qualified path.
         * @throws winrt::hresult_error if the function fails to get the path for the handle.
        */
        std::wstring getPathForHandle(const HANDLE& handle) const;
    };
}

