#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "LocalSettings.h"
#include "utilities.h"

#include "DatedMessageViewModel.h"

#include <DispatcherQueue.h>
#include <microsoft.ui.xaml.window.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.Composition.SystemBackdrops.h>
#include <winrt/Microsoft.UI.Composition.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Interop.h>
#include <winrt/Microsoft.Windows.ApplicationModel.Resources.h>


namespace Graphics = winrt::Windows::Graphics;

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

using namespace winrt::PCHealth::implementation;

winrt::PCHealth::MainWindow MainWindow::singleton{ nullptr };

MainWindow::MainWindow()
{
    singleton = *this;
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
        Xaml::Controls::InfoBar infoBar{};
        infoBar.IsClosable(true);
        infoBar.Title(longMessage);
        infoBar.Message(shortMessage);
        infoBar.Closed([this](auto s, auto)
        {
            //auto infoBar = s.as<Xaml::Controls::InfoBar>();
            uint32_t index = 0;
            if (InfoBarListView().Items().IndexOf(s, index))
            {
                InfoBarListView().Items().RemoveAt(index);
            }
        });
        infoBar.IsOpen(true);
        InfoBarListView().Items().Append(infoBar);
    }
}

void MainWindow::NavigateTo(const Windows::UI::Xaml::Interop::TypeName& typeName, const Windows::Foundation::IInspectable& param)
{
    Frame().Navigate(typeName, param);

    auto&& items = PageButtonsStackPanel().Children();
    for (auto&& item : items)
    {
        auto&& button = item.as<Xaml::Controls::Button>();
        if (button.Tag().as<hstring>() == typeName.Name)
        {
            button.Style(Xaml::Application::Current().Resources().Lookup(box_value(L"AccentButtonStyle")).as<Xaml::Style>());
        }
        else
        {
            button.Style(Xaml::Application::Current().Resources().Lookup(box_value(L"IconButtonStyle")).as<Xaml::Style>());
        }
    }

    /*PageButtonsStackPanel().Visibility(typeName.Name != L"PCHealth.HomePage"
        ? Xaml::Visibility::Visible
        : Xaml::Visibility::Collapsed);*/

    //PageTitleTextBlock().Text(typeName.Name);
    /*try
    {
        Microsoft::Windows::ApplicationModel::Resources::ResourceLoader loader{};
        PageTitleTextBlock().Text(loader.GetString(typeName.Name));
    }
    catch (winrt::hresult_error err)
    {
        PageTitleTextBlock().Text(typeName.Name);
    }*/
        
    BackButton().IsEnabled(Frame().CanGoBack());
}

void MainWindow::RootGrid_Loading(Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args)
{
    InitializeWindow();
}

void MainWindow::ScrollViewer_Loaded(winrt::Windows::Foundation::IInspectable const& sender, Xaml::RoutedEventArgs const& e)
{
    SetDragRectangles();
}

void MainWindow::RootGrid_SizeChanged(winrt::Windows::Foundation::IInspectable const&, Xaml::SizeChangedEventArgs const&)
{
    SetDragRectangles();
}

void MainWindow::HomeButton_Click(winrt::Windows::Foundation::IInspectable const&, Xaml::RoutedEventArgs const&)
{
    NavigateTo(xaml_typename<HomePage>(), {});
}

void MainWindow::BackButton_Click(winrt::Windows::Foundation::IInspectable const&, Xaml::RoutedEventArgs const&)
{
    if (Frame().CanGoBack())
    {
        Frame().GoBack();
        BackButton().IsEnabled(Frame().BackStack().Size() != 0);
    }
}

void MainWindow::Frame_Loaded(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
{
    NavigateTo(xaml_typename<HomePage>(), {});
}

void MainWindow::DrivesPageTitleBarButton_Click(winrt::Windows::Foundation::IInspectable const&, Xaml::RoutedEventArgs const&)
{
    NavigateTo(xaml_typename<DrivesPage>(), {});
}

void MainWindow::WatchedFoldersPageTitleBarButton_Click(winrt::Windows::Foundation::IInspectable const&, Xaml::RoutedEventArgs const&)
{
    NavigateTo(xaml_typename<WatchedFoldersPage>(), {});
}

void MainWindow::SystemSearchPageTitleBarButton_Click(winrt::Windows::Foundation::IInspectable const&, Xaml::RoutedEventArgs const&)
{
    NavigateTo(xaml_typename<SystemSearchPage2>(), {});
}

void MainWindow::Frame_Navigated(winrt::Windows::Foundation::IInspectable const&, Xaml::Navigation::NavigationEventArgs const&)
{
    SetDragRectangles();
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

    RestoreWindow();
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

void MainWindow::SetDragRectangles()
{
    if (!usingCustomTitleBar)
    {
        OUTPUT_DEBUG("[MainWindow] Not using custom title bar, skipping setting drag rectangles.");
        return;
    }

    std::vector<Graphics::RectInt32> dragRectangles{};

    int32_t titleGridWidth = static_cast<int32_t>(WindowTitleBarGrid().ActualWidth());
    int32_t titleGridHeight = static_cast<int32_t>(WindowTitleBarGrid().ActualHeight());

    // Rectangle from navigation and back button to navigations button in the middle of the window.
    Graphics::RectInt32 leftDragRectangle{};
    leftDragRectangle.X = static_cast<int32_t>(LeftPaddingColumn().ActualWidth() + SettingsButtonColumn().ActualWidth());
    leftDragRectangle.Y = 0;
    leftDragRectangle.Width = static_cast<int32_t>(CenterLeftColumn().ActualWidth());
    leftDragRectangle.Height = titleGridHeight;
    dragRectangles.push_back(leftDragRectangle);

    Graphics::RectInt32 rightDragRectangle{};
    rightDragRectangle.X = static_cast<int32_t>
        (
            LeftPaddingColumn().ActualWidth() + SettingsButtonColumn().ActualWidth() + CenterLeftColumn().ActualWidth() + TitleBarGridContentColumn().ActualWidth()
        );
    rightDragRectangle.Y = 0;
    rightDragRectangle.Width = static_cast<int32_t>(CenterRightColumn().ActualWidth());
    rightDragRectangle.Height = titleGridHeight;
    dragRectangles.push_back(rightDragRectangle);

    // Set the drag rectangles regions.
    appWindow.TitleBar().SetDragRectangles(dragRectangles);
}

void MainWindow::AppWindow_Closing(winrt::Microsoft::UI::Windowing::AppWindow, winrt::Microsoft::UI::Windowing::AppWindowClosingEventArgs)
{
    SaveWindow();

    backdropController.Close();
    dispatcherQueueController.ShutdownQueueAsync();
}

void MainWindow::SaveWindow()
{
    pchealth::storage::LocalSettings settings{ winrt::Windows::Storage::ApplicationData::Current().LocalSettings() };
    settings.openOrCreateAndMoveTo(L"MainWindow");

    auto&& windowSize = appWindow.Size();
    auto&& windowPos = appWindow.Position();
    OUTPUT_DEBUG(std::format("[MainWindow] {}x{}, [{},{}]", windowSize.Width, windowSize.Height, windowPos.X, windowPos.Y));
    settings.insert(L"Height", windowSize.Height);
    settings.insert(L"Width", windowSize.Width);
    settings.insert(L"PosX", windowPos.X);
    settings.insert(L"PosY", windowPos.Y);
}

void MainWindow::RestoreWindow()
{
    pchealth::storage::LocalSettings settings{ winrt::Windows::Storage::ApplicationData::Current().LocalSettings() };
    settings.openOrCreateAndMoveTo(L"MainWindow");

    winrt::Windows::Graphics::RectInt32 display{};
    display.Height = settings.tryLookupValue<int32_t>(L"Height").value_or(600);
    display.Width = settings.tryLookupValue<int32_t>(L"Width").value_or(800);
    display.X = settings.tryLookupValue<int32_t>(L"PosX").value_or(30);
    display.Y = settings.tryLookupValue<int32_t>(L"PosY").value_or(30);
    appWindow.MoveAndResize(display);

    OUTPUT_DEBUG(std::format("[MainWindow] {}x{}, [{},{}]", display.Width, display.Height, display.X, display.Y));
}
