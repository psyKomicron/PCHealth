#pragma once

#include "SystemSearchViewer.g.h"

#include "Searcher.h"
#include "VisualStateManager.hpp"

namespace winrt::PCHealth::implementation
{
    struct SystemSearchViewer : SystemSearchViewerT<SystemSearchViewer>
    {
    public:
        SystemSearchViewer();


        winrt::Windows::Foundation::Collections::IObservableVector<winrt::hstring> Drives() const;

        winrt::Windows::Foundation::IAsyncAction Search(const winrt::hstring& query);


        winrt::Windows::Foundation::IAsyncAction UserControl_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);
        
        winrt::Windows::Foundation::IAsyncAction AutoSuggestBox_QuerySubmitted(winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBox const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& args);
        
        void CopyPathButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
        winrt::Windows::Foundation::IAsyncAction OpenFileExplorerButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
        void OpenFilterBoxButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
        void OpenOptionsButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
        void FilterAutoSuggestBox_QuerySubmitted(winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBox const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& args);
        
        void IncludeAllDrivesToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
        void FilterAutoSuggestBox_TextChanged(winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBox const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxTextChangedEventArgs const& args);
        
        void MatchFilePathToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
        void SearchResultsListView_DoubleTapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::DoubleTappedRoutedEventArgs const& e);
        
        void CancelSearchButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        bool loaded = false;
        std::unique_ptr<pchealth::windows::search::Searcher> searcher;
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::hstring> _drives = winrt::single_threaded_observable_vector<winrt::hstring>();
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::PCHealth::FileInfoViewModel> _searchResults = winrt::single_threaded_observable_vector<winrt::PCHealth::FileInfoViewModel>();
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::PCHealth::FileInfoViewModel> _filterResults = winrt::single_threaded_observable_vector<winrt::PCHealth::FileInfoViewModel>();
        std::vector<winrt::hstring> searchHistory{};
        std::atomic_uint32_t searchApplicationCount = 0;
        std::atomic_uint32_t searchFileCount = 0;
        std::atomic_uint32_t searchFolderCount = 0;
        PCHealth::VisualStateManager<SystemSearchViewer> visualStateManager{ *this };
        bool _loadThumbnails = true;


        void AppendMany(const winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Storage::StorageFolder>& vect, const winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Storage::StorageFolder>& toAppend);
        
        void AddSearchResult(std::vector<pchealth::windows::search::SearchResult> finds, const bool& fromThread);
        
        winrt::Windows::Foundation::IAsyncAction AddNewFile(std::wstring path);
        
        winrt::Windows::Foundation::IAsyncAction AddNewFolder(std::wstring path);
        
        void AddNewApplication(const std::wstring& name);
        
        void SaveControl();
        
        void FilterSearch(std::wstring query);
        
        void AddMessage(winrt::hstring message, int32_t level);
    };
}

namespace winrt::PCHealth::implementation::SystemSearchViewerStates
{
    const PCHealth::VisualState<SystemSearchViewer> NormalState{ L"NormalSearch", 1, true };
    const PCHealth::VisualState<SystemSearchViewer> FilterSearchState{ L"FilterSearch", 1, false };
    const PCHealth::VisualState<SystemSearchViewer> OptionsCloseState{ L"OptionsClose", 2, true };
    const PCHealth::VisualState<SystemSearchViewer> OptionsOpenState{ L"OptionsOpen", 2, false };
    const PCHealth::VisualState<SystemSearchViewer> RestState{ L"Rest", 3, true };
    const PCHealth::VisualState<SystemSearchViewer> SearchState{ L"Search", 3, false };
}

namespace winrt::PCHealth::factory_implementation
{
    struct SystemSearchViewer : SystemSearchViewerT<SystemSearchViewer, implementation::SystemSearchViewer>
    {
    };
}
