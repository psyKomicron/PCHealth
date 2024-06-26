#pragma once

#include "MainWindow.g.h"

namespace winrt::PCHealth::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
    public:
        MainWindow();

        static winrt::PCHealth::MainWindow Current()
        {
            return singleton;
        };

        void PostMessageToWindow(const winrt::param::hstring& longMessage, const winrt::param::hstring& shortMessage, bool recursive = false);

        void NavigateTo(const Windows::UI::Xaml::Interop::TypeName& typeName, const Windows::Foundation::IInspectable& param);


        void RootGrid_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);
        
        void ScrollViewer_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
        void RootGrid_SizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e);
        
        void HomeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
        void BackButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
        void Frame_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
        void DrivesPageTitleBarButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
        void WatchedFoldersPageTitleBarButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
        void SystemSearchPageTitleBarButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        
        void Frame_Navigated(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);

    private:
        static winrt::PCHealth::MainWindow singleton;

        Windows::Foundation::Collections::IObservableVector<winrt::hstring> drivesList{ winrt::single_threaded_observable_vector<winrt::hstring>() };
        bool usingCustomTitleBar = false;
        winrt::Windows::Graphics::RectInt32 displayRect{};
        winrt::Microsoft::UI::Windowing::AppWindow appWindow{ nullptr };
#pragma region Backdrop
        winrt::Microsoft::UI::Composition::SystemBackdrops::DesktopAcrylicController backdropController = nullptr;
        winrt::Windows::System::DispatcherQueueController dispatcherQueueController = nullptr;
        winrt::Microsoft::UI::Composition::SystemBackdrops::SystemBackdropConfiguration systemBackdropConfiguration = nullptr;
        //winrt::Microsoft::UI::Xaml::FrameworkElement::ActualThemeChanged_revoker themeChangedRevoker{};
#pragma endregion
        
        
        void InitializeWindow();
        
        void SetBackground();
        
        void SetDragRectangles();
        
        void AppWindow_Closing(winrt::Microsoft::UI::Windowing::AppWindow, winrt::Microsoft::UI::Windowing::AppWindowClosingEventArgs);
        
        void SaveWindow();
        
        void RestoreWindow();
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
