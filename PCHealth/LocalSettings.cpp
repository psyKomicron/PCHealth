#include "pch.h"
#include "LocalSettings.h"

#include "utilities.h"

#include <boost/functional/hash.hpp>

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
            printContainer(localSettings);
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

    std::map<winrt::hstring, CompositeSetting> LocalSettings::restoreObjectList(const winrt::hstring& key) const
    {
        std::map<winrt::hstring, CompositeSetting> objectList{};
        auto&& containers = key.empty() ? localSettings.Values() : localSettings.Containers().Lookup(key).Values();
        for (auto&& container : containers)
        {
            winrt::hstring objectKey = container.Key();
            objectList.insert(std::make_pair(objectKey, CompositeSetting(container.Value().as<winrt::Windows::Storage::ApplicationDataCompositeValue>())));
        }

        return objectList;
    }

    void LocalSettings::openOrCreateAndMoveTo(const winrt::hstring& key)
    {
        localSettings = openOrCreate(key);
    }

    std::optional<winrt::Windows::Storage::ApplicationDataContainer> LocalSettings::tryLookup(const winrt::hstring& key)
    {
        auto&& obj = localSettings.Values().TryLookup(key);
        if (obj != nullptr)
        {
            return obj.try_as<winrt::Windows::Storage::ApplicationDataContainer>();
        }
        return {};
    }

    bool LocalSettings::moveTo(const winrt::hstring& key)
    {
        if (localSettings.Containers().HasKey(key))
        {
            OutputDebug(std::format(L"[LocalSettings] Container '{}' moved to {}.", localSettings.Name(), key));
            localSettings = localSettings.Containers().Lookup(key);
            if constexpr (SHOW_ALL_CONTAINERS)
            {
                printContainer(localSettings);
            }
            return true;
        }
        return false;
    }

    void LocalSettings::append(const winrt::hstring& key, const winrt::Windows::Storage::ApplicationDataCompositeValue& value)
    {
        localSettings.Values().Insert(key, value);
    }

    winrt::Windows::Storage::ApplicationDataCompositeValue LocalSettings::getComposite()
    {
        return winrt::Windows::Storage::ApplicationDataCompositeValue();
    }

    bool LocalSettings::contains(const winrt::hstring& key)
    {
        return localSettings.Values().HasKey(key);
    }

    void LocalSettings::clear(const winrt::hstring& key)
    {
        if (key.empty())
        {
            localSettings.Values().Clear();
        }
        else
        {
            localSettings.Containers().Lookup(key).Values().Clear();
        }
    }


    winrt::Windows::Storage::ApplicationDataContainer LocalSettings::createContainer(const winrt::hstring& key) const
    {
        return localSettings.CreateContainer(key, winrt::Windows::Storage::ApplicationDataCreateDisposition::Always);
    }

    void LocalSettings::printContainer(const winrt::Windows::Storage::ApplicationDataContainer& container) const
    {
        OutputDebug(std::format(L"[LocalSettings] Listing '{}' containers.", container.Name()));
        auto&& containers = container.Containers();
        for (auto&& pair : containers)
        {
            //container.Key()
            OutputDebug(std::format(L"{} : {} keys.", std::wstring(pair.Key()), pair.Value().Values().Size()));
        }
    }
}