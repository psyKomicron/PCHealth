#include "pch.h"
#include "DisplayMonitor.hpp"

#include "utilities.h"

#include "Windows.h"

#include <limits>
#include <thread>
#include <chrono>

namespace pchealth::windows
{
    std::vector<DisplayMonitor> DisplayMonitor::enumerateDisplays()
    {
        auto displays = std::vector<DisplayMonitor>();

        std::vector<DISPLAYCONFIG_PATH_INFO> paths{};
        std::vector<DISPLAYCONFIG_MODE_INFO> modes{};
        sizeVectors(paths, modes);

        for (auto&& path : paths)
        {
            auto&& display = DisplayMonitor(path);
            uint32_t targetId = path.targetInfo.id;
            for (auto&& mode : modes)
            {
                if (mode.id == targetId)
                {
                    display.addMode(mode);
                }
            }

            displays.push_back(std::move(display));
        }

        return displays;
    }

    std::optional<DisplayMonitor> DisplayMonitor::getDisplayMonitorForDisplayId(const uint32_t& displayId)
    {
        std::vector<DISPLAYCONFIG_PATH_INFO> paths{};
        std::vector<DISPLAYCONFIG_MODE_INFO> modes{};
        sizeVectors(paths, modes);

        for (auto&& path : paths)
        {
            if ((path.sourceInfo.id + 1) == displayId)
            {
                auto&& display = DisplayMonitor(path);
                uint32_t targetId = path.targetInfo.id;
                for (auto&& mode : modes)
                {
                    if (mode.id == targetId)
                    {
                        display.addMode(mode);
                    }
                }

                return display;
            }
        }
        return {};
    }


    DisplayMonitor::DisplayMonitor(const DISPLAYCONFIG_PATH_INFO& path)
    {
        _id = path.sourceInfo.id + 1;

        setFriendlyName(path);
        enumDisplaySettings(_id);

        // Find the adapter device name
        DISPLAYCONFIG_ADAPTER_NAME adapterName = {};
        adapterName.header.adapterId = path.targetInfo.adapterId;
        adapterName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;
        adapterName.header.size = sizeof(adapterName);
        winrt::check_hresult(HRESULT_FROM_WIN32(DisplayConfigGetDeviceInfo(&adapterName.header)));
    }


    std::wstring DisplayMonitor::friendlyName()
    {
        return _friendlyName;
    }

    int32_t DisplayMonitor::id() const
    {
        return _id;
    }

    std::map<std::pair<uint32_t, uint32_t>, std::vector<uint32_t>>& DisplayMonitor::supportedResolutions()
    {
        return _supportedResolutions;
    }

    uint32_t DisplayMonitor::vsync() const
    {
        return _vsync;
    }

    std::pair<uint32_t, uint32_t> DisplayMonitor::currentResolution() const
    {
        return _currentResolution;
    }


    bool DisplayMonitor::changeDisplayRefreshRate(const int32_t& /*refreshRate*/)
    {
        return false;
    }

    void DisplayMonitor::addMode(const DISPLAYCONFIG_MODE_INFO& mode)
    {
        if (mode.infoType == DISPLAYCONFIG_MODE_INFO_TYPE_TARGET)
        {
            DISPLAYCONFIG_VIDEO_SIGNAL_INFO signalInfo = mode.targetMode.targetVideoSignalInfo;
            uint32_t x = signalInfo.activeSize.cx;
            uint32_t y = signalInfo.activeSize.cy;
            uint64_t clockRate = signalInfo.pixelRate;
            double hsync = static_cast<double>(signalInfo.hSyncFreq.Numerator) / signalInfo.hSyncFreq.Denominator;
            double vsync = static_cast<double>(signalInfo.vSyncFreq.Numerator) / signalInfo.vSyncFreq.Denominator;

            _currentResolution = std::make_pair(x, y);
            _vsync = vsync;
        }
    }

    void DisplayMonitor::reapplyCurrentVsync()
    {
        auto dm = getCurrentMode();
        dm.dmFields = DM_DISPLAYFREQUENCY;
        uint32_t closestFrequency = dm.dmDisplayFrequency;
        std::wstring deviceNameString = std::format(LR"(\\.\DISPLAY{})", _id);

        // Testing if the chosen frequency is good.
        auto res = ChangeDisplaySettingsExW(deviceNameString.c_str(), &dm, nullptr, CDS_TEST, nullptr);
        dm.dmDisplayFrequency = _vsync;
        res = ChangeDisplaySettingsExW(deviceNameString.c_str(), &dm, nullptr, CDS_TEST, nullptr);

        if (res == DISP_CHANGE_SUCCESSFUL)
        {
            OUTPUT_DEBUG(std::format(L"[DisplayMonitor] Test successful, frequency: {}/{}.", closestFrequency, dm.dmDisplayFrequency));

            dm.dmDisplayFrequency = closestFrequency;
            res = ChangeDisplaySettingsExW(deviceNameString.c_str(), &dm, nullptr, CDS_UPDATEREGISTRY, nullptr);
            if (res != DISP_CHANGE_SUCCESSFUL)
            {
                OUTPUT_DEBUG(std::format(L"[DisplayMonitor] Failed to apply frequency ({} Hz).", dm.dmDisplayFrequency));
            }

            OUTPUT_DEBUG(std::format(L"[DisplayMonitor] Applied temporary lower frequency ({}), reverting back to old frequency ({}).", closestFrequency, _vsync));
            std::this_thread::sleep_for(std::chrono::seconds(3));

            dm.dmDisplayFrequency = _vsync;
            res = ChangeDisplaySettingsExW(deviceNameString.c_str(), &dm, nullptr, CDS_UPDATEREGISTRY, nullptr);
            if (res != DISP_CHANGE_SUCCESSFUL)
            {
                OUTPUT_DEBUG(std::format(L"[DisplayMonitor] Test failed, frequency: {}.", _vsync));
            }

            OUTPUT_DEBUG(L"[DisplayMonitor] Reverted back to old frequency, display frequency refresh successful !");
        }
        else
        {
            OUTPUT_DEBUG(std::format(L"[DisplayMonitor] Test failed, frequency: {}/{}.", closestFrequency, dm.dmDisplayFrequency));
        }
    }


    void DisplayMonitor::enumDisplaySettings(const uint32_t& id)
    {
        DEVMODEW dm = utilities::createWin32Struct<DEVMODEW>();
        std::wstring deviceNameString = std::format(LR"(\\.\DISPLAY{})", id);
        for (uint32_t iModeNum = 0; EnumDisplaySettingsW(deviceNameString.c_str(), iModeNum, &dm) != 0; iModeNum++)
        {
            std::pair<uint32_t, uint32_t> resolution{ dm.dmPelsWidth, dm.dmPelsHeight };
            uint32_t frequency = dm.dmDisplayFrequency;
            std::vector<uint32_t>& freq = _supportedResolutions[resolution];

            bool contains = false;
            for (size_t i = 0; i < freq.size(); i++)
            {
                if (freq[i] == frequency)
                {
                    contains = true;
                    break;
                }
            }
            if (!contains)
            {
                freq.push_back(frequency);
            }
        }
    }

    void DisplayMonitor::setFriendlyName(const DISPLAYCONFIG_PATH_INFO& path)
    {
        // Find the target (monitor) friendly name
        DISPLAYCONFIG_TARGET_DEVICE_NAME targetName = {};
        targetName.header.adapterId = path.targetInfo.adapterId;
        targetName.header.id = path.targetInfo.id;
        targetName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        targetName.header.size = sizeof(targetName);
        winrt::check_hresult(HRESULT_FROM_WIN32(DisplayConfigGetDeviceInfo(&targetName.header)));

        _friendlyName = (targetName.flags.friendlyNameFromEdid ? targetName.monitorFriendlyDeviceName : L"");
        _displayName = std::wstring(targetName.monitorDevicePath);

        /*OutputDebug
        (
            std::format(L"'{}' ({}) display friendly name: {}", 
                targetName.monitorDevicePath, 
                targetName.connectorInstance,
                (targetName.flags.friendlyNameFromEdid ? targetName.monitorFriendlyDeviceName : L"unknown")
            )
        );*/
    }

    void DisplayMonitor::sizeVectors(std::vector<DISPLAYCONFIG_PATH_INFO>& paths, std::vector<DISPLAYCONFIG_MODE_INFO>& modes)
    {
        UINT32 flags = QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE;
        LONG result = ERROR_SUCCESS;
        do
        {
            // Determine how many path and mode structures to allocate
            uint32_t pathCount = 0;
            uint32_t modeCount = 0;
            winrt::check_hresult(GetDisplayConfigBufferSizes(flags, &pathCount, &modeCount));

            // Allocate the path and mode arrays
            paths.resize(pathCount);
            modes.resize(modeCount);

            // Get all active paths and their modes
            result = QueryDisplayConfig(flags, &pathCount, paths.data(), &modeCount, modes.data(), nullptr);

            // The function may have returned fewer paths/modes than estimated
            paths.resize(pathCount);
            modes.resize(modeCount);

            // It's possible that between the call to GetDisplayConfigBufferSizes and QueryDisplayConfig
            // that the display state changed, so loop on the case of ERROR_INSUFFICIENT_BUFFER.
        } 
        while (result == ERROR_INSUFFICIENT_BUFFER);
    }

    DEVMODEW DisplayMonitor::getCurrentMode()
    {
        auto& supportedFrequencies = _supportedResolutions[_currentResolution];
        int32_t maxDiff = (std::numeric_limits<int32_t>::max)();
        uint32_t closestFrequency = _vsync;
        for (size_t i = 0; i < supportedFrequencies.size(); i++)
        {
            int32_t diff = _vsync - supportedFrequencies[i];
            if (diff > 0 && diff < maxDiff)
            {
                maxDiff = diff;
                closestFrequency = supportedFrequencies[i];
            }
        }

        DEVMODEW dm = utilities::createWin32Struct<DEVMODEW>();
        std::wstring deviceNameString = std::format(LR"(\\.\DISPLAY{})", _id);
        for (uint32_t iModeNum = 0; EnumDisplaySettingsW(deviceNameString.c_str(), iModeNum, &dm) != 0; iModeNum++)
        {
            std::pair<uint32_t, uint32_t> resolution{ dm.dmPelsWidth, dm.dmPelsHeight };
            uint32_t frequency = dm.dmDisplayFrequency;
            if (frequency == closestFrequency && resolution == _currentResolution)
            {
                return dm;
            }
        }

        throw std::out_of_range("[DisplayMonitor] Unable to get current mode.");
    }
}