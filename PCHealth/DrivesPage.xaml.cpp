#include "pch.h"
#include "DrivesPage.xaml.h"
#if __has_include("DrivesPage.g.cpp")
#include "DrivesPage.g.cpp"
#endif

#include "DirectoryInfo.h"
#include "System.h"
#include "FileSize.h"

#include "MainWindow.xaml.h"

#include <microsoft.ui.xaml.window.h>

#include <shellapi.h>
#include <ppl.h>
#include <algorithm>

namespace xaml = winrt::Microsoft::UI::Xaml;

namespace winrt::PCHealth::implementation
{
    winrt::Windows::Foundation::IAsyncAction DrivesPage::DrivesGrid_Loading(xaml::FrameworkElement const& sender, winrt::Windows::Foundation::IInspectable const& args)
    {
        auto&& drives = pchealth::filesystem::DriveInfo::GetDrives();

        ConnectedDrivesNumberTextBlock().Text(std::to_wstring(drives.size()));

        uint64_t recycleBinTotalSize = 0;
        for (auto&& drive : drives)
        {
            DriveView driveView{};
            driveView.DriveName(drive.name());
            driveView.Capacity(static_cast<double>(drive.capacity()));
            driveView.UsedSpace(static_cast<double>(drive.totalUsedSpace()));
            DrivesList().Children().Append(driveView);

            recycleBinTotalSize += drive.getRecycleBinSize();
        }

        Common::FileSize hibernationFileSize = pchealth::windows::System::GetFileSize(L"c:\\hiberfil.sys");
        HibernationFileSize().Text(hibernationFileSize.ToString());
        if (hibernationFileSize.Size() == 0)
        {
            HibernationFileSizeButton().IsEnabled(false);
        }

        Common::FileSize systemRecycleBinSize = recycleBinTotalSize;
        SystemRecycleBinSize().Text(systemRecycleBinSize.ToString());
        if (recycleBinTotalSize == 0)
        {
            SystemRecycleBinSizeButton().IsEnabled(false);
        }

        co_await winrt::resume_background();
        auto&& libs = pchealth::windows::System::GetLibraries().get();
        auto&& downloadsFolder = libs.DownloadsFolder();
        if (!downloadsFolder.empty())
        {
            pchealth::filesystem::DirectoryInfo downloadsFolderInfo{ downloadsFolder };
            auto&& downloadsFolderSize = downloadsFolderInfo.GetSize();
            DispatcherQueue().TryEnqueue([this, downloadsSize = Common::FileSize(downloadsFolderSize)]
            {
                DownloadsFolderSize().Text(downloadsSize.ToString());
            });
        }
    }

    winrt::Windows::Foundation::IAsyncAction DrivesPage::SystemRecycleBinSizeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, xaml::RoutedEventArgs const& e)
    {
        auto&& mainWindow = PCHealth::MainWindow::Current();
        auto nativeWindow{ mainWindow.try_as<::IWindowNative>() };
        HWND handle = nullptr;
        if (&nativeWindow != nullptr)
        {
            nativeWindow->get_WindowHandle(&handle);
        }

        co_await winrt::resume_background();

        SHQUERYRBINFO queryBinInfo{ sizeof(SHQUERYRBINFO) };
        winrt::hstring message{};
        xaml::Controls::InfoBarSeverity severity{};
        if (SUCCEEDED(SHEmptyRecycleBin(handle, nullptr, SHERB_NOCONFIRMATION | SHERB_NOSOUND)))
        {
            message = L"Recycle bin emptied (all drives).";
            severity = xaml::Controls::InfoBarSeverity::Success;
        }
        else
        {
            message = L"Failed to empty recycle bin.";
            severity = xaml::Controls::InfoBarSeverity::Error;
        }

        DispatcherQueue().TryEnqueue([this, severity, message]
        {
            InfoBar().Severity(severity);
            //TODO: i18n.
            InfoBar().Message(message);
            InfoBar().IsOpen(true);

            DrivesList().Children().Clear();
            DrivesGrid_Loading(nullptr, nullptr);
        });
    }

    void DrivesPage::HibernationFileSizeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, xaml::RoutedEventArgs const& e)
    {

    }

    winrt::Windows::Foundation::IAsyncAction DrivesPage::DownloadsFolderSizeButton_Click(winrt::Windows::Foundation::IInspectable const& sender, xaml::RoutedEventArgs const& e)
    {
        // Prompt user for recycling and parallelization :
        auto&& result = co_await DirectoryDeleteConfirmation().ShowAsync();
        if (result != xaml::Controls::ContentDialogResult::Primary)
        {
            co_return;
        }

        bool recycle = RecycleDeleteCheckbox().IsChecked().GetBoolean();
        bool parallelize = ParallelizeDeleteCheckbox().IsChecked().GetBoolean();

        co_await winrt::resume_background();

        auto&& libs = pchealth::windows::System::GetLibraries().get();
        if (pchealth::windows::System::pathExists(libs.DownloadsFolder()))
        {
            try
            {
                pchealth::filesystem::DirectoryInfo dirInfo{ libs.DownloadsFolder() };
                dirInfo.EmptyDirectory(recycle, parallelize);
            }
            catch (std::exception ex)
            {
                OutputDebugStringA(ex.what());
            }
            catch (winrt::hresult_error ex)
            {
                OutputDebugString(ex.message().c_str());
            }
        }
        else
        {
            DispatcherQueue().TryEnqueue([this, path = libs.DownloadsFolder()]
            {
                InfoBar().Message(std::format(L"Downloads folder ('{}') does not exist.", path));
            });
        }
    }
}

