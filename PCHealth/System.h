#pragma once
#include "ICancellable.h"
#include "DriveInfo.h"
#include "LibraryPathes.h"
#include "DirectoryInfo.h"
#include "DisplayMonitor.hpp"

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
		
		static uint64_t getFileSize(const std::wstring& filePath);
		
		static bool pathExists(const std::wstring& path);
		
		static void openExplorer(std::wstring args);

		/**
		 * @brief Creates a process with the supplied path.
		 * @param path Path to execute.
		 * @throws winrt::hresult
		 */
		static void launch(const std::wstring path);

	private:
		static concurrency::task<std::vector<std::wstring>> GetLibraryFolders(winrt::Windows::System::User* user,
			const winrt::Windows::Storage::KnownFolderId& id);
	};
}
