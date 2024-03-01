#pragma once

#include "SearchResult.h"

#include <functional>
#include <vector>
#include <mutex>
#include <queue>
#include <string>
#include <filesystem>
#include <chrono>
#include <atomic>
#include <regex>

using CallbackType = std::function<void(const std::vector<pchealth::windows::search::SearchResult>&, const bool&)>;

namespace pchealth::windows::search
{
    class Searcher
    {
    public:
        Searcher(const std::chrono::milliseconds& notificationDelay, const bool& includeRecycleBin, const CallbackType& callback);

        bool includeRecycleBin() const;
        void includeRecycleBin(const bool& value);
        int64_t remaining() const;

        std::vector<pchealth::windows::search::SearchResult> search(const std::wstring& query);

    private:
        CallbackType callback{};
        //
        std::chrono::milliseconds notificationDelay;
        std::thread notificationThread{};
        std::vector<std::thread> searchThreads{};
        std::timed_mutex queueMutex{};
        std::queue<std::wstring> pathQueue{};
        std::atomic<uint64_t> searchCount = 0;
        std::atomic_bool searchRunning = false;
        bool _includeRecycleBin = false;
        int32_t fillQueueMaxDepth = 4;
        std::atomic_int64_t pathQueueCount{};

        std::vector<pchealth::windows::search::SearchResult> fillQueue(const std::wstring& root, const std::wstring& query, const int32_t& maxDepth, int32_t depth = 1);
        std::wstring consumePath();
        void search(const std::filesystem::path& path, const std::wregex& queryRe, std::vector<pchealth::windows::search::SearchResult>& results, const uint32_t& threadId);
        void __runSearch(std::wstring query, uint32_t id);
    };
}