#pragma once

#include "BaseSearcher.hpp"

#include <boost/filesystem.hpp>

#include <thread>
#include <ppltasks.h>


namespace pchealth::windows::search
{
    class CachedSearcher : public BaseSearcher
    {
    public:
        CachedSearcher();
        CachedSearcher(const CallbackType& callback);

        virtual std::vector<SearchResult> search(const std::wstring& query, const std::vector<pchealth::filesystem::DriveInfo>& drives = {});


    protected:
        virtual bool consumePath(std::wstring& path);
        
        virtual void specificPrepareSearch();

        virtual void performSearch(const std::filesystem::path& path, const boost::wregex& re, std::vector<SearchResult>& results, const uint32_t& threadId);


    private:
        std::atomic_uint64_t pathListIndex{};
        //std::vector<std::pair<std::wstring, int32_t>> dirsPathList{};
        std::timed_mutex filesListMutex{};
        std::timed_mutex foldersListMutex{};
        std::vector<std::wstring> filesList{};
        std::vector<std::wstring> foldersList{};

        std::thread buildPathListThread{};


        void buildPathList();
        
        void dive(const std::wstring& path, const uint32_t& maxDepth, uint32_t currentDepth, const bool& treatOnlyFiles);
    };
}
