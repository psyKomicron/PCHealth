#pragma once

#include "SystemSearchViewer.g.h"

#include "Searcher.h"

namespace winrt::PCHealth::implementation
{
    struct SystemSearchViewer : SystemSearchViewerT<SystemSearchViewer>
    {
    public:
        SystemSearchViewer();

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::hstring> Drives() const;

        winrt::Windows::Foundation::IAsyncAction UserControl_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);
        winrt::Windows::Foundation::IAsyncAction AutoSuggestBox_QuerySubmitted(winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBox const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& args);
        void CopyPathButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::Windows::Foundation::IAsyncAction OpenFileExplorerButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void OpenFilterBoxButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void OpenOptionsButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void FilterAutoSuggestBox_QuerySubmitted(winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBox const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& args);
        void IncludeAllDrivesToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void FilterAutoSuggestBox_TextChanged(winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBox const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxTextChangedEventArgs const& args);
        void SplitQueryOnSpacesToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        bool loaded = false;
        std::unique_ptr<pchealth::windows::search::Searcher> searcher;
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::hstring> _drives = winrt::single_threaded_observable_vector<winrt::hstring>();
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::PCHealth::FileInfoViewModel> _searchResults = winrt::single_threaded_observable_vector<winrt::PCHealth::FileInfoViewModel>();
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::PCHealth::FileInfoViewModel> _filterResults = winrt::single_threaded_observable_vector<winrt::PCHealth::FileInfoViewModel>();
        std::vector<winrt::hstring> searchHistory{};
        int32_t searchApplicationCount = 0;
        int32_t searchFileCount = 0;
        int32_t searchFolderCount = 0;

        void AppendMany(const winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Storage::StorageFolder>& vect, const winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Storage::StorageFolder>& toAppend);
        void AddSearchResult(const std::vector<pchealth::windows::search::SearchResult>& finds, const bool& fromThread);
        void AddNewFile(const std::wstring& path);
        void AddNewFolder(const std::wstring& path);
        void AddNewApplication(const std::wstring& name);
        void SaveControl();
        void GoToVisualState(bool& b, const winrt::hstring& stateA, const winrt::hstring& stateB);
        void FilterSearch(std::wstring query);
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct SystemSearchViewer : SystemSearchViewerT<SystemSearchViewer, implementation::SystemSearchViewer>
    {
    };
}
