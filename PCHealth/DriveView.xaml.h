#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "DriveView.g.h"

#include <map>

namespace winrt::PCHealth::implementation
{
    struct DriveView : DriveViewT<DriveView>
    {
    public:
        DriveView();

        winrt::hstring DriveName();
        void DriveName(const winrt::hstring& value);
        double UsedSpace();
        void UsedSpace(const double& value);
        double Capacity();
        void Capacity(const double& value);

        std::map<winrt::Windows::Storage::KnownFolderId, winrt::Windows::UI::Color> GetStatsColorStack();
        Windows::Foundation::IAsyncAction ExpanderContentGrid_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);

        void DocumentsTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void VideosTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ImagesTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void MusicTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void RecycleBinTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void SystemTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void DownloadsTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);

    private:
        winrt::hstring driveName{};
        double usedSpace{};
        double capacity{};

        inline void TagClicked();
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct DriveView : DriveViewT<DriveView, implementation::DriveView>
    {
    };
}
