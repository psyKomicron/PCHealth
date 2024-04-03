#include "pch.h"
#include "DisplayView.xaml.h"
#if __has_include("DisplayView.g.cpp")
#include "DisplayView.g.cpp"
#endif

#include "DisplayMonitor.hpp"
#include "utilities.h"

namespace xaml = winrt::Microsoft::UI::Xaml;

namespace winrt::PCHealth::implementation
{
    int32_t DisplayView::DisplayIndex() const
    {
        return _displayIndex;
    }

    void DisplayView::DisplayIndex(const int32_t& value)
    {
        _displayIndex = value;
    }

    winrt::hstring DisplayView::DisplayName() const
    {
        return _displayName;
    }

    void DisplayView::DisplayName(const winrt::hstring& value)
    {
        _displayName = value;
    }


    void DisplayView::AddSupportedResolution(const uint32_t& x, const uint32_t& y, const uint32_t& frequency)
    {
        map[{x, y}].push_back(frequency);
    }

    void DisplayView::SetCurrentResolution(const uint32_t& x, const uint32_t& y, const uint32_t& frequency)
    {
        _currentResolution = Windows::Graphics::SizeInt32(x, y);
        _currentFrequency = frequency;
    }


    void DisplayView::UserControl_Loaded(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
    {
        ChangeState(DisplayViewMode::Simple);
        switch (currentDisplayMode)
        {
            case DisplayViewMode::Simple:
            {
                FrequencyTextBlock().Text(winrt::to_hstring(_currentFrequency) + L"Hz");

            }
            case DisplayViewMode::Full:
            {
                LoadContentForFullMode();
                loaded = true;
                ResolutionsComboBox_SelectionChanged(nullptr, nullptr);
            }
        }

        loaded = true;
    }

    void DisplayView::ResolutionsComboBox_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, xaml::Controls::SelectionChangedEventArgs const& e)
    {
        if (!loaded) return;

        auto&& index = ResolutionsComboBox().SelectedItem().as<xaml::Controls::ComboBoxItem>().Tag().try_as<hstring>();
        if (index.has_value())
        {
            auto&& s = std::wstring(index.value());
            std::pair<uint32_t, uint32_t> resolution{ std::stoi(s.substr(0, s.find(L","))), std::stoi(s.substr(s.find(L",") + 1)) };
            if (map.contains(resolution))
            {
                RefreshRatesComboBox().Items().Clear();
                RefreshRatesComboBox().IsEnabled(true);

                auto&& supportedFrequencies = map[resolution];
                for (auto&& freq : supportedFrequencies)
                {
                    xaml::Controls::ComboBoxItem item{};
                    item.Tag(winrt::box_value(freq));
                    item.Content(winrt::box_value(winrt::to_hstring(freq) + L"Hz"));
                    RefreshRatesComboBox().Items().Append(item);
                    if ((_currentResolution.Width == resolution.first && _currentResolution.Height == resolution.second)
                        && freq == _currentFrequency)
                    {
                        RefreshRatesComboBox().SelectedItem(item);
                    }
                }
            }
        }
    }

    void DisplayView::ChangeRefreshRateButton_Click(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
    {
        if (currentDisplayMode == DisplayViewMode::Full)
        {
            std::optional<xaml::Controls::ComboBoxItem> selected = RefreshRatesComboBox().SelectedItem().try_as<xaml::Controls::ComboBoxItem>();
            if (selected.has_value())
            {
                auto&& freq = selected.value().Tag().as<uint32_t>();
                if (freq == _currentFrequency)
                {
                    auto display = pchealth::windows::DisplayMonitor::getDisplayMonitorForDisplayId(_displayIndex);
                    if (display.has_value())
                    {
                        display.value().reapplyCurrentVsync();
                    }
                }
                /*else
                {
                }*/
            }
        }
        else if (currentDisplayMode == DisplayViewMode::Simple)
        {
            auto display = pchealth::windows::DisplayMonitor::getDisplayMonitorForDisplayId(_displayIndex);
            display.value().reapplyCurrentVsync();
        }
    }

    void DisplayView::ChangeResolutionButton_Click(winrt::Windows::Foundation::IInspectable const&, xaml::RoutedEventArgs const&)
    {
        // TODO: Change to the selected resolution.
    }


    void DisplayView::LoadContentForFullMode()
    {
        {
            hstring tag = hstring(std::format(L"{},{}", _currentResolution.Width, _currentResolution.Height));
            for (size_t i = 0; i < ResolutionsComboBox().Items().Size(); i++)
            {
                auto&& item = ResolutionsComboBox().Items().GetAt(i).as<xaml::Controls::ComboBoxItem>();
                if (item.Tag().try_as<hstring>().value_or(L"") == tag)
                {
                    ResolutionsComboBox().SelectedIndex(i);
                }
            }
        }

        for (auto&& inspectable : RefreshRatesComboBox().Items())
        {
            auto&& item = inspectable.as<xaml::Controls::ComboBoxItem>();
            auto&& tag = item.Tag().try_as<hstring>();
            if (tag.has_value())
            {
                uint32_t u = std::stoi(std::wstring(tag.value()));
                if (u == _currentFrequency)
                {
                    RefreshRatesComboBox().SelectedItem(inspectable);
                    RefreshRatesComboBox().IsEnabled(true);
                }
            }
        }
    }

    void DisplayView::ChangeState(const DisplayViewMode& mode)
    {
        currentDisplayMode = mode;
        switch (mode)
        {
            case DisplayViewMode::Full:
                xaml::VisualStateManager::GoToState(*this, L"FullStateMode", true);
                break;
            case DisplayViewMode::Simple:
                xaml::VisualStateManager::GoToState(*this, L"SimpleStateMode", true);
                break;
            default:
                throw std::out_of_range("DisplayViewMode not recognized.");
        }
    }
}
