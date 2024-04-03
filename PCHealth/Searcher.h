#pragma once

#include "SearchResult.h"
#include "DriveInfo.h"

#include <boost/regex.hpp>

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

using CallbackType = std::function<void(std::vector<pchealth::windows::search::SearchResult>, const bool&)>;

namespace pchealth::windows::search
{
    class Searcher
    {
    public:
        Searcher(const bool& includeRecycleBin, const CallbackType& callback);

        int64_t remaining() const;

        bool includeRecycleBin() const;
        void includeRecycleBin(const bool& value);

        bool includeWindowsDirectory() const;
        void includeWindowsDirectory(const bool& value);

        bool matchFilePath() const;
        void matchFilePath(const bool& value);

        bool isSearchRunning() const;

        inline bool isSearchCancelled() const;

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
        std::atomic_bool _includeWindowsDirectory = false;
        bool _rebuildQueue = true;
        boost::wregex recycleBinRe{ LR"([A-Z]+:\\\$RECYCLE\.BIN.*)", boost::regex_constants::icase };
        boost::wregex windowsDirRe{ LR"(^C:\\WINDOWS.*)", boost::regex_constants::icase };
        bool _matchFilePath = false;


        inline std::wstring consumePath();

        inline void getBackoff(uint32_t& backoff);

        inline void checkQueueReady();

        inline bool canQueuePath(const std::filesystem::path& path);

        inline void checkIfSearchCancelled() const;

        inline bool matchPath(const std::filesystem::path& path, const boost::wregex& regex) const;

        inline boost::wregex createRegexOnQuery(const std::wstring& query);

        void prepareSearch(const std::wstring& query);

        void waitForThreads(const size_t& drivesCount, const std::wstring& query);

        concurrency::task<void> searchApplications(const std::wstring& query);

        std::vector<pchealth::windows::search::SearchResult> fillQueue(const std::wstring& root, const std::wstring& query, const int32_t& maxDepth, int32_t depth = 1);
        
        void search(const std::filesystem::path& path, const boost::wregex& queryRe, std::vector<pchealth::windows::search::SearchResult>& results, const uint32_t& threadId);
        

        void __runSearch(std::wstring query, uint32_t id);
    };
}