#pragma once

#include "WatchedFoldersPage.g.h"

namespace winrt::PCHealth::implementation
{
    struct WatchedFoldersPage : WatchedFoldersPageT<WatchedFoldersPage>
    {
    private:
        winrt::event_token timerEventToken{};
        winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer timer{ nullptr };

        void AddWatchedFolder(const winrt::hstring& path);
        void PostMessageToWindow(const winrt::param::hstring& longMessage, const winrt::param::hstring& shortMessage, bool recursive = false);

    public:
        WatchedFoldersPage();
        winrt::Windows::Foundation::IAsyncAction RootGrid_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);
        winrt::Windows::Foundation::IAsyncAction AppBarAddButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void AppBarRemoveButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void AppBarEditButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void AppBarSaveButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct WatchedFoldersPage : WatchedFoldersPageT<WatchedFoldersPage, implementation::WatchedFoldersPage>
    {
    };
}
