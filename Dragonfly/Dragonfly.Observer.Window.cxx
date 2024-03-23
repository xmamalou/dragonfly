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

module;

#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dwmapi.h>

module Dragonfly.Observer.Window;

namespace DflOb = Dfl::Observer;

constexpr BOOL WinVerum = TRUE;
constexpr BOOL WinFalsum = FALSE;

// Window stuff

LRESULT CALLBACK INT_WindowProc(
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

static HWND INT_GetWindow(const DflOb::Window::Info info) {
    // Why is this done before the creation of the window?
    // So we can pass the windowProp pointer as additional 
    // data when the window is being created.

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
        throw DflOb::Window::Error::Win32WindowInitError;
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
            throw DflOb::Window::Error::Win32WindowTitleBarRemoveError;
        }
    }

    ShowWindow(
        window,
        SW_NORMAL);

    return window;
}

DflOb::Window::Window(const Info& info)
try : pInfo( new Info(info) ),
      Win32( Handles( { INT_GetWindow(info) } ) ) {  
}
catch (Error e) { this->ErrorCode = e; }

DflOb::Window::~Window() {
    DestroyWindow(this->Win32.hWin32Window);
}

inline const bool DflOb::Window::GetCloseStatus() const noexcept {
    BOOL result{ 0 };
    MSG  windowMessage{ };
    if ( (result = GetMessageW(
                        &windowMessage,
                        this->Win32.hWin32Window,
                        0x0, 0x0)) == -1 ) {
        return true;
    }

    DispatchMessageW(&windowMessage);

    if ( IsWindowVisible(this->Win32.hWin32Window) == FALSE ||
         IsWindow(this->Win32.hWin32Window) == FALSE ) { return true; } else { return false; }
}

inline void DflOb::Window::SetTitle(const wchar_t* newTitle) const noexcept {
    if ( SetWindowTextW(
            this->Win32.hWin32Window,
            newTitle) == TRUE ) { 
        this->pInfo->WindowTitle = newTitle; 
    }
}