#pragma once
#include <thread>
#include <atomic>
#include <functional>

namespace pchealth::win32
{
    class HotKey
    {
    public:
        HotKey(const wchar_t& key, const uint32_t& modifier, const bool& once);
        ~HotKey();

        void registerHotKey();
        winrt::guid registerHotKey(const std::function<void(winrt::guid)>& callback);

        wchar_t key() const;
        uint32_t modifier() const;

    private:
        static uint32_t hotKeyCount;

        std::atomic<uint32_t> threadId{};
        std::atomic<uint32_t> hotKeyId{};
        wchar_t _key = L'\0';
        uint32_t _modifier = 0;
        bool _once = false;
        std::thread notificationThread{};
        std::atomic_flag threadNotificationFlag{};
        std::function<void(winrt::guid)> callback{};
        std::atomic_bool threadLaunched = false;

        uint32_t getHotKeyId() const;
        void __threadFunc(bool* failed);
    };
}

