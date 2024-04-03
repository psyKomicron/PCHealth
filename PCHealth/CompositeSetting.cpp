#include "pch.h"
#include "CompositeSetting.hpp"

using namespace pchealth::storage;

CompositeSetting::CompositeSetting(const winrt::Windows::Storage::ApplicationDataCompositeValue& _composite)
{
    composite = _composite;
}

winrt::Windows::Storage::ApplicationDataCompositeValue CompositeSetting::asComposite()
{
    return composite;
}
