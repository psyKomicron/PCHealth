#pragma once

#include "SystemSearchPage2.g.h"

namespace winrt::PCHealth::implementation
{
    struct SystemSearchPage2 : SystemSearchPage2T<SystemSearchPage2>
    {
    public:
        SystemSearchPage2() = default;

        void OnNavigatedTo(const Microsoft::UI::Xaml::Navigation::NavigationEventArgs& args);
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct SystemSearchPage2 : SystemSearchPage2T<SystemSearchPage2, implementation::SystemSearchPage2>
    {
    };
}
