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
#include <tuple>
#include <memory>
#include <string>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <Windows.h>

module Dragonfly.Observer:Window;

namespace DflOb = Dfl::Observer;

DflOb::WindowFunctor::WindowFunctor(const WindowInfo& info) : Info(info), Error(DflOb::WindowError::ThreadNotReadyWarning)
{
}

DflOb::WindowFunctor::~WindowFunctor()
{
    
}

void DflOb::WindowFunctor::operator() ()
{
    if (!glfwInit())
    {
        this->Error = DflOb::WindowError::APIInitError;
        return;
    }

    glfwWindowHint(GLFW_NO_API, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    this->pGLFWwindow = glfwCreateWindow(
        (std::get<0>(this->Info.Resolution) == 0) ? DflOb::DefaultWidth : std::get<0>(this->Info.Resolution),
        (std::get<1>(this->Info.Resolution) == 0) ? DflOb::DefaultHeight : std::get<1>(this->Info.Resolution),
        (this->Info.WindowTitle.empty()) ? "Dragonfly App" : this->Info.WindowTitle.data(),
        (this->Info.DoFullscreen) ? glfwGetPrimaryMonitor() : nullptr,
        nullptr);

    if (this->pGLFWwindow == nullptr)
    {
        this->Error = DflOb::WindowError::WindowInitError;
        return;
    }

    this->Error = DflOb::WindowError::Success;

    this->hWin32Window = glfwGetWin32Window(this->pGLFWwindow);

    this->ShouldClose = false;

    while ((!glfwWindowShouldClose(this->pGLFWwindow)) && (!this->ShouldClose))
    {
        glfwPollEvents();
        this->AccessProcess.lock();
        for (auto process : this->Info.pProcesses)
            (*process)(this->Arguments);
        this->AccessProcess.unlock();
        Sleep(1000/this->Info.Rate);
    }

    this->AccessProcess.lock();
    for (auto& process : this->Info.pProcesses)
        process->Destroy();
    this->AccessProcess.unlock();

    this->ShouldClose = true; 

    glfwDestroyWindow(this->pGLFWwindow);
    glfwTerminate();

    return;
}

// Window stuff

DflOb::Window::Window(const WindowInfo& createInfo) : Functor(createInfo)
{
}

DflOb::Window::~Window()
{
    this->Functor.ShouldClose = true;
    this->Thread.join();
    this->Functor.~WindowFunctor();
}

DflOb::WindowError DflOb::Window::OpenWindow()
{
    if (this->Functor.hWin32Window == nullptr)
    {
        this->Thread = std::thread::thread(std::ref(this->Functor));
        while(this->Functor.Error == DflOb::WindowError::ThreadNotReadyWarning)
            Sleep(50); // we are waiting for the window to be ready
        return this->Functor.Error;
    }
    // we don't need to do anything if the above doesn't hold true

    return WindowError::Success;
}

inline void DflOb::PushProcess(WindowProcess& process, Window& window)
{
    window.Functor.AccessProcess.lock();
    window.Functor.Info.pProcesses.push_back(&process);
    window.Functor.AccessProcess.unlock();
};