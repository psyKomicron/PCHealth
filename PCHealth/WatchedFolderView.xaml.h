#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "WatchedFolderView.g.h"

#include "DirectoryWatcher.h"

namespace winrt::PCHealth::implementation
{
    struct WatchedFolderView : WatchedFolderViewT<WatchedFolderView>
    {
    public:
        WatchedFolderView();
        WatchedFolderView(const winrt::hstring& folderPath);

        event_token PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& value);
        void PropertyChanged(event_token const& token);

        winrt::hstring FolderPath() const;
        void FolderPath(const winrt::hstring& value);
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::hstring> Items() const;

        winrt::Windows::Foundation::IAsyncAction UserControl_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);
        void ThumbnailsViewToggleButton_Checked(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ListViewToggleButton_Checked(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ListViewToggleButton_Unchecked(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::Windows::Foundation::IAsyncAction FilesGridView_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);
        void ThumbnailSizeSlider_ValueChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e);
        winrt::Windows::Foundation::IAsyncAction WatchChangesToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        winrt::hstring _folderPath{};
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::hstring> _items = winrt::multi_threaded_observable_vector<winrt::hstring>();
        bool loaded = false;
        std::unique_ptr<pchealth::filesystem::DirectoryWatcher> watcherPtr{};

        event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> e_propertyChanged;
    
        void ViewModeChanged();
        void ChangesDetected(std::wstring e);
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct WatchedFolderView : WatchedFolderViewT<WatchedFolderView, implementation::WatchedFolderView>
    {
    };
}
