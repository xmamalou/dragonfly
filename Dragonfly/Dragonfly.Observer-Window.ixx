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
#include <atomic>
#include <chrono>

#include <Windows.h>

#include <GLFW/glfw3.h>

#include "Dragonfly.h"

export module Dragonfly.Observer:Window;

namespace Dfl
{
    // Observers are the things that the engine uses to render to.
    namespace Observer
    {
        export DFL_API constexpr uint32_t DefaultWidth{ 1920 };
        export DFL_API constexpr uint32_t DefaultHeight{ 1080 };
        // standard resolution used as a default
        export DFL_API const std::tuple<uint32_t, uint32_t> DefaultResolution{ std::make_tuple(DefaultWidth, DefaultHeight) };

        export struct WindowProcessArgs
        {

        };

        /// <summary> A functor that concerns processes run in the thread that handles the window. 
        /// <para>The functor is responsible for destroying itself when the window is closing, by defining <c>Destroy</c></para></summary>
        export class WindowProcess 
        {
        public:
            virtual void operator () (WindowProcessArgs& args) = 0;
            // called when the window is closing.
            virtual void Destroy() = 0; 
        };

        /// <summary>Information used for initializing a Window. See reference for details.</summary>
        export struct WindowInfo
        {
            std::tuple<uint32_t, uint32_t> Resolution{ DefaultResolution }; // the dimensions of the window and the image resolution
            std::tuple<uint32_t, uint32_t> View{ DefaultResolution }; // currently equivalent to resolution
            bool DoFullscreen{ false };

            bool     DoVsync{ true };
            uint32_t Rate{ 60 }; // Refresh rate, in Hz. Ignored if Vsync is enabled.

            std::tuple<int, int> Position{ std::make_tuple(0, 0) }; // Relative to the screen space
            std::string_view     WindowTitle{ "Dragonfly App" };

            std::vector<WindowProcess*> pProcesses{ }; // the process that will execute in the thread handling the window

            //Dfl::Graphics::Image& icon;

            HWND hWindow{ nullptr }; // instead of creating a new window, set this to render to children windows
        };

        ///<summary>Errors that may occur when creating a window.</summary>
        export enum class [[nodiscard]] WindowError {
            Success = 0,
            ThreadCreationError = -0x1001,
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

            long                          FrameTime{ 0 }; // in milliseconds
            std::chrono::microseconds     LastFrameTime{ 0 };

            std::atomic<WindowError> Error{ WindowError::ThreadNotReadyWarning };
            std::atomic_bool         ShouldClose{ false };

            std::mutex        AccessProcess;
            WindowProcessArgs Arguments;

            friend class Window; // Windows should be able to access everything about them
        public:
            WindowFunctor(const WindowInfo& info);
            ~WindowFunctor();

            void operator() ();

            friend DFL_API inline void DFL_CALL PushProcess(WindowProcess& process, Window& window);
        };

        /// <summary>
        /// The Window class is responsible for creating and managing a window.
        /// <para>Windows are created in their own thread.</para>
        /// </summary>
        export class Window
        {
            WindowFunctor Functor;
            std::thread   Thread;
        public:
            DFL_API DFL_CALL Window(const WindowInfo& createInfo);
            DFL_API DFL_CALL ~Window();

            /// <summary>
            /// Opens a Window and starts its thread.
            /// </summary>
            /// <returns>A <c>WindowError</c> code, that could be one of the following:
            /// <para>- <c>WindowError::Success</c>: The window was created successfully.</para>
            /// <para>- <c>WindowError::ThreadCreationError</c>: The thread couldn't be created.</para>
            /// <para>- <c>WindowError::APIInitError</c>: GLFW couldn't be initialized.</para>
            /// <para>- <c>WindowError::WindowInitError</c>: Window couldn't be created</para>
            /// </returns>
            DFL_API WindowError DFL_CALL OpenWindow() noexcept;

            std::tuple<uint32_t, uint32_t> Resolution() const noexcept
            {
                return this->Functor.Info.Resolution;
            }

            HWND WindowHandle() const noexcept
            {
                return this->Functor.hWin32Window;
            }

            bool IsOnscreen() const noexcept
            {
                // we don't mess with the visibility of windows created with GLFW
                // so we know that if it's a GLFW window, it's visible
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

            bool CloseStatus() const noexcept
            {
                return this->Functor.ShouldClose;
            }

            friend DFL_API inline void DFL_CALL PushProcess(WindowProcess& process, Window& window);
        };

        /// <summary>
        /// Push a process (specifically its pointer) to the window's thread.
        /// <para>
        /// Automatically locks the caller thread until process can be pushed. Depending on the load of
        /// the window thread's stack, this means that the caller thread could wait a long time. Do not offload unrelated
        /// logic here
        /// </para>
        /// </summary>
        /// <param name="process">: The process, whose pointer to push</param>
        /// <param name="window">: The window, where to push the process pointer to</param>
        /// <returns></returns>
        export DFL_API inline void DFL_CALL PushProcess(WindowProcess& process, Window& window);
    }
}