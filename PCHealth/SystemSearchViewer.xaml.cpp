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

#include <ppltasks.h>
#include <ppl.h>
#include <limits>

namespace collections = winrt::Windows::Foundation::Collections;
namespace xaml = winrt::Microsoft::UI::Xaml;

namespace winrt::PCHealth::implementation
{
    SystemSearchViewer::SystemSearchViewer()
    {
        searcher = std::unique_ptr<pchealth::windows::search::Searcher > (
            new pchealth::windows::search::Searcher(std::chrono::milliseconds(300),
            false,
            [this](auto&& s, auto&& b)
            {
                AddSearchResult(s, b);
            })
        );
    }

    winrt::Windows::Foundation::IAsyncAction SystemSearchViewer::UserControl_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const&, winrt::Windows::Foundation::IInspectable const&)
    {
        /*auto&& folders = winrt::single_threaded_vector<winrt::Windows::Storage::StorageFolder>();
        AppendMany(folders, (co_await winrt::Windows::Storage::StorageLibrary::GetLibraryAsync(winrt::Windows::Storage::KnownLibraryId::Documents)).Folders());
        AppendMany(folders, (co_await winrt::Windows::Storage::StorageLibrary::GetLibraryAsync(winrt::Windows::Storage::KnownLibraryId::Music)).Folders());
        AppendMany(folders, (co_await winrt::Windows::Storage::StorageLibrary::GetLibraryAsync(winrt::Windows::Storage::KnownLibraryId::Pictures)).Folders());
        AppendMany(folders, (co_await winrt::Windows::Storage::StorageLibrary::GetLibraryAsync(winrt::Windows::Storage::KnownLibraryId::Videos)).Folders());

        for (auto&& folder : folders)
        {
            AddNewFile(std::wstring(folder.Path()), true);
            auto&& files = co_await folder.GetFilesAsync();
            for (auto&& file : files)
            {
                AddNewFile(std::wstring(file.Path()), false);
            }
        }*/
        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction SystemSearchViewer::AutoSuggestBox_QuerySubmitted(winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBox const&, winrt::Microsoft::UI::Xaml::Controls::AutoSuggestBoxQuerySubmittedEventArgs const& args)
    {
        SearchResultsListView().Items().Clear();
        xaml::VisualStateManager::GoToState(*this, L"Search", true);

        winrt::hstring queryText = args.QueryText();
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
                searcher->search(std::wstring(queryText));
            }
            catch (std::exception& ex)
            {
                OutputDebug(ex.what());
            }

            DispatcherQueue().TryEnqueue([&]()
            {
                xaml::VisualStateManager::GoToState(*this, L"Rest", true);
            });
        }
    }

    void SystemSearchViewer::CopyPathButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        winrt::hstring tag = sender.as<winrt::Microsoft::UI::Xaml::Controls::Button>().Tag().as<winrt::hstring>();
        winrt::Windows::ApplicationModel::DataTransfer::DataPackage package{};
        package.SetText(tag);
        winrt::Windows::ApplicationModel::DataTransfer::Clipboard::SetContent(package);
    }

    void SystemSearchViewer::OpenFileExplorerButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        winrt::hstring tag = sender.as<winrt::Microsoft::UI::Xaml::Controls::Button>().Tag().as<winrt::hstring>();
        std::filesystem::path path{ std::wstring(tag) };
        auto&& parent = path.parent_path();
        winrt::Windows::System::Launcher::LaunchUriAsync(winrt::Windows::Foundation::Uri(parent.wstring()));
    }

    void SystemSearchViewer::OpenFilterBoxButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        static bool state = true;
        if (state)
        {
            xaml::VisualStateManager::GoToState(*this, L"FilterSearch", true);
        }
        else
        {
            xaml::VisualStateManager::GoToState(*this, L"NormalSearch", true);
        }

        state = !state;
    }


    void SystemSearchViewer::AppendMany(const collections::IVector<winrt::Windows::Storage::StorageFolder>& vect, const collections::IVector<winrt::Windows::Storage::StorageFolder>& toAppend)
    {
        for (auto&& item : toAppend)
        {
            vect.Append(item);
        }
    }

    void SystemSearchViewer::AddSearchResult(const std::vector<pchealth::windows::search::SearchResult>& finds, const bool& update)
    {
        concurrency::create_task([this, finds, update]()
        {
            DispatcherQueue().TryEnqueue([this, update]()
            {
                auto remaining = static_cast<double>(searcher->remaining());
                RemainingTextBlock().Text(winrt::to_hstring(LoadingProgressRing().Maximum()) + L"/" + winrt::to_hstring(remaining));

                if (remaining > 0)
                {
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
                        OutputDebug(std::format(L"Failed to add '{}': {}", searchResult.result(), std::wstring(e.message())));
                    }
                    catch (...)
                    {
                        OutputDebug(std::format(L"Unspecified error ('{}').", searchResult.result()));
                    }
                });
            }

        });
    }

    void SystemSearchViewer::AddNewFile(const std::wstring& path)
    {
        auto&& storageFile = winrt::Windows::Storage::StorageFile::GetFileFromPathAsync(path).get();
        auto&& thumbnail = storageFile.GetThumbnailAsync(winrt::Windows::Storage::FileProperties::ThumbnailMode::SingleItem).get();
        DispatcherQueue().TryEnqueue([this, thumbnail, name = storageFile.Name(), path = storageFile.Path(), displayType = storageFile.DisplayType()]()
        {
            FileInfoViewModel viewModel{};
            viewModel.Name(name);
            viewModel.Path(path);
            viewModel.Type(displayType);

            winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage img{};
            img.SetSourceAsync(thumbnail);
            viewModel.Thumbnail(img);

            SearchResultsListView().Items().InsertAt(0, viewModel);
        });
    }

    void SystemSearchViewer::AddNewFolder(const std::wstring& path)
    {
        auto&& storageFolder = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(path).get();
        auto&& thumbnail = storageFolder.GetThumbnailAsync(winrt::Windows::Storage::FileProperties::ThumbnailMode::SingleItem).get();
        DispatcherQueue().TryEnqueue([this, thumbnail, name = storageFolder.Name(), path = storageFolder.Path()]()
        {
            FileInfoViewModel viewModel{};
            viewModel.Name(name);
            viewModel.Path(path);
            viewModel.Type(L"Directory");
            viewModel.IsDirectory(true);

            winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage img{};
            img.SetSourceAsync(thumbnail);
            viewModel.Thumbnail(img);
            SearchResultsListView().Items().InsertAt(0, viewModel);
        });
    }

    void SystemSearchViewer::AddNewApplication(const std::wstring& name)
    {
        DispatcherQueue().TryEnqueue([this, name]()
        {
            FileInfoViewModel viewModel{};
            viewModel.Name(name);
            viewModel.Path(L"");
            viewModel.Type(L"Application");
            viewModel.IsApplication(true);

            SearchResultsListView().Items().InsertAt(0, viewModel);
        });
    }

    void SystemSearchViewer::SaveControl()
    {
        pchealth::storage::LocalSettings settings{ winrt::Windows::Storage::ApplicationData::Current().LocalSettings() };
        settings.openOrCreateAndMoveTo(L"SystemSearchViewer");
        settings.saveList(L"History", searchHistory);
    }
}
