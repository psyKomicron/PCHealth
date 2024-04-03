#pragma once
#include "VisualState.hpp"

#include "utilities.h"

#include <vector>
#include <map>

namespace xaml = winrt::Microsoft::UI::Xaml;

namespace winrt::PCHealth
{
    template<typename T>
    class VisualStateManager
    {
    public:
        VisualStateManager(const xaml::Controls::Control& userControl)
        {
            control = userControl;
        }

        VisualState<T> getCurrentState(const int32_t& group)
        {
            for (size_t i = 0; i < _visualStates[group].size(); i++)
            {
                if (_visualStates[group][i].active())
                {
                    return _visualStates[group][i];
                }
            }
            return {};
        }

        void initializeStates(const std::vector<VisualState<T>>& states)
        {
            for (auto&& state : states)
            {
                _visualStates[state.group()].push_back(state);
            }
        }

        void goToState(const VisualState<T>& state, const bool& useTransitions = true)
        {
            try
            {
                xaml::VisualStateManager::GoToState(control, state, useTransitions);
                for (VisualState<T>& visualState : _visualStates[state.group()])
                {
                    if (visualState == state)
                    {
                        visualState.active(true);
                    }
                }
                OutputDebug(std::format(L"[VisualStateManager] Activated state '{}'", std::wstring(state.name())));
            }
            catch (winrt::hresult_wrong_thread)
            {
                OutputDebug(std::format(L"[VisualStateManager] Failed to activate '{}', caller called from the wrong thread.", std::wstring(state.name())));
                throw;
            }
        }

        void switchState(int32_t group, const bool& useTransitions = true)
        {
            std::vector<VisualState<T>>& visualStates = _visualStates[group];
            for (size_t i = 0; i < visualStates.size(); i++)
            {
                if (visualStates[i].active())
                {
                    size_t nextIndex = (i + 1) % visualStates.size();
                    goToState(visualStates[nextIndex], useTransitions);
                    visualStates[i].active(false);
                    break;
                }
            }
        }

    private:
        xaml::Controls::Control control{ nullptr };
        //VisualState<T> _currentState{};
        std::map<int32_t, std::vector<VisualState<T>>> _visualStates;
    };
}

