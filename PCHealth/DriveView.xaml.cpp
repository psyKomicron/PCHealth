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

    std::map<wrt::Windows::Storage::KnownFolderId, wrt::Windows::UI::Color> DriveView::GetStatsColorStack()
    {
        auto&& resources = wrt::Microsoft::UI::Xaml::Application::Current().Resources();
        auto&& heatmapColors = resources.Lookup(wrt::box_value(L"HeatmapColors")).as<wrt::Microsoft::UI::Xaml::ResourceDictionary>();
        std::map<wrt::Windows::Storage::KnownFolderId, wrt::Windows::UI::Color> colors{};
        for (auto&& color : heatmapColors)
        {
            if (color.Key().as<wrt::hstring>().starts_with(L"Docs"))
            {
                colors[wrt::Windows::Storage::KnownFolderId::DocumentsLibrary] = color.Value().as<wrt::Windows::UI::Color>();
            }
            else if (color.Key().as<wrt::hstring>().starts_with(L"Images"))
            {
                colors[wrt::Windows::Storage::KnownFolderId::PicturesLibrary] = color.Value().as<wrt::Windows::UI::Color>();
            }
            else if (color.Key().as<wrt::hstring>().starts_with(L"Videos"))
            {
                colors[wrt::Windows::Storage::KnownFolderId::VideosLibrary] = color.Value().as<wrt::Windows::UI::Color>();
            }
            else if (color.Key().as<wrt::hstring>().starts_with(L"Music"))
            {
                colors[wrt::Windows::Storage::KnownFolderId::MusicLibrary] = color.Value().as<wrt::Windows::UI::Color>();
            }
            else if (color.Key().as<wrt::hstring>().starts_with(L"Archives"))
            {
                colors[wrt::Windows::Storage::KnownFolderId::AppCaptures] = color.Value().as<wrt::Windows::UI::Color>();
            }
            else if (color.Key().as<wrt::hstring>().starts_with(L"System"))
            {
                colors[wrt::Windows::Storage::KnownFolderId::AllAppMods] = color.Value().as<wrt::Windows::UI::Color>();
            }
            else if (color.Key().as<wrt::hstring>().starts_with(L"Downloads"))
            {
                colors[wrt::Windows::Storage::KnownFolderId::DownloadsFolder] = color.Value().as<wrt::Windows::UI::Color>();
            }
        }

        return colors;
    }

    wrt::Windows::Foundation::IAsyncAction DriveView::ExpanderContentGrid_Loading(wrt::Microsoft::UI::Xaml::FrameworkElement const&, wrt::Windows::Foundation::IInspectable const&)
    {
        using namespace Common::Filesystem;
        LoadingProgressRing().Visibility(wrt::Microsoft::UI::Xaml::Visibility::Visible);
        
        auto driveInfo = DriveInfo(std::wstring(driveName.c_str()));
        FreeDiskSpaceTextBlock().Text(Common::FileSize(driveInfo.totalUsedSpace()).ToString());
        if (driveInfo.isMainDrive())
        {
            //I18N: No system files.
            SystemTag().Text(L"No system files");
        }

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
        auto&& sizes = std::array<std::tuple<uint64_t, wrt::Windows::Storage::KnownFolderId>, 6>
        {
            std::tuple<uint64_t, wrt::Windows::Storage::KnownFolderId>(downloadsSize, wrt::Windows::Storage::KnownFolderId::DownloadsFolder),
            std::tuple<uint64_t, wrt::Windows::Storage::KnownFolderId>(documentsSize, wrt::Windows::Storage::KnownFolderId::DocumentsLibrary),
            std::tuple<uint64_t, wrt::Windows::Storage::KnownFolderId>(musicSize, wrt::Windows::Storage::KnownFolderId::MusicLibrary),
            std::tuple<uint64_t, wrt::Windows::Storage::KnownFolderId>(picturesSize, wrt::Windows::Storage::KnownFolderId::PicturesLibrary),
            std::tuple<uint64_t, wrt::Windows::Storage::KnownFolderId>(videosSize, wrt::Windows::Storage::KnownFolderId::VideosLibrary),
            std::tuple<uint64_t, wrt::Windows::Storage::KnownFolderId>(recycleBinSize, winrt::Windows::Storage::KnownFolderId::AppCaptures)
        };

        DispatcherQueue().TryEnqueue([this, sizes, driveInfo]() mutable
        {
            double drivetotalUsedSpace = static_cast<double>(driveInfo.totalUsedSpace());
            auto&& colors = GetStatsColorStack();
            double offset = 0;
            for (size_t i = 0; i < sizes.size(); i++)
            {
                auto gradient = wrt::Microsoft::UI::Xaml::Media::GradientStop();
                gradient.Offset(offset);
                gradient.Color(colors[std::get<1>(sizes[i])]);
                DiskStatsGradients().Append(gradient);

                offset += std::get<0>(sizes[i]) / drivetotalUsedSpace;
                gradient = wrt::Microsoft::UI::Xaml::Media::GradientStop();
                gradient.Offset(offset);
                gradient.Color(colors[std::get<1>(sizes[i])]);
                DiskStatsGradients().Append(gradient);

                switch (std::get<1>(sizes[i]))
                {
                    case winrt::Windows::Storage::KnownFolderId::AppCaptures:
                        RecycleBinTag().Text(Common::FileSize(std::get<0>(sizes[i])).ToString());
                        break;
                    case winrt::Windows::Storage::KnownFolderId::DocumentsLibrary:
                        DocumentsTag().Text(Common::FileSize(std::get<0>(sizes[i])).ToString());
                        break;
                    case winrt::Windows::Storage::KnownFolderId::MusicLibrary:
                        MusicTag().Text(Common::FileSize(std::get<0>(sizes[i])).ToString());
                        break;
                    case winrt::Windows::Storage::KnownFolderId::PicturesLibrary:
                        ImagesTag().Text(Common::FileSize(std::get<0>(sizes[i])).ToString());
                        break;
                    case winrt::Windows::Storage::KnownFolderId::VideosLibrary:
                        VideosTag().Text(Common::FileSize(std::get<0>(sizes[i])).ToString());
                        break;
                    case winrt::Windows::Storage::KnownFolderId::DownloadsFolder:
                        DownloadsTag().Text(Common::FileSize(std::get<0>(sizes[i])).ToString());
                        break;
                    //TODO: Missing case for system files size.
                    default:
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
            }

            if (offset < 1)
            {
                auto gradient = wrt::Microsoft::UI::Xaml::Media::GradientStop();
                gradient.Offset(offset);
                auto&& resources = wrt::Microsoft::UI::Xaml::Application::Current().Resources();
                gradient.Color(resources.Lookup(wrt::box_value(L"SolidBackgroundFillColorSecondary")).as<wrt::Windows::UI::Color>());
                DiskStatsGradients().Append(gradient);
            }

            LoadingProgressRing().Visibility(wrt::Microsoft::UI::Xaml::Visibility::Collapsed);
        });

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


    inline void DriveView::TagClicked()
    {
    }
}