#include "pch.h"
#include "SystemSearchViewer.xaml.h"
#if __has_include("SystemSearchViewer.g.cpp")
#include "SystemSearchViewer.g.cpp"
#endif

#include "System.h"
#include "LocalSettings.h"
#include "utilities.h"
#include "CachedSearcher.hpp"

#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.FileProperties.h>
#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Windows.System.Diagnostics.h>

#include <ppltasks.h>
#include <ppl.h>
#include <limits>

namespace collections = winrt::Windows::Foundation::Collections;
namespace xaml = winrt::Microsoft::UI::Xaml;
namespace storage = winrt::Windows::Storage;

using namespace winrt::PCHealth::implementation;

SystemSearchViewer::SystemSearchViewer()
{
    searcher = std::unique_ptr<pchealth::windows::search::CachedSearcher>(
        new pchealth::windows::search::CachedSearcher(
            [this](auto&& s, auto&& b)
            {
                concurrency::create_task([this, vector = std::move(s), b]()
                {
                    AddSearchResults(vector, b);
                });
            }));

    // Initialize visual state manager states.
    visualStateManager.initializeStates
    ({
        SystemSearchViewerStates::FilterSearchState,
        SystemSearchViewerStates::NormalState,
        SystemSearchViewerStates::OptionsCloseState,
        SystemSearchViewerStates::OptionsOpenState,
        SystemSearchViewerStates::RestState,
        SystemSearchViewerStates::SearchState,
    });
}

winrt::Windows::Foundation::Collections::IObservableVector<winrt::hstring> SystemSearchViewer::Drives() const
{
    return _drives;
}

winrt::Windows::Foundation::IAsyncAction SystemSearchViewer::Search(const winrt::hstring& _query)
{
    // Capture parameter.
    std::wstring query{ _query };

    if (query.empty())
    {
        AddMessage(L"Query is empty.", 3);
        co_return;
    }

    searchApplicationCount.store(0);
    searchFileCount.store(0);
    searchFolderCount.store(0);

    ActivateUIForSearch();

    TryParseQuery(query);

    std::vector<pchealth::filesystem::DriveInfo> includedDrives{};
    if (IncludeAllDrivesToggleSwitch().IsOn())
    {
        auto&& selected = DrivesGridView().SelectedItems();
        bool includeAll = selected.Size() == Drives().Size();
        if (!includeAll)
        {
            for (auto&& item : selected)
            {
                includedDrives.push_back(pchealth::filesystem::DriveInfo(std::wstring(item.as<winrt::hstring>())));
            }
        }
    }

    co_await winrt::resume_background();


    DispatcherQueue().TryEnqueue([&]()
    {
        RemainingTextBlock().Text(winrt::to_hstring(searcher->remaining()));
    });

    try
    {
        searcher->search(query, includedDrives);
        searchHistory.push_back(query.data());
    }
    catch (boost::regex_error err)
    {
        DispatcherQueue().TryEnqueue([this, text = winrt::to_hstring(err.what())]()
        {
            SearchTextBlock().Text(text);
            SearchTextBlock().Visibility(xaml::Visibility::Visible);
        });
    }
    catch (std::invalid_argument& ex)
    {
        DispatcherQueue().TryEnqueue([this, text = winrt::to_hstring(ex.what())]()
        {
            SearchTextBlock().Text(text);
            SearchTextBlock().Visibility(xaml::Visibility::Visible);
        });
    }

    DispatcherQueue().TryEnqueue([this, text = winrt::hstring(query)]()
    {
        visualStateManager.goToState(SystemSearchViewerStates::RestState);
        AddMessage(L"Finished searching: " + text, 0);
    });
}

winrt::Windows::Foundation::IAsyncAction SystemSearchViewer::UserControl_Loading(xaml::FrameworkElement const&, winrt::Windows::Foundation::IInspectable const&)
{
    co_await resume_background();
    
    auto&& drives = pchealth::filesystem::DriveInfo::getDrives();
    for (auto&& drive : drives)
    {
        DispatcherQueue().TryEnqueue([this, driveName = drive.name()]()
        {
            _drives.Append(driveName);
        });
    }
}

winrt::Windows::Foundation::IAsyncAction SystemSearchViewer::AutoSuggestBox_QuerySubmitted(xaml::Controls::AutoSuggestBox const&, xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& args)
{
    SearchAutoSuggestBox().Text(args.QueryText());
    co_await Search(args.QueryText());
}

void SystemSearchViewer::CopyPathButton_Click(winrt::Windows::Foundation::IInspectable const& sender, xaml::RoutedEventArgs const&)
{
    hstring tag = sender.as<xaml::Controls::Button>().Tag().as<hstring>();
    Windows::ApplicationModel::DataTransfer::DataPackage package{};
    package.SetText(tag);
    Windows::ApplicationModel::DataTransfer::Clipboard::SetContent(package);
}

winrt::Windows::Foundation::IAsyncAction SystemSearchViewer::OpenFileExplorerButton_Click(winrt::Windows::Foundation::IInspectable const& sender, xaml::RoutedEventArgs const&)
{
    winrt::hstring tag = sender.as<xaml::Controls::Button>().Tag().as<winrt::hstring>();
    std::filesystem::path path{ std::wstring(tag) };

    co_await winrt::resume_background();

    try
    {
        pchealth::windows::System::openExplorer(path.wstring());
    }
    catch (winrt::hresult_error err)
    {
        OutputDebug(std::wstring(err.message()));
    }
}

void SystemSearchViewer::OpenFilterBoxButton_Click(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
{
    visualStateManager.switchState(SystemSearchViewerStates::FilterSearchState.group());
}

void SystemSearchViewer::OpenOptionsButton_Click(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
{
    visualStateManager.switchState(SystemSearchViewerStates::OptionsOpenState.group());
}

void SystemSearchViewer::FilterAutoSuggestBox_QuerySubmitted(xaml::Controls::AutoSuggestBox const&, xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& args)
{
    auto queryText = std::wstring(args.QueryText());
    Filter(queryText);
}

void SystemSearchViewer::IncludeAllDrivesToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
{
    DrivesGridView().IsEnabled(IncludeAllDrivesToggleSwitch().IsOn());
}

void SystemSearchViewer::FilterAutoSuggestBox_TextChanged(xaml::Controls::AutoSuggestBox const&, xaml::Controls::AutoSuggestBoxTextChangedEventArgs const& args)
{
    if (args.CheckCurrent())
    {
        auto queryText = std::wstring(FilterAutoSuggestBox().Text());
        Filter(queryText);
    }
}

void SystemSearchViewer::MatchFilePathToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
{
    searcher->searchFilePath(MatchFilePathToggleSwitch().IsOn());
}

void SystemSearchViewer::SearchResultsListView_DoubleTapped(winrt::Windows::Foundation::IInspectable const&, xaml::Input::DoubleTappedRoutedEventArgs const&)
{
    auto&& selectedItem = SearchResultsListView().SelectedItem();
    if (selectedItem != nullptr)
    {
        auto view = selectedItem.try_as<PCHealth::FileInfoViewModel>();
        if (view != nullptr)
        {
            Windows::System::Launcher::LaunchUriAsync(Windows::Foundation::Uri(view.Path())).Completed([this, view](auto&& task, auto&&)
            {
                bool b = task.GetResults();
                if (!b)
                {
                    OUTPUT_DEBUG(L"[SystemSearchViewer] Launcher couldn't launch file, falling back to Win32 api.");
                    try
                    {
                        pchealth::windows::System::launch(std::wstring(view.Path()));
                        b = true;
                    }
                    catch (const winrt::hresult_error& ex)
                    {
                        OUTPUT_DEBUG(std::format(L"[SystemSearchViewer] System::launch failed: {}", ex.message().data()));
                    }
                }

                DispatcherQueue().TryEnqueue([this, b, viewName = view.Name()]()
                {
                    if (b)
                    {
                        AddMessage(L"Launched " + viewName, 1);
                    }
                    else
                    {
                        AddMessage(L"Failed to launch " + viewName, 3);
                    }
                });
            });
        }
    }
}

void SystemSearchViewer::CancelSearchButton_Click(Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
{
    AddMessage(L"Cancelling current search...", 2);
    if (searcher->isSearchRunning())
    {
        searcher->cancelCurrentSearch();
        AddMessage(L"Cancelled search.", 2);
    }
}


void SystemSearchViewer::AddSearchResults(std::vector<pchealth::windows::search::SearchResult> finds, const bool& update)
{
    if (!searcher->isSearchCancelled() 
        && (searchFileCount.load() + searchFolderCount.load() + searchApplicationCount.load()) > 5000)
    {
        searcher->cancelCurrentSearch();
        DispatcherQueue().TryEnqueue([this]()
        {
            AddMessage(L"Cancelled search, over 1k results.", 2);
        });
    }
    else if (!searcher->isSearchCancelled())
    {
        DispatcherQueue().TryEnqueue([this]()
        {
            auto remaining = static_cast<double>(searcher->remaining());
            RemainingTextBlock().Text(winrt::to_hstring(remaining));

            if (remaining > 0)
            {
                LoadingProgressRing().IsIndeterminate(false);
                if (LoadingProgressRing().Maximum() < remaining)
                {
                    LoadingProgressRing().Maximum(remaining);
                }
                LoadingProgressRing().Value(remaining);
            }
            else
            {
                LoadingProgressRing().IsIndeterminate(true);
            }
        });

        if (update)
        {
            const bool dispatchForElements = true;

            if constexpr (dispatchForElements)
            {
                for (auto&& searchResult : finds)
                {
                    if (!searchResult.result().empty())
                    {
                        switch (searchResult.kind())
                        {
                            case pchealth::windows::search::SearchResultKind::Directory:
                            {
                                DispatcherQueue().TryEnqueue([this, result = searchResult.result()]()
                                {
                                    AddNewFolder(result);
                                });
                                break;
                            }
                            case pchealth::windows::search::SearchResultKind::File:
                            {
                                DispatcherQueue().TryEnqueue([this, result = searchResult.result()]()
                                {
                                    AddNewFile(result);
                                });
                                break;
                            }
                            case pchealth::windows::search::SearchResultKind::Application:
                            {
                                DispatcherQueue().TryEnqueue([this, result = searchResult.result()]()
                                {
                                    AddNewApplication(result);
                                });
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                DispatcherQueue().TryEnqueue([this, update, results = std::move(finds)]()
                {
                    for (auto&& searchResult : results)
                    {
                        if (!searchResult.result().empty())
                        {
                            switch (searchResult.kind())
                            {
                                case pchealth::windows::search::SearchResultKind::Directory:
                                {
                                    AddNewFolder(searchResult.result());
                                    break;
                                }
                                case pchealth::windows::search::SearchResultKind::File:
                                {
                                    AddNewFile(searchResult.result());
                                    break;
                                }
                                case pchealth::windows::search::SearchResultKind::Application:
                                {
                                    AddNewApplication(searchResult.result());
                                    break;
                                }
                            }
                        }
                    }
                });
            }
        }
    }
}

winrt::Windows::Foundation::IAsyncAction SystemSearchViewer::AddNewFile(std::wstring path)
{
    FileInfoViewModel viewModel{};
    viewModel.Kind(static_cast<int32_t>(pchealth::windows::search::SearchResultKind::File));

    try
    {
        auto&& storageFile = co_await winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(path);

        winrt::hstring name = storageFile.Name();
        auto&& displayType = storageFile.DisplayType();
        auto&& storageFilePath = storageFile.Path();

        viewModel.Name(name);
        viewModel.Path(storageFilePath);
        viewModel.Type(displayType);

        if (_loadThumbnails)
        {
            try
            {
                auto&& thumbnail = co_await storageFile.GetScaledImageAsThumbnailAsync(storage::FileProperties::ThumbnailMode::SingleItem, 50, storage::FileProperties::ThumbnailOptions::ReturnOnlyIfCached);
                xaml::Media::Imaging::BitmapImage img{};
                img.DecodePixelHeight(50);
                img.SetSourceAsync(thumbnail);
                viewModel.Thumbnail(img);
            }
            catch (winrt::hresult_error& err)
            {
                OutputDebug(std::format(L"[SystemSearchViewer] Failed to get file thumbnail: {}\n\t({})", path, std::wstring(err.message())));
            }
        }

        _searchResults.InsertAt(0, viewModel);

        SearchFileCountTextBlock().Text(winrt::to_hstring(searchFileCount.fetch_add(1)));
    }
    catch (winrt::hresult_error& err)
    {
        OutputDebug(std::format(L"[SystemSearchViewer] Failed to get file from path: {}", path, std::wstring(err.message())));

        std::filesystem::path filePath{ path };
        viewModel.Name(filePath.filename().wstring());
        viewModel.Path(filePath.wstring());
        viewModel.Type(L"File");

        _searchResults.InsertAt(0, viewModel);

        SearchFileCountTextBlock().Text(winrt::to_hstring(searchFileCount.fetch_add(1)));
    }   
}

winrt::Windows::Foundation::IAsyncAction SystemSearchViewer::AddNewFolder(std::wstring path)
{
    FileInfoViewModel viewModel{};
    viewModel.Kind(static_cast<int32_t>(pchealth::windows::search::SearchResultKind::Directory));
    viewModel.Type(L"Directory");
    viewModel.IsDirectory(true);

    try
    {
        auto&& storageFolder = co_await winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(path);
        viewModel.Name(storageFolder.Name());
        viewModel.Path(storageFolder.Path());

        if (_loadThumbnails)
        {
            try
            {
                auto&& thumbnail = co_await storageFolder.GetScaledImageAsThumbnailAsync(storage::FileProperties::ThumbnailMode::SingleItem, 50, storage::FileProperties::ThumbnailOptions::ReturnOnlyIfCached);
                xaml::Media::Imaging::BitmapImage img{};
                img.SetSourceAsync(thumbnail);
                img.DecodePixelHeight(50);
                viewModel.Thumbnail(img);
            }
            catch (const winrt::hresult_error& error)
            {
                OutputDebug(std::format(L"[SystemSearchViewer] Failed to get folder thumbnail: {}\n\t({}).", path, std::wstring(error.message())));
            }
        }

        _searchResults.InsertAt(0, viewModel);

        SearchFolderCountTextBlock().Text(winrt::to_hstring(searchFolderCount.fetch_add(1)));
    }
    catch (const winrt::hresult_error& error)
    {
        OUTPUT_DEBUG(std::format(L"[SystemSearchViewer] Failed to get folder from path: {}", std::wstring(error.message())));

        std::filesystem::path filePath{ path };
        viewModel.Name(filePath.filename().wstring());
        viewModel.Path(filePath.wstring());

        _searchResults.InsertAt(0, viewModel);

        SearchFolderCountTextBlock().Text(winrt::to_hstring(searchFolderCount.fetch_add(1)));
    }
}

void SystemSearchViewer::AddNewApplication(const std::wstring& name)
{
    FileInfoViewModel viewModel{};
    viewModel.Kind(static_cast<int32_t>(pchealth::windows::search::SearchResultKind::Application));
    viewModel.Name(name);
    viewModel.Path(L"");
    viewModel.Type(L"Application");
    viewModel.IsApplication(true);

    _searchResults.InsertAt(0, viewModel);

    SearchApplicationCountTextBlock().Text(winrt::to_hstring(searchApplicationCount.fetch_add(1)));
}

void SystemSearchViewer::SaveControl()
{
    pchealth::storage::LocalSettings settings{ winrt::Windows::Storage::ApplicationData::Current().LocalSettings() };
    settings.openOrCreateAndMoveTo(L"SystemSearchViewer");
    settings.saveList(L"History", searchHistory);
}

void SystemSearchViewer::Filter(std::wstring queryText)
{
    _filterResults.Clear();
    SearchResultsListView().ItemsSource(_filterResults);
    visualStateManager.goToState(SystemSearchViewerStates::FilterSearchState);
    try
    {
        std::wregex re{ queryText };

        pchealth::windows::search::SearchResultKind kind = pchealth::windows::search::SearchResultKind::None;
        if (queryText.size() >= 2)
        {
            std::array<std::pair<std::wstring, pchealth::windows::search::SearchResultKind>, 3> array
            {
                std::make_pair(L"a:", pchealth::windows::search::SearchResultKind::Application),
                std::make_pair(L"d:", pchealth::windows::search::SearchResultKind::Directory), 
                std::make_pair(L"f:", pchealth::windows::search::SearchResultKind::File) 
            };

            for (size_t i = 0; i < array.size(); i++)
            {
                if (queryText.starts_with(array[i].first))
                { 
                    kind = array[i].second;
                    queryText = queryText.size() > 2
                        ? queryText.substr(2)
                        : L"";

                    break;
                }
            }
        }

        for (auto&& entry : _searchResults)
        {
            auto&& item = entry.as<FileInfoViewModel>();
            auto itemName = std::wstring(item.Path());

            if (itemName == queryText)
            {
                // If the name fully matches the query, insert it at the top of the list;
                _filterResults.InsertAt(0, item);
            }
            else if ((item.Kind() == static_cast<int32_t>(kind) || kind == pchealth::windows::search::SearchResultKind::None) 
                && (queryText.empty() || std::regex_search(itemName, re)))
            {
                _filterResults.Append(item);
            }
        }
    }
    catch (std::regex_error& ex)
    {
        OutputDebug(std::format("Filter search failed (regex ?): {}.", ex.what()));
        FilterSearchTextBlock().Text(winrt::to_hstring(ex.what()));
    }
}

void SystemSearchViewer::AddMessage(winrt::hstring message, int32_t level)
{
    xaml::Controls::InfoBar infoBar{};
    infoBar.Title(message);
    infoBar.IsOpen(true);
    infoBar.Severity(static_cast<xaml::Controls::InfoBarSeverity>(level));
    infoBar.Closed([this, infoBar](auto&&, auto&&)
    {
        uint32_t index = 0;
        if (InfoBarList().Children().IndexOf(infoBar, index))
        {
            InfoBarList().Children().RemoveAt(index);
        }
    });
    InfoBarList().Children().Append(infoBar);
}

void SystemSearchViewer::ActivateUIForSearch()
{
    _searchResults.Clear();
    _filterResults.Clear();

    SearchResultsListView().ItemsSource(_searchResults);
    
    SearchFileCountTextBlock().Text(L"");
    SearchFolderCountTextBlock().Text(L"");
    SearchApplicationCountTextBlock().Text(L"");
    
    visualStateManager.goToState(SystemSearchViewerStates::SearchState);
}

void SystemSearchViewer::TryParseQuery(std::wstring& query)
{
    if (!searcher->searchFilePath() && query.find(L'\\') != std::wstring::npos)
    {
        AddMessage(L"Query contains backslash ('\\'), activating 'match file path' on searcher.", 0);
    }

    const boost::wregex tokenRegex{ L"^(e|w):(.*)" };
    boost::match_results<std::wstring::const_iterator> matchResults{};
    if (query.size() > 2 && boost::regex_search(query, matchResults, tokenRegex))
    {
        auto& actualQuery = matchResults[2];
        auto& prefix = matchResults[1];

        if (prefix == L"e")
        {
            const boost::wregex escapeRegex{ L"[.^$|()\\[\\]{}*+?\\\\]" };
            const std::wstring rep{ L"\\\\&" };
            query = boost::regex_replace(std::wstring(actualQuery.first, actualQuery.second), escapeRegex, rep, boost::match_default | boost::format_sed);

            OUTPUT_DEBUG(std::format(L"[SystemSearchViewer] Escaped query '{}'.", query));

            AddMessage(std::format(L"Escaped query: {}", query).data(), 0);
        }
        else if (prefix == L"w")
        {
            query = std::format(L"[[:space:]]{}[[:space:]]", std::wstring(actualQuery.first, actualQuery.second));
        }
    }
}