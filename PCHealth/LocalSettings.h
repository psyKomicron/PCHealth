#pragma once
#include "CompositeSetting.hpp"

namespace pchealth::storage
{
    class LocalSettings
    {
    public:
        LocalSettings(const winrt::Windows::Storage::ApplicationDataContainer& container)
        {
            localSettings = container;
        };

        winrt::Windows::Storage::ApplicationDataContainer openOrCreate(const winrt::hstring& key) const;
        
        void saveList(const winrt::hstring& key, const std::vector<winrt::hstring> list) const;
        
        std::vector<winrt::hstring> restoreList(const winrt::hstring& key) const;
        
        void saveObject(const winrt::hstring& key, const std::map<winrt::hstring, winrt::Windows::Foundation::IInspectable>& objectAsMap) const;
        
        std::map<winrt::hstring, CompositeSetting> restoreObjectList(const winrt::hstring& key = {}) const;
        
        void openOrCreateAndMoveTo(const winrt::hstring& key);

        std::optional<winrt::Windows::Storage::ApplicationDataContainer> tryLookup(const winrt::hstring& key);

        bool moveTo(const winrt::hstring& key);

        void append(const winrt::hstring& key, const winrt::Windows::Storage::ApplicationDataCompositeValue& value);

        winrt::Windows::Storage::ApplicationDataCompositeValue getComposite();

        bool contains(const winrt::hstring& key);

        void clear(const winrt::hstring& key = {});


        template <typename T>
        std::optional<T> tryLookupValue(const winrt::hstring& key)
        {
            auto container = localSettings.Values().TryLookup(key);
            if (container != nullptr)
            {
                return container.as<T>();
            }
            else
            {
                return std::optional<T>();
            }
        }

        template <>
        std::optional<CompositeSetting> tryLookupValue(const winrt::hstring& key)
        {
            auto containerOpt = tryLookupValue<winrt::Windows::Storage::ApplicationDataCompositeValue>(key);
            if (containerOpt.has_value())
            {
                return CompositeSetting(containerOpt.value());
            }
            else
            {
                return {};
            }
        }

        template <typename T>
        std::optional<T> tryLookup(const winrt::hstring& key)
        {
            auto container = localSettings.Containers().TryLookup(key);
            if (container != nullptr && container.Values().Size() > 0)
            {
                return std::optional<T>(container.as<T>());
            }
            else
            {
                return std::optional<T>();
            }
        };

        template <typename T>
        std::optional<T> tryLookupWith(const winrt::hstring& key, const winrt::Windows::Storage::ApplicationDataContainer& container)
        {
            if (container.Values().HasKey(key))
            {
                return container.Values().Lookup(key).as<T>();
            }
            else
            {
                return std::optional<T>();
            }
        }

        template <typename T>
        void insert(const winrt::hstring& key, const T& value)
        {
            localSettings.Values().Insert(key, winrt::box_value(value));
        }

    private:
        winrt::Windows::Storage::ApplicationDataContainer localSettings{ nullptr };


        winrt::Windows::Storage::ApplicationDataContainer createContainer(const winrt::hstring& key) const;
        void printContainer(const winrt::Windows::Storage::ApplicationDataContainer& container) const;
        winrt::hstring hash(const winrt::hstring& key);
    };
}

