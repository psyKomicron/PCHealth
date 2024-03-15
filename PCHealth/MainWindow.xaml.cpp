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
    }

    void MainWindow::PostMessageToWindow(const winrt::param::hstring& longMessage, const winrt::param::hstring& shortMessage, bool recursive)
    {
        //if (!DispatcherQueue().HasThreadAccess())
        //{
        //    if (recursive)
        //    {
        //        throw winrt::hresult_out_of_bounds(L"Preventing recursive stack overflow.");
        //    }
        //    DispatcherQueue().TryEnqueue([this, _longMessage = winrt::hstring(longMessage), _shortMessage = winrt::hstring(shortMessage)]
        //    {
        //        PostMessageToWindow(_longMessage, _shortMessage, true);
        //    });
        //}
        //else
        //{
        //    BottomStatusText().Text(shortMessage);
        //    if (!timer.IsRunning())
        //    {
        //        timer.Start();
        //    }

        //    auto item = winrt::PCHealth::DatedMessageViewModel();
        //    item.Message(longMessage);
        //    MessagesListView().Items().Append(item);
        //    if (MessagesListView().Items().Size() > 100)
        //    {
        //        // I18N
        //        MessagesCountTextBlock().Text(L"100+");
        //    }
        //    else
        //    {
        //        MessagesCountTextBlock().Text(winrt::to_hstring(MessagesListView().Items().Size()));
        //    }
        //}
    }

    void MainWindow::RootGrid_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args)
    {
        InitializeWindow();
    }

    void MainWindow::ScrollViewer_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
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
        SaveWindow();

        backdropController.Close();
        dispatcherQueueController.ShutdownQueueAsync();
    }

    void MainWindow::SaveWindow()
    {
        pchealth::storage::LocalSettings settings{ winrt::Windows::Storage::ApplicationData::Current().LocalSettings() };
        settings.openOrCreateAndMoveTo(L"MainWindow");
        settings.insert(L"Height", appWindow.Size().Height);
        settings.insert(L"Width", appWindow.Size().Width);
        settings.insert(L"PosX", appWindow.Position().X);
        settings.insert(L"PosY", appWindow.Position().Y);
    }

    void MainWindow::RestoreWindow()
    {
        pchealth::storage::LocalSettings settings{ winrt::Windows::Storage::ApplicationData::Current().LocalSettings() };
        settings.openOrCreateAndMoveTo(L"MainWindow");
        winrt::Windows::Graphics::RectInt32 display{};
        display.Height = settings.tryLookup<int32_t>(L"Height").value_or(600);
        display.Width = settings.tryLookup<int32_t>(L"Width").value_or(800);
        display.X = settings.tryLookup<int32_t>(L"PosX").value_or(30);
        display.Y = settings.tryLookup<int32_t>(L"PosY").value_or(30);
    }
}
