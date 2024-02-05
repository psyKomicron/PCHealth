#pragma once
namespace PcHealth::Filesystem
{
    class LocalSettings
    {
    public:
        LocalSettings(const winrt::Windows::Storage::ApplicationDataContainer& container)
        {
            localSettings = container;
        };

        template <typename T>
        std::optional<T> tryLookup(const winrt::hstring& key)
        {
            auto container = localSettings.Containers().TryLookup(key);
            if (container.Values().Size() > 0)
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

        winrt::Windows::Storage::ApplicationDataContainer openOrCreate(const winrt::hstring& key);
        void saveList(const winrt::hstring& key, const std::vector<winrt::hstring> list);
        std::vector<winrt::hstring> restoreList(const winrt::hstring& key);

    private:
        winrt::Windows::Storage::ApplicationDataContainer localSettings{ nullptr };

        winrt::Windows::Storage::ApplicationDataContainer createContainer(const winrt::hstring& key);
    };
}
