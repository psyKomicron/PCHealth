#include "pch.h"
#include "FileInfoViewModel.h"
#if __has_include("FileInfoViewModel.g.cpp")
#include "FileInfoViewModel.g.cpp"
#endif

namespace winrt::PCHealth::implementation
{
    winrt::hstring FileInfoViewModel::Name() const
    {
        return _name;
    }

    void FileInfoViewModel::Name(const winrt::hstring& value)
    {
        _name = value;
    }

    winrt::hstring FileInfoViewModel::Path() const
    {
        return _path;
    }

    void FileInfoViewModel::Path(const winrt::hstring& value)
    {
        _path = value;
    }

    winrt::hstring FileInfoViewModel::Type() const
    {
        return _type;
    }

    void FileInfoViewModel::Type(const winrt::hstring& value)
    {
        _type = value;
    }

    bool FileInfoViewModel::IsDirectory() const
    {
        return _isDirectory;
    }

    void FileInfoViewModel::IsDirectory(const bool& value)
    {
        _isDirectory = value;
    }

    bool FileInfoViewModel::IsApplication() const
    {
        return _isApplication;
    }

    void FileInfoViewModel::IsApplication(const bool& value)
    {
        _isApplication = value;
    }

    bool FileInfoViewModel::IsFileSystemEntry() const
    {
        return _isDirectory || !_isApplication;
    }

    winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage FileInfoViewModel::Thumbnail() const
    {
        return _thumbnail;
    }

    void FileInfoViewModel::Thumbnail(const winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage& value)
    {
        _thumbnail = value;
    }
    
    int32_t FileInfoViewModel::Kind() const
    {
        return _kind;
    }

    void FileInfoViewModel::Kind(const int32_t value)
    {
        _kind = value;
    }
}
