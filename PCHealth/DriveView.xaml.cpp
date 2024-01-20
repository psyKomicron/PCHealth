#include "pch.h"
#include "DriveView.xaml.h"
#if __has_include("DriveView.g.cpp")
#include "DriveView.g.cpp"
#endif

#include "System.h"
#include "DirectoryInfo.h"
#include "FileSize.h"

#include <ppltasks.h>
#include <filesystem>
#include <regex>
#include <iostream>

namespace wrt = winrt;

namespace winrt::PCHealth::implementation
{
    DriveView::DriveView()
    {
        InitializeComponent();
    }

    wrt::hstring DriveView::DriveName()
    {
        return driveName;
    }

    void DriveView::DriveName(const wrt::hstring& value)
    {
        driveName = value;
    }

    double DriveView::UsedSpace()
    {
        return usedSpace;
    }

    void DriveView::UsedSpace(const double& value)
    {
        usedSpace = value;
    }

    double DriveView::Capacity()
    {
        return capacity;
    }

    void DriveView::Capacity(const double& value)
    {
        capacity = value;
    }


    void DriveView::RootGrid_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const&, winrt::Windows::Foundation::IInspectable const&)
    {
        driveInfo = Common::Filesystem::DriveInfo(std::wstring(driveName.c_str()));
        FreeDiskSpaceTextBlock().Text(Common::FileSize(driveInfo.totalUsedSpace()).ToString());
    }

    wrt::Windows::Foundation::IAsyncAction DriveView::SecondPivotContentGrid_Loading(wrt::Microsoft::UI::Xaml::FrameworkElement const&, wrt::Windows::Foundation::IInspectable const&)
    {
        using namespace Common::Filesystem;
        //LoadingProgressGrid().Visibility(wrt::Microsoft::UI::Xaml::Visibility::Visible);

        co_await wrt::resume_background(); // Do not use display properties after this line without using DispatcherQueue callback.

        uint64_t downloadsSize, documentsSize, musicSize, picturesSize, videosSize;
        if (driveInfo.isMainDrive())
        {
            auto&& libraries = Common::System::GetLibraries().get();
            downloadsSize = DirectoryInfo(libraries.DownloadsFolder()).GetSize();
            documentsSize = DirectoryInfo::GetSize(libraries.DocumentsLibraryFolders());
            musicSize = DirectoryInfo::GetSize(libraries.MusicLibraryFolders());
            picturesSize = DirectoryInfo::GetSize(libraries.PicturesLibraryFolders());
            videosSize = DirectoryInfo::GetSize(libraries.VideosLibraryFolders());
        }
        else
        {
            auto&& libraries = driveInfo.getLibraries();
            downloadsSize = DirectoryInfo(libraries.DownloadsFolder()).GetSize();
            documentsSize = DirectoryInfo(libraries.DocumentsLibraryPath()).GetSize();
            musicSize = DirectoryInfo(libraries.MusicLibraryPath()).GetSize();
            picturesSize = DirectoryInfo(libraries.PicturesLibraryPath()).GetSize();
            videosSize = DirectoryInfo(libraries.VideosLibraryPath()).GetSize();
        }

        uint64_t recycleBinSize = driveInfo.getRecycleBinSize();
        auto&& sizes = std::array<std::tuple<uint64_t, FileClass>, 7>
        {
            std::tuple<uint64_t, FileClass>(downloadsSize, FileClass::Downloads),
            std::tuple<uint64_t, FileClass>(documentsSize, FileClass::Document),
            std::tuple<uint64_t, FileClass>(musicSize, FileClass::Music),
            std::tuple<uint64_t, FileClass>(picturesSize, FileClass::Picture),
            std::tuple<uint64_t, FileClass>(videosSize, FileClass::Video),
            std::tuple<uint64_t, FileClass>(recycleBinSize, FileClass::RecycleBin),
            std::tuple<uint64_t, FileClass>(0, FileClass::System)
        };

        DispatcherQueue().TryEnqueue([this, sizes]() mutable
        {
            FillGradients(sizes, DiskStatsGradients());
            //LoadingProgressGrid().Visibility(wrt::Microsoft::UI::Xaml::Visibility::Collapsed);
        });
    }

    winrt::Windows::Foundation::IAsyncAction DriveView::ExtensionsPivotContentGrid_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const&, winrt::Windows::Foundation::IInspectable const&)
    {
        std::wcout << L"Drive view loading extensions stats..." << std::endl;
        LoadingProgressGrid().Visibility(wrt::Microsoft::UI::Xaml::Visibility::Visible);
        action = LoadExtensionsStats();
        co_return;
    }

    void DriveView::DocumentsTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args)
    {
        static bool toggled = true;
        winrt::Microsoft::UI::Xaml::VisualStateManager::GoToState(sender, toggled ? L"TagsSublistVisible" : L"TagsSublistHidden", true);
        toggled = !toggled;
    }

    void DriveView::VideosTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args)
    {
    }

    void DriveView::ImagesTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args)
    {
    }

    void DriveView::MusicTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args)
    {
    }

    void DriveView::RecycleBinTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args)
    {
    }

    void DriveView::SystemTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args)
    {
    }

    void DriveView::DownloadsTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args)
    {
    }

    void DriveView::CancelButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        if (!action.Completed())
        {
            action.Cancel();
            LoadingProgressGrid().Visibility(wrt::Microsoft::UI::Xaml::Visibility::Collapsed);
            ResetButton().Visibility(wrt::Microsoft::UI::Xaml::Visibility::Visible);
        }
    }

    void DriveView::ResetButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        if (action.Status() != winrt::Windows::Foundation::AsyncStatus::Started)
        {
            ResetButton().Visibility(wrt::Microsoft::UI::Xaml::Visibility::Collapsed);
            LoadingProgressGrid().Visibility(wrt::Microsoft::UI::Xaml::Visibility::Visible);

            action = LoadExtensionsStats();
        }
    }


    inline void DriveView::TagClicked()
    {
    }

    std::map<FileClass, wrt::Windows::UI::Color> DriveView::GetStatsColorStack()
    {
        auto&& resources = wrt::Microsoft::UI::Xaml::Application::Current().Resources();
        auto&& heatmapColors = resources.Lookup(wrt::box_value(L"HeatmapColors")).as<wrt::Microsoft::UI::Xaml::ResourceDictionary>();
        std::map<FileClass, wrt::Windows::UI::Color> colors{};
        for (auto&& color : heatmapColors)
        {
            if (color.Key().as<wrt::hstring>().starts_with(L"Docs"))
            {
                colors[FileClass::Document] = color.Value().as<wrt::Windows::UI::Color>();
            }
            else if (color.Key().as<wrt::hstring>().starts_with(L"Images"))
            {
                colors[FileClass::Picture] = color.Value().as<wrt::Windows::UI::Color>();
            }
            else if (color.Key().as<wrt::hstring>().starts_with(L"Videos"))
            {
                colors[FileClass::Video] = color.Value().as<wrt::Windows::UI::Color>();
            }
            else if (color.Key().as<wrt::hstring>().starts_with(L"Music"))
            {
                colors[FileClass::Music] = color.Value().as<wrt::Windows::UI::Color>();
            }
            else if (color.Key().as<wrt::hstring>().starts_with(L"Archives"))
            {
                colors[FileClass::RecycleBin] = color.Value().as<wrt::Windows::UI::Color>();
            }
            else if (color.Key().as<wrt::hstring>().starts_with(L"System"))
            {
                colors[FileClass::System] = color.Value().as<wrt::Windows::UI::Color>();
            }
            else if (color.Key().as<wrt::hstring>().starts_with(L"Downloads"))
            {
                colors[FileClass::Downloads] = color.Value().as<wrt::Windows::UI::Color>();
            }
        }

        return colors;
    }

    void DriveView::FillGradients(const std::array<std::tuple<uint64_t, FileClass>, 7>& sizes, const winrt::Microsoft::UI::Xaml::Media::GradientStopCollection& gradientStopCollection)
    {
        if (!DispatcherQueue().HasThreadAccess())
        {
            throw winrt::hresult_wrong_thread();
        }

        double drivetotalUsedSpace = static_cast<double>(driveInfo.totalUsedSpace());
        auto&& colors = GetStatsColorStack();
        double offset = 0;
        for (size_t i = 0; i < sizes.size(); i++)
        {
            auto gradient = wrt::Microsoft::UI::Xaml::Media::GradientStop();
            gradient.Offset(offset);
            gradient.Color(colors[std::get<1>(sizes[i])]);
            gradientStopCollection.Append(gradient);

            offset += std::get<0>(sizes[i]) / drivetotalUsedSpace; // TODO: operation can result in NaN thus making WinUI throw an exception when assigning the offset to the gradient stop.
            gradient = wrt::Microsoft::UI::Xaml::Media::GradientStop();
            gradient.Offset(offset);
            gradient.Color(colors[std::get<1>(sizes[i])]);
            gradientStopCollection.Append(gradient);

            winrt::PCHealth::Tag tag{ nullptr };
            switch (std::get<1>(sizes[i]))
            {
                case FileClass::RecycleBin:
                    tag = RecycleBinTag();
                    break;
                case FileClass::Document:
                    tag = DocumentsTag();
                    break;
                case FileClass::Music:
                    tag = MusicTag();
                    break;
                case FileClass::Picture:
                    tag = ImagesTag();
                    break;
                case FileClass::Video:
                    tag = VideosTag();
                    break;
                case FileClass::Downloads:
                    tag = DownloadsTag();
                    break;
                case FileClass::System:
                    tag = SystemTag();
                    break;
                default:
                    tag = OthersTag();
                    break;
                    /*case winrt::Windows::Storage::KnownFolderId::CameraRoll:
                    break;
                    case winrt::Windows::Storage::KnownFolderId::SavedPictures:
                    break;
                    case winrt::Windows::Storage::KnownFolderId::HomeGroup:
                    break;
                    case winrt::Windows::Storage::KnownFolderId::MediaServerDevices:
                    break;
                    case winrt::Windows::Storage::KnownFolderId::Objects3D:
                    break;
                    case winrt::Windows::Storage::KnownFolderId::Playlists:
                    break;
                    case winrt::Windows::Storage::KnownFolderId::RecordedCalls:
                    break;
                    case winrt::Windows::Storage::KnownFolderId::RemovableDevices:
                    break;
                    case winrt::Windows::Storage::KnownFolderId::Screenshots:
                    break;
                    case winrt::Windows::Storage::KnownFolderId::AllAppMods:
                    break;
                    case winrt::Windows::Storage::KnownFolderId::CurrentAppMods:
                    break;*/
            }
            tag.Text(Common::FileSize(std::get<0>(sizes[i])).ToString());
        }

        if (offset < 1)
        {
            auto gradient = wrt::Microsoft::UI::Xaml::Media::GradientStop();
            gradient.Offset(offset);
            auto&& resources = wrt::Microsoft::UI::Xaml::Application::Current().Resources();
            gradient.Color(resources.Lookup(wrt::box_value(L"SolidBackgroundFillColorSecondary")).as<wrt::Windows::UI::Color>());
            gradientStopCollection.Append(gradient);
        }
    }

    winrt::Windows::Foundation::IAsyncAction DriveView::LoadExtensionsStats()
    {
        co_await winrt::resume_background();

        const std::wstring videoRe = L"^\\.(mp4|mkv|flv|vob|avi|mov|qt|wmv|yuv|m4p|m4v|mpg|mp2|mpeg|mpe|mpv|mpg|m2v|m4v|svi|3gp|3g2|f4v|f4p|f4a|f4b)$";
        const std::wstring systemRe = L"^\\.(dll|exe|msi)$";
        const std::wstring pictureRe = L"^\\.(jpg|png|gif|webp|tiff|psd|raw|bmp|heif|indd|svg|ai|eps)$";
        std::unordered_map<std::wstring, uint64_t> extensions
        {
            { videoRe, 0 },
            { systemRe, 0 },
            { pictureRe, 0 }
        };

        driveInfo.getExtensionsStats(&extensions);

        DispatcherQueue().TryEnqueue([this, videosSize = extensions[videoRe], system = extensions[systemRe]]
        {
            auto&& sizes = std::array<std::tuple<uint64_t, FileClass>, 7>
            {
                std::tuple<uint64_t, FileClass>(0, FileClass::Downloads),
                    std::tuple<uint64_t, FileClass>(0, FileClass::Document),
                    std::tuple<uint64_t, FileClass>(0, FileClass::Music),
                    std::tuple<uint64_t, FileClass>(0, FileClass::Picture),
                    std::tuple<uint64_t, FileClass>(videosSize, FileClass::Video),
                    std::tuple<uint64_t, FileClass>(driveInfo.getRecycleBinSize(), FileClass::RecycleBin),
                    std::tuple<uint64_t, FileClass>(system, FileClass::System)
            };
            FillGradients(sizes, ExtensionsStatsGradients());
            LoadingProgressGrid().Visibility(wrt::Microsoft::UI::Xaml::Visibility::Collapsed);
        });

        co_return;
    }
}
