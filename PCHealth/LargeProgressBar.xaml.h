#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "LargeProgressBar.g.h"

namespace winrt::PCHealth::implementation
{
    struct LargeProgressBar : LargeProgressBarT<LargeProgressBar>
    {
    public:
        LargeProgressBar();

        event_token PropertyChanged(Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& value)
        {
            return e_propertyChanged.add(value);
        };
        void PropertyChanged(event_token const& token)
        {
            e_propertyChanged.remove(token);
        };

        void Value(double const& value);
        double Value() const;
        void Maximum(double const& value);
        double Maximum() const;
        double Percentage() const;

        void UserControl_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void Grid_SizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e);

    private:
        double currentValue = 0;
        double maximum = 100;
        winrt::event_token windowSizeChanged;

        winrt::event<Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> e_propertyChanged;

        void SetWidth();
        void RaisePropChanged(const winrt::hstring& propName);
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct LargeProgressBar : LargeProgressBarT<LargeProgressBar, implementation::LargeProgressBar>
    {
    };
}
