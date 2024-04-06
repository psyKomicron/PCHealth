#include "pch.h"
#include "BaseSearcher.hpp"

#include "CancellationException.hpp"
#include "Directory.hpp"

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


using namespace pchealth::windows::search;


BaseSearcher::BaseSearcher(const CallbackType& callback)
{
    this->callback = callback;
}

uint64_t BaseSearcher::remaining() const
{
    return pathQueueCount.load();
}

bool BaseSearcher::includeRecycleBin() const
{
    return _includeRecycleBin;
}

void BaseSearcher::includeRecycleBin(const bool& value)
{
    if (searchRunning.load())
    {
        throw std::out_of_range("Cannot change recycle bin settings while the search is already running.");
    }

    _includeRecycleBin = value;
}

bool BaseSearcher::includeWindowsDirectory() const
{
    return _includeWindowsDirectory.load();
}

void BaseSearcher::includeWindowsDirectory(const bool& value)
{
    _includeWindowsDirectory.store(value);
}

bool BaseSearcher::searchFilePath() const
{
    return _matchFilePath.load();
}

void BaseSearcher::searchFilePath(const bool& value)
{
    _matchFilePath.store(value);
}

bool BaseSearcher::isSearchRunning() const
{
    return searchRunning.load();
}

inline bool BaseSearcher::isSearchCancelled() const
{
    return searchCancelled.load();
}

void BaseSearcher::cancelCurrentSearch()
{
    if (searchRunning.load())
    {
        searchCancelled.store(true);
        searchRunning.wait(false);
    }
}


void BaseSearcher::waitForThreads(const size_t& drivesCount, const std::wstring& query)
{
    PRINT(L"[BaseSearcher]  Waiting for search threads to end...");

    std::vector<std::thread> threads{ drivesCount * 2 };
    
    for (uint32_t i = 0; i < (drivesCount * 2); i++)
    {
        threads[i] = std::thread(&BaseSearcher::__runSearch, this, query, i);
    }

    for (size_t i = 0; i < threads.size(); i++)
    {
        threads[i].join();
        PRINT(std::format("[BaseSearcher]  Thread {} stopped.", i));
    }
}

void BaseSearcher::prepareSearch(const std::wstring& query)
{
    if (searchRunning.load())
    {
        // Cancel search ?
        PRINT(L"[BaseSearcher]  Search is already running, impossible to prepare new search.");
        throw std::runtime_error("Cancel search before starting a new one.");
    }

    PRINT(L"[BaseSearcher]  Clearing flags.");
    searchCancelled.store(false);

    PRINT(L"[BaseSearcher]  Clearing path queue count & queue filling flag.");

    // Try to create a boost::regex with the user input (query).
    try
    {
        std::ignore = boost::wregex(query);
    }
    catch (boost::regex_error)
    {
        PRINT("[BaseSearcher]  Regex error creating from query while preparing search.");
        throw;
    }

    specificPrepareSearch();
}


inline void BaseSearcher::getBackoff(uint32_t& backoff)
{
    double d = static_cast<double>(backoff);
    d = std::floor(d * std::log10(d));
    if (d < 2000)
    {
        backoff = static_cast<uint32_t>(d);
    }
}

void BaseSearcher::__runSearch(std::wstring query, uint32_t threadId)
{
    OUTPUT_THREAD(std::format(L"[BaseSearcher]  Thread {} waiting for queue to be ready.", threadId));
    queueReady.wait(false);
    OUTPUT_THREAD(std::format(L"[BaseSearcher]  Thread {} queue ready. Search starting.", threadId));

    uint_fast64_t bound = 0;
    uint32_t backoff = 66;
    auto wregex = boost::wregex(query, boost::regex_constants::icase);

    try
    {
        while ((++bound) < 500000 && !isSearchCancelled())
        {
            std::unique_lock<std::timed_mutex> lock{ threadPathListMutex, std::defer_lock };
            if (lock.try_lock_for(std::chrono::milliseconds(333)))
            {
                if (isSearchCancelled()) return;

                std::wstring path{};
                if (consumePath(path))
                {
                    backoff = 33;
                    lock.unlock();

                    std::vector<SearchResult> results{};
                    performSearch(std::filesystem::path(path), wregex, results, threadId);

                    callback({}, false);
                }
                else
                {
                    lock.unlock();

                    if (queueFillingEnded.load())
                    {
                        OUTPUT_THREAD(std::format(L"[BaseSearcher]  Thread {} path queue is empty.", threadId));
                        return;
                    }

                    getBackoff(backoff);

                    OUTPUT_THREAD(std::format(L"[BaseSearcher]  Thread {} sleeping for {}ms.", threadId, backoff));
                    Sleep((backoff / 2));
                    if (isSearchCancelled()) return;
                    Sleep((backoff / 2));
                }
            }
            else
            {
                Sleep(111);
            }
        }
    }
    catch (std::invalid_argument& invalidArgument)
    {
        OUTPUT_THREAD(std::format("[BaseSearcher]  {}> Invalid argument: {}", threadId, invalidArgument.what()));
    }
    catch (boost::regex_error err)
    {
        OUTPUT_THREAD(std::format("[BaseSearcher]  {}> Regex error (boost): {}", threadId, err.what()));
    }
    catch (pchealth::CancellationException ex)
    {
        OUTPUT_THREAD(std::format("[BaseSearcher]  Thread {} search cancelled.", threadId));
    }
    catch (...)
    {
        if (IsDebuggerPresent())
        {
            __debugbreak();
        }
    }

#ifdef _DEBUG
    if (bound == 500000 && IsDebuggerPresent())
    {
        __debugbreak();
    }
#endif // _DEBUG

    OUTPUT_THREAD(std::format(L"[BaseSearcher]  Thread {} exited.", threadId));
}
