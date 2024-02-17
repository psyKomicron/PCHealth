#include "pch.h"
#include "HotKey.h"

#include "utilities.h"

namespace pchealth::win32
{
    uint32_t hotKeyCount = 1;

    HotKey::HotKey(const wchar_t& key, const uint32_t& modifier, const bool& once)
    {
        _key = key;
        _modifier = modifier;
        _once = once;
    }

    HotKey::~HotKey()
    {
        if (threadLaunched.load())
        {
            UnregisterHotKey(nullptr, hotKeyId.load());
            PostThreadMessage(threadId.load(), WM_QUIT, 0, 0); // Post quit message to the HotKey thread (GetMessage will return 0).
            notificationThread.join(); // Wait for the thread to exit.
            /*if (notificationThread.joinable())
            {
            }*/
        }
    }

    void HotKey::registerHotKey()
    {
        bool failed = false;
        notificationThread = std::thread(&HotKey::__threadFunc, this, &failed);
        threadNotificationFlag.wait(false);
        threadNotificationFlag.clear();
        if (failed)
        {
            throw std::invalid_argument("Hot key is not valid (rejected by the system).");
        }
        
        if (_once)
        {
            threadNotificationFlag.wait(false);
        }
    }

    winrt::guid HotKey::registerHotKey(const std::function<void(winrt::guid)>& _callback)
    {
        callback = _callback;
        return {};
    }

    wchar_t HotKey::key() const
    {
        return _key;
    }

    uint32_t HotKey::modifier() const
    {
        return _modifier;
    }


    uint32_t HotKey::getHotKeyId() const
    {
        return 0;
        //return hotKeyCount.fetch_add(1);
    }

    void HotKey::__threadFunc(bool* failed)
    {
        threadLaunched.store(true);
        threadId.store(GetCurrentThreadId());
        auto&& id = threadId.load();
        hotKeyId.store(getHotKeyId());

        if (!RegisterHotKey(nullptr, hotKeyId.load(), _modifier, _key))
        {
            *failed = true;
            threadNotificationFlag.test_and_set();
            threadNotificationFlag.notify_one();
            return;
        }

        threadNotificationFlag.test_and_set();
        threadNotificationFlag.notify_one();

        OutputDebug(std::format(L"(HotKey) {:x} Hot key registered - vk: {} mod: {}.", id, _key, _modifier));

        MSG message{};
        while (true)
        {
            //PeekMessage()
            if (!(GetMessage(&message, (HWND)(-1), 0, 0) & 1))
            {
                OutputDebug(std::format(L"(HotKey) {:x} Exiting message loop.", id));
                break;
            }

            if (message.message == WM_HOTKEY)
            {
                //TODO: Hot key fired.
                callback(winrt::guid());
                if (_once)
                {
                    threadNotificationFlag.test_and_set();
                    threadNotificationFlag.notify_one();
                    break;
                }
            }
        }

        // Unregister the HotKey when exiting the thread.
        if (!UnregisterHotKey(nullptr, hotKeyId.load()))
        {
            OutputDebug(std::format(L"(HotKey) {:x} WARNING : Failed to unregister hot key - vk: {} mod: {}.", id, _key, _modifier));
        }

        OutputDebug(std::format(L"(HotKey) {:x} Shutting down notification thread.", id));

        threadLaunched.store(false);
    }
}