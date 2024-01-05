#include "pch.h"
#include "DirectorySizeCalculator.h"

#include <ppl.h>
#include <shlwapi.h>

namespace Foundation = winrt::Windows::Foundation;

namespace Common::Filesystem
{
    const int ALTERNATE_MAX_PATH = 2048;

    uint_fast64_t DirectorySizeCalculator::GetSize(std::wstring const& path, const bool& parallelize)
    {
        if (path.starts_with(L"\\")) // Prevents infinite looping when the directory name is invalid (often buffer too small)
        {
            return 0;
        }

        uint_fast64_t dirSize = 0;
        WIN32_FIND_DATA findData{};
        HANDLE findHandle = nullptr;
        if (!path.ends_with(L"\\"))
        {
            findHandle = FindFirstFile((path + L"\\*").c_str(), &findData);
        }
        else
        {
            findHandle = FindFirstFile((path + L"*").c_str(), &findData);
        }

        if (findHandle != INVALID_HANDLE_VALUE)
        {
            std::vector<std::wstring> pathes = std::vector<std::wstring>();
            do
            {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    uint_fast64_t fileSize = (static_cast<int64_t>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow;
                    dirSize += fileSize;
                    RaiseProgress(fileSize);
                }
                else
                {
                    std::wstring filePath = std::wstring(std::move(findData.cFileName));
                    if (filePath != L"." && filePath != L"..")
                    {
                        pathes.push_back(filePath);
                    }
                }
            } while (FindNextFile(findHandle, &findData));
            FindClose(findHandle);

            std::atomic_uint_fast64_t atomicDirSize{};
            if (parallelize && pathes.size() > 1)
            {
                concurrency::parallel_for_each(begin(pathes), end(pathes), [this, wstr = path, &atomicDirSize, &parallelize](const std::wstring& dir)
                {
                    WCHAR combinedPath[ALTERNATE_MAX_PATH]{};
                    PathCombine(combinedPath, wstr.c_str(), dir.c_str());
                    std::wstring deepPath = std::wstring(combinedPath) + L"\\";

                    uint_fast64_t size = GetSize(deepPath, parallelize);
                    atomicDirSize.fetch_add(size);
                });
                dirSize += atomicDirSize.load();
            }
            else
            {
                size_t max = pathes.size();
                for (size_t i = 0; i < max; i++)
                {
                    WCHAR combinedPath[ALTERNATE_MAX_PATH];
                    PathCombine(combinedPath, path.c_str(), pathes[i].c_str());
                    std::wstring deepPath = std::wstring(combinedPath) + L"\\";

                    uint_fast64_t size = GetSize(deepPath, parallelize);
                    dirSize += size;
                }
            }
        }

        return dirSize;
    }

    inline void DirectorySizeCalculator::RaiseProgress(uint_fast64_t newSize)
    {
        m_event(nullptr, Foundation::IReference{ newSize });
    }
}