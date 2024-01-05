#pragma once
#include "ICancellable.h"
#include "DriveInfo.h"
#include "LibraryPathes.h"

#include <string>
#include <vector>
#include <ppltasks.h>
#include <DirectoryInfo.h>

namespace Common
{
	class System
	{
	public:
		System() = default;

		static int GetGeneralHealth();
		static concurrency::task<Filesystem::LibraryPathes> GetLibraries();
		static std::vector<Filesystem::DirectoryInfo> EnumerateDirectories(const std::wstring& path);
		static std::wstring GetCurrentUserName();
		static uint64_t GetFileSize(const std::wstring& filePath);
		static bool RemoveHibernationFile();
		static bool PathExists(const std::wstring& path);

	private:
		static concurrency::task<std::vector<std::wstring>> GetLibraryFolders(winrt::Windows::System::User* user,
			const winrt::Windows::Storage::KnownFolderId& id);
	};
}
