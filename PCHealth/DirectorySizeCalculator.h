#pragma once
namespace pchealth::filesystem
{
    class DirectorySizeCalculator
    {
    public:
        DirectorySizeCalculator() = default;

        winrt::event_token Progress(winrt::Windows::Foundation::TypedEventHandler<winrt::Windows::Foundation::IInspectable, winrt::Windows::Foundation::IReference<uint_fast64_t>> const& handler)
        {
            return m_event.add(handler);
        };
        void Progress(winrt::event_token const& token)
        {
            m_event.remove(token);
        };

        uint_fast64_t GetSize(const std::wstring& path, const bool& parallelize = false);

    private:
        winrt::event<winrt::Windows::Foundation::TypedEventHandler<winrt::Windows::Foundation::IInspectable, winrt::Windows::Foundation::IReference<uint_fast64_t>>> m_event;

        inline void RaiseProgress(uint_fast64_t newSize);
    };
}

