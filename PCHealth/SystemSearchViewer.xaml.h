#pragma once

#include "SystemSearchViewer.g.h"

#include "Searcher.h"

namespace winrt::PCHealth::implementation
{
    struct SystemSearchViewer : SystemSearchViewerT<SystemSearchViewer>
    {
    public:
        SystemSearchViewer();

        winrt::Windows::Foundation::IAsyncAction UserControl_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);
        winrt::Windows::Foundation::IAsyncAction AutoSuggestBox_QuerySubmitted(winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBox const& sender, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& args);
        void CopyPathButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void OpenFileExplorerButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void OpenFilterBoxButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        std::unique_ptr<pchealth::windows::search::Searcher> searcher;
        std::vector<winrt::hstring> searchHistory{};

        void AppendMany(const winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Storage::StorageFolder>& vect, const winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Storage::StorageFolder>& toAppend);
        void AddSearchResult(const std::vector<pchealth::windows::search::SearchResult>& finds, const bool& fromThread);
        void AddNewFile(const std::wstring& path);
        void AddNewFolder(const std::wstring& path);
        void AddNewApplication(const std::wstring& name);
        void SaveControl();
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct SystemSearchViewer : SystemSearchViewerT<SystemSearchViewer, implementation::SystemSearchViewer>
    {
    };
}
