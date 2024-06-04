/*
   Copyright 2023 Christopher-Marios Mamaloukas

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "Dragonfly.hxx"

#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dwmapi.h>

namespace DflUI = Dfl::UI;

constexpr BOOL WinVerum = TRUE;
constexpr BOOL WinFalsum = FALSE;

// Window stuff

static LRESULT CALLBACK INT_WindowProc(
                    HWND   hWindow,
                    UINT   message,
                    WPARAM wParams,
                    LPARAM lParams) {
    switch (message) {
    case WM_CREATE:
        break;
    case WM_QUIT:
    case WM_CLOSE:
        ShowWindow(
                hWindow,
                SW_HIDE);
        break;
    case WM_SYSCOMMAND:
        if (wParams == SC_CLOSE) {
            ShowWindow(
                hWindow,
                SW_HIDE);
            break;
        }

        if (wParams == SC_MINIMIZE) {
            ShowWindow(
                hWindow,
                SW_MINIMIZE);
            
            break;
        }
        
        if (wParams == SC_RESTORE) {
            ShowWindow(
                hWindow,
                SW_RESTORE);

            break;
        }
    default: 
        return DefWindowProc(hWindow, message, wParams, lParams);
    }

    return false;
}

static inline std::array<uint32_t, 2> INT_GetFullscreen() {
    DISPLAY_DEVICE display{};
    DEVMODE        displayMode{};
    uint32_t       dispIndex{ 0 };

    while (true) {
        if (EnumDisplayDevicesW(
                nullptr,
                dispIndex,
                &display,
                0) == FALSE) {
            break;
        }

        if (EnumDisplayDevicesW(
                display.DeviceName,
                0,
                &display,
                0) == FALSE ||
             display.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
            dispIndex++;
            continue;
        }

        if (EnumDisplaySettingsW(
            display.DeviceName,
            ENUM_CURRENT_SETTINGS,
            &displayMode) == FALSE) {
            dispIndex++;
            continue;
        }

        if ( displayMode.dmPosition.x == 0 && 
             displayMode.dmPosition.y == 0 ) {
            break;
        } 

        dispIndex++;
    }

    return { displayMode.dmPelsWidth, displayMode.dmPelsHeight };
}

static DflUI::Window::Handles INT_GetWindow(const DflUI::Window::Info& info) 
{
    const WNDCLASSW windowClass{
        .lpfnWndProc{ INT_WindowProc },
        .hInstance{ GetModuleHandle( L"Dragonfly" ) },
        .lpszClassName{ L"DragonflyApp" }
    };
    RegisterClassW(&windowClass);

    auto displaySize{ INT_GetFullscreen() };

    const HWND window{ CreateWindowExW(
                            0,
                            windowClass.lpszClassName,
                            info.HasTitleBar || !info.DoFullscreen ? 
                                info.WindowTitle.data() :
                                nullptr,
                            info.hWindow == nullptr ? 
                                ( info.HasTitleBar ? 
                                    WS_OVERLAPPEDWINDOW ^ ( WS_MAXIMIZEBOX | WS_THICKFRAME ) :
                                    0 ) :
                                WS_CHILD,
                            info.DoFullscreen ?
                                0, 0 :
                                info.Position[0], info.Position[1],
                            info.DoFullscreen ?
                                displaySize[0], displaySize[1] :
                                info.Resolution[0], info.Resolution[1],
                            info.hWindow,
                            nullptr,
                            GetModuleHandle(L"Dragonfly"),
                            nullptr) };

    if (window == nullptr) {
        throw Dfl::Error::HandleCreation(
                L"Unable to create window",
                L"INT_GetWindow");
    }

    DwmSetWindowAttribute(
        window,
        DWMWA_USE_IMMERSIVE_DARK_MODE,
        &WinVerum,
        sizeof(BOOL));

    if ( !info.HasTitleBar || info.Extends || info.DoFullscreen ) {
        MARGINS margins = { -1 };
        if ( DwmExtendFrameIntoClientArea(
                window,
                &margins) != S_OK ) {
            throw Dfl::Error::HandleCreation(
                    L"Unable to extend window frame",
                    L"INT_GetWindow");
        }
    }

    ShowWindow(
        window,
        SW_NORMAL);

    return { window };
}

DflUI::Window::Window(const Info& info)
: Win32( INT_GetWindow(info) ) {  }

DflUI::Window::~Window() {
    DestroyWindow(this->Win32.hWin32Window);
}

inline const bool DflUI::Window::GetCloseStatus() const noexcept 
{
    if (IsWindowVisible(this->Win32.hWin32Window) == FALSE ||
        IsWindow(this->Win32.hWin32Window) == FALSE) {
        return true;
    }
    
    MSG windowMessage{ };
    if ( GetMessageW(
            &windowMessage,
            this->Win32.hWin32Window,
            0x0, 0x0) == -1 ) {
        return true;
    }

    DispatchMessageW(&windowMessage);

    return false;
}

inline const DflUI::Window& DflUI::Window::SetTitle(const wchar_t* newTitle) const noexcept 
{
    SetWindowTextW(
            this->Win32.hWin32Window,
            newTitle);

    return *this;
}