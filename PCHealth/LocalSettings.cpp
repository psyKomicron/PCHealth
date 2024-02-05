#include "pch.h"
#include "LocalSettings.h"

#include <string>

namespace PcHealth::Filesystem
{
    const bool SHOW_ALL_CONTAINERS = true;

    winrt::Windows::Storage::ApplicationDataContainer LocalSettings::openOrCreate(const winrt::hstring& key)
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

    void LocalSettings::saveList(const winrt::hstring& key, const std::vector<winrt::hstring> list)
    {
        if (localSettings.Containers().HasKey(key))
        {
            auto container = localSettings.Containers().Lookup(key);
            auto values = container.Values();
            bool contains = false;
            for (size_t i = 0; i < list.size(); i++)
            {
                winrt::hstring existingKey = L"";
                for (auto&& keyValuePair : values)
                {
                    winrt::hstring value = keyValuePair.Value().as<winrt::hstring>();
                    if (list[i] == value)
                    {
                        existingKey = keyValuePair.Key();
                        contains = true;
                        break;
                    }
                }

                if (!contains)
                {
                    std::hash<winrt::hstring> hasher{};
                    existingKey = winrt::to_hstring(hasher(list[i]));
                }

                container.Values().Insert(key, winrt::box_value(list[i]));
            }
        }
        else
        {
            winrt::Windows::Storage::ApplicationDataContainer container = createContainer(key);
            for (size_t i = 0; i < list.size(); i++)
            {
                std::hash<winrt::hstring> hasher{};
                container.Values().Insert(winrt::to_hstring(hasher(list[i])), winrt::box_value(list[i]));
            }
        }
    }

    std::vector<winrt::hstring> LocalSettings::restoreList(const winrt::hstring& key)
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
            for (auto&& container : localSettings.Containers())
            {
                OutputDebugString(container.Key().c_str());
            }
        }

        return list;
    }

    winrt::Windows::Storage::ApplicationDataContainer LocalSettings::createContainer(const winrt::hstring& key)
    {
        return localSettings.CreateContainer(key, winrt::Windows::Storage::ApplicationDataCreateDisposition::Always);
    }
}