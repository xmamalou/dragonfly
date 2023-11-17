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

#include <tuple>
#include <string>
#include <thread>
#include <vector>
#include <mutex>

#include <Windows.h>

#include <GLFW/glfw3.h>

#include "Dragonfly.h"

export module Dragonfly.Observer:Window;

namespace Dfl
{
    // Observers are the things that the engine uses to render to.
    namespace Observer
    {
        // standard resolution used as a default
        export DFL_API constexpr uint32_t DefaultWidth{ 1920 };
        export DFL_API constexpr uint32_t DefaultHeight{ 1080 };

        export DFL_API const std::tuple<uint32_t, uint32_t> DefaultResolution{ std::make_tuple(DefaultWidth, DefaultHeight) };

        export struct WindowProcessArgs
        {

        };

        // a function pointer to processes meant to be executed during a Window's existence.
        export class WindowProcess 
        {
        public:
            virtual void operator () (WindowProcessArgs& args) = 0;
            virtual void Destroy() = 0; // called when the window is closing.
        };

        /*!
         * \brief Information used for initializing a Window
        */
        export struct WindowInfo
        {
            std::tuple<uint32_t, uint32_t> Resolution{ DefaultResolution }; // the dimensions of the window and the image resolution
            std::tuple<uint32_t, uint32_t> View{ DefaultResolution }; // render area of the window
            bool                           DoFullscreen{ false };

            bool     DoVsync{ true };
            uint32_t Rate{ 60 }; // Refresh rate, in Hz

            std::tuple<int, int> Position{ std::make_tuple(0, 0) }; // Relative to the monitor
            std::string_view     WindowTitle{ "Dragonfly App" };

            std::vector<WindowProcess*> Processes{ }; // the process that will execute in the thread concerning the window

            //Dfl::Graphics::Image& icon;

            HWND hWindow{ nullptr }; // instead of creating a new window, set this to render to children windows
        };

        /*!
         * \brief An enum that contains error codes for the Window management part of Dragonfly
        */
        export enum class [[nodiscard]] WindowError {
            Success = 0,
            APIInitError = -0x3101,
            WindowInitError = -0x3201,
            // warnings
            ThreadNotReadyWarning = 0x1001
        };

        // the window functor is responsible for tracking the state of the thread of the window and the window itself
        class WindowFunctor
        {
            WindowInfo  Info;

            GLFWwindow* pGLFWwindow{ nullptr };
            HWND        hWin32Window{ nullptr };

            WindowError Error{ WindowError::ThreadNotReadyWarning };
            bool        ShouldClose{ false };

            std::mutex        AccessProcess;
            WindowProcessArgs Arguments;

            friend class Window; // Windows should be able to access everything about them
        public:
            WindowFunctor(const WindowInfo& info);
            ~WindowFunctor();

            void operator() ();

            friend DFL_API inline void DFL_CALL PushProcess(WindowProcess& process, Window& window);
        };

        /*!
        * \brief The `Window` class concerns objects that are used to
        * be rendered onto. May concern actual independent windows, child
        * windows of other windows (ex. a canvas on a window) or "imaginary"
        * windows; AKA offscreen renders.
        */
        export class Window
        {
            WindowFunctor Functor;
            std::thread   Thread;
        public:
            DFL_API DFL_CALL Window(const WindowInfo& createInfo);
            DFL_API DFL_CALL ~Window();

            /*!
            * \brief Reserves resources for and launches the thread that opens (if applicable) 
            * the associated window.
            *
            * Window creation depends upon the values passed using `Observer::WindowInformation`.
            * If some fields are missing, default values are used.
            * 
            * Windows are launched in their own thread.
            */
            DFL_API WindowError DFL_CALL OpenWindow();

            std::tuple<uint32_t, uint32_t> Resolution() const
            {
                return this->Functor.Info.Resolution;
            }

            HWND WindowHandle() const
            {
                return this->Functor.hWin32Window;
            }

            bool IsOnscreen() const
            {
                if (this->Functor.pGLFWwindow != nullptr)
                    return true;
                else if (this->Functor.hWin32Window != nullptr)
                {
                    LONG status = 0;
                    GetWindowLong(this->Functor.hWin32Window, status);

                    return (status & WS_VISIBLE) ? true : false;
                }

                return false;
            }

            bool CloseStatus() const
            {
                return this->Functor.ShouldClose;
            }

            friend DFL_API inline void DFL_CALL PushProcess(WindowProcess& process, Window& window);
        };

        /*!
        * \brief Add a new process to be executed by the window.
        * 
        * Temporarily locks the calling thread until the process can be pushed. Likewise, blocks the window thread
        * until the process is pushed. There shouldn't be any noticeable delay due to this.
        */
        export DFL_API inline void DFL_CALL PushProcess(WindowProcess& process, Window& window);
    }
}