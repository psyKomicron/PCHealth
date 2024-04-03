#pragma once

#include "LocalSettings.h"

#include <vector>

namespace pchealth::storage
{
    struct WatchedFolder
    {
        winrt::hstring path{};
        bool favorite{};
    };

    class LocalSettingsShortcuts
    {
    public:
        LocalSettingsShortcuts() = default;

        std::vector<WatchedFolder> getWatchedFolders();
    };
}

