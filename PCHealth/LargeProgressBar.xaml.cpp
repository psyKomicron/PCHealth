#include "pch.h"
#include "LargeProgressBar.xaml.h"
#if __has_include("LargeProgressBar.g.cpp")
#include "LargeProgressBar.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::PCHealth::implementation
{
    LargeProgressBar::LargeProgressBar()
    {
        InitializeComponent();
    }

    void LargeProgressBar::Value(double const& value)
    {
        currentValue = value;
        SetWidth();
        RaisePropChanged(L"Value");
    }

    double LargeProgressBar::Value() const
    {
        return currentValue;
    }

    void LargeProgressBar::Maximum(double const& value)
    {
        maximum = value;
        SetWidth();
        RaisePropChanged(L"Maximum");
    }

    double LargeProgressBar::Maximum() const
    {
        return maximum;
    }

    double LargeProgressBar::Percentage() const
    {
        return maximum == 0 ? 0 : round((currentValue / maximum) * 100.0);
    }

    void LargeProgressBar::UserControl_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        DeterminateProgressBarIndicator().Width(BackgroundBorder().ActualWidth());
        SetWidth();
    }

    void LargeProgressBar::Grid_SizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e)
    {
        DeterminateProgressBarIndicator().Width(BackgroundBorder().ActualWidth());
        SetWidth();
    }

    void LargeProgressBar::SetWidth()
    {
        if (maximum == 0)
        {
            //ProgressBarAnimation().To(0.0);
            DeterminateProgressBarIndicator().Width(0);
        }
        else
        {
            DeterminateProgressBarIndicator().Width((currentValue / maximum) * BackgroundBorder().ActualWidth());
            //DeterminateProgressBarIndicator().Scale(winrt::Windows::Foundation::Numerics::float3(currentValue / maximum, 1, 1));
        }

        /*if (ProgressBarStoryboard().GetCurrentState() != winrt::Microsoft::UI::Xaml::Media::Animation::ClockState::Filling)
        {
        ProgressBarStoryboard().Begin();
        }*/
    }

    void LargeProgressBar::RaisePropChanged(const winrt::hstring& propName)
    {
        e_propertyChanged(*this, Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(propName));
    }
}
