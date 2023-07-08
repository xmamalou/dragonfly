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
#include "../Internal.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#ifdef _WIN32
#include <Windows.h>
#pragma comment(lib, "user32.lib")
#endif

/* -------------------- *
 *   INTERNAL           *
 * -------------------- */

inline static struct DflSession_T* _dflSessionAlloc();
inline static struct DflSession_T* _dflSessionAlloc() {
    return calloc(1, sizeof(struct DflSession_T));
}

static VKAPI_ATTR VkBool32 VKAPI_CALL _dflDebugCLBK(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* data, void* userData);
static VKAPI_ATTR VkBool32 VKAPI_CALL _dflDebugCLBK(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* data, void* userData)
{
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        printf("\n\x1b[31mVULKAN ENCOUNTERED AN ERROR\n===ERROR===\x1b[0m\n%s\n", data->pMessage);
        return VK_FALSE;
    }
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        printf("\n\x1b[33mVULKAN WARNS\n===WARNING===\x1b[0m\n%s\n", data->pMessage);
        return VK_FALSE;
    }
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        printf("\n\x1b[35mVULKAN INFORMS\n===MESSAGE===\x1b[0m\n%s\n", data->pMessage);
        return VK_FALSE;
    }

    return VK_FALSE;
}

static int _dflSessionVulkanInstanceInit(struct DflSessionInfo* info, VkInstance* instance, VkDebugUtilsMessengerEXT* pMessenger, int flags);
static int _dflSessionVulkanInstanceInit(struct DflSessionInfo* info, VkInstance* instance, VkDebugUtilsMessengerEXT* pMessenger, int flags) 
{
    int extensionCount = 0;
    const char** extensions = calloc(1, sizeof(const char*)); // this will make sure that `realloc` in line 140 will work. If onscreen rendering is enabled, it will get replaced by a non NULL pointer anyways, so `realloc` will still work.
    if (extensions == NULL) {
        return DFL_GENERIC_OOM_ERROR;
    }

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = info->pAppName,
        .applicationVersion = info->appVersion,
        .pEngineName = "Dragonfly",
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .apiVersion = VK_API_VERSION_1_3
    };
    VkInstanceCreateInfo instInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pApplicationInfo = &appInfo
    };

    if (!(flags & DFL_SESSION_CRITERIA_ONLY_OFFSCREEN))
    {
        extensionCount++;
        extensions[extensionCount - 1] = "VK_KHR_surface";

#ifdef _WIN32 // Windows demands an extra extension for surfaces to work.
        extensionCount++;
        const char** dummy = (const char**)realloc(extensions, sizeof(const char*) * extensionCount);
        if (dummy == NULL)
            return DFL_GENERIC_OOM_ERROR;
        extensions = dummy;
        extensions[extensionCount - 1] = "VK_KHR_win32_surface";
#endif
    }

    int desiredLayerCount = 1;
    const char* desiredLayers[] = {
            "VK_LAYER_KHRONOS_validation" };

    if (flags & DFL_SESSION_CRITERIA_DO_DEBUG)
    {
        extensionCount++;
        const char** dummy = (const char**)realloc(extensions, sizeof(const char*) * extensionCount);
        if (dummy == NULL)
            return DFL_GENERIC_OOM_ERROR;
        extensions = dummy;
        extensions[extensionCount - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

        uint32_t layerCount = 0;
        VkLayerProperties* layers = NULL;
        vkEnumerateInstanceLayerProperties(&layerCount, NULL);
        if (!layerCount) 
            return DFL_VULKAN_LAYER_ERROR;
        layers = calloc(layerCount, sizeof(VkLayerProperties));
        if (layers == NULL)
            return DFL_GENERIC_OOM_ERROR;
        vkEnumerateInstanceLayerProperties(&layerCount, layers);

        for (int j = 0; j < desiredLayerCount; j++)
        {
            for (int i = 0; i < layerCount; i++)
            {
                if (strcmp(layers[i].layerName, desiredLayers[j]) == 0)
                    break;
                if (i == layerCount - 1)
                    return DFL_VULKAN_LAYER_ERROR;
            }
        }
    }
    instInfo.enabledLayerCount = !(flags & DFL_SESSION_CRITERIA_DO_DEBUG) ? 0 : desiredLayerCount;
    instInfo.ppEnabledLayerNames = !(flags & DFL_SESSION_CRITERIA_DO_DEBUG) ?  NULL : (const char**)desiredLayers;

    instInfo.enabledExtensionCount = extensionCount;
    instInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&instInfo, NULL, instance) != VK_SUCCESS)
    {
        return DFL_VULKAN_INSTANCE_ERROR;
    }

    if (flags & DFL_SESSION_CRITERIA_DO_DEBUG)
    {
        VkDebugUtilsMessengerCreateInfoEXT debugInfo = {
           .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
           .pNext = NULL,
           .flags = 0,
           .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
           .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
           .pfnUserCallback = _dflDebugCLBK,
           .pUserData = NULL
        };
        PFN_vkCreateDebugUtilsMessengerEXT createDebug = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT");
        if (createDebug == NULL)
        {
            return DFL_VULKAN_DEBUG_ERROR;
        }
        
        if (createDebug(*instance, &debugInfo, NULL, pMessenger) != VK_SUCCESS)
        {
            return DFL_VULKAN_DEBUG_ERROR;
        }
    }

    return DFL_SUCCESS;
}

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

DflSession dflSessionCreate(struct DflSessionInfo* pInfo)
{
    struct DflSession_T* session = _dflSessionAlloc();

    if (pInfo == NULL)
    {
        struct DflSessionInfo info =
        {
            .pAppName = "Dragonfly-App",
            .appVersion = VK_MAKE_VERSION(1, 0, 0),
            .sessionCriteria = NULL
        };
        session->info = info;
    }
    else
        session->info = *pInfo;

    
    session->error = _dflSessionVulkanInstanceInit(&(session->info), &(session->instance), &(session->messenger), session->info.sessionCriteria);

//gather system info
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    session->processorCount = (sysInfo.dwNumberOfProcessors)/2; // usually, the number of physical cores is half the number of logical cores.
    session->threadCount = sysInfo.dwNumberOfProcessors;

    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    session->memorySize = memInfo.ullTotalPhys;
#else
    // TODO: Implement for POSIX platforms.
#endif

    return (DflSession)session;
}

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

int dflSessionErrorGet(DflSession hSession)
{
    return DFL_SESSION->error;
}

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void dflSessionDestroy(DflSession hSession)
{
    if (DFL_SESSION->messenger != NULL) // that means debugging was enabled.
    {
        PFN_vkDestroyDebugUtilsMessengerEXT destroyDebug = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(DFL_SESSION->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyDebug != NULL)
            destroyDebug(DFL_SESSION->instance, DFL_SESSION->messenger, NULL);
    }
    vkDestroyInstance(DFL_SESSION->instance, NULL);
    free(hSession);
}