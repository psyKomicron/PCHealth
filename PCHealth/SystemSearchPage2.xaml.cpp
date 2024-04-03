#include "pch.h"
#include "SystemSearchPage2.xaml.h"
#if __has_include("SystemSearchPage2.g.cpp")
#include "SystemSearchPage2.g.cpp"
#endif

using namespace winrt::PCHealth::implementation;


void SystemSearchPage2::OnNavigatedTo(const Microsoft::UI::Xaml::Navigation::NavigationEventArgs& args)
{
    auto opt = args.Parameter().try_as<hstring>();
    if (opt.has_value())
    {
        hstring query = opt.value();
        SystemSearchViewer().Search(query);
    }
}
