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

DflOb::WindowProcessArgs::WindowProcessArgs(const WindowInfo& info) 
    : pInfo(&info) { }

DflOb::WindowFunctor::WindowFunctor(const WindowInfo& info) 
    : pInfo(new WindowInfo(info)),
      pArguments(new WindowProcessArgs(info)) { }

DflOb::WindowFunctor::~WindowFunctor( ) { }

void DflOb::WindowFunctor::operator() ( ) {
    if (!glfwInit()) {
        this->Error = DflOb::WindowError::APIInitError;
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    if ( (this->pGLFWwindow = glfwCreateWindow(
                                this->pInfo->Resolution[0] == 0  ? DflOb::DefaultWidth 
                                                                 : this->pInfo->Resolution[0],
                                this->pInfo->Resolution[1] == 0  ? DflOb::DefaultHeight 
                                                                 : this->pInfo->Resolution[1],
                                this->pInfo->WindowTitle.empty() ? "Dragonfly App" 
                                                                 : (const char*)this->pInfo->WindowTitle.data(),
                                this->pInfo->DoFullscreen        ? glfwGetPrimaryMonitor() 
                                                                 : nullptr,
                                nullptr)) == nullptr ){
        this->Error = DflOb::WindowError::WindowInitError;
        return;
    }

    this->Error = DflOb::WindowError::Success;
    this->hWin32Window = glfwGetWin32Window(const_cast<GLFWwindow*>(this->pGLFWwindow));
    this->ShouldClose = false;

    this->pArguments->CurrentTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    auto currentFrameTimeInit{ std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()) };
    auto currentFrameTimePoint{ std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()) };
    while ( !glfwWindowShouldClose(const_cast<GLFWwindow*>(this->pGLFWwindow)) ){
        this->pArguments->CurrentTime = (currentFrameTimePoint - currentFrameTimeInit).count();

        glfwPollEvents();

        this->AccessProcess.lock();
        for(auto process : this->pInfo->pProcesses)
            (*process)( *(this->pArguments) );
        this->AccessProcess.unlock();

        currentFrameTimePoint = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
        this->pArguments->LastFrameTime = (currentFrameTimePoint - currentFrameTimeInit).count() - this->pArguments->CurrentTime;

        if( this->pInfo->DoVsync ){
            std::this_thread::sleep_for(std::chrono::microseconds(1000000/this->pInfo->Rate -this->pArguments->LastFrameTime));

            currentFrameTimePoint = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
            this->pArguments->LastFrameTime = (currentFrameTimePoint - currentFrameTimeInit).count() - this->pArguments->CurrentTime;
        }
    }

    this->AccessProcess.lock();
    for (auto& process : this->pInfo->pProcesses)
        process->Destroy();
    this->AccessProcess.unlock();
    this->ShouldClose = true;

    glfwDestroyWindow(const_cast<GLFWwindow*>(this->pGLFWwindow));
    glfwTerminate();

    return;
}

// Window stuff

DflOb::Window::Window(const WindowInfo& createInfo)
try : Functor(createInfo),
      Thread(std::ref(this->Functor)) {
    while (this->Functor.Error == DflOb::WindowError::ThreadNotReadyWarning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    };     
}
catch (...) { this->Functor.Error = WindowError::ThreadCreationError; }

DflOb::Window::~Window() noexcept{
    this->Functor.ShouldClose = true;
    this->Thread.join();
}