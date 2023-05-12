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

#define DFL_QUEUE_TYPE_GRAPHICS 0
#define DFL_QUEUE_TYPE_COMPUTE 1
#define DFL_QUEUE_TYPE_TRANSFER 2
#define DFL_QUEUE_TYPE_PRESENTATION 3

struct DflQueueCollection_T {
    VkQueue* graphics;
    VkQueue* compute;
    VkQueue* transfer;
    VkQueue* presentation;

    uint32_t graphicsCount;
    uint32_t computeCount;
    uint32_t transferCount;
    uint32_t presentationCount;

    uint32_t graphicsIndex;
    uint32_t computeIndex;
    uint32_t transferIndex;
    uint32_t presentationIndex;
};

struct DflLocalMem_T {
    uint64_t size;
    uint32_t heapIndex;
};

// this is some data about the device that will be used.
// TODO: More fields to be added.
struct DflDevice_T {
    VkPhysicalDevice physDevice;

    /* -------------------- *
     *   GPU PROPERTIES     *
     * -------------------- */

    char name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];

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

struct DflSession_T {
    struct DflSessionInfo       info;
    VkInstance                  instance;
    VkDevice                    GPU;
    VkDebugUtilsMessengerEXT    messenger;

    struct DflDevice_T          physDeviceData;

    VkDevice                    device;
    struct DflQueueCollection_T queues;

    int error; // Error code

    // Presentation
    
    bool          canPresent;

    VkSurfaceKHR* surfaces; // multiple surfaces for multiple windows
    int           surfaceCount;
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

    const char* desiredLayers[] = {
            "VK_LAYER_KHRONOS_validation" };

    if (flags & DFL_SESSION_CRITERIA_DO_DEBUG)
    {
        extensionCount++;
        const char** dummy = (const char**)realloc(extensions, sizeof(const char*) * extensionCount);
        if (dummy == NULL)
            return DFL_VULKAN_NOT_INITIALIZED;
        extensions = dummy;
        extensions[extensionCount - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

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
    }
    instInfo.enabledLayerCount = !(flags & DFL_SESSION_CRITERIA_DO_DEBUG) ? 0 : 1;
    instInfo.ppEnabledLayerNames = !(flags & DFL_SESSION_CRITERIA_DO_DEBUG) ?  NULL : (const char**)desiredLayers;

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
static int dflSessionDeviceOrganiseDataHIDN(int GPUCrit, int deviceCount, struct DflDevice_T* device);
static int dflSessionDeviceOrganiseDataHIDN(int GPUCrit, int deviceCount, struct DflDevice_T* device)
{
    VkDeviceCreateInfo deviceInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = NULL,
            .flags = NULL
    };
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device->physDevice, &props);
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(device->physDevice, &memProps);
    VkPhysicalDeviceFeatures feats;
    vkGetPhysicalDeviceFeatures(device->physDevice, &feats);

    strcpy_s(&device->name, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE, &props.deviceName);

    for (int i = 0; i < memProps.memoryHeapCount; i++)
    {
        if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            device->localHeaps++;
            void* dummy = realloc(device->localMem, device->localHeaps * sizeof(struct DflLocalMem_T));
            if (dummy == NULL)
                return DFL_ALLOC_ERROR;
            device->localMem = dummy;
            device->localMem->size = memProps.memoryHeaps[i].size;
            device->localMem->heapIndex = memProps.memoryTypes[i].heapIndex;
        }
    }

    device->maxDim1D = props.limits.maxImageDimension1D;
    device->maxDim2D = props.limits.maxImageDimension2D;
    device->maxDim3D = props.limits.maxImageDimension3D;

    device->canDoGeomShade = feats.geometryShader;
    device->canDoTessShade = feats.tessellationShader;

    return DFL_SUCCESS;
}


static int dflSessionDeviceQueuesGetHIDN(VkSurfaceKHR* surface, struct DflSession_T* session);
static int dflSessionDeviceQueuesGetHIDN(VkSurfaceKHR* surface, struct DflSession_T* session)
{
    session->queues.graphics = NULL;
    session->queues.compute = NULL;
    session->queues.transfer = NULL;
    session->queues.presentation = NULL;

    VkQueueFamilyProperties* queueProps = NULL;
    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(session->physDeviceData.physDevice, &queueCount, NULL);
    if (queueCount == 0)
        return DFL_NO_QUEUES_FOUND;
    queueProps = calloc(queueCount, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(session->physDeviceData.physDevice, &queueCount, queueProps);

    for (int i = 0; i < queueCount; i++)
    {
        if (queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            session->queues.graphicsIndex = i;
            session->queues.graphicsCount = queueProps[i].queueCount;
            continue;
        }
        if (queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            session->queues.computeIndex = i;
            session->queues.computeCount = queueProps[i].queueCount;
            continue;
        }
        if (queueProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            session->queues.transferIndex = i;
            session->queues.transferCount = queueProps[i].queueCount;
            continue;
        }
    }
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

    session->physDeviceData.doLowPerf = (sessionCriteria & DFL_GPU_CRITERIA_LOW_PERFORMANCE) ? true : false;
    session->physDeviceData.doAbuseMemory = (sessionCriteria & DFL_GPU_CRITERIA_ABUSE_MEMORY) ? true : false;
    session->canPresent = (sessionCriteria & DFL_SESSION_CRITERIA_ONLY_OFFSCREEN) ? false : true;

    if (dflSessionVulkanInstanceInitHIDN(&(session->info), &(session->instance), &(session->messenger), sessionCriteria) != DFL_SUCCESS)
        return NULL;

    VkPhysicalDevice* devices = NULL;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(session->instance, &deviceCount, NULL);
    devices = calloc(deviceCount, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(session->instance, &deviceCount, devices);

    switch (deviceCount) // this just selects the device it thinks is appropriate, but doesn't actually make a VkDevice yet. Doesn't do any ranking if there's only one device (or DFL_GPU_CRITERIA_HASTY is set)
    {
    case 0:
        return NULL;
    case 1:
        session->physDeviceData.physDevice = devices[deviceCount - 1];
        if (dflSessionDeviceOrganiseDataHIDN(GPUCriteria, devices[deviceCount - 1], &(session->physDeviceData)) != DFL_SUCCESS)
            return NULL;
        break;
    default:
        if (GPUCriteria & DFL_GPU_CRITERIA_HASTY)
        {
            session->physDeviceData.physDevice = devices[deviceCount - 1];
            if (dflSessionDeviceOrganiseDataHIDN(GPUCriteria, devices[deviceCount - 1], &(session->physDeviceData)) != DFL_SUCCESS)
                return NULL;
        }
        break;
    }

    if (sessionCriteria & DFL_SESSION_CRITERIA_ONLY_OFFSCREEN)
        dflSessionDeviceInit(NULL, &session);

    return (DflSession)session;
}

void dflSessionDeviceInit(VkSurfaceKHR* surface, DflSession* pSession)
{
    dflSessionDeviceQueuesGetHIDN(surface, ((struct DflSession_T*)*pSession));
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