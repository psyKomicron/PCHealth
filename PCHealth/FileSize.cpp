#include "pch.h"
#include "FileSize.h"

namespace Common
{
    FileSize::FileSize(const uint64_t& size)
    {
        this->size = size;
    }

    uint64_t FileSize::Size() const
    {
        return size;
    }

    std::wstring FileSize::ToString() const
    {
        const double multiplicator = 2 * 10.0;
        double sizeToFormat = static_cast<double>(size);
        std::wstring ext = L"b";

        if (sizeToFormat >= 0x10000000000)
        {
            sizeToFormat *= multiplicator;
            sizeToFormat = round(sizeToFormat / 0x10000000000);
            sizeToFormat /= multiplicator;
            ext = L"Tb";
        }
        if (sizeToFormat >= 0x40000000)
        {
            sizeToFormat *= multiplicator;
            sizeToFormat = round(sizeToFormat / 0x40000000);
            sizeToFormat /= multiplicator;
            ext = L"Gb";
        }
        if (sizeToFormat >= 0x100000)
        {
            sizeToFormat *= multiplicator;
            sizeToFormat = round(sizeToFormat / 0x100000);
            sizeToFormat /= multiplicator;
            ext = L"Mb";
        }
        if (sizeToFormat >= 0x400)
        {
            sizeToFormat *= multiplicator;
            sizeToFormat = round(sizeToFormat / 0x400);
            sizeToFormat /= multiplicator;
            ext = L"Kb";
        }
        return std::format(L"{} {}", sizeToFormat, ext);
    }

    FileSize FileSize::operator=(const uint64_t& value)
    {
        return FileSize(value);
    }
}