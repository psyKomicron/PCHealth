#include "pch.h"
#include "HomePage.xaml.h"
#if __has_include("HomePage.g.cpp")
#include "HomePage.g.cpp"
#endif

#include "DisplayMonitor.h"
#include "LocalSettings.h"

namespace xaml = winrt::Microsoft::UI::Xaml;
namespace collections = winrt::Windows::Foundation::Collections;

namespace winrt::PCHealth::implementation
{
    void HomePage::Page_Loading(xaml::FrameworkElement const&, winrt::Windows::Foundation::IInspectable const&)
    {
        LoadDisplayWidgets();
        LoadFavoritesWatchedFolders();
    }


    void HomePage::LoadDisplayWidgets()
    {
        auto displays = pchealth::windows::DisplayMonitor::enumerateDisplays();
        for (int i = displays.size() - 1; i >= 0; i--)
        {
            auto& display = displays[i];

            PCHealth::DisplayView displayView{};
            displayView.DisplayName(display.friendlyName());
            displayView.DisplayIndex(display.id());
            auto currentRes = display.currentResolution();
            displayView.SetCurrentResolution(display.currentResolution().first, display.currentResolution().second, display.vsync());

            auto&& modes = display.supportedResolutions();
            for (auto&& mode : modes)
            {
                for (auto&& freq : mode.second)
                {
                    displayView.AddSupportedResolution(mode.first.first, mode.first.second, freq);
                }
            }

            xaml::Controls::VariableSizedWrapGrid::SetColumnSpan(displayView, 1);
            xaml::Controls::VariableSizedWrapGrid::SetRowSpan(displayView, 2);
            SecondGrid().Children().InsertAt(0, displayView);
        }
    }

    void HomePage::LoadFavoritesWatchedFolders()
    {
        pchealth::storage::LocalSettings settings{ Windows::Storage::ApplicationData::Current().LocalSettings() };
        auto watchedFolders = settings.tryLookup(L"WatchedFolders");

    }
}
