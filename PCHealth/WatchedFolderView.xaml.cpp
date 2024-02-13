#include "pch.h"
#include "WatchedFolderView.xaml.h"
#if __has_include("WatchedFolderView.g.cpp")
#include "WatchedFolderView.g.cpp"
#endif

#include "DirectorySizeCalculator.h"
#include "FileSize.h"

#include <winrt/Microsoft.UI.Xaml.Media.Imaging.h>

namespace winrt::PCHealth::implementation
{
    WatchedFolderView::WatchedFolderView()
    {
        InitializeComponent();
    }

    WatchedFolderView::WatchedFolderView(const winrt::hstring& folderPath)
        : WatchedFolderView()
    {
        _folderPath = folderPath;
        auto&& func = [this](std::vector<pchealth::filesystem::Win32FileInformation>& e)
        {
            std::wstring pathes = L"";
            for (auto&& fileInfo : e)
            {
                pathes += fileInfo.name() + L", ";
            }

            winrt::PCHealth::MainWindow::Current().PostMessageToWindow(
                std::format(L"Directory watcher detected changes in '{}', {} file(s) changed : {}", watcherPtr->path(), e.size(), pathes),
                std::format(L"'{}': {} file(s) changed.", watcherPtr->path(), e.size()),
                false
            );
        };
        //std::function<void(const WatchedFolderView&, std::wstring)> objectFunc = &WatchedFolderView::ChangesDetected;
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
        e_propertyChanged(*this, winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(L"FolderPath"));
    }

    winrt::Windows::Foundation::Collections::IObservableVector<winrt::hstring> WatchedFolderView::Items() const
    {
        return _items;
    }


    winrt::Windows::Foundation::IAsyncAction WatchedFolderView::UserControl_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const&, winrt::Windows::Foundation::IInspectable const&)
    {
        co_await winrt::resume_background();

        try
        {
            auto&& folder = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(_folderPath).get();
            auto&& files = folder.GetFilesAsync().get();

            if (files.Size() == 0)
            {
                DispatcherQueue().TryEnqueue([&]
                {
                    winrt::Microsoft::UI::Xaml::VisualStateManager::GoToState(*this, L"NoFiles", false);
                });
            }
            else
            {
                DispatcherQueue().TryEnqueue([&]
                {
                    LoadingProgressRing().IsIndeterminate(true);
                    LoadingProgressRing().Visibility(winrt::Microsoft::UI::Xaml::Visibility::Visible);
                });
                for (auto&& file : files)
                {
                    DispatcherQueue().TryEnqueue([this, name = file.Name()]
                    {
                        Items().Append(name);
                    });
                }
                DispatcherQueue().TryEnqueue([this]
                {
                    LoadingProgressRing().Visibility(winrt::Microsoft::UI::Xaml::Visibility::Collapsed);
                });
            }

            Common::Filesystem::DirectorySizeCalculator calculator{};
            uint_fast64_t size = calculator.GetSize(_folderPath.data());
            DispatcherQueue().TryEnqueue([this, size]
            {
                FolderSizeTextBlock().Text(Common::FileSize(size).ToString());
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
        auto&& files = co_await folder.GetFilesAsync();

        const uint32_t thumbnailHeight = 300u;
        for (auto&& file : files)
        {
            winrt::Microsoft::UI::Xaml::Media::Imaging::BitmapImage imageSource{};
            DispatcherQueue().TryEnqueue([this, imageSource]
            {
                winrt::Microsoft::UI::Xaml::Controls::Image image{};
                image.MaxHeight(80);
                image.Source(imageSource);
                FilesGridView().Items().Append(image);
            });
            auto&& source = co_await file.GetThumbnailAsync(winrt::Windows::Storage::FileProperties::ThumbnailMode::SingleItem, thumbnailHeight);
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
}
