#include "pch.h"
#include "ClipboardManagerView.xaml.h"
#if __has_include("ClipboardManagerView.g.cpp")
#include "ClipboardManagerView.g.cpp"
#endif

#include "utilities.h"
#include "LocalSettings.h"

#include <winrt/Windows.ApplicationModel.DataTransfer.h>
#include <winrt/Microsoft.Windows.AppNotifications.Builder.h>

#include <regex>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::PCHealth::implementation
{
    ClipboardManagerView::ClipboardManagerView()
    {
        /*winrt::PCHealth::ClipboardTrigger trigger{};
        trigger.Name(L"secure website");
        trigger.Query(L"https://www.youtube.com/watch?v={}");
        trigger.Match(L"[A-z0-9_-]{11}");
        trigger.Action(winrt::PCHealth::ClipboardTriggerAction::OpenWebBrowser);
        trigger.ShowNotification(true);
        _formats.Append(trigger);*/

        appNotifManager.NotificationInvoked({ this, &ClipboardManagerView::NotificationManager_NotificationInvoked });
        appNotifManager.Register();
    }

    ClipboardManagerView::~ClipboardManagerView()
    {
        appNotifManager.Unregister();
        //appNotifManager.NotificationInvoked(token);
    }

    winrt::Windows::Foundation::Collections::IVector<winrt::PCHealth::ClipboardTrigger> ClipboardManagerView::Formats()
    {
        return _formats;
    }

    winrt::Windows::Foundation::IAsyncAction ClipboardManagerView::UserControl_Loading(winrt::Microsoft::UI::Xaml::FrameworkElement const&, winrt::Windows::Foundation::IInspectable const&)
    {
        co_await winrt::resume_background();

        if (winrt::Windows::Storage::ApplicationData::Current().LocalSettings().Containers().HasKey(L"ClipboardTriggers"))
        {
            auto&& clipboardTriggers = pchealth::storage::LocalSettings(winrt::Windows::Storage::ApplicationData::Current().LocalSettings());
            auto&& objectList = clipboardTriggers.restoreObjectList(L"ClipboardTriggers");

            for (auto&& object : objectList)
            {
                auto&& members = object.second;
                PCHealth::ClipboardTrigger trigger{};
                trigger.Name(object.first);
                trigger.Match(members[L"Match"].as<winrt::hstring>());
                trigger.Query(members[L"Query"].as<winrt::hstring>());
                trigger.Action(static_cast<winrt::PCHealth::ClipboardTriggerAction>(members[L"Action"].as<int32_t>()));
                trigger.ShowNotification(members[L"ShowNotification"].as<bool>());

                _formats.Append(trigger);
            }
        }
    }

    winrt::Windows::Foundation::IAsyncAction ClipboardManagerView::ClipboardListeningToggleSwitch_Toggled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e)
    {
        /*auto _ = copyHotKey.registerHotKey([this](auto&& guid)
        {
            DispatcherQueue().TryEnqueue([this]()
            {
                PopulateListViewFromClipboard();
            });
        });*/

        if (sender.as<Controls::ToggleSwitch>().IsOn())
        {
            clipboardChangedToken = winrt::Windows::ApplicationModel::DataTransfer::Clipboard::ContentChanged({ this, &ClipboardManagerView::Clipboard_ContentChanged });
            if (loadClipboardHistory)
            {
                auto&& history = co_await GetClipboardHistory();
                DispatcherQueue().TryEnqueue([this, history]()
                {
                    for (auto&& item : history)
                    {
                        ClipboardContentListView().Items().Append(winrt::box_value(item));
                    }
                });
            }
        }
        else
        {
            winrt::Windows::ApplicationModel::DataTransfer::Clipboard::ContentChanged(clipboardChangedToken);
        }
    }

    winrt::Windows::Foundation::IAsyncAction ClipboardManagerView::AppBarAddButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        auto res = co_await AddContentDialog().ShowAsync();
        if (res != Controls::ContentDialogResult::Primary || ActionComboBox().SelectedIndex() == -1)
        {
            co_return;
        }

        winrt::hstring match = AddContentDialogMatchTextBox().Text();
        winrt::hstring query = AddContentDialogQueryTextBox().Text();
        winrt::hstring name = AddContentDialogNameTextBox().Text();
        AddNewTrigger(
            name,
            query,
            match,
            ActionComboBox().SelectedItem().as<Controls::ComboBoxItem>().Tag().as<hstring>(),
            ShowNotificationToggleSwitch().IsOn()
        );
    }

    void ClipboardManagerView::AppBarRemoveButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        throw winrt::hresult_not_implemented();
    }

    void ClipboardManagerView::AppBarEditButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        throw winrt::hresult_not_implemented();
    }

    void ClipboardManagerView::AppBarSaveButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&)
    {
        auto&& settings = pchealth::storage::LocalSettings(winrt::Windows::Storage::ApplicationData::Current().LocalSettings());
        auto&& clipboardTriggers = pchealth::storage::LocalSettings(settings.openOrCreate(L"ClipboardTriggers"));
        for (auto&& trigger : _formats)
        {
            std::map<winrt::hstring, winrt::Windows::Foundation::IInspectable> map{};
            map.insert(std::make_pair(L"Match", winrt::box_value(trigger.Match())));
            map.insert(std::make_pair(L"Query", winrt::box_value(trigger.Query())));
            map.insert(std::make_pair(L"Action", winrt::box_value(static_cast<int32_t>(trigger.Action()))));
            map.insert(std::make_pair(L"ShowNotification", winrt::box_value(trigger.ShowNotification())));

            clipboardTriggers.saveObject(trigger.Name(), map);
        }
    }


    winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Foundation::Collections::IVector<winrt::hstring>> ClipboardManagerView::GetClipboardHistory()
    {
        auto vect = single_threaded_vector<winrt::hstring>();
        auto&& items = co_await winrt::Windows::ApplicationModel::DataTransfer::Clipboard::GetHistoryItemsAsync();
        for (auto&& item : items.Items())
        {
            vect.Append(co_await item.Content().GetTextAsync());
        }
        co_return vect;
    }

    void ClipboardManagerView::SendNotification(const winrt::PCHealth::ClipboardTrigger& trigger, const winrt::hstring& content)
    {
        if (trigger.ShowNotification())
        {
            winrt::hstring buttonText = L"";
            switch (trigger.Action())
            {
                case winrt::PCHealth::ClipboardTriggerAction::OpenWebBrowser:
                    buttonText = L"Open";
                    break;
                case winrt::PCHealth::ClipboardTriggerAction::SaveToFile:
                case winrt::PCHealth::ClipboardTriggerAction::SaveToHistory:
                    buttonText = L"Save";
                    break;
                case winrt::PCHealth::ClipboardTriggerAction::PipeClipboardContentToExe:
                    buttonText = L"Start";
                    break;
                    default:
#ifdef _DEBUG
                        throw std::exception("Clipboard trigger action not recognized, add missing switch cases.");
#endif // _DEBUG
                        break;
            }
            
            winrt::Microsoft::Windows::AppNotifications::Builder::AppNotificationBuilder appNotificationBuilder{};
            appNotificationBuilder
                .AddText(L"Clipboard contents matches with 1 of your triggers : " + buttonText)
                .AddText(content)
                .SetAttributionText(L"Clipboard manager");

            appNotificationBuilder.AddArgument(L"content", content);
            appNotificationBuilder.AddArgument(L"triggerName", trigger.Name());

            appNotifManager.Show(appNotificationBuilder.BuildNotification());
        }
    }

    winrt::Windows::Foundation::IAsyncAction ClipboardManagerView::TriggerClipboardTrigger(const winrt::PCHealth::ClipboardTrigger& trigger, const winrt::hstring& content)
    {
        auto&& query = std::wstring(trigger.Query());
        auto&& index = query.find(L"{}");
        auto&& left = query.substr(0, index);
        std::wstring s{};
        if ((index + 2) < query.size())
        {
            auto&& right = query.substr(index + 2);
            s = left + content + right;
        }
        else
        {
            s = left + content;
        }

        switch (trigger.Action())
        {
            case ClipboardTriggerAction::OpenWebBrowser:
                if (!co_await winrt::Windows::System::Launcher::LaunchUriAsync(winrt::Windows::Foundation::Uri(s)))
                {
                    OutputDebug(L"Failed to browser launch: " + s);
                }
                break;
            case ClipboardTriggerAction::PipeClipboardContentToExe:
                break;
            case ClipboardTriggerAction::SaveToFile:
                break;
            case ClipboardTriggerAction::SaveToHistory:
                break;
            case ClipboardTriggerAction::None:
                break;
        }
    }

    bool ClipboardManagerView::AddNewTrigger(const winrt::hstring& name, const winrt::hstring& query, const winrt::hstring& match, const winrt::hstring& actionString, const bool& showNotification)
    {
        if (match.empty() || name.empty() || query.empty() || actionString.empty())
        {
            return false;
        }
        else
        {
            for (auto&& trigger : _formats)
            {
                if (trigger.Name() == name)
                {
                    return false;
                }
            }
        }

        auto&& action = pchealth::utilities::getEnum<winrt::PCHealth::ClipboardTriggerAction>(actionString);
        winrt::PCHealth::ClipboardTrigger trigger{};
        trigger.Action(action);
        trigger.Query(query);
        trigger.Name(name);
        trigger.Match(match);
        trigger.ShowNotification(showNotification);

        _formats.Append(trigger);

        return true;
    }

    winrt::Windows::Foundation::IAsyncAction ClipboardManagerView::Clipboard_ContentChanged(const winrt::Windows::Foundation::IInspectable&, const winrt::Windows::Foundation::IInspectable&)
    {
        static winrt::hstring last = L"";

        auto&& content = winrt::Windows::ApplicationModel::DataTransfer::Clipboard::GetContent();
        winrt::hstring text = co_await content.GetTextAsync();

        if (text == last) co_return;
        last = text;

        for (auto&& trigger : _formats)
        {
            std::wregex re{ std::wstring(trigger.Match()), std::regex_constants::icase};
            if (std::regex_search(std::wstring(text), re))
            {
                SendNotification(trigger, text);
                OutputDebug(L"Clipboard content matches format: " + std::wstring(trigger.Match()) + L" - " + std::wstring(text));
                DispatcherQueue().TryEnqueue([this, text]()
                {
                    ClipboardContentTextBlock().Text(text);
                    ClipboardContentListView().Items().Append(winrt::box_value(text));
                });
                break;
            }
        }

        DispatcherQueue().TryEnqueue([this, text]()
        {
            ClipboardContentTextBlock().Text(text);
        });
    }

    void ClipboardManagerView::NotificationManager_NotificationInvoked(const winrt::Windows::Foundation::IInspectable&, const winrt::Microsoft::Windows::AppNotifications::AppNotificationActivatedEventArgs& e)
    {
        std::optional<winrt::hstring> content{};
        std::optional<winrt::hstring> triggerName{};
        if ((content = e.Arguments().TryLookup(L"content")).has_value() 
            && (triggerName = e.Arguments().TryLookup(L"triggerName")).has_value())
        {
            if (!content.value().empty() && !triggerName.value().empty())
            {
                for (auto&& trigger : _formats)
                {
                    TriggerClipboardTrigger(trigger, content.value());
                }
            }
        }
    }
}
