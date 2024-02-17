#pragma once

#include "ClipboardManagerView.g.h"

#include "HotKey.h"

#include <winrt/Microsoft.Windows.AppNotifications.h>

namespace winrt::PCHealth::implementation
{
    struct ClipboardManagerView : ClipboardManagerViewT<ClipboardManagerView>
    {
    public:
        ClipboardManagerView();
        ~ClipboardManagerView();

        winrt::Windows::Foundation::Collections::IVector<winrt::PCHealth::ClipboardTrigger> Formats();

        winrt::Windows::Foundation::IAsyncAction ClipboardListeningToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::Windows::Foundation::IAsyncAction AppBarAddButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void AppBarRemoveButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void AppBarEditButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void AppBarSaveButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        bool loadClipboardHistory = false;
        pchealth::win32::HotKey copyHotKey{ L'C', MOD_CONTROL, false };
        winrt::event_token clipboardChangedToken{};
        winrt::Windows::Foundation::Collections::IVector<winrt::PCHealth::ClipboardTrigger> _formats
        { 
            winrt::single_threaded_observable_vector<winrt::PCHealth::ClipboardTrigger>() 
        };
        winrt::Microsoft::Windows::AppNotifications::AppNotificationManager appNotifManager = winrt::Microsoft::Windows::AppNotifications::AppNotificationManager::Default();

        winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Foundation::Collections::IVector<winrt::hstring>> GetClipboardHistory();
        void SendNotification(const winrt::PCHealth::ClipboardTrigger& trigger, const winrt::hstring& content);
        winrt::Windows::Foundation::IAsyncAction TriggerClipboardTrigger(const winrt::PCHealth::ClipboardTrigger& trigger, const winrt::hstring& content);
        bool AddNewTrigger(const winrt::hstring& name, const winrt::hstring& query, const winrt::hstring& match, const winrt::hstring& actionString, const bool& showNotification);

        winrt::Windows::Foundation::IAsyncAction Clipboard_ContentChanged(const winrt::Windows::Foundation::IInspectable&, const winrt::Windows::Foundation::IInspectable&);
        void NotificationManager_NotificationInvoked(const winrt::Windows::Foundation::IInspectable&, const winrt::Microsoft::Windows::AppNotifications::AppNotificationActivatedEventArgs& e);
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct ClipboardManagerView : ClipboardManagerViewT<ClipboardManagerView, implementation::ClipboardManagerView>
    {
    };
}
