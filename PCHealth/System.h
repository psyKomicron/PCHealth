#pragma once
#include "ICancellable.h"
#include "DriveInfo.h"
#include "LibraryPathes.h"

#include "DirectoryInfo.h"
#include "DisplayMonitor.h"

#include <string>
#include <vector>
#include <ppltasks.h>

namespace pchealth::windows
{
	class System
	{
	public:
		System() = default;

		static int GetGeneralHealth();
		static concurrency::task<pchealth::filesystem::LibraryPathes> GetLibraries();
		static std::wstring GetCurrentUserName();
		static uint64_t GetFileSize(const std::wstring& filePath);
		static bool removeHibernationFile();
		static bool pathExists(const std::wstring& path);
		static bool openExplorer(std::wstring args);

	private:
		static concurrency::task<std::vector<std::wstring>> GetLibraryFolders(winrt::Windows::System::User* user,
			const winrt::Windows::Storage::KnownFolderId& id);
	};
}
