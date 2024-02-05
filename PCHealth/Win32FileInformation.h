#pragma once
namespace pchealth::filesystem
{
    class Win32FileInformation
    {
    public:
        Win32FileInformation(WIN32_FIND_DATA* findData);

        std::wstring name() const;
        uint64_t size() const;
        bool directory() const;

        bool operator==(const Win32FileInformation& right);

    private:
        std::wstring _name{};
        uint64_t _size = 0;
        bool _directory = false;
    };
}

