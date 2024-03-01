#pragma once

#include "FileInfoViewModel.g.h"

#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

namespace winrt::PCHealth::implementation
{
    struct FileInfoViewModel : FileInfoViewModelT<FileInfoViewModel>
    {
    public:
        FileInfoViewModel() = default;

        winrt::hstring Name() const;
        void Name(const winrt::hstring& value);
        winrt::hstring Path() const;
        void Path(const winrt::hstring& value);
        winrt::hstring Type() const;
        void Type(const winrt::hstring& value);
        bool IsDirectory() const;
        void IsDirectory(const bool& value);
        bool IsApplication() const;
        void IsApplication(const bool& value);
        bool IsFileSystemEntry() const;
        winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage Thumbnail() const;
        void Thumbnail(const winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage& value);

    private:
        winrt::hstring _name{};
        winrt::hstring _path{};
        winrt::hstring _type{};
        bool _isDirectory = false;
        bool _isApplication = false;
        winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage _thumbnail{ nullptr };
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct FileInfoViewModel : FileInfoViewModelT<FileInfoViewModel, implementation::FileInfoViewModel>
    {
    };
}
