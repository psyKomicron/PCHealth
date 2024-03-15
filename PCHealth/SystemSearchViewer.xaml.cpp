#include "pch.h"
#include "SystemSearchViewer.xaml.h"
#if __has_include("SystemSearchViewer.g.cpp")
#include "SystemSearchViewer.g.cpp"
#endif

#include "System.h"
#include "LocalSettings.h"
#include "utilities.h"

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

namespace winrt::PCHealth::implementation
{
    SystemSearchViewer::SystemSearchViewer()
    {
        searcher = std::unique_ptr<pchealth::windows::search::Searcher>(
            new pchealth::windows::search::Searcher(
                false,
                [this](auto&& s, auto&& b)
                {
                    concurrency::create_task([this, s, b]()
                    {
                        AddSearchResult(s, b);
                    });
                })
        );
    }

    winrt::Windows::Foundation::Collections::IObservableVector<winrt::hstring> SystemSearchViewer::Drives() const
    {
        return _drives;
    }

    winrt::Windows::Foundation::IAsyncAction SystemSearchViewer::UserControl_Loading(xaml::FrameworkElement const&, winrt::Windows::Foundation::IInspectable const&)
    {
        auto&& drives = pchealth::filesystem::DriveInfo::GetDrives();
        for (auto&& drive : drives)
        {
            _drives.Append(drive.name());
        }
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SystemSearchViewer::AutoSuggestBox_QuerySubmitted(xaml::Controls::AutoSuggestBox const&, xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& args)
    {
        searchApplicationCount = 0;
        searchFileCount = 0;
        searchFolderCount = 0;

        _searchResults.Clear();
        _filterResults.Clear();
        SearchResultsListView().ItemsSource(_searchResults);
        xaml::VisualStateManager::GoToState(*this, L"Search", true);

        winrt::hstring queryText = args.QueryText();
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

        if (!queryText.empty())
        {
            searchHistory.push_back(queryText);

            DispatcherQueue().TryEnqueue([&]()
            {
                RemainingTextBlock().Text(winrt::to_hstring(searcher->remaining()));
            });

            try
            {
                searcher->search(std::wstring(queryText), includedDrives);
            }
            catch (std::regex_error& ex)
            {
                SearchTextBlock().Text(winrt::to_hstring(ex.what()));
                SearchTextBlock().Visibility(xaml::Visibility::Visible);
            }

            DispatcherQueue().TryEnqueue([&]()
            {
                xaml::VisualStateManager::GoToState(*this, L"Rest", true);
            });
        }
    }

    void SystemSearchViewer::CopyPathButton_Click(winrt::Windows::Foundation::IInspectable const& sender, xaml::RoutedEventArgs const&)
    {
        winrt::hstring tag = sender.as<xaml::Controls::Button>().Tag().as<winrt::hstring>();
        winrt::Windows::ApplicationModel::DataTransfer::DataPackage package{};
        package.SetText(tag);
        winrt::Windows::ApplicationModel::DataTransfer::Clipboard::SetContent(package);
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
        static bool state = true;
        GoToVisualState(state, L"FilterSearch", L"NormalSearch");
    }

    void SystemSearchViewer::OpenOptionsButton_Click(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
    {
        static bool state = true;
        GoToVisualState(state, L"OptionsOpen", L"OptionsClose");
    }

    void SystemSearchViewer::FilterAutoSuggestBox_QuerySubmitted(xaml::Controls::AutoSuggestBox const& sender, xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& args)
    {
        auto queryText = std::wstring(args.QueryText());
        FilterSearch(queryText);
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
            FilterSearch(queryText);
        }
    }

    void SystemSearchViewer::SplitQueryOnSpacesToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
    {
    }


    void SystemSearchViewer::AddSearchResult(const std::vector<pchealth::windows::search::SearchResult>& finds, const bool& update)
    {
        DispatcherQueue().TryEnqueue([this, update]()
        {
            auto remaining = static_cast<double>(searcher->remaining());
            //RemainingTextBlock().Text(winrt::to_hstring(remaining));

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
            concurrency::parallel_for_each(std::begin(finds), std::end(finds), [this](auto searchResult)
            {
                if (searchResult.result().empty())
                {
                    return;
                }

                try
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
                catch (winrt::hresult_error e)
                {
                    OutputDebug(std::format(L"Failed to add '{}' ({}): {}", searchResult.result(), static_cast<int32_t>(searchResult.kind()), std::wstring(e.message())));
                }
                catch (...)
                {
                    OutputDebug(std::format(L"Unspecified error ('{}').", searchResult.result()));
                }
            });
        }
    }

    void SystemSearchViewer::AddNewFile(const std::wstring& path)
    {
        try
        {
            auto&& storageFile = winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(path).get();
            auto&& thumbnail = storageFile.GetThumbnailAsync(winrt::Windows::Storage::FileProperties::ThumbnailMode::SingleItem).get();

            DispatcherQueue().TryEnqueue([this, thumbnail, name = storageFile.Name(), path = storageFile.Path(), displayType = storageFile.DisplayType(), count = ++searchFileCount]()
            {
                FileInfoViewModel viewModel{};
                viewModel.Kind(static_cast<int32_t>(pchealth::windows::search::SearchResultKind::File));
                viewModel.Name(name);
                viewModel.Path(path);
                viewModel.Type(displayType);

                xaml::Media::Imaging::BitmapImage img{};
                img.SetSourceAsync(thumbnail);
                viewModel.Thumbnail(img);

                _searchResults.InsertAt(0, viewModel);

                SearchFileCountTextBlock().Text(winrt::to_hstring(count));
            });
        }
        catch (winrt::hresult_error& err)
        {
            OutputDebug(std::format(L"Failed to get thumbnail for '{}': {}", path, std::wstring(err.message())));

            DispatcherQueue().TryEnqueue([this, path, count = ++searchFileCount]()
            {
                std::filesystem::path filePath{ path };
                FileInfoViewModel viewModel{};
                viewModel.Kind(static_cast<int32_t>(pchealth::windows::search::SearchResultKind::File));
                viewModel.Name(filePath.filename().wstring());
                viewModel.Path(path);
                viewModel.Type(L"File");

                _searchResults.InsertAt(0, viewModel);

                SearchFileCountTextBlock().Text(winrt::to_hstring(count));
            });
        }   
    }

    void SystemSearchViewer::AddNewFolder(const std::wstring& path)
    {
        auto&& storageFolder = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(path).get();
        auto&& thumbnail = storageFolder.GetThumbnailAsync(winrt::Windows::Storage::FileProperties::ThumbnailMode::SingleItem).get();
        DispatcherQueue().TryEnqueue([this, thumbnail, name = storageFolder.Name(), path = storageFolder.Path(), count = ++searchFolderCount]()
        {
            FileInfoViewModel viewModel{};
            viewModel.Kind(static_cast<int32_t>(pchealth::windows::search::SearchResultKind::Directory));
            viewModel.Name(name);
            viewModel.Path(path);
            viewModel.Type(L"Directory");
            viewModel.IsDirectory(true);

            xaml::Media::Imaging::BitmapImage img{};
            img.SetSourceAsync(thumbnail);
            viewModel.Thumbnail(img);

            _searchResults.InsertAt(0, viewModel);

            SearchFolderCountTextBlock().Text(winrt::to_hstring(count));
        });
    }

    void SystemSearchViewer::AddNewApplication(const std::wstring& name)
    {
        DispatcherQueue().TryEnqueue([this, name, count = ++searchApplicationCount]()
        {
            FileInfoViewModel viewModel{};
            viewModel.Kind(static_cast<int32_t>(pchealth::windows::search::SearchResultKind::Application));
            viewModel.Name(name);
            viewModel.Path(L"");
            viewModel.Type(L"Application");
            viewModel.IsApplication(true);

            _searchResults.InsertAt(0, viewModel);

            SearchApplicationCountTextBlock().Text(winrt::to_hstring(count));
        });
    }

    void SystemSearchViewer::SaveControl()
    {
        pchealth::storage::LocalSettings settings{ winrt::Windows::Storage::ApplicationData::Current().LocalSettings() };
        settings.openOrCreateAndMoveTo(L"SystemSearchViewer");
        settings.saveList(L"History", searchHistory);
    }

    void SystemSearchViewer::GoToVisualState(bool& b, const winrt::hstring& stateA, const winrt::hstring& stateB)
    {
        xaml::VisualStateManager::GoToState(*this, 
            b ? stateA : stateB,
            true);
        b = !b;
    }

    void SystemSearchViewer::FilterSearch(std::wstring queryText)
    {
        _filterResults.Clear();
        SearchResultsListView().ItemsSource(_filterResults);
        xaml::VisualStateManager::GoToState(*this, L"FilterSearch", true);
        try
        {
            std::wregex re{ queryText, std::regex_constants::icase | std::regex_constants::egrep };

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
                        if (queryText.size() > 2)
                        {
                            queryText = queryText.substr(2);
                        }
                        else
                        {
                            queryText = L"";
                        }
                        break;
                    }
                }
            }

            for (auto&& entry : _searchResults)
            {
                auto&& item = entry.as<FileInfoViewModel>();
                auto itemName = std::wstring(item.Name());
                if ((item.Kind() == static_cast<int32_t>(kind) || kind == pchealth::windows::search::SearchResultKind::None) 
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
}
