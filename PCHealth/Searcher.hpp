#pragma once

#include "BaseSearcher.hpp"

#include <mutex>
#include <queue>
#include <ppltasks.h>


namespace pchealth::windows::search
{
    class Searcher : public BaseSearcher
    {
    public:
        Searcher(const bool& includeRecycleBin, const CallbackType& callback);

        virtual std::vector<pchealth::windows::search::SearchResult> search(const std::wstring& query, const std::vector<pchealth::filesystem::DriveInfo>& drives = {});


    private:
        std::timed_mutex queueMutex{};
        std::queue<std::wstring> pathQueue{};
        int32_t fillQueueMaxDepth = 2;


        virtual inline bool consumePath(std::wstring& path);

        virtual void performSearch(const std::filesystem::path& path, const boost::wregex& queryRe, std::vector<SearchResult>& results, const uint32_t& threadId);

        virtual void specificPrepareSearch();

        concurrency::task<void> searchApplications(const std::wstring& query);

        std::vector<pchealth::windows::search::SearchResult> fillQueue(const std::wstring& root, const std::wstring& query, const int32_t& maxDepth, int32_t depth = 1);
    };
}