#pragma once
#include <winrt/Microsoft.Windows.AppNotifications.Builder.h>

namespace pchealth::windows
{
    class NotificationBuilder
    {
    public:
        NotificationBuilder() = default;

        winrt::Microsoft::Windows::AppNotifications::Builder::AppNotificationBuilder createAppNotification(const winrt::hstring& message) const;
        void addGuid(const winrt::Microsoft::Windows::AppNotifications::AppNotification& notification);
    };
}

