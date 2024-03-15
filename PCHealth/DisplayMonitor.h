#pragma once

namespace pchealth::windows
{
    class DisplayMonitor
    {
    public:
        static std::vector<DisplayMonitor> enumerateDisplays();
        static std::optional<DisplayMonitor> getDisplayMonitorForDisplayId(const uint32_t& displayId);

    public:
        DisplayMonitor(const DISPLAYCONFIG_PATH_INFO& path);

    public:
        int32_t id() const;
        std::wstring friendlyName();
        std::vector<std::pair<uint32_t, uint32_t>> displayResolutions();
        std::vector<uint32_t> displayRefreshRates(const std::pair<uint32_t, uint32_t>& resolution);
        std::map<std::pair<uint32_t, uint32_t>, std::vector<uint32_t>>& supportedResolutions();
        std::pair<uint32_t, uint32_t> currentResolution() const;
        uint32_t vsync() const;

    public:
        bool changeDisplayRefreshRate(const int32_t& refreshRate);
        void addMode(const DISPLAYCONFIG_MODE_INFO& mode);
        void reapplyCurrentVsync();

    private:
        std::wstring _displayName;
        std::wstring _friendlyName;
        uint32_t _id = static_cast<uint32_t>(-1);
        std::map<std::pair<uint32_t, uint32_t>, std::vector<uint32_t>> _supportedResolutions{};
        std::pair<uint32_t, uint32_t> _currentResolution{};
        uint32_t _vsync = 0;

    private:
        static void sizeVectors(std::vector<DISPLAYCONFIG_PATH_INFO>& paths, std::vector<DISPLAYCONFIG_MODE_INFO>& modes);
        void enumDisplaySettings(const uint32_t& id);
        void setFriendlyName(const DISPLAYCONFIG_PATH_INFO& path);
        DEVMODEW getCurrentMode();
    };
}
