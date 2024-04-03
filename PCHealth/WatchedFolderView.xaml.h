#pragma once

#include "DirectoryWatcher.h"
#include "VisualStateManager.hpp"
#include "VisualState.hpp"

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "WatchedFolderView.g.h"


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

        winrt::hstring FolderName() const;
        void FolderName(const winrt::hstring& value);

        winrt::hstring FolderSize() const;
        void FolderSize(const winrt::hstring& value);

        uint32_t ItemCount() const;
        void ItemCount(const uint32_t& count);
        
        uint32_t FolderCount() const;
        void FolderCount(const uint32_t& count);

        bool IsFavorite() const;
        void IsFavorite(const bool& value);

    public:
        winrt::Windows::Foundation::IAsyncAction UserControl_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);
        void ThumbnailsViewToggleButton_Checked(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ListViewToggleButton_Checked(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ListViewToggleButton_Unchecked(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::Windows::Foundation::IAsyncAction FilesGridView_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);
        void ThumbnailSizeSlider_ValueChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e);
        winrt::Windows::Foundation::IAsyncAction WatchChangesToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::Windows::Foundation::IAsyncAction HomeViewGrid_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);
        void HomeViewGrid_PointerEntered(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void HomeViewGrid_PointerExited(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void FavoriteButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        const uint32_t thumbnailHeight = 110u;
        winrt::hstring _folderPath{};
        winrt::hstring _folderSizeString{};
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::hstring> _items = winrt::multi_threaded_observable_vector<winrt::hstring>();
        bool loaded = false;
        std::unique_ptr<pchealth::filesystem::DirectoryWatcher> watcherPtr{};
        Microsoft::UI::Dispatching::DispatcherQueueTimer thumbnailSwitchingTimer{ nullptr };
        event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> e_propertyChanged;
        std::vector<Windows::Storage::FileProperties::StorageItemThumbnail> folderThumbnails{};
        bool thumbnailListLoaded = false;
        size_t thumbnailIndex = 0;
        PCHealth::VisualStateManager<WatchedFolderView> visualStateManager{ *this };
        uint32_t _folderCount = 0;
        bool _isFavorite = false;
    
    private:

        template<typename IStorageItem>
        winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::FileProperties::StorageItemThumbnail> getItemThumbnail(const IStorageItem& item, const uint32_t& size)
        {
            return item.GetScaledImageAsThumbnailAsync(Windows::Storage::FileProperties::ThumbnailMode::SingleItem, size, Windows::Storage::FileProperties::ThumbnailOptions::ResizeThumbnail);
        }

        void ViewModeChanged();
        
        void ChangesDetected(std::wstring e);
        
        void RaisePropChanged(std::source_location location = std::source_location::current());
        
        winrt::Windows::Foundation::IAsyncAction GetFolderSize();
        
        winrt::Windows::Foundation::IAsyncAction LoadThumbnailList();
        
        void SwitchThumbnail(const Microsoft::UI::Dispatching::DispatcherQueueTimer&, const Windows::Foundation::IInspectable&);
        
        void ClearCurrentThumbnail();


    private:
        const PCHealth::VisualState<WatchedFolderView> PointerOffState{ L"PointerOff", 0, true };
        const PCHealth::VisualState<WatchedFolderView> PointerOnState{ L"PointerOn", 0, false };
        const PCHealth::VisualState<WatchedFolderView> NormalViewState{ L"NormalView", 1, true };
        const PCHealth::VisualState<WatchedFolderView> HomeViewState{ L"HomeView", 1, false };
        const PCHealth::VisualState<WatchedFolderView> NonFavoriteState{ L"NonFavorite", 2, true };
        const PCHealth::VisualState<WatchedFolderView> FavoriteState{ L"Favorite", 2, false };
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct WatchedFolderView : WatchedFolderViewT<WatchedFolderView, implementation::WatchedFolderView>
    {
    };
}
