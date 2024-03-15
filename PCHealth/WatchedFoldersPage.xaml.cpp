#include "pch.h"
#include "WatchedFoldersPage.xaml.h"
#if __has_include("WatchedFoldersPage.g.cpp")
#include "WatchedFoldersPage.g.cpp"
#endif

#include "LocalSettings.h"
#include "System.h"

#include "MainWindow.xaml.h"

namespace xaml = winrt::Microsoft::UI::Xaml;

namespace winrt::PCHealth::implementation
{
    WatchedFoldersPage::WatchedFoldersPage()
    {
        using namespace std::chrono_literals;
        timer = DispatcherQueue().CreateTimer();
        timer.Interval(15s);
        timer.IsRepeating(false);
        timerEventToken = timer.Tick([this](auto s, auto e)
        {
            BottomStatusText().Text(L"");
        });
    }

    winrt::Windows::Foundation::IAsyncAction WatchedFoldersPage::RootGrid_Loading(xaml::FrameworkElement const&, winrt::Windows::Foundation::IInspectable const&)
    {
        co_await winrt::resume_background();

        auto settings = pchealth::storage::LocalSettings(winrt::Windows::Storage::ApplicationData::Current().LocalSettings());
        auto pathes = settings.restoreList(L"WatchedFolders");
        PostMessageToWindow(
            std::format(L"Restored {} folders.", pathes.size()), 
            std::format(L"Restored {}.", pathes.size())
        );
        for (winrt::hstring path : pathes)
        {
            DispatcherQueue().TryEnqueue([this, path]
            {
                AddWatchedFolder(path);
            });
        }
    }

    winrt::Windows::Foundation::IAsyncAction WatchedFoldersPage::AppBarAddButton_Click(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
    {
        co_await FolderAddContentDialog().ShowAsync();
        AddWatchedFolder(FolderAddContentDialogTextBox().Text());
    }

    void WatchedFoldersPage::AppBarRemoveButton_Click(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
    {
        auto selectedItems = WatchedFoldersListView().SelectedItems();
        uint32_t count = 0u;
        for (int i = selectedItems.Size() - 1; i >= 0; i--)
        {
            auto&& selectedItem = selectedItems.GetAt(i);
            uint32_t index = 0;
            WatchedFoldersListView().Items().IndexOf(selectedItem, index);
            WatchedFoldersListView().Items().RemoveAt(index);
            count++;
        }

        DispatcherQueue().TryEnqueue([this, count]
        {
            PostMessageToWindow(std::format(L"Removed {} folder(s)", count), std::format(L"🗑️ x {}", count));
        });
    }

    void WatchedFoldersPage::AppBarEditButton_Click(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
    {
        WatchedFoldersListView().SelectionMode(
            WatchedFoldersListView().SelectionMode() == winrt::Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Multiple
            ? winrt::Microsoft::UI::Xaml::Controls::ListViewSelectionMode::None
            : winrt::Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Multiple);

        RemoveAppBarButton().IsEnabled(WatchedFoldersListView().SelectionMode() == winrt::Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Multiple);
    }

    void WatchedFoldersPage::AppBarSaveButton_Click(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
    {
        auto settings = pchealth::storage::LocalSettings(winrt::Windows::Storage::ApplicationData::Current().LocalSettings());
        auto items = WatchedFoldersListView().Items();
        std::vector<winrt::hstring> list{};
        for (auto item : items)
        {
            list.push_back(item.as<winrt::PCHealth::WatchedFolderView>().FolderPath());
        }
        settings.saveList(L"WatchedFolders", list);
    }


    void WatchedFoldersPage::AddWatchedFolder(const winrt::hstring& path)
    {
        if (pchealth::windows::System::pathExists(path.data()))
        {
            for (auto&& item : WatchedFoldersListView().Items())
            {
                auto&& view = item.try_as<winrt::PCHealth::WatchedFolderView>();
                if (view && view.FolderPath() == path)
                {
                    return;
                }
            }

            WatchedFoldersListView().Items().Append(winrt::PCHealth::WatchedFolderView(path));

            // I18N
            PostMessageToWindow(std::format(L"Added new folder : {}", path), std::format(L"+📂 {}", path));
        }
    }

    void WatchedFoldersPage::PostMessageToWindow(const winrt::param::hstring& longMessage, const winrt::param::hstring& shortMessage, bool recursive)
    {
        if (!DispatcherQueue().HasThreadAccess())
        {
            if (recursive)
            {
                throw winrt::hresult_out_of_bounds(L"Preventing recursive stack overflow.");
            }
            DispatcherQueue().TryEnqueue([this, _longMessage = winrt::hstring(longMessage), _shortMessage = winrt::hstring(shortMessage)]
            {
                PostMessageToWindow(_longMessage, _shortMessage, true);
            });
        }
        else
        {
            BottomStatusText().Text(shortMessage);
            if (!timer.IsRunning())
            {
                timer.Start();
            }

            auto item = winrt::PCHealth::DatedMessageViewModel();
            item.Message(longMessage);
            MessagesListView().Items().Append(item);
            if (MessagesListView().Items().Size() > 100)
            {
                // I18N
                MessagesCountTextBlock().Text(L"100+");
            }
            else
            {
                MessagesCountTextBlock().Text(winrt::to_hstring(MessagesListView().Items().Size()));
            }
        }
    }
}



