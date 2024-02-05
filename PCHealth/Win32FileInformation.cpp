#include "pch.h"
#include "Win32FileInformation.h"

pchealth::filesystem::Win32FileInformation::Win32FileInformation(WIN32_FIND_DATA* findData)
{
    _name = std::wstring(findData->cFileName);
    _size = (static_cast<int64_t>(findData->nFileSizeHigh) << 32) | findData->nFileSizeLow;
    _directory = findData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
}

std::wstring pchealth::filesystem::Win32FileInformation::name() const
{
    return _name;
}

uint64_t pchealth::filesystem::Win32FileInformation::size() const
{
    return _size;
}

bool pchealth::filesystem::Win32FileInformation::directory() const
{
    return _directory;
}

bool pchealth::filesystem::Win32FileInformation::operator==(const Win32FileInformation& right)
{
    return directory() && right.directory() && size() == right.size() && (name() == right.name());
}
