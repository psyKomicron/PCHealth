#include "pch.h"
#include "DrivesView.xaml.h"
#if __has_include("DrivesView.g.cpp")
#include "DrivesView.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::PCHealth::implementation
{
    int32_t DrivesView::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void DrivesView::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void DrivesView::myButton_Click(IInspectable const&, RoutedEventArgs const&)
    {
        myButton().Content(box_value(L"Clicked"));
    }
}
