#pragma once
#include <string>

namespace pchealth::filesystem
{
    class DirectoryInfo
    {
    public:
        DirectoryInfo() = default;
        DirectoryInfo(const std::wstring& directoryPath);

        static DirectoryInfo CreateWithSize(const std::wstring& directoryPath);
        static uint64_t GetSize(const std::vector<std::wstring>* directories);

        std::wstring_view Name();
        std::wstring Path();
        uint64_t Size();
        bool IsSizeComputed();

        uint64_t GetSize();
        winrt::Windows::Foundation::IAsyncAction EmptyDirectoryAsync();
        void EmptyDirectory(const bool& recycle, const bool& parallelize);

    private:
        std::wstring path{};
        uint64_t size{};
        bool isSizeComputed = false;

        bool DeleteFileItem(const bool& recycle, const std::wstring& filePath);
    };
}