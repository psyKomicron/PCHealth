#include "pch.h"
#include "utilities.h"

void OutputDebug(const std::wstring message)
{
    OutputDebugStringW((message + L"\n").c_str());
}

void OutputDebug(const std::string message)
{
    OutputDebugStringA((message + "\n").c_str());
}

uint64_t pchealth::utilities::convert(const uint32_t& high, const uint32_t& low)
{
    return (static_cast<uint64_t>(high) << 32) | low;
}

pchealth::storage::LocalSettings pchealth::utilities::getLocalSettings()
{
    return pchealth::storage::LocalSettings(winrt::Windows::Storage::ApplicationData::Current().LocalSettings());
}
