#pragma once

#include "ClipboardTrigger.g.h"

namespace winrt::PCHealth::implementation
{
    struct ClipboardTrigger : ClipboardTriggerT<ClipboardTrigger>
    {
    public:
        ClipboardTrigger() = default;

        winrt::hstring Match() const;
        void Match(const winrt::hstring& value);
        winrt::hstring Query() const;
        void Query(const winrt::hstring& value);
        winrt::hstring ActionText() const;
        void ActionText(const winrt::hstring& value);
        ClipboardTriggerAction Action() const;
        void Action(const ClipboardTriggerAction& value);
        bool ShowNotification() const;
        void ShowNotification(const bool& value);
        winrt::hstring Name() const;
        void Name(const winrt::hstring& value);

    private:
        winrt::hstring _match{};
        winrt::hstring _query{};
        winrt::hstring _actionText{};
        ClipboardTriggerAction _action = ClipboardTriggerAction::None;
        bool _showNotification = false;
        winrt::hstring _name{};
    };
}

namespace winrt::PCHealth::factory_implementation
{
    struct ClipboardTrigger : ClipboardTriggerT<ClipboardTrigger, implementation::ClipboardTrigger>
    {
    };
}
