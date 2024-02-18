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
#include <memory>
#include <chrono>
#include <thread>

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
    case WM_CLOSE:
        ShowWindow(
                hWindow,
                SW_HIDE);
        break;
    default: 
        return DefWindowProc(hWindow, message, wParams, lParams);
    }

    return false;
}

DflOb::WindowHandles INT_GetHandles(const DflOb::WindowInfo info) {
    const WNDCLASSW windowClass{
        .lpfnWndProc{ INT_WindowProc },
        .hInstance{ GetModuleHandle( L"Dragonfly" ) },
        .lpszClassName{ L"DragonflyApp" }
    };
    RegisterClassW(&windowClass);
    const HWND window{ CreateWindowExW(
                            0,
                            windowClass.lpszClassName,
                            info.WindowTitle.data(),
                            WS_OVERLAPPEDWINDOW ^ ( WS_MAXIMIZEBOX | WS_THICKFRAME ),
                            info.Position[0], info.Position[1],
                            info.Resolution[0], info.Resolution[1],
                            nullptr,
                            nullptr,
                            GetModuleHandle(L"Dragonfly"),
                            nullptr) };

    if (window == nullptr) {
        throw DflOb::WindowError::Win32WindowInitError;
    }

    DwmSetWindowAttribute(
        window,
        DWMWA_USE_IMMERSIVE_DARK_MODE,
        &WinVerum,
        sizeof(BOOL));

    ShowWindow(
        window,
        SW_NORMAL);

    return { window };
}

DflOb::Window::Window(const WindowInfo& info)
try : pInfo( new WindowInfo(info) ),
      Handles( INT_GetHandles(info) ) {  
}
catch (WindowError e) { this->Error = e; }

DflOb::Window::~Window() {
    DestroyWindow(this->Handles.hWin32Window);
}

inline const bool DflOb::Window::GetCloseStatus() const noexcept {
    BOOL result{ 0 };
    MSG  windowMessage{ };
    if ( (result = GetMessageW(
                        &windowMessage,
                        this->Handles.hWin32Window,
                        0x0, 0x0)) == -1 ) {
        return true;
    }

    DispatchMessageW(&windowMessage);

    if ( IsWindowVisible(this->Handles.hWin32Window) == FALSE ) { return true; } else { return false; }
}