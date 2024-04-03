#include "pch.h"
#include "Searcher.h"

#include "System.h"
#include "DriveInfo.h"
#include "Directory.hpp"
#include "utilities.h"
#include "CancellationException.hpp"

#include <winrt/Windows.System.Inventory.h>

#include <boost/filesystem/path.hpp>

#include <ppl.h>
#include <windows.h>
#include <cwctype>
#include <cwchar>
#include <cmath>

//#define SHOW_OUTPUT
//#define SHOW_QUEUE_BUILD
//#define SHOW_MATCH

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


using namespace pchealth::windows::search;

Searcher::Searcher(const bool& includeRecycleBin, const CallbackType& callback)
{
    this->callback = callback;
    this->_includeRecycleBin = includeRecycleBin;
}

int64_t Searcher::remaining() const
{
    return pathQueueCount.load();
}

bool Searcher::includeRecycleBin() const
{
    return _includeRecycleBin;
}

void Searcher::includeRecycleBin(const bool& value)
{
    if (searchRunning)
    {
        throw std::out_of_range("Cannot change recycle bin settings while the search is already running.");
    }

    _includeRecycleBin = value;
}

void Searcher::includeWindowsDirectory(const bool& value)
{
    _includeWindowsDirectory.store(value);
}

bool pchealth::windows::search::Searcher::matchFilePath() const
{
    return _matchFilePath;
}

void pchealth::windows::search::Searcher::matchFilePath(const bool& value)
{
    _matchFilePath = value;
}

bool pchealth::windows::search::Searcher::isSearchRunning() const
{
    return searchRunning.load();
}

inline bool Searcher::isSearchCancelled() const
{
    return searchCancelled.load();
}

bool Searcher::includeWindowsDirectory() const
{
    return _includeWindowsDirectory.load();
}


std::vector<SearchResult> Searcher::search(const std::wstring& query, const std::vector<pchealth::filesystem::DriveInfo>& _drives)
{
    prepareSearch(query);

    std::vector<pchealth::filesystem::DriveInfo> drives = _drives.empty() 
        ? pchealth::filesystem::DriveInfo::getDrives()
        : _drives;

    auto fillQueueTask = concurrency::create_task([this, query, drives]()
    {
        try
        {
            concurrency::parallel_for_each(std::begin(drives), std::end(drives), [this, query](auto drive)
            {
                PRINT(L"[Searcher] Building queue for '" + drive.name() + L"'.");
                auto files = fillQueue(drive.name(), query, fillQueueMaxDepth);
                callback(std::move(files), true);
            });

            queueFillingEnded.store(true);
            if (!queueReady.test())
            {
                queueReady.test_and_set();
                queueReady.notify_all();
                PRINT(std::format(L"[Searcher] Queue filling ended ({} path(s)).", pathQueueCount.load()));
            }
        }
        catch (pchealth::CancellationException)
        {
            PRINT(L"[Searcher] Queue filling cancelled, unlocking threads to cancel them.");
            if (!queueReady.test())
            {
                queueReady.test_and_set();
                queueReady.notify_all();
            }
        }
    });

    auto applicationsTask = searchApplications(query);
    waitForThreads(drives.size(), query);

    PRINT(L"[Searcher] Waiting for tasks...");
    fillQueueTask.get();
    applicationsTask.get();

    searchRunning.store(false);
    queueReady.clear();

    PRINT(L"[Searcher] Search finished !");
    // TODO: Return aggregated search results.
    return {};
}

void Searcher::cancelCurrentSearch()
{
    if (searchRunning.load())
    {
        searchCancelled.store(true);
        searchRunning.wait(false);
    }
}


inline std::wstring Searcher::consumePath()
{
    std::wstring path = pathQueue.front();
    pathQueue.pop();
    pathQueueCount.fetch_add(-1);
    return path;
}

inline void Searcher::getBackoff(uint32_t& backoff)
{
    double d = static_cast<double>(backoff);
    d = std::floor(d * std::log10(d));
    if (d < 2000)
    {
        backoff = static_cast<uint32_t>(d);
    }
}

inline void Searcher::checkQueueReady()
{
    if (pathQueueCount.load() > 500 && !queueReady.test())
    {
        queueReady.test_and_set();
        queueReady.notify_all();

        OUTPUT_DEBUG(L"[Searcher] Queue ready.");
    }
}

inline bool Searcher::canQueuePath(const std::filesystem::path& path)
{
    auto&& filename = path.wstring();

    bool isRecycleBin = boost::regex_search(filename, recycleBinRe);
    bool isWinDir = boost::regex_search(filename, windowsDirRe);

    return (_includeRecycleBin || !isRecycleBin)
        && (_includeWindowsDirectory || !isWinDir);
}

inline void Searcher::checkIfSearchCancelled() const
{
    if (isSearchCancelled())
    {
        throw pchealth::CancellationException();
    }
}

inline bool Searcher::matchPath(const std::filesystem::path& path, const boost::wregex& regex) const
{
    if (_matchFilePath)
    {
        bool search = boost::regex_search(path.wstring(), regex);
        OUTPUT_MATCH(
            std::format(L"[Searcher] /{}/ '{}' : {}",
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
            std::format(L"[Searcher] /{}/ '{}' : {}",
                path.filename().wstring(),
                regex.str(),
                (search ? L"match" : L"no match"))
        );
        return search;
    }
}

inline boost::wregex pchealth::windows::search::Searcher::createRegexOnQuery(const std::wstring& query)
{
    return boost::wregex(query, boost::regex_constants::icase);
}

void Searcher::prepareSearch(const std::wstring& query)
{
    if (searchRunning.load())
    {
        // Cancel search ?
        PRINT(L"[Searcher] Search is already running, impossible to prepare new search.");
        throw std::out_of_range("Cancel search before starting a new one.");
    }

    PRINT(L"[Searcher] Clearing flags.");
    searchCancelled.store(false);
    searchRunning.store(true);
    pathQueueCount.store(0);
    queueFillingEnded.store(false);

    /*if (!pathQueue.empty() && _rebuildQueue)
    {
        PRINT(L"[Searcher] Rebuild queue is on, clearing queue.");
        while (!pathQueue.empty())
        {
            pathQueue.pop();
        }
    }*/

    PRINT(L"[Searcher] Clearing queue ready flag.");
    if (queueReady.test())
    {
        queueReady.clear();
    }

    // Try to create a boost::regex with the user input (query).
    try
    {
        std::ignore = boost::wregex(query);
    }
    catch (boost::regex_error)
    {
        searchRunning.store(false);
        PRINT("[Searcher] Regex error creating from query while preparing search.");
        throw;
    }
}

void Searcher::waitForThreads(const size_t& drivesCount, const std::wstring& query)
{
    PRINT(L"[Searcher] Waiting for search threads to end...");

    std::vector<std::thread> threads{ drivesCount * 2 };
    for (uint32_t i = 0; i < (drivesCount * 2); i++)
    {
        threads[i] = std::thread(&Searcher::__runSearch, this, query, i);
    }
    for (size_t i = 0; i < threads.size(); i++)
    {
        threads[i].join();
        PRINT(std::format("[Searcher] Thread {} stopped.", i));
    }
}

concurrency::task<void> Searcher::searchApplications(const std::wstring& query)
{
    return concurrency::create_task([this, query]()
    {
        PRINT(L"[Searcher] Searching for apps matching the query...");
        try
        {
            auto re = boost::wregex(query);
            auto&& inventory = winrt::Windows::System::Inventory::InstalledDesktopApp::GetInventoryAsync().get();
            std::vector<SearchResult> results{};
            for (auto&& app : inventory)
            {
                auto&& displayName = std::wstring(app.DisplayName());
                if (boost::regex_search(displayName, re))
                {
                    results.push_back(SearchResult(displayName, SearchResultKind::Application));
                }
            }
            callback(results, true);
        }
        catch (std::system_error err)
        {
            __debugbreak();
        }
    });
}

std::vector<SearchResult> Searcher::fillQueue(const std::wstring& root, const std::wstring& query, const int32_t& maxDepth, int32_t depth)
{
    const boost::wregex re = createRegexOnQuery(query);
    std::vector<SearchResult> files{};

    pchealth::filesystem::Directory rootDir{ root };
    auto&& entries = rootDir.enumerate();
    for (auto&& entry : entries)
    {
        checkIfSearchCancelled();

        if (entry.second && canQueuePath(entry.first))
        {
            if (depth < maxDepth)
            {
                // Current depth is less than the max depth, we can go 1 path deeper and fill the path queue.
                OUTPUT_QUEUE_BUILD(std::format(L"[Searcher] Navigating to '{}' (depth {})", entry.first.wstring(), depth));
                auto res = fillQueue(entry.first.wstring(), query, maxDepth, depth + 1);
                if (res.size() > 0)
                {
                    files.insert(files.end(), res.begin(), res.end());
                }
            }
            else
            {
                // The max depth is reached, we can't go 1 path deeper so we queue the current path to the path queue.
                {
                    std::unique_lock<std::timed_mutex> lock{ queueMutex };
                    pathQueue.push(entry.first.wstring());
                    pathQueueCount.store(pathQueue.size());
                }

                checkQueueReady();

                OUTPUT_QUEUE_BUILD(std::format(L"[Searcher] Queued '{}'", entry.first.wstring()));
                callback({}, false);
            }
        }
        else 
        {
            bool match = matchPath(entry.first, re);
            OUTPUT_QUEUE_BUILD(std::format(L"[Searcher] '{}'({}) is match (re: {}) ? {}", entry.first.filename().wstring(), entry.first.wstring(), re.str(), match));
            if (!entry.second && match)
            {
                // The current entry is a file, to not skip it we check if the file matches the query if so push it the the matches list.
                files.push_back(SearchResult(entry.first.wstring(), SearchResultKind::File));
            }
        }
    }

    return files;
}

void Searcher::search(const std::filesystem::path& path, const boost::wregex& queryRe, std::vector<SearchResult>& results, const uint32_t& threadId)
{
    pchealth::filesystem::Directory dir{ path };
    auto entries = dir.enumerate();

    std::vector<SearchResult> toNotify{}; // Use a view on results instead of copying data to 'toNotify'.
    for (size_t i = 0; i < entries.size(); i++)
    {
        checkIfSearchCancelled();

        auto&& filePath = entries[i].first;
        if (matchPath(filePath, queryRe))
        {
            SearchResult res{ entries[i].first.wstring(), entries[i].second ? SearchResultKind::Directory : SearchResultKind::File};
            results.push_back(res);
            toNotify.push_back(res);
        }
    }

    if (!toNotify.empty())
    {
        callback(std::move(toNotify), true);
    }

    for (size_t i = 0; i < entries.size(); i++)
    {
        checkIfSearchCancelled();
        if (entries[i].second)
        {
            search(entries[i].first, queryRe, results, threadId);
        }
    }
}


void Searcher::__runSearch(std::wstring query, uint32_t id)
{
    OUTPUT_THREAD(std::format(L"[Searcher] Thread {} waiting for queue to be ready.", id));
    queueReady.wait(false);
    OUTPUT_THREAD(std::format(L"[Searcher] Thread {} queue ready. Search starting.", id));

    uint_fast64_t bound = 0;
    uint32_t backoff = 66;
    auto wregex = boost::wregex(query, boost::regex_constants::icase);
    
    try
    {
        while ((++bound) < 500000 && !isSearchCancelled())
        {
            std::unique_lock<std::timed_mutex> lock{ queueMutex, std::defer_lock };
            if (lock.try_lock_for(std::chrono::milliseconds(333)))
            {
                if (isSearchCancelled()) return;

                if (pathQueue.empty())
                {
                    lock.unlock();

                    if (queueFillingEnded.load())
                    {
                        OUTPUT_THREAD(std::format(L"[Searcher] Thread {}: path queue is empty.", id));
                        return;
                    }

                    getBackoff(backoff);

                    OUTPUT_THREAD(std::format(L"[Searcher] Thread {}: sleeping for {}ms.", id, backoff));
                    Sleep((backoff / 2));
                    if (isSearchCancelled()) return;
                    Sleep((backoff / 2));
                }
                else
                {
                    backoff = 33;
                    std::wstring path = consumePath();
                    lock.unlock();

                    std::vector<SearchResult> results{};
                    search(std::filesystem::path(path), wregex, results, id);
                    callback({}, false);
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
        OUTPUT_THREAD(std::format("[Searcher] {}> Invalid argument: {}", id, invalidArgument.what()));
    }
    catch (boost::regex_error err)
    {
        OUTPUT_THREAD(std::format("[Searcher] {}> Regex error (boost): {}", id, err.what()));
    }
    catch (pchealth::CancellationException ex)
    {
        OUTPUT_THREAD(std::format("[Searcher] Thread {}: search cancelled.", id));
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

    OUTPUT_THREAD(std::format(L"[Searcher] Thread {}: Exited.", id));
}