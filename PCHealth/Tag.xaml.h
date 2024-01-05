#pragma once

#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "Tag.g.h"

#include <winrt/Microsoft.UI.Xaml.Input.h>

namespace winrt::PCHealth::implementation
{
    struct Tag : TagT<Tag>
    {
        Tag();

        static Microsoft::UI::Xaml::DependencyProperty HeaderProperty() { return headerProperty; }
        static Microsoft::UI::Xaml::DependencyProperty TextProperty() { return textProperty; }
        static Microsoft::UI::Xaml::DependencyProperty ColorProperty() { return colorProperty; }

        winrt::hstring Text() const;
        void Text(const winrt::hstring& value);
        winrt::hstring Header() const;
        void Header(const winrt::hstring& value);
        winrt::Windows::UI::Color Color() const;
        void Color(const winrt::Windows::UI::Color& value);
        bool IsToggable();
        void IsToggable(const bool& value);

        inline winrt::event_token Click(const Windows::Foundation::TypedEventHandler<winrt::PCHealth::Tag, Microsoft::UI::Xaml::RoutedEventArgs>& handler)
        { return e_click.add(handler); };
        inline void Click(const winrt::event_token token)
        { e_click.remove(token); };

        void Grid_PointerPressed(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void Grid_PointerEntered(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void Grid_PointerExited(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);
        void RootGrid_PointerReleased(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e);

    private:
        static Microsoft::UI::Xaml::DependencyProperty headerProperty;
        static Microsoft::UI::Xaml::DependencyProperty textProperty;
        static Microsoft::UI::Xaml::DependencyProperty colorProperty;

        bool pointerOver = false;
        bool isToggable = false;
        bool toggled = false;

        winrt::event<Windows::Foundation::TypedEventHandler<winrt::PCHealth::Tag, Microsoft::UI::Xaml::RoutedEventArgs>> e_click{};
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct Tag : TagT<Tag, implementation::Tag>
    {
    };
}
