#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "System.h"
#include "DriveInfo.h"
#include "FileSize.h"
#include "LibraryPathes.h"
#include "LocalSettings.h"
#include "HotKey.h"
#include "utilities.h"

#include <shellapi.h>
#include <ppl.h>
#include <algorithm>

#include "DatedMessageViewModel.h"
#include <DispatcherQueue.h>
#include <microsoft.ui.xaml.window.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>
#include <winrt/Microsoft.UI.Composition.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Interop.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

/**
* @brief Microsoft::UI, Microsoft::UI::Composition, Microsoft::UI::Composition::SystemBackdrops, Microsoft::UI::Windowing
*/
namespace UI
{
    using namespace winrt::Microsoft::UI;
    using namespace winrt::Microsoft::UI::Composition;
    using namespace winrt::Microsoft::UI::Composition::SystemBackdrops;
    using namespace winrt::Microsoft::UI::Windowing;
}
/**
* @brief Microsoft::UI::Xaml, Microsoft::UI::Xaml::Input, Microsoft::UI::Xaml::Controls, Microsoft::UI::Xaml::Media, Microsoft::UI::Xaml::Media::Imaging, Microsoft::UI::Xaml::Controls::Primitives
*/
namespace Xaml
{
    using namespace winrt::Microsoft::UI::Xaml;
    using namespace winrt::Microsoft::UI::Xaml::Input;
    using namespace winrt::Microsoft::UI::Xaml::Controls;
    using namespace winrt::Microsoft::UI::Xaml::Media;
    using namespace winrt::Microsoft::UI::Xaml::Media::Imaging;
    using namespace winrt::Microsoft::UI::Xaml::Controls::Primitives;
}

namespace winrt::PCHealth::implementation
{
    winrt::PCHealth::MainWindow MainWindow::singleton{ nullptr };

    MainWindow::MainWindow()
    {
        singleton = *this;

        using namespace std::chrono_literals;
        timer = DispatcherQueue().CreateTimer();
        timer.Interval(15s);
        timer.IsRepeating(false);
        timerEventToken = timer.Tick([this](auto s, auto e)
        {
            BottomStatusText().Text(L"");
        });
        //InitializeComponent();
    }

    double MainWindow::SystemGeneralHealth()
    {
        return systemGeneralHealth;
    }

    void MainWindow::SystemGeneralHealth(const double& value)
    {
        systemGeneralHealth = value;
        e_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(L"SystemGeneralHealth"));
    }

    void MainWindow::RootGrid_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args)
    {
        InitializeWindow();
    }

    void MainWindow::ScrollViewer_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        winrt::hstring path = L"v:\\videos\\p\\vanilla\\";
        if (Common::System::PathExists(path.data()))
        {
            WatchedFoldersListView().Items().Append(winrt::PCHealth::WatchedFolderView(path));
        }
    }

    winrt::Windows::Foundation::IAsyncAction MainWindow::SystemRecycleBinSizeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        auto nativeWindow{ this->try_as<::IWindowNative>() };
        HWND handle = nullptr;
        if (&nativeWindow != nullptr)
        {
            nativeWindow->get_WindowHandle(&handle);
        }

        co_await winrt::resume_background();

        SHQUERYRBINFO queryBinInfo{ sizeof(SHQUERYRBINFO) };
        if (SUCCEEDED(SHEmptyRecycleBin(handle, nullptr, SHERB_NOCONFIRMATION | SHERB_NOSOUND)))
        {
            DispatcherQueue().TryEnqueue([this]
            {
                InfoBar().Severity(winrt::Microsoft::UI::Xaml::Controls::InfoBarSeverity::Success);
                //TODO: i18n.
                InfoBar().Message(L"Recycle bin emptied (all drives).");
                InfoBar().IsOpen(true);

                DrivesList().Children().Clear();
                RootGrid_Loading(nullptr, nullptr);
            });
        }
        else
        {
            DispatcherQueue().TryEnqueue([this]
            {
                InfoBar().Severity(winrt::Microsoft::UI::Xaml::Controls::InfoBarSeverity::Error);
                //TODO: i18n.
                InfoBar().Message(L"Failed to empty recycle bin.");
                InfoBar().IsOpen(true);

                DrivesList().Children().Clear();
                RootGrid_Loading(nullptr, nullptr);
            });
        }
    }

    void MainWindow::HibernationFileSizeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, Xaml::RoutedEventArgs const& e)
    {
        throw winrt::hresult_not_implemented();
    }

    winrt::Windows::Foundation::IAsyncAction MainWindow::DownloadsFolderSizeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, Xaml::RoutedEventArgs const& e)
    {
        // Prompt user for recycling and parallelization :
        auto&& result = co_await DirectoryDeleteConfirmation().ShowAsync();
        if (result != Controls::ContentDialogResult::Primary)
        {
            co_return;
        }

        bool recycle = RecycleDeleteCheckbox().IsChecked().GetBoolean();
        bool parallelize = ParallelizeDeleteCheckbox().IsChecked().GetBoolean();

        co_await winrt::resume_background();

        auto&& libs = Common::System::GetLibraries().get();
        if (Common::System::PathExists(libs.DownloadsFolder()))
        {
            try
            {
                Common::Filesystem::DirectoryInfo dirInfo{ libs.DownloadsFolder() };
                dirInfo.EmptyDirectory(recycle, parallelize);
            }
            catch (std::exception ex)
            {
                OutputDebugStringA(ex.what());
            }
            catch (winrt::hresult_error ex)
            {
                OutputDebugString(ex.message().c_str());
            }
        }
        else
        {
            DispatcherQueue().TryEnqueue([this, path = libs.DownloadsFolder()]
            {
                InfoBar().Message(std::format(L"Downloads folder ('{}') does not exist.", path));
            });
        }
    }

    void MainWindow::AppBarEditButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        WatchedFoldersListView().SelectionMode(
            WatchedFoldersListView().SelectionMode() == winrt::Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Multiple
            ? winrt::Microsoft::UI::Xaml::Controls::ListViewSelectionMode::None
            : winrt::Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Multiple);

        RemoveAppBarButton().IsEnabled(WatchedFoldersListView().SelectionMode() == winrt::Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Multiple);
    }

    winrt::Windows::Foundation::IAsyncAction MainWindow::SecondPivotContentGrid_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const&, winrt::Windows::Foundation::IInspectable const&)
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

    winrt::Windows::Foundation::IAsyncAction MainWindow::AppBarAddButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        co_await FolderAddContentDialog().ShowAsync();
        AddWatchedFolder(FolderAddContentDialogTextBox().Text());
    }

    void winrt::PCHealth::implementation::MainWindow::AppBarRemoveButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
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

    void winrt::PCHealth::implementation::MainWindow::AppBarSaveButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
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

    winrt::Windows::Foundation::IAsyncAction MainWindow::DrivesGrid_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const&, winrt::Windows::Foundation::IInspectable const&)
    {
        //TODO: Check if we need to go to background work sooner than when calculating the size of the downloads folder.
        auto&& drives = Common::Filesystem::DriveInfo::GetDrives();

        ConnectedDrivesNumberTextBlock().Text(std::to_wstring(drives.size()));

        uint64_t recycleBinTotalSize = 0;
        for (auto&& drive : drives)
        {
            DriveView driveView{};
            driveView.DriveName(drive.name());
            driveView.Capacity(drive.capacity());
            driveView.UsedSpace(drive.totalUsedSpace());
            DrivesList().Children().Append(driveView);

            recycleBinTotalSize += drive.getRecycleBinSize();
        }

        Common::FileSize hibernationFileSize = Common::System::GetFileSize(L"c:\\hiberfil.sys");
        HibernationFileSize().Text(hibernationFileSize.ToString());
        if (hibernationFileSize.Size() == 0)
        {
            HibernationFileSizeButton().IsEnabled(false);
        }

        Common::FileSize systemRecycleBinSize = recycleBinTotalSize;
        SystemRecycleBinSize().Text(systemRecycleBinSize.ToString());
        if (recycleBinTotalSize == 0)
        {
            SystemRecycleBinSizeButton().IsEnabled(false);
        }

        co_await winrt::resume_background();
        auto&& libs = Common::System::GetLibraries().get();
        auto&& downloadsFolder = libs.DownloadsFolder();
        if (!downloadsFolder.empty())
        {
            Common::Filesystem::DirectoryInfo downloadsFolderInfo{ downloadsFolder };
            auto&& downloadsFolderSize = downloadsFolderInfo.GetSize();
            DispatcherQueue().TryEnqueue([this, downloadsSize = Common::FileSize(downloadsFolderSize)]
            {
                DownloadsFolderSize().Text(downloadsSize.ToString());
            });
        }

        try
        {
            pchealth::win32::HotKey hotKey{ L'C', MOD_CONTROL, true };
            OutputDebug(L"Waiting for hot key to be fired...");
            hotKey.registerHotKey();
            OutputDebug(L"Hot key fired !");
        }
        catch (const std::invalid_argument& ex)
        {
            OutputDebug("Failed to register hotkey: " + std::string(ex.what()));
        }
    }


    void MainWindow::InitializeWindow()
    {
        auto nativeWindow{ this->try_as<::IWindowNative>() };
        check_bool(nativeWindow);
        HWND handle = nullptr;
        nativeWindow->get_WindowHandle(&handle);
        UI::WindowId windowID = UI::GetWindowIdFromWindow(handle);
        appWindow = UI::AppWindow::GetFromWindowId(windowID);
        if (appWindow != nullptr)
        {
#ifdef DEBUG
            Xaml::TextBlock timestamp{};
            timestamp.IsHitTestVisible(false);
            timestamp.Opacity(0.6);
            timestamp.HorizontalAlignment(Xaml::HorizontalAlignment::Right);
            timestamp.VerticalAlignment(Xaml::VerticalAlignment::Bottom);
            timestamp.Text(to_hstring(__TIME__) + L", " + to_hstring(__DATE__));

            Xaml::Grid::SetRow(timestamp, WindowGrid().RowDefinitions().GetView().Size() - 1);
            RootGrid().Children().Append(timestamp);
#endif // _DEBUG
            //appWindow.Title(Xaml::Application::Current().Resources().Lookup(box_value(L"AppTitle")).as<hstring>());
            appWindow.Resize(winrt::Windows::Graphics::SizeInt32{ 1100, 700 });

            appWindow.Closing({ this, &MainWindow::AppWindow_Closing });
#if false
            appWindow.Changed([this](UI::AppWindow, winrt::Microsoft::UI::Windowing::AppWindowChangedEventArgs args)
            {
                UI::OverlappedPresenter presenter = nullptr;
                if ((presenter = appWindow.Presenter().try_as<UI::OverlappedPresenter>()))
                {
                    if (args.DidSizeChange())
                    {
                        int32_t minimum = presenter.IsMinimizable() ? 250 : 130;
                        if (appWindow.Size().Width < minimum)
                        {
                            appWindow.Resize(winrt::Windows::Graphics::SizeInt32(minimum, appWindow.Size().Height));
                        }
                    }

                    if (presenter.State() == UI::OverlappedPresenterState::Restored)
                    {
                        if (args.DidPositionChange())
                        {
                            displayRect.X = appWindow.Position().X;
                            displayRect.Y = appWindow.Position().Y;
                        }

                        if (args.DidSizeChange())
                        {
                            displayRect.Width = appWindow.Size().Width;
                            displayRect.Height = appWindow.Size().Height;
                        }
                    }
                }
            });
#endif

            if (appWindow.TitleBar().IsCustomizationSupported())
            {
                usingCustomTitleBar = true;

                appWindow.TitleBar().ExtendsContentIntoTitleBar(true);
                appWindow.TitleBar().IconShowOptions(UI::IconShowOptions::HideIconAndSystemMenu);

                LeftPaddingColumn().Width(Xaml::GridLengthHelper::FromPixels(static_cast<double>(appWindow.TitleBar().LeftInset())));

                appWindow.TitleBar().ButtonBackgroundColor(UI::Colors::Transparent());
                appWindow.TitleBar().ButtonInactiveBackgroundColor(UI::Colors::Transparent());
                appWindow.TitleBar().ButtonInactiveForegroundColor(
                    Xaml::Application::Current().Resources().TryLookup(box_value(L"AppTitleBarHoverColor")).as<Windows::UI::Color>());
                appWindow.TitleBar().ButtonHoverBackgroundColor(
                    Xaml::Application::Current().Resources().TryLookup(box_value(L"ButtonHoverBackgroundColor")).as<Windows::UI::Color>());
                appWindow.TitleBar().ButtonPressedBackgroundColor(UI::Colors::Transparent());

                if (RootGrid().ActualTheme() == Xaml::ElementTheme::Light)
                {
                    appWindow.TitleBar().ButtonForegroundColor(UI::Colors::Black());
                    appWindow.TitleBar().ButtonHoverForegroundColor(UI::Colors::Black());
                    appWindow.TitleBar().ButtonPressedForegroundColor(UI::Colors::Black());
                }
                else if (RootGrid().ActualTheme() == Xaml::ElementTheme::Dark)
                {
                    appWindow.TitleBar().ButtonForegroundColor(UI::Colors::White());
                    appWindow.TitleBar().ButtonHoverForegroundColor(UI::Colors::White());
                    appWindow.TitleBar().ButtonPressedForegroundColor(UI::Colors::White());
                }
            }

            SetBackground();
        }
    }

    void MainWindow::SetBackground()
    {
        if (UI::DesktopAcrylicController::IsSupported())
        {
            auto&& supportsBackdrop = try_as<UI::ICompositionSupportsSystemBackdrop>();
            if (supportsBackdrop)
            {
                RootGrid().Background(Xaml::SolidColorBrush(UI::Colors::Transparent()));

                if (!winrt::Windows::System::DispatcherQueue::GetForCurrentThread() && !dispatcherQueueController)
                {
                    DispatcherQueueOptions options
                    {
                        sizeof(DispatcherQueueOptions),
                        DQTYPE_THREAD_CURRENT,
                        DQTAT_COM_NONE
                    };

                    ABI::Windows::System::IDispatcherQueueController* ptr{ nullptr };
                    check_hresult(CreateDispatcherQueueController(options, &ptr));
                    dispatcherQueueController = winrt::Windows::System::DispatcherQueueController(ptr, take_ownership_from_abi);
                }

                systemBackdropConfiguration = UI::SystemBackdropConfiguration();
                systemBackdropConfiguration.IsInputActive(true);
                systemBackdropConfiguration.Theme((UI::SystemBackdropTheme)RootGrid().ActualTheme());

                backdropController = {};
                backdropController.TintColor(Xaml::Application::Current().Resources().TryLookup(box_value(L"SolidBackgroundFillColorBase")).as<Windows::UI::Color>());
                backdropController.FallbackColor(Xaml::Application::Current().Resources().TryLookup(box_value(L"SolidBackgroundFillColorBase")).as<Windows::UI::Color>());
                backdropController.TintOpacity(static_cast<float>(Xaml::Application::Current().Resources().TryLookup(box_value(L"BackdropTintOpacity")).as<double>()));
                backdropController.LuminosityOpacity(static_cast<float>(Xaml::Application::Current().Resources().TryLookup(box_value(L"BackdropLuminosityOpacity")).as<double>()));
                backdropController.SetSystemBackdropConfiguration(systemBackdropConfiguration);
                backdropController.AddSystemBackdropTarget(supportsBackdrop);
            }
        }
    }

    void MainWindow::AppWindow_Closing(winrt::Microsoft::UI::Windowing::AppWindow, winrt::Microsoft::UI::Windowing::AppWindowClosingEventArgs)
    {
        AppBarSaveButton_Click(nullptr, nullptr);
            
        backdropController.Close();
        dispatcherQueueController.ShutdownQueueAsync();
    }

    void MainWindow::AddWatchedFolder(const winrt::hstring& path)
    {
        if (Common::System::PathExists(path.data()))
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

    void MainWindow::PostMessageToWindow(const winrt::param::hstring& longMessage, const winrt::param::hstring& shortMessage, bool recursive)
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
