#pragma once

namespace Common
{
    class ICancellable
    {
    public:
        void Cancel();
        void Start();
    };
}