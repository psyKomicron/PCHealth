#pragma once

#include "HomePage.g.h"

namespace winrt::PCHealth::implementation
{
    struct HomePage : HomePageT<HomePage>
    {
    public:
        HomePage() = default;

        void Page_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args);

    private:
        void LoadDisplayWidgets();
        void LoadFavoritesWatchedFolders();
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct HomePage : HomePageT<HomePage, implementation::HomePage>
    {
    };
}
