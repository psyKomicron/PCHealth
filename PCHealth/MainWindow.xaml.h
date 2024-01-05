#pragma once

#include "MainWindow.g.h"



namespace winrt::PCHealth::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
    public:
        MainWindow();

        double SystemGeneralHealth();
        void SystemGeneralHealth(const double& value);

        inline winrt::event_token PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& value) { return e_propertyChanged.add(value); };
        inline void PropertyChanged(winrt::event_token const& token) { e_propertyChanged.remove(token); };

        winrt::Windows::Foundation::IAsyncAction RootGrid_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);
        void ScrollViewer_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::Windows::Foundation::IAsyncAction SystemRecycleBinSizeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void HibernationFileSizeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        winrt::Windows::Foundation::IAsyncAction DownloadsFolderSizeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        double systemGeneralHealth = 0.0;
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

        winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> e_propertyChanged;
        
        void InitializeWindow();
        void SetBackground();
        void SetDragRectangles();
        void AppWindow_Closing(winrt::Microsoft::UI::Windowing::AppWindow, winrt::Microsoft::UI::Windowing::AppWindowClosingEventArgs);
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
