#pragma once
namespace pchealth::windows::search
{
    enum SearchResultKind
    {
        None = 0,
        Application = 1,
        Directory = 2,
        QueueFill = 4,
        File = 8,
        Shortcut = 16
    };
}