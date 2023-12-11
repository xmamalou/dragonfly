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

#include <array>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>

#include <Windows.h>

#include <GLFW/glfw3.h>

#include "Dragonfly.h"

export module Dragonfly.Observer:Window;

namespace Dfl
{
    namespace Observer
    {
        export DFL_API constexpr uint32_t   DefaultWidth{ 1920 };
        export DFL_API constexpr uint32_t   DefaultHeight{ 1080 };
        export DFL_API const     std::array DefaultResolution{ DefaultWidth, DefaultHeight};

        export struct WindowProcessArgs{

        };

        export class WindowProcess {
        public:
            // called when the window runs, every frame.
            virtual void operator() (WindowProcessArgs& args)   = 0;
            // called when the window is closing.
            virtual void Destroy()                              = 0; 
        };

        export struct WindowInfo{
            std::array<uint32_t, 2>     Resolution{ DefaultResolution }; 
            std::array<uint32_t, 2>     View{ DefaultResolution }; // currently equivalent to resolution
            bool                        DoFullscreen{ false };

            bool                        DoVsync{ true };
            uint32_t                    Rate{ 60 };

            std::array<int, 2>          Position{ {0, 0} }; // Relative to the screen space
            std::string_view            WindowTitle{ "Dragonfly App" };

            std::vector<WindowProcess*> pProcesses{ }; // these execute on the window thread, per frame

            //Dfl::Graphics::Image& icon;

            const HWND                  hWindow{ nullptr }; // instead of creating a new window, set this to render to children windows
        };

        export enum class [[nodiscard]] WindowError{
            Success               = 0,
            ThreadCreationError   = -0x1001,
            APIInitError          = -0x3101,
            WindowInitError       = -0x3201,

            // warnings

            ThreadNotReadyWarning = 0x1001
        };

        class WindowFunctor
        {
                    WindowInfo                Info;

                    GLFWwindow*               pGLFWwindow{ nullptr };
                    HWND                      hWin32Window{ nullptr };

                    long                      FrameTime{ 0 }; // refers to the target frame time, in microseconds
                    std::chrono::microseconds LastFrameTime{ 0 }; // refers to the last frame time, in microseconds

                    std::atomic<WindowError>  Error{ WindowError::ThreadNotReadyWarning };
                    std::atomic_bool          ShouldClose{ false };

                    std::mutex                AccessProcess;
                    WindowProcessArgs         Arguments;

            friend 
            class   Window; 
        public:
                                         WindowFunctor(const WindowInfo& info);
                                         ~WindowFunctor();

                           void          operator() ();

            friend 
            DFL_API inline void DFL_CALL PushProcess(WindowProcess& process, Window& window) noexcept;
        };

        export class Window
        {
            WindowFunctor Functor;
            std::thread   Thread;
        public:
            DFL_API                                    DFL_CALL Window(const WindowInfo& createInfo) noexcept;
            DFL_API                                    DFL_CALL ~Window() noexcept;

            DFL_API        WindowError                 DFL_CALL InitWindow() noexcept;

                           std::array<uint32_t, 2>              Resolution() const noexcept { return this->Functor.Info.Resolution; }

                           HWND                                 WindowHandle() const noexcept { return this->Functor.hWin32Window; }

                    inline bool                                 IsOnscreen() const noexcept;

                           bool                                 CloseStatus() const noexcept { return this->Functor.ShouldClose; }

            friend 
            DFL_API inline void                        DFL_CALL PushProcess(WindowProcess& process, Window& window) noexcept;
        };

        export DFL_API inline void DFL_CALL PushProcess(WindowProcess& process, Window& window) noexcept;
    }
}