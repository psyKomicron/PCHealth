#include "pch.h"
#include "WatchedFolderView.xaml.h"
#if __has_include("WatchedFolderView.g.cpp")
#include "WatchedFolderView.g.cpp"
#endif

#include "DirectorySizeCalculator.h"
#include "FileSize.h"
#include "utilities.h"

#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

#include <filesystem>
#include <LocalSettings.h>

namespace xaml = winrt::Microsoft::UI::Xaml;

namespace winrt::PCHealth::implementation
{
    WatchedFolderView::WatchedFolderView()
    {
        std::vector<VisualState<WatchedFolderView>> states
        {
            PointerOffState,
            PointerOnState,
            NormalViewState,
            HomeViewState,
            NonFavoriteState,
            FavoriteState
        };
        visualStateManager.initializeStates(states);
    }

    WatchedFolderView::WatchedFolderView(const winrt::hstring& folderPath)
        : WatchedFolderView()
    {
        _folderPath = folderPath;
        // Callback function used by the directory watcher.
        auto&& func = [this](std::vector<pchealth::filesystem::Win32FileInformation>& e)
        {
            std::wstring pathes = L"";
            uint64_t addedSize = 0;
            for (auto&& fileInfo : e)
            {
                pathes += fileInfo.name() + L", ";
                addedSize += fileInfo.size();
            }

            //I18N: Translate messages.
            winrt::PCHealth::MainWindow::Current().PostMessageToWindow(
                std::format(L"Directory watcher detected changes in '{}', {} file(s) changed : {}", watcherPtr->path(), e.size(), pathes),
                std::format(L"'{}': {} file(s) changed, ~{} bytes.", watcherPtr->path(), e.size(), addedSize),
                false
            );
        };
        watcherPtr = std::unique_ptr<pchealth::filesystem::DirectoryWatcher>(new pchealth::filesystem::DirectoryWatcher(std::wstring(_folderPath), func));
    }

    event_token WatchedFolderView::PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& value)
    {
        return e_propertyChanged.add(value);
    }

    void WatchedFolderView::PropertyChanged(event_token const& token)
    {
        e_propertyChanged.remove(token);
    }
    
    winrt::hstring WatchedFolderView::FolderPath() const
    {
        return _folderPath;
    }
    
    void WatchedFolderView::FolderPath(const winrt::hstring& value)
    {
        _folderPath = value;
        RaisePropChanged();
    }

    winrt::Windows::Foundation::Collections::IObservableVector<winrt::hstring> WatchedFolderView::Items() const
    {
        return _items;
    }

    winrt::hstring WatchedFolderView::FolderName() const
    {
        auto folderPath = std::filesystem::path(std::wstring(_folderPath));
        winrt::hstring folderName = L"";
        if (!folderPath.has_filename())
        {
            folderName = winrt::hstring(folderPath.parent_path().filename().wstring());
        }
        else
        {
            folderName = winrt::hstring(folderPath.filename().wstring());
        }
        return folderName;
    }

    void WatchedFolderView::FolderName(const winrt::hstring& /*value*/)
    {
        // TODO: To do ?
        RaisePropChanged();
    }

    winrt::hstring WatchedFolderView::FolderSize() const
    {
        return _folderSizeString;
    }

    void WatchedFolderView::FolderSize(const winrt::hstring& value)
    {
        _folderSizeString = value;
        RaisePropChanged();
    }

    uint32_t WatchedFolderView::ItemCount() const
    {
        return _items.Size();
    }

    void WatchedFolderView::ItemCount(const uint32_t& /*count*/)
    {
        RaisePropChanged();
    }

    uint32_t WatchedFolderView::FolderCount() const
    {
        return _folderCount;
    }

    void WatchedFolderView::FolderCount(const uint32_t& count)
    {
        _folderCount = count;
        RaisePropChanged();
    }

    bool WatchedFolderView::IsFavorite() const
    {
        return _isFavorite;
    }

    void WatchedFolderView::IsFavorite(const bool& value)
    {
        _isFavorite = value;
        visualStateManager.goToState(value ? FavoriteState : NonFavoriteState);
    }


    winrt::Windows::Foundation::IAsyncAction WatchedFolderView::UserControl_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const&, winrt::Windows::Foundation::IInspectable const&)
    {
        try
        {
            auto&& folder = co_await winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(_folderPath);
            auto&& items = co_await folder.GetItemsAsync();

            if (items.Size() > 0)
            {
                DispatcherQueue().TryEnqueue([&]
                {
                    LoadingProgressRing().IsIndeterminate(true);
                    LoadingProgressRing().Visibility(winrt::Microsoft::UI::Xaml::Visibility::Visible);
                });

                for (auto&& file : items)
                {
                    DispatcherQueue().TryEnqueue([this, name = file.Name()]
                    {
                        Items().Append(name);
                    });
                }
            }
            else
            {
                DispatcherQueue().TryEnqueue([&]
                {
                    winrt::Microsoft::UI::Xaml::VisualStateManager::GoToState(*this, L"NoFiles", false);
                });
            }

            co_await GetFolderSize();

            DispatcherQueue().TryEnqueue([this]
            {
                LoadingProgressRing().Visibility(winrt::Microsoft::UI::Xaml::Visibility::Collapsed);
            });
        }
        catch (const winrt::hresult_error& hresultErr)
        {
            DispatcherQueue().TryEnqueue([this, errText = hresultErr.message()]
            {
                winrt::Microsoft::UI::Xaml::VisualStateManager::GoToState(*this, L"NoFiles", false);
                NoFilesTextBlock().Text(errText);
            });
        }
    }

    winrt::Windows::Foundation::IAsyncAction WatchedFolderView::FilesGridView_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const&, winrt::Windows::Foundation::IInspectable const&)
    {
        auto&& folder = co_await winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(_folderPath);
        auto&& items = co_await folder.GetItemsAsync();

        for (auto&& item : items)
        {
            winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage imageSource{};
            DispatcherQueue().TryEnqueue([this, imageSource]
            {
                winrt::Microsoft::UI::Xaml::Controls::Image image{};
                image.MaxHeight(80);
                image.Source(imageSource);
                FilesGridView().Items().Append(image);
            });
            
            winrt::Windows::Storage::FileProperties::StorageItemThumbnail source = nullptr;
            if (item.IsOfType(winrt::Windows::Storage::StorageItemTypes::Folder))
            {
                source = co_await getItemThumbnail(item.try_as<winrt::Windows::Storage::StorageFolder>(), 350);
            }
            else
            {
                source = co_await getItemThumbnail(item.try_as<winrt::Windows::Storage::StorageFile>(), 350);
            }

            imageSource.DecodePixelHeight(thumbnailHeight);
            co_await imageSource.SetSourceAsync(source);
        }

        loaded = true;
    }

    void WatchedFolderView::ThumbnailsViewToggleButton_Checked(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        ListViewToggleButton().IsChecked(!ThumbnailsViewToggleButton().IsChecked().GetBoolean());
        ViewModeChanged();
    }

    void WatchedFolderView::ListViewToggleButton_Checked(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        if (ListViewToggleButton().IsChecked().GetBoolean())
        {
            ThumbnailsViewToggleButton().IsChecked(!ListViewToggleButton().IsChecked().GetBoolean());
        }
        else
        {
            ListViewToggleButton().IsChecked(true);
        }

        ViewModeChanged();
    }

    void WatchedFolderView::ThumbnailSizeSlider_ValueChanged(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs const& e)
    {
        if (!loaded) return;
        double newSize = e.NewValue();
        for (auto&& item : FilesGridView().Items())
        {
            auto picture = item.as<Microsoft::UI::Xaml::Controls::Image>();
            picture.MaxHeight(newSize);
        }
    }

    winrt::Windows::Foundation::IAsyncAction WatchedFolderView::WatchChangesToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        bool enable = WatchChangesToggleSwitch().IsOn();
        winrt::PCHealth::MainWindow::Current().PostMessageToWindow(L"Activated watcher for '" + _folderPath + L"'.", L"Activated watcher for '" + _folderPath + L"'.", false);

        co_await winrt::resume_background();
        if (enable)
        {
            watcherPtr->startWatching();
        }
        else
        {
            watcherPtr->stopWatching();
        }
    }

    winrt::Windows::Foundation::IAsyncAction WatchedFolderView::HomeViewGrid_Loading(xaml::FrameworkElement const&, winrt::Windows::Foundation::IInspectable const&)
    {
        co_await GetFolderSize();

        try
        {
            auto&& folder = co_await winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(_folderPath);
            auto&& getFoldersAsync = folder.GetFoldersAsync();
            auto&& files = co_await folder.GetFilesAsync();

            for (auto&& file : files)
            {
                DispatcherQueue().TryEnqueue([this, name = file.Name()]
                {
                    Items().Append(name);
                    ItemCount(0);
                });
            }

            auto&& folders = co_await getFoldersAsync;
            DispatcherQueue().TryEnqueue([this, count = folders.Size()]()
            {
                FolderCount(count);
            });
        }
        catch (const winrt::hresult_access_denied& hres)
        {
            OutputDebug(hres.message().data());
        }

        DispatcherQueue().TryEnqueue([&]
        {
            HomeViewProgressRing().IsIndeterminate(false);
            HomeViewProgressRing().Visibility(xaml::Visibility::Collapsed);
        });
    }

    void WatchedFolderView::HomeViewGrid_PointerEntered(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&)
    {
        if (!thumbnailListLoaded)
        {
            LoadThumbnailList();
        }

        if (thumbnailSwitchingTimer != nullptr)
        {
            if (!thumbnailSwitchingTimer.IsRunning())
            {
                thumbnailSwitchingTimer.Start();
            }

            visualStateManager.goToState(PointerOnState);
        }
    }

    void WatchedFolderView::HomeViewGrid_PointerExited(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&)
    {
        if (thumbnailSwitchingTimer != nullptr)
        {
            thumbnailSwitchingTimer.Stop();
            visualStateManager.goToState(PointerOffState);
        }

        ClearCurrentThumbnail();
    }

    void WatchedFolderView::FavoriteButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        visualStateManager.switchState(FavoriteState.group());
        _isFavorite = visualStateManager.getCurrentState(FavoriteState.group()) == FavoriteState;
    }


    void WatchedFolderView::ViewModeChanged()
    {
        if (ListViewToggleButton().IsChecked().GetBoolean())
        {
            Microsoft::UI::Xaml::VisualStateManager::GoToState(*this, L"ListView", false);
        }
        else if (ThumbnailsViewToggleButton().IsChecked().GetBoolean())
        {
            Microsoft::UI::Xaml::VisualStateManager::GoToState(*this, L"GridView", false);
        }
    }

    void WatchedFolderView::RaisePropChanged(std::source_location location)
    {
        auto functionName = std::string(location.function_name());
        functionName = functionName.substr(0, functionName.find_first_of('('));
        functionName = functionName.substr(functionName.find_last_of(':') + 1);
        e_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(winrt::to_hstring(functionName)));
    }

    winrt::Windows::Foundation::IAsyncAction WatchedFolderView::GetFolderSize()
    {
        co_await winrt::resume_background();

        pchealth::filesystem::DirectorySizeCalculator calculator{};
        uint_fast64_t size = calculator.GetSize(_folderPath.data());
        OutputDebug(std::format(L"[WatchedFolderView] '{}' size {}b", std::wstring(FolderName()), size));
        DispatcherQueue().TryEnqueue([this, size]
        {
            FolderSize(winrt::hstring(pchealth::filesystem::FileSize(size).ToString()));
        });

        co_return;
    }

    winrt::Windows::Foundation::IAsyncAction WatchedFolderView::LoadThumbnailList()
    {
        try
        {
            auto&& folder = co_await winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(_folderPath);
            auto&& items = co_await folder.GetItemsAsync();
            if (items.Size() < 100)
            {
                if (thumbnailSwitchingTimer == nullptr)
                {
                    thumbnailSwitchingTimer = DispatcherQueue().CreateTimer();
                    thumbnailSwitchingTimer.Interval(std::chrono::milliseconds(700));
                    thumbnailSwitchingTimer.Tick({ this, &WatchedFolderView::SwitchThumbnail });
                }

                for (auto&& item : items)
                {
                    winrt::Windows::Storage::FileProperties::StorageItemThumbnail thumbnail = nullptr;
                    if (item.IsOfType(winrt::Windows::Storage::StorageItemTypes::Folder))
                    {
                        thumbnail = co_await getItemThumbnail(item.try_as<winrt::Windows::Storage::StorageFolder>(), 80);
                    }
                    else
                    {
                        thumbnail = co_await getItemThumbnail(item.try_as<winrt::Windows::Storage::StorageFile>(), 80);
                    }

                    if (thumbnail != nullptr)
                    {
                        folderThumbnails.push_back(thumbnail);
                    }
                }
            }
        }
        catch (winrt::hresult_access_denied ex)
        {
            OUTPUT_DEBUG(L"[WatchedFolderView] Failed to load thumbnail list, access is denied.");
        }
        thumbnailListLoaded = true;
    }

    void WatchedFolderView::SwitchThumbnail(const Microsoft::UI::Dispatching::DispatcherQueueTimer&, const Windows::Foundation::IInspectable&)
    {
        assert(thumbnailIndex < folderThumbnails.size());

        auto&& thumbnail = folderThumbnails[thumbnailIndex];

        winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage imageSource{};
        imageSource.SetSourceAsync(thumbnail);
        imageSource.DecodePixelHeight(thumbnailHeight);
        ThumbnailImage().Source(imageSource);

        ++thumbnailIndex;
        if (thumbnailIndex >= folderThumbnails.size())
        {
            thumbnailIndex = 0;
            thumbnailSwitchingTimer.Stop();
            folderThumbnails.clear();
            thumbnailListLoaded = false;
        }
    }

    void WatchedFolderView::ClearCurrentThumbnail()
    {
        if (ThumbnailImage().Source() != nullptr)
        {
            ThumbnailImage().Source(nullptr);
        }
    }
}
