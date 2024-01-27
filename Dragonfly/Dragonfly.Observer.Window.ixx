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
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <GLFW/glfw3.h>

export module Dragonfly.Observer.Window;

namespace Dfl { namespace Graphics { class Renderer; } }

namespace Dfl{
    namespace Observer{
        export DFL_API constexpr uint32_t   DefaultWidth{ 1920 };
        export DFL_API constexpr uint32_t   DefaultHeight{ 1080 };
        export DFL_API const     std::array DefaultResolution{ DefaultWidth, DefaultHeight};

        export enum class [[nodiscard]] WindowError {
            Success = 0,
            GlfwAPIInitError = -0x3101,
            GlfwWindowInitError = -0x3201,
        };

        export struct WindowInfo {
            std::array<uint32_t, 2>         Resolution{ DefaultResolution };
            std::array<uint32_t, 2>         View{ DefaultResolution }; // currently equivalent to resolution
            bool                            DoFullscreen{ false };

            bool                            DoVsync{ true };
            uint32_t                        Rate{ 60 };

            std::array<int, 2>              Position{ {0, 0} }; // Relative to the screen space
            std::basic_string_view<char8_t> WindowTitle{ u8"Dragonfly App" };

            //Dfl::Graphics::Image& icon;

            const HWND                      hWindow{ nullptr }; // instead of creating a new window, set this to render to children windows
        };

        struct WindowHandles {
                  GLFWwindow* const pGLFWwindow{ nullptr };
            const HWND              hWin32Window{ nullptr };

                                    operator GLFWwindow* () { return this->pGLFWwindow; }
        };

        export class Window
        {
                    const  std::unique_ptr<const WindowInfo>          pInfo{ nullptr };
                    const  WindowHandles                              Handles{ };

                           WindowError                                Error{ WindowError::Success };

            DFL_API inline void                              DFL_CALL PollEvents();
        public:
            DFL_API                                          DFL_CALL Window(const WindowInfo& info);
            DFL_API                                          DFL_CALL ~Window();

                           const WindowError                          GetErrorCode() const noexcept { return this->Error; }
                           
                           const WindowInfo                           GetCurrentInfo() const noexcept { return *this->pInfo; }
                           const HWND                                 GetHandle() const noexcept { return this->Handles.hWin32Window; }
            DFL_API inline const bool                        DFL_CALL GetCloseStatus() const noexcept;
            
            friend class
                                 Dfl::Graphics::Renderer;
        };
    }
}