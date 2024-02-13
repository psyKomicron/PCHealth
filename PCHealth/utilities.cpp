#include "pch.h"
#include "utilities.h"

void OutputDebug(const std::wstring message)
{
    OutputDebugStringW((message + L"\n").c_str());
}
