/*
    Copyright 2022 Christopher-Marios Mamaloukas

    Redistribution and use in source and binary forms, with or without modification,
    are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation and/or
    other materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its contributors
    may be used to endorse or promote products derived from this software without
    specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
    BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
    EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "Vulkan.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <GLFW/glfw3.h>

#include "../Data.h" 

struct DflLocalMem_T {
    uint64_t size;
    uint32_t heapIndex;
};

struct DflSession_T {
    struct DflSessionInfo       info;
    VkInstance                  instance;
    VkDevice                    GPU;
    VkDebugUtilsMessengerEXT    messenger;

    VkPhysicalDevice* devices;
    int               deviceCount;
    int               choice;

    VkDevice          device;

    int error; // Error code

    // Presentation
    
    bool          canPresent;

    VkSurfaceKHR* surfaces; // multiple surfaces for multiple windows
    int           surfaceCount;

    /* -------------------- *
     *   GPU PROPERTIES     *
     * -------------------- */
    
    // memory

    uint32_t              localHeaps;
    struct DflLocalMem_T* localMem;

    // capabilities 

    uint32_t maxDim1D;
    uint32_t maxDim2D;
    uint32_t maxDim3D;

    bool canDoGeomShade;
    bool canDoTessShade;

    // GPU flags

    bool doLowPerf;
    bool doAbuseMemory;
};

/* -------------------- *
 *   INTERNAL           *
 * -------------------- */

inline static struct DflSession_T* dflSessionAllocHIDN();
inline static struct DflSession_T* dflSessionAllocHIDN() {
    return calloc(1, sizeof(struct DflSession_T));
}

static VKAPI_ATTR VkBool32 VKAPI_CALL dflDebugCLBK_HIDN(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* data, void* userData);
static VKAPI_ATTR VkBool32 VKAPI_CALL dflDebugCLBK_HIDN(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* data, void* userData)
{
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        printf("VULKAN INFORMS: %s\n", data->pMessage);
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        printf("VULKAN WARNS: %s\n", data->pMessage);
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        printf("VULKAN ENCOUNTERED AN ERROR: %s\n", data->pMessage);
    return VK_FALSE;
}

static int dflSessionVulkanInstanceInitHIDN(struct DflSessionInfo* info, VkInstance* instance, VkDebugUtilsMessengerEXT* pMessenger, int flags);
static int dflSessionVulkanInstanceInitHIDN(struct DflSessionInfo* info, VkInstance* instance, VkDebugUtilsMessengerEXT* pMessenger, int flags) 
{
    int extensionCount = 0;
    const char** extensions = calloc(1, sizeof(const char*)); // this will make sure that `realloc` in line 140 will work. If onscreen rendering is enabled, it will get replaced by a non NULL pointer anyways, so `realloc` will still work.

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = info->appName,
        .applicationVersion = VK_MAKE_API_VERSION(0,0,1,0),
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
        extensions[0] = "VK_KHR_surface";
    }

    if (flags & DFL_SESSION_CRITERIA_DO_DEBUG)
    {
        extensionCount++;
        const char** dummy = (const char**)realloc(extensions, sizeof(const char*) * extensionCount);
        if (dummy == NULL)
            return DFL_VULKAN_NOT_INITIALIZED;
        extensions = dummy;
        extensions[extensionCount - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

        const char* desiredLayers[] = {
            "VK_LAYER_KHRONOS_validation" };

        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, NULL);
        if (!layerCount)
            return DFL_NO_LAYERS_FOUND;
        VkLayerProperties* layers = calloc(layerCount, sizeof(VkLayerProperties));
        vkEnumerateInstanceLayerProperties(&layerCount, layers);

        if (layers == NULL)
            return DFL_NO_LAYERS_FOUND;

        for (int j = 0; j < 1; j++)
        {
            for (int i = 0; i < layerCount; i++)
            {
                if (strcmp(layers[i].layerName, desiredLayers[j]) == 0)
                    break;
                if (i == layerCount - 1)
                    return DFL_NO_LAYERS_FOUND;
            }
        }

        instInfo.enabledLayerCount = 1;
        instInfo.ppEnabledLayerNames = (const char**)desiredLayers;
    }
    else
    {
        instInfo.enabledLayerCount = 0;
    }

    instInfo.enabledExtensionCount = extensionCount;
    instInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&instInfo, NULL, instance) != VK_SUCCESS)
        return DFL_VULKAN_NOT_INITIALIZED;

    if (flags & DFL_SESSION_CRITERIA_DO_DEBUG) // if debug is enabled, create a debug messenger (enables all severity messages, except verbose)
    {
        VkDebugUtilsMessengerCreateInfoEXT debugInfo = {
           .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
           .pNext = NULL,
           .flags = 0,
           .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
           .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
           .pfnUserCallback = dflDebugCLBK_HIDN,
           .pUserData = NULL
        };
        PFN_vkCreateDebugUtilsMessengerEXT createDebug = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT");
        if(createDebug == NULL)
            return DFL_NO_LAYERS_FOUND;
        
        if(createDebug(*instance, &debugInfo, NULL, pMessenger) != VK_SUCCESS)
            return DFL_NO_LAYERS_FOUND;
    }

    return DFL_SUCCESS;
}

// just a helper function that fills GPU specific information in DflSession_T. When ranking devices,
// Dragonfly will always sort each checked device using this function, unless the previous device
// was already ranked higher. This will help avoid calling this function too many times.
static int dflSessionDeviceOrganiseDataHIDN(int GPUCrit, int deviceCount, struct DflSession_T** session);
static int dflSessionDeviceOrganiseDataHIDN(int GPUCrit, int deviceCount, struct DflSession_T** session)
{
    VkDeviceCreateInfo deviceInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = NULL,
            .flags = NULL
    };
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties((*session)->devices[deviceCount], &props);
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties((*session)->devices[deviceCount], &memProps);
    VkPhysicalDeviceFeatures feats;
    vkGetPhysicalDeviceFeatures((*session)->devices[deviceCount], &feats);

    for (int i = 0; i < memProps.memoryHeapCount; i++)
    {
        if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            (*session)->localHeaps++;
            void* dummy = realloc((*session)->localMem, (*session)->localHeaps * sizeof(struct DflLocalMem_T));
            if (dummy == NULL)
                return DFL_ALLOC_ERROR;
            (*session)->localMem = dummy;
            (*session)->localMem->size = memProps.memoryHeaps[i].size;
            (*session)->localMem->heapIndex = memProps.memoryTypes[i].heapIndex;
        }
    }

    (*session)->maxDim1D = props.limits.maxImageDimension1D;
    (*session)->maxDim2D = props.limits.maxImageDimension2D;
    (*session)->maxDim3D = props.limits.maxImageDimension3D;

    (*session)->canDoGeomShade = feats.geometryShader;
    (*session)->canDoTessShade = feats.tessellationShader;

    return DFL_SUCCESS;
}

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

DflSession dflSessionInit(struct DflSessionInfo* pInfo, int sessionCriteria, int GPUCriteria)
{
    struct DflSession_T* session = dflSessionAllocHIDN();

    if (pInfo == NULL)
    {
        pInfo = calloc(1, sizeof(struct DflSessionInfo));
        pInfo->appName = "Dragonfly-App";
        pInfo->appVersion = VK_MAKE_VERSION(1, 0, 0);
    }
    else
        session->info = *pInfo;

    if (GPUCriteria & DFL_GPU_CRITERIA_LOW_PERFORMANCE)
        session->doLowPerf = true;

    if (GPUCriteria & DFL_GPU_CRITERIA_ABUSE_MEMORY)
        session->doAbuseMemory = true;

    if (sessionCriteria & DFL_SESSION_CRITERIA_ONLY_OFFSCREEN)
        session->canPresent = false;
    else
        session->canPresent = true;

    if (dflSessionVulkanInstanceInitHIDN(&(session->info), &(session->instance), &(session->messenger), sessionCriteria) != DFL_SUCCESS)
        return NULL;

    vkEnumeratePhysicalDevices(session->instance, &session->deviceCount, NULL);
    session->devices = calloc(session->deviceCount, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(session->instance, &session->deviceCount, session->devices);

    switch (session->deviceCount)
    {
    case 0:
        return NULL;
    case 1:
        if (dflSessionDeviceOrganiseDataHIDN(GPUCriteria, 0, &session) != DFL_SUCCESS)
            return NULL;
        break;
    default:
        if (GPUCriteria & DFL_GPU_CRITERIA_HASTY)
            if (dflSessionDeviceOrganiseDataHIDN(GPUCriteria, 0, &session) != DFL_SUCCESS)
                return NULL;
        break;
    }

    return (DflSession)session;
}

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

bool dflSessionCanPresentGet(DflSession session)
{
    return ((struct DflSession_T*)session)->canPresent;
}

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void dflSessionEnd(DflSession* pSession)
{
    if (((struct DflSession_T*)*pSession)->messenger != NULL)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT destroyDebug = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(((struct DflSession_T*)*pSession)->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyDebug != NULL)
            destroyDebug(((struct DflSession_T*)*pSession)->instance, ((struct DflSession_T*)*pSession)->messenger, NULL);
    }
    vkDestroyInstance(((struct DflSession_T*)*pSession)->instance, NULL);

    free(*pSession);
}