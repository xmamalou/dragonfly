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
#include <string.h>
#include <stdio.h>

int dflWindowIniting(const char* windowName, int width, int height, int fullscreen)
{
    dflWindowName = windowName;
    
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
        return DFL_SDLV_INIT_ERROR;

    if(SDL_GetDesktopDisplayMode(0, dflDisplay))
        return DFL_SDL_MONITOR_INFO_ERROR;

    if(!fullscreen)
        dflWindow = SDL_CreateWindow(windowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
    else 
        dflWindow = SDL_CreateWindow(windowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 2560, 1440, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);

    if(dflWindow == NULL)
        return DFL_SDL_WINDOW_INIT_ERROR;

    //dflSurface = SDL_GetWindowSurface(dflWindow);
    return DFL_SUCCESS;
}

void dflWindowKilling()
{
    //SDL_FreeSurface(dflSurface);
    SDL_DestroyWindow(dflWindow);
    SDL_Quit();

    free(dflDisplay);
}

// VULKAN FUNCTIONS

int dflVulkanIniting()
{
    int error = 0;

    #ifndef DFL_NO_DEBUG 
        if(error = dflVulkLayersMaking())
            return error;
    #endif
    if(error = dflVulkInstanceMaking())
        return error;
    
    return DFL_SUCCESS;
}

void dflVulkanKilling()
{
    vkDestroyInstance(*dflVulkInstance, NULL);

    dflVectorClearing(&dflVulkExtensions);
    #ifndef DFL_NO_DEBUG
        dflVectorClearing(&dflVulkLayers);
    #endif
}

// VULKAN SUBFUNCTIONS 

static int dflVulkInstanceMaking()
{
    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = dflWindowName,
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "Dragonfly",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = VK_API_VERSION_1_3
    }; 
    
    int error = 0;
    if(error = dflVulkExtensionSupplying())
        return error;

    VkInstanceCreateInfo instanceInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = dflVulkExtensions.size,
        .ppEnabledExtensionNames = dflVulkExtensions.data,
    };

    #ifndef DFL_NO_DEBUG
        instanceInfo.enabledLayerCount = dflVulkLayers.size;
        instanceInfo.ppEnabledExtensionNames = dflVulkLayers.data;
    #else
        instanceInfo.enabledExtensionCount = 0;
    #endif

    if(vkCreateInstance(&instanceInfo, NULL, dflVulkInstance) != VK_SUCCESS)
        return DFL_VULKINSTANCE_ERROR;

    return DFL_SUCCESS;
}

#ifndef DFL_NO_DEBUG
static int dflVulkLayersMaking()
{
    // we insert one layer here
    dflVectorPushing(&dflVulkLayers, "VK_LAYER_KHRONOS_validation");

    int error = 0;

    if(error = dflVulkValLayersVerifying())
        return error;

    return DFL_SUCCESS;
}
#endif

// VULKAN SUBSUBFUNCTIONS

#ifndef DFL_NO_DEBUG
static int dflVulkValLayersVerifying()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    VkLayerProperties* layers = malloc(sizeof(VkLayerProperties) * layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers);

    short layerFound = 0;
    for(int j = 0; j < dflVulkLayers.size; j++) // size of array / size of pointer = number of items in array
    {
        layerFound = 0;
        for(int i = 0; i < layerCount; i++)
        {
            if(strcmp(layers[i].layerName, dflVulkLayers.data[j]) == 0)
            {
                layerFound = 1;
                break;
            }
        }

        if(layerFound == 0)
            return DFL_VULKLAYERS_ERROR;
    }

    return DFL_SUCCESS;
}
#endif

static int dflVulkExtensionSupplying()
{
    unsigned int extensionCount = 0;
    if(SDL_Vulkan_GetInstanceExtensions(dflWindow, &extensionCount, NULL) == SDL_FALSE)
        return DFL_SDL_VULKEXTENS_ENUM_ERROR;
    dflVectorExtending(&dflVulkExtensions, extensionCount);
    if(SDL_Vulkan_GetInstanceExtensions(dflWindow, &extensionCount, dflVulkExtensions.data) == SDL_FALSE)
        return DFL_SDL_VULKEXTENS_ERROR;

    #ifndef DFL_NO_DEBUG
        dflVectorPushing(&dflVulkExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif

    return DFL_SUCCESS;
}