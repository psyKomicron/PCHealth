#include "pch.h"
#include "Tag.xaml.h"
#if __has_include("Tag.g.cpp")
#include "Tag.g.cpp"
#endif

#include <winrt/Windows.UI.Xaml.Interop.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::PCHealth::implementation
{
    DependencyProperty Tag::headerProperty = DependencyProperty::Register(
        L"Header",
        winrt::xaml_typename<winrt::hstring>(),
        xaml_typename<PCHealth::Tag>(),
        PropertyMetadata(box_value(L""))
    );
    DependencyProperty Tag::textProperty = DependencyProperty::Register(
        L"Text",
        winrt::xaml_typename<hstring>(),
        winrt::xaml_typename<PCHealth::Tag>(),
        PropertyMetadata(box_value(L""))
    );
    DependencyProperty Tag::colorProperty = DependencyProperty::Register(
        L"Color",
        winrt::xaml_typename<winrt::Windows::Foundation::IInspectable>(),
        winrt::xaml_typename<PCHealth::Tag>(),
        PropertyMetadata(nullptr)
    );

    Tag::Tag()
    {
        InitializeComponent();
    }

    winrt::hstring Tag::Text() const
    {
        return GetValue(textProperty).as<winrt::hstring>();
    }

    void Tag::Text(const winrt::hstring& value)
    {
        SetValue(textProperty, winrt::box_value(value));
    }

    winrt::hstring Tag::Header() const
    {
        return GetValue(headerProperty).as<winrt::hstring>();
    }

    void Tag::Header(const winrt::hstring& value)
    {
        SetValue(headerProperty, winrt::box_value(value));
    }

    winrt::Windows::UI::Color Tag::Color() const
    {
        return GetValue(colorProperty).as<winrt::Windows::UI::Color>();
    }

    void Tag::Color(const winrt::Windows::UI::Color& value)
    {
        SetValue(colorProperty, winrt::box_value(value));
    }

    bool Tag::IsToggable()
    {
        return isToggable;
    }

    void Tag::IsToggable(const bool& value)
    {
        isToggable = value;

        if (toggled)
        {
            if (isToggable)
            {
                VisualStateManager::GoToState(*this, L"Toggled", true);
            }
            else
            {
                VisualStateManager::GoToState(*this, L"NotToggled", true);
            }
        }
    }


    void Tag::Grid_PointerPressed(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
        toggled = !toggled;

        if (isToggable)
        {
            if (toggled)
            {
                VisualStateManager::GoToState(*this, L"Toggled", true);
            }
            else
            {
                VisualStateManager::GoToState(*this, L"NotToggled", true);
            }
        }

        VisualStateManager::GoToState(*this, L"PointerPressed", true);
        e_click(*this, e);
    }

    void Tag::Grid_PointerEntered(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
        pointerOver = true;
        VisualStateManager::GoToState(*this, L"PointerOver", true);
    }

    void Tag::Grid_PointerExited(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
        pointerOver = false;
        VisualStateManager::GoToState(*this, L"Normal", true);
    }

    void Tag::RootGrid_PointerReleased(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
        if (pointerOver)
        {
            VisualStateManager::GoToState(*this, L"PointerOver", true);
        }
    }
}
