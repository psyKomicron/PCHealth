#include "pch.h"
#include "LocalSettings.h"

#include "utilities.h"

#include <string>

namespace pchealth::storage
{
    const bool SHOW_ALL_CONTAINERS = true;

    winrt::Windows::Storage::ApplicationDataContainer LocalSettings::openOrCreate(const winrt::hstring& key) const
    {
        if (localSettings.Containers().HasKey(key))
        {
            return localSettings.Containers().Lookup(key);
        }
        else
        {
            return localSettings.CreateContainer(key, winrt::Windows::Storage::ApplicationDataCreateDisposition::Always);
        }
    }

    void LocalSettings::saveList(const winrt::hstring& key, const std::vector<winrt::hstring> list) const
    {
        winrt::Windows::Storage::ApplicationDataContainer container{ nullptr };
        if (localSettings.Containers().HasKey(key))
        {
            container = localSettings.Containers().Lookup(key);
            container.Values().Clear();
        }
        else
        {
            container = createContainer(key);
        }

        for (size_t i = 0; i < list.size(); i++)
        {
            std::hash<winrt::hstring> hasher{};
            container.Values().Insert(winrt::to_hstring(hasher(list[i])), winrt::box_value(list[i]));
        }
    }

    std::vector<winrt::hstring> LocalSettings::restoreList(const winrt::hstring& key) const
    {
        std::vector<winrt::hstring> list{};

        if (localSettings.Containers().HasKey(key))
        {
            auto container = localSettings.Containers().Lookup(key);
            auto values = container.Values();
            for (auto&& keyValuePair : values)
            {
                list.push_back(keyValuePair.Value().as<winrt::hstring>());
            }
        }
        else if constexpr (SHOW_ALL_CONTAINERS)
        {
            OutputDebug(L"Showing all containers...");
            for (auto&& container : localSettings.Containers())
            {
                OutputDebug(std::wstring(container.Key()));
            }
        }

        return list;
    }

    void LocalSettings::saveObject(const winrt::hstring& key, const std::map<winrt::hstring, winrt::Windows::Foundation::IInspectable>& objectAsMap) const
    {
        auto&& container = openOrCreate(key);
        for (auto&& pair : objectAsMap)
        {
            container.Values().Insert(pair.first, pair.second);
        }
    }

    std::map<winrt::hstring, std::map<winrt::hstring, winrt::Windows::Foundation::IInspectable>> LocalSettings::restoreObjectList(const winrt::hstring& key) const
    {
        std::map<winrt::hstring, std::map<winrt::hstring, winrt::Windows::Foundation::IInspectable>> objectList{};
        auto&& containers = localSettings.Containers().Lookup(key);
        for (auto&& container : containers.Containers())
        {
            winrt::hstring objectKey = container.Key();

            std::map<winrt::hstring, winrt::Windows::Foundation::IInspectable> membersMap{};
            for (auto&& value : container.Value().Values())
            {
                membersMap.insert(std::make_pair(value.Key(), value.Value()));
            }

            objectList.insert(std::make_pair(objectKey, membersMap));
        }

        return objectList;
    }

    void LocalSettings::openOrCreateAndMoveTo(const winrt::hstring& key)
    {
        localSettings = openOrCreate(key);
    }

    std::optional<winrt::Windows::Storage::ApplicationDataContainer> LocalSettings::tryLookup(const winrt::hstring& key)
    {
        return {};
    }

    winrt::Windows::Storage::ApplicationDataContainer LocalSettings::createContainer(const winrt::hstring& key) const
    {
        return localSettings.CreateContainer(key, winrt::Windows::Storage::ApplicationDataCreateDisposition::Always);
    }
}