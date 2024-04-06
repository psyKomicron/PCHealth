#include "pch.h"
#include "Searcher.hpp"

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

using namespace pchealth::windows::search;

Searcher::Searcher(const bool& includeRecycleBin, const CallbackType& callback)
    : BaseSearcher(callback)
{
    this->includeRecycleBin(includeRecycleBin);
}

std::vector<SearchResult> Searcher::search(const std::wstring& query, const std::vector<pchealth::filesystem::DriveInfo>& _drives)
{
    prepareSearch(query);

    searchRunning.store(true);

    std::vector<pchealth::filesystem::DriveInfo> drives = _drives.empty() 
        ? pchealth::filesystem::DriveInfo::getDrives()
        : _drives;

    auto fillQueueTask = concurrency::create_task([this, query, drives]()
    {
        try
        {
            concurrency::parallel_for_each(std::begin(drives), std::end(drives), [this, query](auto drive)
            {
                PRINT(L"[Searcher]  Building queue for '" + drive.name() + L"'.");
                
                auto files = fillQueue(drive.name(), query, fillQueueMaxDepth);
                callback(std::move(files), true);
                
                PRINT(std::format(L"[Searcher]  Queue built for '{}'.", drive.name()));
            });

            queueFillingEnded.store(true);

            PRINT(L"[Searcher]  Queue built for all drives.");
        }
        catch (pchealth::CancellationException)
        {
            if (!queueFillingEnded.load())
            {
                // Queue filling has been cancelled, to avoid any 'corruption' of the queue we just clear it for it to be rebuilt next time.
                while (!pathQueue.empty())
                {
                    std::ignore = pathQueue.front();
                    pathQueue.pop();
                }
            }
            PRINT(L"[Searcher]  Queue filling cancelled, unlocking threads to cancel them.");
        }

        if (!queueReady.test())
        {
            queueReady.test_and_set();
            queueReady.notify_all();
        }
    });

    auto applicationsTask = searchApplications(query);
    waitForThreads(drives.size(), query);

    PRINT(L"[Searcher]  Waiting for tasks...");
    fillQueueTask.get();
    applicationsTask.get();

    searchRunning.store(false);

    PRINT(L"[Searcher]  Search finished !");
    // TODO: Return aggregated search results.
    return {};
}


inline bool Searcher::consumePath(std::wstring& path)
{
    if (!pathQueue.empty())
    {
        path = pathQueue.front();
        pathQueue.pop();
        pathQueueCount.fetch_add(-1);
        return true;
    }
    return false;
}

void Searcher::performSearch(const std::filesystem::path& path, const boost::wregex& queryRe, std::vector<SearchResult>& results, const uint32_t& threadId)
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
            performSearch(entries[i].first, queryRe, results, threadId);
        }
    }
}

void Searcher::specificPrepareSearch()
{
    pathQueueCount.store(0);
    queueFillingEnded.store(false);
    queueReady.clear();
}

concurrency::task<void> Searcher::searchApplications(const std::wstring& query)
{
    return concurrency::create_task([this, query]()
    {
        PRINT(L"[Searcher]  Searching for apps matching the query...");
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
                OUTPUT_QUEUE_BUILD(std::format(L"[Searcher]  Navigating to '{}' (depth {})", entry.first.wstring(), depth));
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
                    pathQueueCount.fetch_add(1);
                }

                checkQueueReady();

                OUTPUT_QUEUE_BUILD(std::format(L"[Searcher]         Queued '{}'", entry.first.wstring()));
                callback({}, false);
            }
        }
        else 
        {
            bool match = matchPath(entry.first, re);
            OUTPUT_MATCH(std::format(L"[Searcher]  '{}'({}) is match (re: {}) ? {}", entry.first.filename().wstring(), entry.first.wstring(), re.str(), match));
            if (!entry.second && match)
            {
                // The current entry is a file, to not skip it we check if the file matches the query if so push it the the matches list.
                files.push_back(SearchResult(entry.first.wstring(), SearchResultKind::File));
            }
        }
    }

    return files;
}