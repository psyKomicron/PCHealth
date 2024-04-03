#include "pch.h"
#include "HomePage.xaml.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif

#include "DisplayMonitor.hpp"
#include "LocalSettings.h"
#include "LocalSettingsShortcuts.hpp"
#include "DriveInfo.h"
#include "FileSize.h"

#include "MainWindow.xaml.h"

#include <ppltasks.h>

namespace xaml = winrt::Microsoft::UI::Xaml;
namespace collections = winrt::Windows::Foundation::Collections;

using namespace winrt::PCHealth::implementation;


void HomePage::Page_Loading(xaml::FrameworkElement const&, winrt::Windows::Foundation::IInspectable const&)
{
    if (!loaded)
    {
        LoadDisplayWidgets();
        LoadFavoritesWatchedFolders();
        concurrency::create_task([this]()
        {
            LoadRecycleBinWidget();
        });
    }
}

void HomePage::Page_Loaded(winrt::Windows::Foundation::IInspectable const& sender, xaml::RoutedEventArgs const& e)
{
    loaded = true;
    SearchBox().Focus(xaml::FocusState::Pointer);
}

void HomePage::DrivesPagesButton_Click(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
{
    PCHealth::MainWindow::Current().NavigateTo(winrt::xaml_typename<PCHealth::DrivesPage>(), {});
}

void HomePage::WatchedFoldersPageButton_Click(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
{
    PCHealth::MainWindow::Current().NavigateTo(winrt::xaml_typename<PCHealth::WatchedFoldersPage>(), {});
}

void HomePage::ClipboardManagerPageButton_Click(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
{
    //PCHealth::MainWindow::Current().NavigateTo(winrt::xaml_typename<PCHealth::ClipboardManagerPage>(), {})
}

void HomePage::SearchPageButton_Click(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
{
    PCHealth::MainWindow::Current().NavigateTo(winrt::xaml_typename<PCHealth::SystemSearchPage2>(), {});
}

void HomePage::SearchBox_TextChanged(winrt::Windows::Foundation::IInspectable const&, xaml::Controls::TextChangedEventArgs const& args)
{
    auto text = SearchBox().Text();
    if (text.size() > 1 && text[text.size() - 1] == L'\r')
    {
        std::wstring_view view = text;
        view = view.substr(0, static_cast<std::size_t>(text.size()) - 1);
        SearchBox().Text(text);
        PCHealth::MainWindow::Current().NavigateTo(winrt::xaml_typename<PCHealth::SystemSearchPage2>(), box_value(hstring(view)));
    }
}


void HomePage::LoadDisplayWidgets()
{
    auto displays = pchealth::windows::DisplayMonitor::enumerateDisplays();
    for (int i = displays.size() - 1; i >= 0; i--)
    {
        auto& display = displays[i];

        PCHealth::DisplayView displayView{};
        displayView.DisplayName(display.friendlyName());
        displayView.DisplayIndex(display.id());
        auto currentRes = display.currentResolution();
        displayView.SetCurrentResolution(display.currentResolution().first, display.currentResolution().second, display.vsync());

        auto&& modes = display.supportedResolutions();
        for (auto&& mode : modes)
        {
            for (auto&& freq : mode.second)
            {
                displayView.AddSupportedResolution(mode.first.first, mode.first.second, freq);
            }
        }

        xaml::Controls::VariableSizedWrapGrid::SetColumnSpan(displayView, 1);
        xaml::Controls::VariableSizedWrapGrid::SetRowSpan(displayView, 2);
        SecondGrid().Children().InsertAt(0, displayView);
    }
}

void HomePage::LoadFavoritesWatchedFolders()
{
    pchealth::storage::LocalSettingsShortcuts settings{};
    auto&& watchedFolders = settings.getWatchedFolders();
    if (!watchedFolders.empty())
    {
        for (auto&& watchedFolder : watchedFolders)
        {
            if (watchedFolder.favorite)
            {
                PCHealth::WatchedFolderView view{ watchedFolder.path };
                xaml::VisualStateManager::GoToState(view, L"HomeView", false);

                xaml::Controls::VariableSizedWrapGrid::SetColumnSpan(view, 1);
                xaml::Controls::VariableSizedWrapGrid::SetRowSpan(view, 2);
                FavoritesWatchedFolders().Children().Append(view);
            }
        }
    }
}

void HomePage::LoadRecycleBinWidget()
{
    auto drives = pchealth::filesystem::DriveInfo::getDrives();
    std::map<std::wstring, uint64_t> recycleBinSizes{};
    uint64_t totalRecycleBinSize = 0u;
    uint64_t systemAvailableSpace = 0u;
    for (size_t i = 0; i < drives.size(); i++)
    {
        uint64_t recycleBinSize = drives[i].getRecycleBinSize();
        totalRecycleBinSize += recycleBinSize;
        systemAvailableSpace += drives[i].capacity() - drives[i].totalUsedSpace();

        recycleBinSizes[drives[i].name()] = recycleBinSize;
    }

    auto p = static_cast<double>(totalRecycleBinSize) / static_cast<double>(systemAvailableSpace);
    DispatcherQueue().TryEnqueue([this, p]()
    {
        xaml::Media::SolidColorBrush brush{};
        if (p > 0.8)
        {
            brush.Color(Windows::UI::Colors::Red());
        }
        else if (p > 0.4)
        {
            brush.Color(Windows::UI::Colors::Orange());
        }
        else
        {
            brush.Color(Windows::UI::Colors::Green());
        }
        RecycleBinProgressRing().Foreground(brush);
        RecycleBinProgressRing().Value(p);
    });

    DispatcherQueue().TryEnqueue([this, totalRecycleBinSize]()
    { 
        pchealth::filesystem::FileSize size = totalRecycleBinSize;
        RecycleBinSizeTextBlock().Text(size.ToString());
    });

    std::wstring sizes = L"";
    for (auto&& pair : recycleBinSizes)
    {
        auto& driveName = pair.first;
        auto size = pchealth::filesystem::FileSize(pair.second);
        sizes += std::format(L"{} {}\n", driveName, size.ToString());
    }
    DispatcherQueue().TryEnqueue([this, sizes = hstring(sizes)]()
    {
        xaml::Controls::ToolTipService::SetToolTip(RecycleBinWidgetViewBox(), box_value(sizes));
    });
}