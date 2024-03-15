#pragma once

#include "DisplayView.g.h"

#include <winrt/Windows.Graphics.h>

namespace collections = winrt::Windows::Foundation::Collections;

namespace winrt::PCHealth::implementation
{
    enum DisplayViewMode
    {
        Simple,
        Full
    };
}

namespace winrt::PCHealth::implementation
{
    using SupportedResolutionsT = std::map<std::pair<uint32_t, uint32_t>, std::vector<uint32_t>>;

    struct DisplayView : DisplayViewT<DisplayView>
    {
    public:
        DisplayView() = default;

        int32_t DisplayIndex() const;
        void DisplayIndex(const int32_t& value);
        
        winrt::hstring DisplayName() const;
        void DisplayName(const winrt::hstring& value);

        Windows::Foundation::Collections::IVector<Windows::Foundation::IInspectable> ResolutionsItems() const
        {
            auto&& items = winrt::single_threaded_vector<Windows::Foundation::IInspectable>();
            for (auto&& pair : map)
            {
                winrt::Microsoft::UI::Xaml::Controls::ComboBoxItem item{};
                item.Tag(winrt::box_value(hstring(std::format(L"{},{}", pair.first.first, pair.first.second))));
                auto&& content = winrt::to_hstring(pair.first.first) + L"x" + winrt::to_hstring(pair.first.second);
                item.Content(box_value(content));
                items.Append(item);
            }
            return items;
        }

    public:
        void AddSupportedResolution(const uint32_t& x, const uint32_t& y, const uint32_t& frequency);
        void SetCurrentResolution(const uint32_t& x, const uint32_t& y, const uint32_t& frequency);

    public:
        void UserControl_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ResolutionsComboBox_SelectionChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Controls::SelectionChangedEventArgs const& e);
        void ChangeRefreshRateButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void ChangeResolutionButton_Click(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);

    private:
        bool loaded = false;
        winrt::hstring _displayName{};
        int32_t _displayIndex = 0;
        SupportedResolutionsT map{};
        std::vector<std::pair<uint32_t, uint32_t>> resolutions{};
        Windows::Graphics::SizeInt32 _currentResolution{};
        uint32_t _currentFrequency = 0;
        DisplayViewMode currentDisplayMode{};

    private:
        void LoadContentForFullMode();
        void ChangeState(const DisplayViewMode& mode);
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct DisplayView : DisplayViewT<DisplayView, implementation::DisplayView>
    {
    };
}
