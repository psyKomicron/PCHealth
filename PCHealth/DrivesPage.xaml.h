#pragma once

#include "DrivesPage.g.h"

namespace winrt::PCHealth::implementation
{
    struct DrivesPage : DrivesPageT<DrivesPage>
    {
        DrivesPage() = default;
        winrt::Windows::Foundation::IAsyncAction DrivesGrid_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);
        winrt::Windows::Foundation::IAsyncAction SystemRecycleBinSizeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void HibernationFileSizeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::Windows::Foundation::IAsyncAction DownloadsFolderSizeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct DrivesPage : DrivesPageT<DrivesPage, implementation::DrivesPage>
    {
    };
}
