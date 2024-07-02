/*
   Copyright 2024 Christopher-Marios Mamaloukas

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

#pragma once

#include <array>
#include <string>

#include <Windows.h>

#include "Dragonfly.h"

#include "Dragonfly.Generics.hxx"

namespace Dfl {
    namespace Graphics { class Renderer; }

    // Dragonfly.UI
    namespace UI {
        // Dragonfly.UI.Window
        class Window
        {
        public:
            struct Info {
                const std::array<uint32_t, 2>         Resolution{ DefaultResolution }; 
                const std::array<uint32_t, 2>         View{ DefaultResolution }; // Currently reserved and not used
                const bool                            DoFullscreen{ false };

                const std::array<int, 2>              Position{ {0, 0} }; // Relative to the screen space
                const std::basic_string_view<wchar_t> WindowTitle{ L"Dragonfly App" }; 

                const bool                            HasTitleBar{ true };
                const bool                            Extends{ false }; // Whether the draw area covers the titlebar as well

                const HWND                            hWindow{ nullptr }; // Instead of creating a new window, set this to render to children windows
            };

            struct Handles {
                const HWND   hWin32Window{ nullptr };
                const HWND   hParentWindow{ nullptr };

                operator HWND() { return this->hWin32Window; }
            };

            enum class Rectangle {
                Resolution,
                Position
            };

            DFL_API static constexpr uint32_t   DefaultWidth{ 1920 };
            DFL_API static constexpr uint32_t   DefaultHeight{ 1080 };
            DFL_API static constexpr std::array DefaultResolution{ DefaultWidth, DefaultHeight };

        protected:
            const  Handles               Win32{ };

        public:
            DFL_API DFL_CALL Window(const Info& info);
            DFL_API DFL_CALL ~Window();

            const HWND GetHandle() const noexcept { 
                          return this->Win32.hWin32Window; }
            DFL_API 
            inline 
            const bool        
            DFL_CALL   GetCloseStatus() const noexcept;

            template< Rectangle R >
            inline
            std::array<uint32_t, 2>
                       GetRectangle() const noexcept;

            DFL_API
            inline
            const Window&        
            DFL_CALL   SetTitle(const wchar_t* newTitle) const noexcept;

            template< Rectangle R >
            inline
            const Window&
                       SetRectangle(std::array<uint32_t, 2>&& newRect) const noexcept;

            friend class
                Dfl::Graphics::Renderer;
        };

        // Dragonfly.UI.Dialog
        class Dialog : public Window {
        public:
            enum class Type {
                Info,
                Warning,
                Error,
                Question,
                Custom
            };

            struct Info {
                DflGen::WindowsString Title{ L"Dragonfly Message Box" };
                DflGen::WindowsString Message{ L"Hello, World!" };
                Type                  DialogType{ Type::Info };
            };

            enum class [[ nodiscard ]] Result : uint32_t {
                NotReady = 0,
                OK = 1,
                Cancel = 2,
            };

            DFL_API static constexpr uint32_t Width{ 400 };
            DFL_API static constexpr uint32_t Height{ 200 };

        public:
            DFL_API DFL_CALL Dialog(const Info& info);
            DFL_API DFL_CALL ~Dialog();

            DFL_API 
            inline
            const Result  
            DFL_CALL GetResult() const noexcept;
        };
    }
    namespace DflUI = Dfl::UI;
}

// TEMPLATE DEFINITIONS

// Dragonfly.UI.Window

template< Dfl::UI::Window::Rectangle R >
inline std::array<uint32_t, 2> Dfl::UI::Window::GetRectangle() const noexcept
{
    RECT rect;

    if (GetWindowRect(
            this->Win32.hWin32Window,
            &rect) == 0) 
    {
        return { 0, 0 };
    }

    if constexpr ( R == Rectangle::Resolution ) {
        return { static_cast<uint32_t>(std::abs(rect.left - rect.right)), 
                 static_cast<uint32_t>(std::abs(rect.top - rect.bottom)) };
    }
    else {
        return { static_cast<uint32_t>(rect.left), 
                 static_cast<uint32_t>(rect.top) };
    }
}

template< Dfl::UI::Window::Rectangle R >
inline const Dfl::UI::Window& Dfl::UI::Window::SetRectangle(std::array<uint32_t, 2>&& newRect) const noexcept
{
    
    std::array<uint32_t, 2> constRect{ 0, 0 };
    if constexpr ( R == Rectangle::Resolution ) {
        constRect = this->GetRectangle<Rectangle::Position>();
    }
    else {
        constRect = this->GetRectangle<Rectangle::Resolution>();
    }

    SetWindowPos(
        this->Win32.hWin32Window,
        nullptr,
        newRect[0],
        newRect[1],
        newRect[0],
        newRect[1],
        R == Rectangle::Resolution ? SWP_NOMOVE : SWP_NOSIZE);

    return *this;
}
