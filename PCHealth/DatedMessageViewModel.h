#pragma once

#include "DatedMessageViewModel.g.h"

#include <winrt/Windows.Globalization.DateTimeFormatting.h>
#include <chrono>

namespace winrt::PCHealth::implementation
{
    struct DatedMessageViewModel : DatedMessageViewModelT<DatedMessageViewModel>
    {
    public:
        DatedMessageViewModel()
        {
            timestamp = winrt::clock::from_sys(std::chrono::system_clock::now());
        };
        DatedMessageViewModel(const winrt::Windows::Foundation::DateTime& time)
        {
            timestamp = time;
        };

        winrt::hstring Message() const
        {
            return message;
        };
        
        void Message(const winrt::hstring& value)
        {
            message = value;
        };

        winrt::hstring TimeStamp() const
        {
            return formatter.Format(timestamp);
        };

        void TimeStamp(const winrt::hstring& value)
        {
        };

    private:
        winrt::Windows::Globalization::DateTimeFormatting::DateTimeFormatter formatter{ L"hour minute second" };
        winrt::hstring message{};
        winrt::Windows::Foundation::DateTime timestamp{};
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct DatedMessageViewModel : DatedMessageViewModelT<DatedMessageViewModel, implementation::DatedMessageViewModel>
    {
    };
}
