#pragma once

#include "DrivesView.g.h"

namespace winrt::PCHealth::implementation
{
    struct DrivesView : DrivesViewT<DrivesView>
    {
        DrivesView()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        int32_t MyProperty();
        void MyProperty(int32_t value);

        void myButton_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct DrivesView : DrivesViewT<DrivesView, implementation::DrivesView>
    {
    };
}
