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
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

module Dragonfly.Observer.Window;

namespace DflOb = Dfl::Observer;

// this is required to not terminate GLFW whilst multiple windows are 
// in use and may not go out of scope simultaneously
uint32_t INT_GLB_GlfwAPIInits{ 0 }; 

// Window stuff

inline void DflOb::Window::PollEvents() {
    glfwPollEvents();
}

DflOb::WindowHandles INT_GetHandles(const DflOb::WindowInfo info) {
    if ( !glfwInit() ) {
        throw DflOb::WindowError::GlfwAPIInitError;
    };

    INT_GLB_GlfwAPIInits++;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* const window{ glfwCreateWindow(
                                    info.Resolution[0],
                                    info.Resolution[1],
                                    (const char*)info.WindowTitle.data(),
                                    (info.DoFullscreen) ? glfwGetPrimaryMonitor() : nullptr,
                                    nullptr) };

    return { window, glfwGetWin32Window(window) };
}

DflOb::Window::Window(const WindowInfo& info)
try : pInfo( new WindowInfo(info) ),
      Handles( INT_GetHandles(info) ) {  
}
catch (WindowError e) { this->Error = e; }

DflOb::Window::~Window() {
    glfwDestroyWindow(this->Handles.pGLFWwindow);
    INT_GLB_GlfwAPIInits--;
    if( INT_GLB_GlfwAPIInits == 0 ) { glfwTerminate(); }
}

inline const bool DflOb::Window::GetCloseStatus() const noexcept {
    return glfwWindowShouldClose(this->Handles.pGLFWwindow);
}