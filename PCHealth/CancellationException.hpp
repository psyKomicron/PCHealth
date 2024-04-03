#pragma once
#include <exception>

namespace pchealth
{
    class CancellationException : public std::exception
    {
    public:
        CancellationException() = default;

        const char* what()
        {
            return "Operation has been cancelled.";
        }
    };
}
