#pragma once
namespace pchealth::storage
{
    class CompositeSetting
    {
    public:
        CompositeSetting(const winrt::Windows::Storage::ApplicationDataCompositeValue& composite);

        winrt::Windows::Storage::ApplicationDataCompositeValue asComposite();

    public:
        template<typename T>
        T extract(const winrt::hstring& key)
        {
            return composite.Lookup(key).as<T>();
        }

        template<>
        bool extract(const winrt::hstring& key)
        {
            auto ref = composite.Lookup(key).as<winrt::Windows::Foundation::IReference<bool>>();
            return ref.GetBoolean();
        }

        template<typename T>
        T tryExtract(const winrt::hstring& key)
        {
            auto inspectable = composite.TryLookup(key);
            if (inspectable != nullptr)
            {
                return inspectable.as<T>();
            }
            else
            {
                return {};
            }
        }

        template<typename T>
        void insert(const winrt::hstring& key, const T& value)
        {
            composite.Insert(key, winrt::box_value(value));
        }

        template<>
        void insert(const winrt::hstring& key, const bool& value)
        {
            composite.Insert(key, winrt::Windows::Foundation::IReference(value));
        }

    private:
        winrt::Windows::Storage::ApplicationDataCompositeValue composite{ nullptr };
    };
}

