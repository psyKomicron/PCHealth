#include "pch.h"
#include "Searcher.h"

#include "System.h"
#include "DriveInfo.h"
#include "Directory.h"
#include "utilities.h"

#include <winrt/Windows.System.Inventory.h>

#include <ppl.h>
#include <windows.h>
#include <cwctype>
#include <cwchar>
#include <cmath>

#define SHOW_OUTPUT

namespace pchealth::windows::search
{
    Searcher::Searcher(const bool& includeRecycleBin, const CallbackType& callback)
    {
        this->callback = callback;
        this->_includeRecycleBin = includeRecycleBin;
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

    int64_t Searcher::remaining() const
    {
        return pathQueueCount.load();
    }

    std::vector<SearchResult> Searcher::search(const std::wstring& _query, const std::vector<pchealth::filesystem::DriveInfo>& _drives)
    {
        prepareSearch();

        
        std::wstring query = LR"(\b)" + _query;
        size_t offset = 0;
        while ((offset = query.find(L" ", offset)) != std::string::npos)
        {
            query = query.replace(offset, 1, LR"(\b)");
        }
        if (offset == std::string::npos)
        {
            query += LR"(\b)";
        }

        std::vector<pchealth::filesystem::DriveInfo> drives{};
        if (_drives.empty())
        {
            drives = pchealth::filesystem::DriveInfo::GetDrives();
        }
        else
        {
            drives = _drives;
        }
        
        auto fillQueueTask = concurrency::create_task([this, query, drives]()
        {
            concurrency::parallel_for_each(std::begin(drives), std::end(drives), [this, query](auto drive)
            {
#ifdef SHOW_OUTPUT
                OutputDebug(L"Building queue for " + drive.name() + L"...");
#endif
                auto files = fillQueue(drive.name(), query, fillQueueMaxDepth);
                if (pathQueueCount.load() > 500 && !queueReady.test())
                {
                    notifyQueueReady();
                }
                callback(files, true);
            });

            queueFillingEnded.store(true);
            if (!queueReady.test())
            {
                notifyQueueReady();
            }
        });

        auto applicationsTask = searchApplications(query);
        waitForThreads(drives.size(), query);

#ifdef SHOW_OUTPUT
        OutputDebug(L"Waiting for tasks...");
#endif
        fillQueueTask.get();
        applicationsTask.get();

        searchRunning.store(false);
        queueReady.clear();

#ifdef SHOW_OUTPUT
        OutputDebug(L"Search finished !");
#endif
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


    std::vector<SearchResult> Searcher::fillQueue(const std::wstring& root, const std::wstring& query, const int32_t& maxDepth, int32_t depth)
    {
        const std::wregex binRe{ L"\\$RECYCLE\\.BIN", std::regex_constants::icase };
        const std::wregex re = std::wregex(query, std::regex_constants::icase);
        std::vector<SearchResult> files{};

        pchealth::filesystem::Directory rootDir{ root };
        auto&& entries = rootDir.enumerate();

        for (auto&& entry : entries)
        {
            if (entry.second)
            {
                if (_includeRecycleBin || !std::regex_search(entry.first.filename().wstring(), binRe))
                {
                    if (depth < maxDepth)
                    {
                        auto res = fillQueue(entry.first.wstring(), query, maxDepth, depth + 1);
                        files.resize(files.size() + res.size());
                        files.insert(files.end(), res.begin(), res.end());
                    }
                    else
                    {
                        {
                            std::unique_lock<std::timed_mutex> lock{ queueMutex };
                            pathQueue.push(entry.first.wstring());
                            pathQueueCount.store(pathQueue.size());
                        }

                        callback({}, false);
                    }
                }
            }
            else
            {
                if (std::regex_search(entry.first.filename().wstring(), re))
                {
                    files.push_back(SearchResult(entry.first.wstring(), SearchResultKind::File));
                }
            }
        }

        return files;
    }

    std::wstring Searcher::consumePath()
    {
        std::wstring path = pathQueue.front();
        pathQueue.pop();
        pathQueueCount.fetch_add(-1);
        return path;
    }

    void Searcher::getBackoff(uint32_t & backoff)
    {
        double d = static_cast<double>(backoff);
        d = std::floor(d * std::log10(d));
        if (d < 2000)
        {
            backoff = static_cast<uint32_t>(d);
        }
    }

    void Searcher::search(const std::filesystem::path& path, const std::wregex& queryRe, std::vector<SearchResult>& results, const uint32_t& threadId)
    {
        pchealth::filesystem::Directory dir{ path };
        auto entries = dir.enumerate();
        std::vector<SearchResult> toNotify{};
        // improve update using a view on the vector.
        for (size_t i = 0; i < entries.size(); i++)
        {
            checkIfSearchCancelled();
            auto&& filePath = entries[i].first;
            if (std::regex_search(filePath.filename().wstring(), queryRe))
            {
                SearchResult res{ entries[i].first.wstring(), entries[i].second ? SearchResultKind::Directory : SearchResultKind::File};
                results.push_back(res);
                toNotify.push_back(res);
            }
        }

        if (!toNotify.empty())
        {
            callback(toNotify, true);
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

    concurrency::task<void> Searcher::searchApplications(const std::wstring& query)
    {
        return concurrency::create_task([this, query]()
        {
            try
            {
#ifdef SHOW_OUTPUT
                OutputDebug(L"Searching for apps matching the query...");
#endif
                const std::wregex re = std::wregex(query);
                auto&& inventory = winrt::Windows::System::Inventory::InstalledDesktopApp::GetInventoryAsync().get();
                std::vector<SearchResult> results{};
                for (auto&& app : inventory)
                {
                    auto&& displayName = std::wstring(app.DisplayName());
                    if (std::regex_search(displayName, re))
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

    void Searcher::checkIfSearchCancelled() const
    {
        if (isSearchCancelled())
        {
            throw std::exception("Search cancelled");
        }
    }

    bool Searcher::isSearchCancelled() const
    {
        return searchCancelled.load();
    }

    void Searcher::prepareSearch()
    {
        if (searchRunning.load())
        {
            // Cancel search ?
            throw std::out_of_range("Cancel search before starting a new one.");
        }

        searchCancelled.store(false);
        searchRunning.store(true);
        pathQueueCount.store(0);

        if (!pathQueue.empty() && _rebuildQueue)
        {
            while (!pathQueue.empty())
            {
                //pathQueue.front();
                pathQueue.pop();
            }
        }

        if (queueReady.test())
        {
            queueReady.clear();
        }
    }

    void Searcher::waitForThreads(const size_t& drivesCount, const std::wstring& query)
    {
        std::vector<std::thread> threads{ drivesCount * 2 };

        for (uint32_t i = 0; i < (drivesCount * 2); i++)
        {
            threads[i] = std::thread(&Searcher::__runSearch, this, query, i);
        }

        for (size_t i = 0; i < threads.size(); i++)
        {
            threads[i].join();
        }
    }

    inline void Searcher::notifyQueueReady()
    {
        queueReady.test_and_set();
        queueReady.notify_one();
    }

    void Searcher::__runSearch(std::wstring query, uint32_t id)
    {
        queueReady.wait(true);

#ifdef SHOW_OUTPUT
        OutputDebug(std::format(L"Thread {} queue ready. Search starting.", id));
#endif // SHOW_OUTPUT

        uint_fast64_t bound = 0;
        uint32_t backoff = 33;
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
#ifdef SHOW_OUTPUT
                            OutputDebug(std::format(L"Thread {}: Pathes queue is empty.", id));
#endif // SHOW_OUTPUT
                            return;
                        }

                        getBackoff(backoff);
#ifdef SHOW_OUTPUT
                        OutputDebug(std::format(L"{}>> Sleeping for {}ms.", id, backoff));
#endif // SHOW_OUTPUT
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
                        search(std::filesystem::path(path), std::wregex(query, std::regex_constants::icase | std::regex_constants::basic), results, id);
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
            OutputDebug(std::format("{}>> Invalid argument: {}", std::to_string(id), invalidArgument.what()));
        }
        catch (std::regex_error reError)
        {
            __debugbreak();
        }
        catch (...)
        {
            __debugbreak();
        }

#ifdef _DEBUG
        if (bound == 500000) __debugbreak();
#endif // _DEBUG

#ifdef SHOW_OUTPUT
        OutputDebug(std::format(L"Thread {}: Exited.", id));
#endif // SHOW_OUTPUT
    }
}