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

namespace pchealth::windows::search
{
    Searcher::Searcher(const std::chrono::milliseconds& notificationDelay, const bool& includeRecycleBin, const CallbackType& callback)
    {
        this->callback = callback;
        this->notificationDelay = notificationDelay;
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

    std::vector<SearchResult> Searcher::search(const std::wstring& query)
    {
        std::vector<pchealth::filesystem::DriveInfo> drives = pchealth::filesystem::DriveInfo::GetDrives();

        searchRunning.store(true);
        pathQueueCount = 0;
        concurrency::parallel_for_each(std::begin(drives), std::end(drives), [this, query](auto drive)
        {
            auto files = fillQueue(std::wstring(drive.name()), query, fillQueueMaxDepth);
            callback(files, true);
        });

        auto task = concurrency::create_task([this, query]()
        {
            const std::wregex re = std::wregex(query, std::regex_constants::icase);

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
        });

        for (size_t i = 0; i < (drives.size() * 2); i++)
        {
            searchThreads.push_back(std::thread(&Searcher::__runSearch, this, query, i));
        }

        for (size_t i = 0; i < searchThreads.size(); i++)
        {
            searchThreads[i].join();
            OutputDebug(std::format(L"Thread n.{} finished.", i));
        }

        searchRunning.store(false);
        OutputDebug(L"Searcher finished searching.");
        searchThreads.clear();

        // TODO: Return aggregated search results.
        return {};
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
                    files.push_back(SearchResult(entry.first.filename().wstring(), SearchResultKind::File));
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

    void Searcher::search(const std::filesystem::path& path, const std::wregex& queryRe, std::vector<SearchResult>& results, const uint32_t& threadId)
    {
        pchealth::filesystem::Directory dir{ path };
        auto entries = dir.enumerate();

        std::vector<SearchResult> toNotify{};

        // improve update using a view on the vector.
        for (size_t i = 0; i < entries.size(); i++)
        {
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
            if (entries[i].second)
            {
                search(entries[i].first, queryRe, results, threadId);
            }
        }
    }

    void Searcher::__runSearch(std::wstring query, uint32_t id)
    {
        std::vector<std::pair<std::filesystem::path, bool>> pathes{};
        uint_fast64_t bound = 0;

        try
        {
            while ((++bound) < 500000)
            {
                auto clock = std::chrono::high_resolution_clock::now();

                std::unique_lock<std::timed_mutex> lock{ queueMutex, std::defer_lock };
                if (lock.try_lock_for(std::chrono::milliseconds(333)))
                {
                    if (pathQueue.empty())
                    {
                        return;
                    }
                    std::wstring path = consumePath();
                    lock.unlock();

                    std::vector<SearchResult> results{};
                    search(std::filesystem::path(path), std::wregex(query, std::regex_constants::icase | std::regex_constants::basic), results, id);
                    callback({}, false);
                    //if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - clock) < notificationDelay)
                    //{
                    //    // notify user about the new results;
                    //    callback(finds, true);
                    //}
                    //else
                    //{
                    //    // add results to a collection.
                    //    pathes.insert(pathes.end(), finds.begin(), finds.end());
                    //}
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

#ifdef _DEBUG
        if (bound == 500000) __debugbreak();
#endif // _DEBUG

        //callback(pathes, true);
        OutputDebug(std::format(L"Thread {} exited.", id));
    }
}