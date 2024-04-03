#include "pch.h"
#include "LocalSettingsShortcuts.hpp"

#include "utilities.h"
#include "CompositeSetting.hpp"

using namespace pchealth::storage;

std::vector<WatchedFolder> LocalSettingsShortcuts::getWatchedFolders()
{
    LocalSettings settings = utilities::getLocalSettings();
    std::vector<WatchedFolder> folders{};

    if (settings.moveTo(L"WatchedFolders"))
    {
        auto&& objectList = settings.restoreObjectList();

        for (auto&& [key, composite] : objectList)
        {
            auto index = std::stoi(key.c_str());
            auto path = composite.extract<winrt::hstring>(L"path");
            auto favorite = composite.extract<bool>(L"favorite");

            WatchedFolder watchedFolder{ path, favorite };
            folders.push_back(std::move(watchedFolder));
        }
    }

    return folders;
}
