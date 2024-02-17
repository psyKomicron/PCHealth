#include "pch.h"
#include "ClipboardTrigger.h"
#if __has_include("ClipboardTrigger.g.cpp")
#include "ClipboardTrigger.g.cpp"
#endif

namespace winrt::PCHealth::implementation
{
    winrt::hstring ClipboardTrigger::Query() const
    {
        return _query;
    }

    void ClipboardTrigger::Query(const winrt::hstring & value)
    {
        _query = value;
    }
    
    winrt::hstring ClipboardTrigger::Match() const
    {
        return _match;
    }

    void ClipboardTrigger::Match(const winrt::hstring & value)
    {
        _match = value;
    }
    
    winrt::hstring ClipboardTrigger::ActionText() const
    {
        //TODO: I18N
        switch (_action)
        {
            case ClipboardTriggerAction::OpenWebBrowser:
                return L"Open web browser with clipboard content.";
            case ClipboardTriggerAction::PipeClipboardContentToExe:
                return L"Start executable with clipboard content.";
            case ClipboardTriggerAction::SaveToFile:
                return L"Save clipboard content to file.";
            case ClipboardTriggerAction::SaveToHistory:
                return L"Save clipboard content to this application's history.";
            case ClipboardTriggerAction::None:
            default:
                return winrt::hstring();
        }
    }

    void ClipboardTrigger::ActionText(const winrt::hstring& value)
    {
        _actionText = value;
    }

    ClipboardTriggerAction ClipboardTrigger::Action() const
    {
        return _action;
    }

    void ClipboardTrigger::Action(const ClipboardTriggerAction& value)
    {
        _action = value;
    }

    bool ClipboardTrigger::ShowNotification() const
    {
        return _showNotification;
    }

    void ClipboardTrigger::ShowNotification(const bool& value)
    {
        _showNotification = value;
    }
    
    winrt::hstring ClipboardTrigger::Name() const
    {
        return _name;
    }

    void ClipboardTrigger::Name(const winrt::hstring& value)
    {
        _name = value;
    }
}
