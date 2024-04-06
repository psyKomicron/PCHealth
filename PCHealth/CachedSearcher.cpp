#include "pch.h"
#include "CachedSearcher.hpp"

#include "Directory.hpp"
#include "DriveInfo.h"
#include "utilities.h"

#include <stack>

using namespace pchealth::windows::search;

/*
* # TODO
* - Drive affinity per thread.
* - Travel through the system tree, list files and directories and keep them in memory.
* - Path heat map, pathes/files that rarely yield results should be removed from the memory.
* - Keep start up performance in check. Use the model of Searcher to build the cache while searching but keep it in memory.
* Good luck me.
*/

CachedSearcher::CachedSearcher() :
    BaseSearcher([](auto, auto){})
{
    buildPathListThread = std::thread(&CachedSearcher::buildPathList, this);
    buildPathListThread.detach();
}

CachedSearcher::CachedSearcher(const CallbackType& callback) :
    BaseSearcher(callback)
{
    buildPathListThread = std::thread(&CachedSearcher::buildPathList, this);
    buildPathListThread.detach();
}

std::vector<SearchResult> CachedSearcher::search(const std::wstring& query, const std::vector<pchealth::filesystem::DriveInfo>& _drives)
{
    prepareSearch(query);

    searchRunning.store(true);

    std::vector<pchealth::filesystem::DriveInfo> drives = _drives.empty() 
        ? pchealth::filesystem::DriveInfo::getDrives()
        : _drives;

    waitForThreads(2, query);

    searchRunning.store(false);
    pathListIndex.store(0);

    return std::vector<SearchResult>();
}

bool CachedSearcher::consumePath(std::wstring& path)
{
    pathQueueCount.fetch_add(-1);

    auto index = pathListIndex.fetch_add(1);
    if (index < filesList.size())
    {
        path = filesList[index];
    }

    return index < filesList.size();
}

void CachedSearcher::specificPrepareSearch()
{
    if (!queueFillingEnded.load())
    {
        OUTPUT_DEBUG(L"[CachedSearcher]  Cache hasn't been built yet, waiting.");
        queueReady.wait(false);
        OUTPUT_DEBUG(L"[CachedSearcher]  Finished building cache, search can resume.");
    }
}

void CachedSearcher::performSearch(const std::filesystem::path& path, const boost::wregex& re, std::vector<SearchResult>& results, const uint32_t& threadId)
{
    if (matchPath(path, re))
    {
        auto result = SearchResult(path.wstring(), SearchResultKind::File);
        results.push_back(result);
        callback({ result }, true);
    }

    if (filesList.size() < pathListIndex.load())
    {
        OUTPUT_DEBUG(L"[CachedSearcher]  Path queue is empty.");
    }
}

void CachedSearcher::buildPathList()
{
    auto drives = pchealth::filesystem::DriveInfo::getDrives();

    std::vector<std::thread> threads{};

    for (size_t i = 0; i < drives.size(); i++)
    {
        threads.push_back(std::thread(&CachedSearcher::dive, this, drives[i].name(), 100, 0, false));
        threads.push_back(std::thread(&CachedSearcher::dive, this, drives[i].name(), 100, 0, true));
    }
   
    for (size_t i = 0; i < threads.size(); i++)
    {
        threads[i].join();
    }

    pathQueueCount.store(filesList.size());

    queueFillingEnded.store(true);
    queueReady.test_and_set();
    queueReady.notify_all();
}

void CachedSearcher::dive(const std::wstring& rootPath, const uint32_t& maxDepth, uint32_t currentDepth, const bool& treatOnlyFiles)
{
    std::stack<std::filesystem::path> stack{};
    stack.push(std::filesystem::path(rootPath));

    std::vector<std::pair<std::wstring, bool>> entries{};
    std::vector<std::filesystem::path> files{};

    uint32_t upperBound = 0;
    while ((upperBound++) <= 5000 && !stack.empty())
    {
        // unstack path.
        std::filesystem::path path = stack.top();
        stack.pop();

        std::ignore = pchealth::filesystem::Directory::enumerateDirectory(path, entries);
        for (auto&& entry : entries)
        {
            if (entry.second)
            {
                stack.push(entry.first);

                if (!treatOnlyFiles)
                {
                    files.push_back(entry.first);
                }
            }
            else if (treatOnlyFiles)
            {
                files.push_back(entry.first);
            }
        }

        entries.clear();
    }

    std::unique_lock<std::timed_mutex> lock{ treatOnlyFiles ? filesListMutex : foldersListMutex };
    for (size_t i = 0; i < files.size(); i++)
    {
        if (treatOnlyFiles)
        {
            filesList.push_back(files[i]);
        }
        else
        {
            foldersList.push_back(files[i]);
        }
    }

    OUTPUT_DEBUG(std::format(L"[CachedSearcher]  <{}> Cached {} {}.", rootPath, files.size(), (treatOnlyFiles ? L"files" : L"folders")));
}
