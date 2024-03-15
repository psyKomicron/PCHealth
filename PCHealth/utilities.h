#pragma once
#include <string>

//#define ENABLE_DEBUG_OUTPUT

constexpr int ALTERNATE_MAX_PATH = 2048;

void OutputDebug(const std::wstring message);
void OutputDebug(const std::string message);

namespace pchealth::utilities
{
    uint64_t convert(const uint32_t& high, const uint32_t& low);

    template<typename T>
    T getEnum(const winrt::Windows::Foundation::IInspectable& inspectable)
    {
        return getEnum<T>(inspectable.as<winrt::hstring>());
    }

    template<typename T>
    T getEnum(const winrt::hstring& s)
    {
        return static_cast<T>(std::stoi(s.c_str()));
    }

    template<typename T>
    T createWin32Struct()
    {
        T s{ sizeof(T) };
        return s;
    }
}