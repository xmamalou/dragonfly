/*Copyright 2022 Christopher-Marios Mamaloukas

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation and/or other materials 
provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors 
may be used to endorse or promote products derived from this software without 
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#include "../include/render.h"
#include <stdio.h>

int dlfWindowIniting(const char* windowName, int width, int height, int fullscreen)
{
    dlfWindowName = windowName;
    
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
        return DLF_SDLV_INIT_ERROR;

    if(SDL_GetDesktopDisplayMode(0, dlfDisplay))
        return DLF_SDL_MONITOR_INFO_ERROR;

    if(!fullscreen)
        dlfWindow = SDL_CreateWindow(windowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
    else 
        dlfWindow = SDL_CreateWindow(windowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 2560, 1440, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);

    if(dlfWindow == NULL)
        return DLF_SDL_WINDOW_INIT_ERROR;

    //dlfSurface = SDL_GetWindowSurface(dlfWindow);
    return DLF_SUCCESS;
}

void dlfWindowKilling()
{
    //SDL_FreeSurface(dlfSurface);
    SDL_DestroyWindow(dlfWindow);
    SDL_Quit();
}

int dlfVulkanIniting()
{
    int error = 0;
    if(error = dlfVulkInstanceMaking())
        return error;
    
    return DLF_SUCCESS;
}

void dlfVulkanKilling()
{
    vkDestroyInstance(*dlfVulkInstance, NULL);
}

int dlfVulkInstanceMaking()
{
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = dlfWindowName,
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "Dragonfly",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = VK_API_VERSION_1_3
    };

    if(SDL_Vulkan_GetInstanceExtensions(dlfWindow, &dlfExtensionsCount, NULL) == SDL_FALSE)
        return DLF_SDL_VULKEXTENS_ENUM_ERROR;

    if(SDL_Vulkan_GetInstanceExtensions(dlfWindow, &dlfExtensionsCount, dlfExtensions) == SDL_FALSE)
        return DLF_SDL_VULKEXTENS_ERROR;
    
    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = dlfExtensionsCount,
        .ppEnabledExtensionNames = dlfExtensions,
        .enabledLayerCount = 0
    };

    if(vkCreateInstance(&instanceInfo, NULL, dlfVulkInstance) != VK_SUCCESS)
        return DLF_SDL_VULKINSTANCE_ERROR;

    return DLF_SUCCESS;
}