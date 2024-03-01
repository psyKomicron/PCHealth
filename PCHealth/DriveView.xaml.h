#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "DriveView.g.h"

#include "DriveInfo.h"

namespace winrt::PCHealth::implementation
{
    enum FileClass
    {
        Video,
        Picture,
        Music,
        Document,
        RecycleBin,
        Downloads,
        System
    };
}

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

        void DocumentsTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void VideosTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void ImagesTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void MusicTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void RecycleBinTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void SystemTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        void DownloadsTag_Click(winrt::PCHealth::Tag const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& args);
        winrt::Windows::Foundation::IAsyncAction ExtensionsPivotContentGrid_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);
        void RootGrid_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);
        void CancelButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ResetButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
    private:
        winrt::hstring driveName{};
        double usedSpace{};
        double capacity{};
        pchealth::filesystem::DriveInfo driveInfo{};
        winrt::Windows::Foundation::IAsyncAction action;

        inline void TagClicked();
        void FillGradients(const std::array<std::tuple<uint64_t, FileClass>, 7>& sizes, const winrt::Microsoft::UI::Xaml::Media::GradientStopCollection& gradientStopCollection);
        std::map<FileClass, winrt::Windows::UI::Color> GetStatsColorStack();
        winrt::Windows::Foundation::IAsyncAction LoadExtensionsStats();
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct DriveView : DriveViewT<DriveView, implementation::DriveView>
    {
    };
}