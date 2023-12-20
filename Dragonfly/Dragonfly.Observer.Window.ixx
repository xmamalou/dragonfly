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
#include <mutex>
#include <atomic>
#include <chrono>

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
            ThreadCreationError = -0x1001,
            APIInitError = -0x3101,
            WindowInitError = -0x3201,

            // warnings

            ThreadNotReadyWarning = 0x1001
        };

        struct WindowInfo;

        export struct WindowProcessArgs{
            const WindowInfo* pInfo{ nullptr };

            const long long LastFrameTime{ 0 }; // actual frame time
            const long long CurrentTime{ 0 }; // time since start of the window loop
        
        private:
                          WindowProcessArgs(const WindowInfo& info);

            friend class
            WindowFunctor;
        };

        export class WindowProcess {
        protected:
            // called when the window runs, every frame.
            virtual void          operator() (const WindowProcessArgs& args)   = 0;
            // called when the window is closing.
            virtual void          Destroy()                                    = 0; 

            friend class
                    WindowFunctor;
        };

        export struct WindowInfo {
            std::array<uint32_t, 2>         Resolution{ DefaultResolution };
            std::array<uint32_t, 2>         View{ DefaultResolution }; // currently equivalent to resolution
            bool                            DoFullscreen{ false };

            bool                            DoVsync{ true };
            uint32_t                        Rate{ 60 };

            std::array<int, 2>              Position{ {0, 0} }; // Relative to the screen space
            std::basic_string_view<char8_t> WindowTitle{ u8"Dragonfly App" };

            std::vector<WindowProcess*>     pProcesses{ }; // these execute on the window thread, per frame

            //Dfl::Graphics::Image& icon;

            const HWND                      hWindow{ nullptr }; // instead of creating a new window, set this to render to children windows
        };

        class WindowFunctor
        {
            const std::unique_ptr<WindowInfo>        pInfo{ nullptr };
            const std::unique_ptr<WindowProcessArgs> pArguments{ nullptr };

                  GLFWwindow*                        pGLFWwindow{ nullptr };
                  HWND                               hWin32Window{ nullptr };

                  bool                               ShouldClose{ false };
                  WindowError                        Error{ WindowError::ThreadNotReadyWarning };

                  std::mutex                         AccessProcess;
        public:
                                         WindowFunctor(const WindowInfo& info);
                                         ~WindowFunctor();

                           void          operator() ();
            
            friend class
                           Dfl::Graphics::Renderer;
            friend class
                           Window;
            friend 
            DFL_API inline void DFL_CALL PushProcess(WindowProcess& process, Window& window) noexcept;
        };

        export class Window
        {
            WindowFunctor Functor;
            std::thread   Thread{ };
        public:
            DFL_API                                          DFL_CALL Window(const WindowInfo& createInfo);
            DFL_API                                          DFL_CALL ~Window() noexcept;

                           const WindowError                          GetErrorCode() const noexcept { return this->Functor.Error; }
                           
                           const WindowInfo                           GetCurrentInfo() const noexcept { return *this->Functor.pInfo; }
                           const HWND                                 GetHandle() const noexcept { return this->Functor.hWin32Window; }
                           const bool                                 GetCloseStatus() const noexcept { return this->Functor.ShouldClose; }
            
            friend class
                                 Dfl::Graphics::Renderer;
            friend 
            DFL_API inline       void                        DFL_CALL PushProcess(WindowProcess& process, Window& window) noexcept;
        };

        export DFL_API inline void DFL_CALL PushProcess(WindowProcess& process, Window& window) noexcept;
    }
}

inline void Dfl::Observer::PushProcess(WindowProcess& process, Window& window) noexcept {
    window.Functor.AccessProcess.lock();
    window.Functor.pInfo->pProcesses.push_back(&process);
    window.Functor.AccessProcess.unlock();
};