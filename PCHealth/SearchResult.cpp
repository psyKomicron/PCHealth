#include "pch.h"
#include "SearchResult.h"

namespace pchealth::windows::search
{
    SearchResult::SearchResult(const std::wstring& result, const SearchResultKind& kind)
    {
        _result = result;
        _kind = kind;
    }

    std::wstring SearchResult::result() const
    {
        return _result;
    }

    void SearchResult::result(const std::wstring& value)
    {
        _result = value;
    }

    SearchResultKind SearchResult::kind() const
    {
        return _kind;
    }

    void SearchResult::kind(const SearchResultKind& value)
    {
        _kind = value;
    }
}