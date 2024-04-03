#pragma once
namespace winrt::PCHealth
{
    template<typename T>
    class VisualState
    {
    public:
        VisualState()
        {
            isInvalidState = true;
        }

        VisualState(const winrt::hstring& name, const int32_t& group, const bool& active) :
            _name{ name },
            _group{ group },
            _active{ active }
        {
        }

        int32_t group() const
        {
            throwIfInvalid();
            return _group;
        }

        bool active() const
        {
            return _active;
        }

        void active(const bool& value)
        {
            _active = value;
        }

        winrt::hstring name() const
        {
            return _name;
        }

        bool operator==(const VisualState<T>& other)
        {
            throwIfInvalid();
            return other.name() == name();
        }

        operator winrt::param::hstring() const
        {
            throwIfInvalid();
            return _name;
        }

    private:
        bool isInvalidState = false;
        winrt::hstring _name{};
        int32_t _group;
        bool _active;

        inline void throwIfInvalid() const
        {
            if (isInvalidState)
            {
                throw winrt::hresult_invalid_argument(L"The current visual state is not valid (empty).");
            }
        }
    };
}