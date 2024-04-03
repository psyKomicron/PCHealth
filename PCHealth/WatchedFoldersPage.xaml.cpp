#include "pch.h"
#include "WatchedFoldersPage.xaml.h"
#if __has_include("WatchedFoldersPage.g.cpp")
#include "WatchedFoldersPage.g.cpp"
#endif

#include "LocalSettingsShortcuts.hpp"
#include "System.h"
#include "utilities.h"
#include "CompositeSetting.hpp"
#include "Directory.hpp"

#include "MainWindow.xaml.h"

namespace xaml = winrt::Microsoft::UI::Xaml;

using namespace winrt::PCHealth::implementation;


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

    auto settings = pchealth::storage::LocalSettingsShortcuts();
    auto pathes = settings.getWatchedFolders();

    PostMessageToWindow(
        std::format(L"Restored {} folders.", pathes.size()), 
        std::format(L"Restored {}.", pathes.size())
    );

    for (auto&& watchedFolder : pathes)
    {
        DispatcherQueue().TryEnqueue([this, watchedFolder]
        {
            auto view = winrt::PCHealth::WatchedFolderView(watchedFolder.path);
            view.IsFavorite(watchedFolder.favorite);
            WatchedFoldersListView().Items().Append(view);
        });
    }
}

winrt::Windows::Foundation::IAsyncAction WatchedFoldersPage::AppBarAddButton_Click(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
{
    auto res = co_await FolderAddContentDialog().ShowAsync();
    if (res == xaml::Controls::ContentDialogResult::Primary)
    {
        AddWatchedFolder(FolderAddContentDialogTextBox().Text());
    }
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
    auto settings = pchealth::utilities::getLocalSettings();
    settings.openOrCreateAndMoveTo(L"WatchedFolders");
    settings.clear();

    auto items = WatchedFoldersListView().Items();
    int32_t index = 0;
    for (auto item : items)
    {
        auto watchedFolderView = item.as<winrt::PCHealth::WatchedFolderView>();

        auto composite = pchealth::storage::CompositeSetting(settings.getComposite());
        composite.insert(L"path", watchedFolderView.FolderPath());
        composite.insert(L"favorite", watchedFolderView.IsFavorite());

        hstring key = winrt::to_hstring(index++);
        settings.append(key, composite.asComposite());
    }
}

void WatchedFoldersPage::FolderAddContentDialogTextBox_TextChanged(winrt::Windows::Foundation::IInspectable const&, xaml::Controls::TextChangedEventArgs const&)
{
    SuggestionGridView().Items().Clear();

    auto&& dir = pchealth::filesystem::Directory::tryCreateDirectory(std::wstring(FolderAddContentDialogTextBox().Text()));
    if (dir.has_value())
    {
        auto&& entries = dir.value().enumerate();
        for (auto&& entry : entries)
        {
            auto&& entryPath = entry.first;
            hstring suggestion{ entryPath.filename().wstring() };

            SuggestionGridView().Items().Append(box_value(suggestion));
        }
    }
}

void WatchedFoldersPage::FolderAddContentDialog_PreviewKeyDown(winrt::Windows::Foundation::IInspectable const& sender, xaml::Input::KeyRoutedEventArgs const& e)
{
    if (e.Key() == winrt::Windows::System::VirtualKey::Enter)
    {
        OUTPUT_DEBUG(L"[WatchedFoldersPage] Pressed enter.");
    }
    else if (e.Key() == winrt::Windows::System::VirtualKey::Tab)
    {
        OUTPUT_DEBUG(L"[WatchedFoldersPage] Pressed tab.");
        SuggestionGridView().Focus(xaml::FocusState::Pointer);
    }
    else
    {
        OUTPUT_DEBUG(L"[WatchedFoldersPage] Pressed key.");
    }
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
    else
    {
        PostMessageToWindow(std::format(L"Can't add '{}', the path doesn't exist.", path), std::format(L"/!\\📂 {}", path));
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
