#pragma once

#include "SearchResult.h"
#include "DriveInfo.h"

#include <functional>
#include <vector>
#include <mutex>
#include <queue>
#include <string>
#include <filesystem>
#include <chrono>
#include <atomic>
#include <regex>
#include <ppltasks.h>

using CallbackType = std::function<void(const std::vector<pchealth::windows::search::SearchResult>&, const bool&)>;

namespace pchealth::windows::search
{
    class Searcher
    {
    public:
        Searcher(const bool& includeRecycleBin, const CallbackType& callback);

        bool includeRecycleBin() const;
        void includeRecycleBin(const bool& value);

        int64_t remaining() const;

        std::vector<pchealth::windows::search::SearchResult> search(const std::wstring& query, const std::vector<pchealth::filesystem::DriveInfo>& drives = {});

        void cancelCurrentSearch();

    private:
        CallbackType callback{};
        //
        std::thread notificationThread{};
        std::timed_mutex queueMutex{};
        std::queue<std::wstring> pathQueue{};
        std::atomic_bool searchRunning = false;
        bool _includeRecycleBin = false;
        int32_t fillQueueMaxDepth = 4;
        std::atomic_int64_t pathQueueCount = 0;
        std::atomic_flag queueReady{};
        std::atomic_bool queueFillingEnded = false;
        std::atomic_bool searchCancelled = false;
        bool _rebuildQueue = true;

        std::vector<pchealth::windows::search::SearchResult> fillQueue(const std::wstring& root, const std::wstring& query, const int32_t& maxDepth, int32_t depth = 1);
        std::wstring consumePath();
        void getBackoff(uint32_t& backoff);
        void search(const std::filesystem::path& path, const std::wregex& queryRe, std::vector<pchealth::windows::search::SearchResult>& results, const uint32_t& threadId);
        concurrency::task<void> searchApplications(const std::wstring& query);
        bool isSearchCancelled() const;
        void checkIfSearchCancelled() const;
        void prepareSearch();
        void waitForThreads(const size_t& drivesCount, const std::wstring& query);
        inline void notifyQueueReady();
        void __runSearch(std::wstring query, uint32_t id);
    };
}