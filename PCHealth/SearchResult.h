#pragma once
#include "SearchResultKind.h"

namespace pchealth::windows::search
{
    class SearchResult
    {
    public:
        SearchResult() = default;
        SearchResult(const std::wstring& result, const SearchResultKind& kind);

        std::wstring result() const;
        void result(const std::wstring& value);
        SearchResultKind kind() const;
        void kind(const SearchResultKind& value);

    private:
        std::wstring _result{};
        SearchResultKind _kind{};
    };
}

