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

#include "Dragonfly.h"

#include <memory>
#include <array>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

export module Dragonfly.Observer.Window;

namespace Dfl { namespace Graphics { class Renderer; } }

namespace Dfl{
    namespace Observer{
        export class Window
        {
        public:
            enum class [[nodiscard]] Error {
                Success = 0,
                // errors
                OutOfMemoryError = -0x1002,
                Win32WindowInitError = -0x2101,
                Win32WindowTitleBarRemoveError = -0x2102,
                Win32WindowPropertiesError = -0x2103,
                Win32HeapAllocationError = -0x2201
            };
            struct Info {
                std::array<uint32_t, 2>         Resolution{ DefaultResolution };
                std::array<uint32_t, 2>         View{ DefaultResolution }; // currently equivalent to resolution
                bool                            DoFullscreen{ false };

                bool                            DoVsync{ true };
                uint32_t                        Rate{ 60 };

                std::array<int, 2>              Position{ {0, 0} }; // Relative to the screen space
                std::basic_string_view<wchar_t> WindowTitle{ L"Dragonfly App" };

                bool                            HasTitleBar{ true };
                bool                            Extends{ false }; // whether the draw area covers the titlebar as well

                const HWND                      hWindow{ nullptr }; // instead of creating a new window, set this to render to children windows
            };
            struct Handles {
                const HWND   hWin32Window{ nullptr };

                operator HWND() { return this->hWin32Window; }
            };

            DFL_API static constexpr uint32_t   DefaultWidth{ 1920 };
            DFL_API static constexpr uint32_t   DefaultHeight{ 1080 };
            DFL_API static constexpr std::array DefaultResolution{ DefaultWidth, DefaultHeight };

            DFL_API static constexpr uint32_t   DefaultRate{ 60 };

        private:
            const  std::unique_ptr<Info> pInfo{ nullptr };
            const  Handles               Win32{ };

                   Error                 ErrorCode{ Error::Success };

        public: 
            DFL_API                          DFL_CALL Window(const Info& info);
            DFL_API                          DFL_CALL ~Window();

                           const Error                GetErrorCode() const noexcept { return this->ErrorCode; }         
                           const HWND                 GetHandle() const noexcept { return this->Win32.hWin32Window; }
            DFL_API inline const bool        DFL_CALL GetCloseStatus() const noexcept;
            
            DFL_API              void        DFL_CALL SetTitle(const wchar_t* newTitle) const noexcept;

            friend class
                                 Dfl::Graphics::Renderer;
        };
    }
}