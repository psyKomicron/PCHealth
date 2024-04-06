#pragma once

#include "SearchResult.h"
#include "DriveInfo.h"
#include "CancellationException.hpp"
#include "utilities.h"

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

#define SHOW_OUTPUT
//#define SHOW_QUEUE_BUILD
//#define SHOW_MATCH
//#define SHOW_QUEUE_CONSUMPTION
#define SHOW_THREAD_OUTPUT

#pragma region Macros
#ifdef SHOW_OUTPUT
#define PRINT(s) OutputDebug(s)
#else
#define PRINT(s)
#endif

#ifdef SHOW_QUEUE_BUILD
#define OUTPUT_QUEUE_BUILD(s) OutputDebug(s)
#else
#define OUTPUT_QUEUE_BUILD(s)
#endif

#ifdef SHOW_MATCH
#define OUTPUT_MATCH(s) OutputDebug(s)
#else
#define OUTPUT_MATCH(s)
#endif

#ifdef SHOW_THREAD_OUTPUT
#define OUTPUT_THREAD(s) OutputDebug(s)
#else
#define OUTPUT_THREAD(s)
#endif

#ifdef SHOW_QUEUE_CONSUMPTION
#define OUTPUT_QUEUE_CONSUMPTION(s) OutputDebug(s)
#else
#define OUTPUT_QUEUE_CONSUMPTION(s)
#endif
#pragma endregion

using CallbackType = std::function<void(std::vector<pchealth::windows::search::SearchResult>, const bool&)>;

namespace pchealth::windows::search
{
    class BaseSearcher
    {
    public:
        virtual ~BaseSearcher() = default;


        virtual std::vector<SearchResult> search(const std::wstring& query, const std::vector<pchealth::filesystem::DriveInfo>& drives = {}) = 0;


        uint64_t remaining() const;

        bool includeRecycleBin() const;
        
        void includeRecycleBin(const bool& value);

        bool includeWindowsDirectory() const;
        
        void includeWindowsDirectory(const bool& value);

        bool searchFilePath() const;

        void searchFilePath(const bool& value);

        bool isSearchRunning() const;

        inline bool isSearchCancelled() const;

        void cancelCurrentSearch();


    protected:
        std::atomic_bool searchRunning = false;
        std::atomic_bool queueFillingEnded = false;
        std::atomic_flag queueReady{};
        CallbackType callback{};
        std::atomic_uint64_t pathQueueCount = 0;


        BaseSearcher() = default;

        BaseSearcher(const CallbackType& callback);

        
        virtual inline bool consumePath(std::wstring& path) = 0;

        virtual void performSearch(const std::filesystem::path& path, const boost::wregex& queryRe, std::vector<SearchResult>& results, const uint32_t& threadId) = 0;

        virtual void specificPrepareSearch() { };


        void prepareSearch(const std::wstring& query);
        
        void waitForThreads(const size_t& drivesCount, const std::wstring& query);


        inline boost::wregex createRegexOnQuery(const std::wstring& query)
        {
            return boost::wregex(query, boost::regex_constants::icase);
        }
        
        inline void checkIfSearchCancelled() const
        {
            if (isSearchCancelled())
            {
                throw pchealth::CancellationException();
            }
        }
        
        inline bool canQueuePath(const std::filesystem::path& path)
        {
            auto filename = path.wstring();

            bool isRecycleBin = boost::regex_search(filename, recycleBinRe);
            bool isWinDir = boost::regex_search(filename, windowsDirRe);

            return (_includeRecycleBin || !isRecycleBin)
                && (_includeWindowsDirectory || !isWinDir);
        }

        inline void checkQueueReady()
        {
            if (pathQueueCount.load() > 500 && !queueReady.test())
            {
                queueReady.test_and_set();
                queueReady.notify_all();

                PRINT(L"[BaseSearcher]  Queue ready.");
            }
        }

        inline bool matchPath(const std::filesystem::path& path, const boost::wregex& regex) const
        {
            if (_matchFilePath)
            {
                bool search = boost::regex_search(path.wstring(), regex);
                OUTPUT_MATCH(
                    std::format(L"[BaseSearcher]  /{}/ '{}' : {}",
                        path.wstring(),
                        regex.str(),
                        (search ? L"match" : L"no match"))
                );
                return search;
            }
            else
            {
                bool search = boost::regex_search(path.filename().wstring(), regex);
                OUTPUT_MATCH(
                    std::format(L"[BaseSearcher]  /{}/ '{}' : {}",
                        path.filename().wstring(),
                        regex.str(),
                        (search ? L"match" : L"no match"))
                );
                return search;
            }
        }


    private:
        const boost::wregex recycleBinRe{ LR"([A-Z]+:\\\$RECYCLE\.BIN.*)", boost::regex_constants::icase };
        const boost::wregex windowsDirRe{ LR"(^C:\\WINDOWS.*)", boost::regex_constants::icase };
        std::atomic_bool searchCancelled = false;
        std::atomic_bool _includeWindowsDirectory = false; // Does it need to be an atomic ? If yes then make _includeRecycleBinSize atomic too.
        std::atomic_bool _matchFilePath = false;
        bool _includeRecycleBin = false;
        std::timed_mutex threadPathListMutex{};


        inline void getBackoff(uint32_t& backoff);

        void __runSearch(std::wstring query, uint32_t threadId);
    };
}

//#pragma region Macros
//#undef PRINT
//#undef OUTPUT_QUEUE_BUILD
//#undef OUTPUT_MATCH
//#undef OUTPUT_THREAD
//#undef OUTPUT_QUEUE_CONSUMPTION
//#pragma endregion
