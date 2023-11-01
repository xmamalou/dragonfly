#include <tuple>
#include <string>
#include <thread>

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
        //export const std::tuple<uint32_t, uint32_t> DefaultResolution = std::make_tuple(1920, 1080);

        // a function pointer to processes meant to be executed during a Window's existence.
        export typedef void (*WindowProcess)(void* args);

        /*!
         * \brief Information used for initializing a Window
        */
        export struct WindowInfo
        {
            std::tuple<uint32_t, uint32_t> Resolution{ std::make_tuple(1920, 1080) }; // the dimensions of the window and the image resolution
            std::tuple<uint32_t, uint32_t> View{ std::make_tuple(1920, 1080) }; // render area of the window
            bool                           DoFullscreen{ false };

            bool     DoVsync{ true };
            uint32_t Rate{ 60 }; // Refresh rate, in Hz

            std::tuple<int, int> Position{ std::make_tuple(0, 0) }; // Relative to the monitor
            std::string_view     WindowTitle{ "Dragonfly App" };

            WindowProcess        Process{ nullptr }; // the process that will execute in the thread concerning the window
            void*                pArguments{ nullptr }; // the arguments for the process that will execute in the thread concerning the window

            HWND hWindow; // instead of creating a new window, set this to render to children windows
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

            WindowError Error{ WindowError::Success };
            bool        ShouldClose{ false };

            friend class Window; // Windows should be able to access everything about them
        public:
            WindowFunctor(const WindowInfo& info);
            ~WindowFunctor();

            void operator() ();
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
        };
    }
}