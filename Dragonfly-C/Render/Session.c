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
#include "Session.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <GLFW/glfw3.h>

#include "../Data.h"

#define DFL_QUEUE_TYPE_GRAPHICS 0
#define DFL_QUEUE_TYPE_COMPUTE 1
#define DFL_QUEUE_TYPE_TRANSFER 2
#define DFL_QUEUE_TYPE_PRESENTATION 3

#define DFL_ACTION_IS_LEGAL 1

struct DflQueueCollection_T { // the order of the queues is: graphics, compute, transfer, presentation (see up)
    VkQueue* handles[4]; 

    uint32_t count[4];

    uint32_t index[4];
};


struct DflLocalMem_T {
    uint64_t size;
    uint32_t heapIndex;
};

// this is some data about the device that will be used.
// TODO: More fields to be added.
struct DflDevice_T {
    VkDevice                    device;
    struct DflQueueCollection_T queues;
    VkPhysicalDevice            physDevice;

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

    // Presentation

    bool          canPresent;
};

struct DflPhysicalDeviceList_T
{
    VkPhysicalDevice* pDevices;
    uint32_t          count;
};

struct DflSession_T {
    struct DflSessionInfo       info;
    VkInstance                  instance;
    VkDebugUtilsMessengerEXT    messenger;

    VkSurfaceKHR* surfaces; // multiple surfaces for multiple windows
    int           surfaceCount;

    /* -------------------- *
     *  FLAGS               *
     * -------------------- */

     int flags;
};

/* -------------------- *
 *   INTERNAL           *
 * -------------------- */

inline static void dflSessionLegalizeHIDN(DflSession* pSession);
inline static void dflSessionLegalizeHIDN(DflSession* pSession) {
    DFL_HANDLE(Session)->flags |= DFL_ACTION_IS_LEGAL;
}

inline static void dflSessionIllegalizeHIDN(DflSession* pSession);
inline static void dflSessionIllegalizeHIDN(DflSession* pSession) {
    DFL_HANDLE(Session)->flags &= ~DFL_ACTION_IS_LEGAL;
}

inline static struct DflSession_T* dflSessionAllocHIDN();
inline static struct DflSession_T* dflSessionAllocHIDN() {
    return calloc(1, sizeof(struct DflSession_T));
}

inline static struct DflDevice_T* dflDeviceAllocHIDN();
inline static struct DflDevice_T* dflDeviceAllocHIDN() {
    return calloc(1, sizeof(struct DflDevice_T));
}

static VKAPI_ATTR VkBool32 VKAPI_CALL dflDebugCLBK_HIDN(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* data, void* userData);
static VKAPI_ATTR VkBool32 VKAPI_CALL dflDebugCLBK_HIDN(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* data, void* userData)
{
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        printf("VULKAN ENCOUNTERED AN ERROR: %s\n", data->pMessage);
        return VK_FALSE;
    }
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        printf("VULKAN WARNS: %s\n", data->pMessage);
        return VK_FALSE;
    }
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        printf("VULKAN INFORMS: %s\n", data->pMessage);
        return VK_FALSE;
    }

    return VK_FALSE;
}

static int dflSessionVulkanInstanceInitHIDN(struct DflSessionInfo* info, VkInstance* instance, VkDebugUtilsMessengerEXT* pMessenger, int flags);
static int dflSessionVulkanInstanceInitHIDN(struct DflSessionInfo* info, VkInstance* instance, VkDebugUtilsMessengerEXT* pMessenger, int flags) 
{
    int extensionCount = 0;
    const char** extensions = calloc(1, sizeof(const char*)); // this will make sure that `realloc` in line 140 will work. If onscreen rendering is enabled, it will get replaced by a non NULL pointer anyways, so `realloc` will still work.
    if (extensions == NULL) {
        if (info->sessionCriteria & DFL_SESSION_CRITERIA_DO_DEBUG)
            printf("DRAGONFLY ERROR: Out of memory! Aborting session creation");
        return DFL_GENERIC_OOM_ERROR;
    }

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = info->appName,
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

#ifdef _WIN32
        extensionCount++;
        const char** dummy = (const char**)realloc(extensions, sizeof(const char*) * extensionCount);
        if (dummy == NULL)
            return DFL_GENERIC_OOM_ERROR;
        extensions = dummy;
        extensions[extensionCount - 1] = "VK_KHR_win32_surface";
#endif
    }

    const char* desiredLayers[] = {
            "VK_LAYER_KHRONOS_validation" };

    if (flags & DFL_SESSION_CRITERIA_DO_DEBUG)
    {
        extensionCount++;
        const char** dummy = (const char**)realloc(extensions, sizeof(const char*) * extensionCount);
        if (dummy == NULL)
        {
            if (info->sessionCriteria & DFL_SESSION_CRITERIA_DO_DEBUG)
                printf("DRAGONFLY ERROR: Out of memory! Aborting session creation");
            return DFL_GENERIC_OOM_ERROR;
        }
        extensions = dummy;
        extensions[extensionCount - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, NULL);
        if (!layerCount)
        {
            if (info->sessionCriteria & DFL_SESSION_CRITERIA_DO_DEBUG)
                printf("DRAGONFLY ERROR: Couldn't find any layers! Aborting session creation");
            return DFL_VULKAN_LAYER_ERROR;
        }
        VkLayerProperties* layers = calloc(layerCount, sizeof(VkLayerProperties));
        vkEnumerateInstanceLayerProperties(&layerCount, layers);

        if (layers == NULL)
        {
            if (info->sessionCriteria & DFL_SESSION_CRITERIA_DO_DEBUG)
                printf("DRAGONFLY ERROR: Couldn't find any layers! Aborting session creation");
            return DFL_VULKAN_LAYER_ERROR;
        }

        for (int j = 0; j < 1; j++)
        {
            for (int i = 0; i < layerCount; i++)
            {
                if (strcmp(layers[i].layerName, desiredLayers[j]) == 0)
                    break;
                if (i == layerCount - 1)
                {
                    if (info->sessionCriteria & DFL_SESSION_CRITERIA_DO_DEBUG)
                        printf("DRAGONFLY ERROR: Couldn't find desired layers! Aborting session creation");
                    return DFL_VULKAN_LAYER_ERROR;
                }
            }
        }
    }
    instInfo.enabledLayerCount = !(flags & DFL_SESSION_CRITERIA_DO_DEBUG) ? 0 : 1;
    instInfo.ppEnabledLayerNames = !(flags & DFL_SESSION_CRITERIA_DO_DEBUG) ?  NULL : (const char**)desiredLayers;

    instInfo.enabledExtensionCount = extensionCount;
    instInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&instInfo, NULL, instance) != VK_SUCCESS)
    {
        if (info->sessionCriteria & DFL_SESSION_CRITERIA_DO_DEBUG)
            printf("DRAGONFLY ERROR: Couldn't find desired extensions! Aborting session creation");
        return DFL_VULKAN_INSTANCE_ERROR;
    }

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
        if (createDebug == NULL)
        {
            if (info->sessionCriteria & DFL_SESSION_CRITERIA_DO_DEBUG)
                printf("DRAGONFLY ERROR: Couldn't find debugging functions! Aborting session creation");
            return DFL_VULKAN_DEBUG_ERROR;
        }
        
        if (createDebug(*instance, &debugInfo, NULL, pMessenger) != VK_SUCCESS)
        {
            if (info->sessionCriteria & DFL_SESSION_CRITERIA_DO_DEBUG)
                printf("DRAGONFLY ERROR: Couldn't initiate debugger! Aborting session creation");
            return DFL_VULKAN_DEBUG_ERROR;
        }
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
                return DFL_GENERIC_OOM_ERROR;
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


static int dflSessionDeviceQueuesGetHIDN(VkSurfaceKHR* surface, struct DflDevice_T* pDevice);
static int dflSessionDeviceQueuesGetHIDN(VkSurfaceKHR* surface, struct DflDevice_T* pDevice)
{
    for(int i = 0; i <= 3; i++)
        pDevice->queues.handles[i] = NULL;

    VkQueueFamilyProperties* queueProps = NULL;
    uint32_t queueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pDevice->physDevice, &queueCount, NULL);
    if (queueCount == 0)
        return DFL_VULKAN_QUEUE_ERROR;
    queueProps = calloc(queueCount, sizeof(VkQueueFamilyProperties));
    if (queueProps == NULL)
        return DFL_GENERIC_OOM_ERROR;
    vkGetPhysicalDeviceQueueFamilyProperties(pDevice->physDevice, &queueCount, queueProps);

    for (int i = 0; i < queueCount; i++)
    {
        if (queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            pDevice->queues.index[DFL_QUEUE_TYPE_GRAPHICS] = i;
            pDevice->queues.count[DFL_QUEUE_TYPE_GRAPHICS] = queueProps[i].queueCount;
            continue;
        }
        if (queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            pDevice->queues.index[DFL_QUEUE_TYPE_COMPUTE] = i;
            pDevice->queues.count[DFL_QUEUE_TYPE_COMPUTE] = queueProps[i].queueCount;
            continue;
        }
        if (queueProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            pDevice->queues.index[DFL_QUEUE_TYPE_TRANSFER] = i;
            pDevice->queues.count[DFL_QUEUE_TYPE_TRANSFER] = queueProps[i].queueCount;
            continue;
        }
    }
    return DFL_SUCCESS;
} 
/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

DflSession dflSessionCreate(struct DflSessionInfo* pInfo)
{
    struct DflSession_T* session = dflSessionAllocHIDN();

    if (pInfo == NULL)
    {
        pInfo = calloc(1, sizeof(struct DflSessionInfo));
        if (pInfo == NULL)
            return NULL;
        pInfo->appName = "Dragonfly-App";
        pInfo->appVersion = VK_MAKE_VERSION(1, 0, 0);
        pInfo->sessionCriteria = NULL;
    }
    else
        session->info = *pInfo;

    session->surfaceCount = 0;

    if (dflSessionVulkanInstanceInitHIDN(&(session->info), &(session->instance), &(session->messenger), pInfo->sessionCriteria) != DFL_SUCCESS)
        return NULL;

    return (DflSession)session;
}

int dflSessionBindWindow(DflWindow* pWindow, DflSession* pSession)
{
    if ((dflWindowSurfaceIndexGet(*pWindow) >= 0))
        return DFL_SUCCESS;

    if((pWindow == NULL) || (*pWindow == NULL) || (*pSession == NULL) || (pSession == NULL))
        return DFL_GENERIC_NULL_POINTER_ERROR;

    if (DFL_HANDLE(Session)->surfaceCount == 0)
    {
        DFL_HANDLE(Session)->surfaces = calloc(1, sizeof(VkSurfaceKHR));
        if (DFL_HANDLE(Session)->surfaces == NULL)
            return DFL_GENERIC_OOM_ERROR;
    }
    else
    {
        VkSurfaceKHR* dummy = realloc(DFL_HANDLE(Session)->surfaces, (DFL_HANDLE(Session)->surfaceCount + 1) * sizeof(VkSurfaceKHR));
        if (dummy == NULL)
            return DFL_GENERIC_OOM_ERROR;
        DFL_HANDLE(Session)->surfaces = dummy;
    }

    dflSessionLegalizeHIDN(pSession);
    ((struct DflSession_T*)*pSession)->surfaceCount++;
    GLFWwindow* window = dflWindowHandleGet(*pWindow, *pSession);
    if (glfwCreateWindowSurface(DFL_HANDLE(Session)->instance, window, NULL, &DFL_HANDLE(Session)->surfaces[DFL_HANDLE(Session)->surfaceCount - 1]) != VK_SUCCESS)
        return DFL_VULKAN_SURFACE_ERROR;

    dflWindowSurfaceIndexSet(DFL_HANDLE(Session)->surfaceCount - 1, pWindow, *pSession);
    dflSessionIllegalizeHIDN(pSession);

    return DFL_SUCCESS;
}

void dflSessionInitWindow(DflWindowInfo* pWindowInfo, DflWindow* pWindow, DflSession* pSession)
{
    if ((pWindow == NULL) || (*pWindow != NULL)) // Dragonfly will not attempt to overwrite an existing window
        return;

    *pWindow = dflWindowCreate(pWindowInfo);

    if (dflSessionBindWindow(pWindow, pSession) != DFL_SUCCESS)
        *pWindow = NULL;
}

DflDevice dflDeviceCreate(int GPUCriteria, DflPhysicalDeviceList* pDevices, int choice, DflSession* pSession)
{
    struct DflDevice_T* device = dflDeviceAllocHIDN();

    device->doLowPerf = (GPUCriteria & DFL_GPU_CRITERIA_LOW_PERFORMANCE) ? true : false;
    device->doAbuseMemory = (GPUCriteria & DFL_GPU_CRITERIA_ABUSE_MEMORY) ? true : false;
    device->canPresent = (GPUCriteria & DFL_GPU_CRITERIA_ONLY_OFFSCREEN) ? false : true;

    if ((pDevices == NULL) || (*pDevices == NULL))
    {
        VkPhysicalDevice* devices = NULL;
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(((struct DflSession_T*)*pSession)->instance, &deviceCount, NULL);
        devices = calloc(deviceCount, sizeof(VkPhysicalDevice));
        if (devices == NULL)
            return NULL;
        vkEnumeratePhysicalDevices(((struct DflSession_T*)*pSession)->instance, &deviceCount, devices);

        switch (deviceCount) // this just selects the device it thinks is appropriate, but doesn't actually make a VkDevice yet. Doesn't do any ranking if there's only one device
        {
        case 0:
            return NULL;
        case 1:
            device->physDevice = devices[deviceCount - 1];
            if (dflSessionDeviceOrganiseDataHIDN(GPUCriteria, devices[deviceCount - 1], device) != DFL_SUCCESS)
                return NULL;
            break;
        default:
            // TODO: actually rank the devices
            break;
        }

        //if (sessionCriteria & DFL_SESSION_CRITERIA_ONLY_OFFSCREEN)
            //dflSessionDeviceInit(NULL, &session);
    }

    return (DflDevice)device;
}
/* -------------------- *
 *   GET & SET          *
 * -------------------- */

DflPhysicalDeviceList dflSessionDevicesGet(DflSession session)
{
    VkPhysicalDevice* devices = NULL;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(((struct DflSession_T*)session)->instance, &deviceCount, NULL);
    devices = calloc(deviceCount, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(((struct DflSession_T*)session)->instance, &deviceCount, devices);

    struct DflPhysicalDeviceList_T* physDevices = calloc(deviceCount, sizeof(struct DflPhysicalDeviceList_T));
    if (physDevices == NULL)
        return NULL;

    physDevices->pDevices = devices;
    physDevices->count = deviceCount;

    return (DflPhysicalDeviceList)physDevices;
}

bool dflDeviceCanPresentGet(DflDevice device)
{
    return ((struct DflDevice_T*)device)->canPresent;
}

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void dflSessionEnd(DflSession* pSession)
{
    for(int i = 0; i < DFL_HANDLE(Session)->surfaceCount; i++)
        vkDestroySurfaceKHR(DFL_HANDLE(Session)->instance, DFL_HANDLE(Session)->surfaces[i], NULL);

    if (DFL_HANDLE(Session)->messenger != NULL)
    {
        PFN_vkDestroyDebugUtilsMessengerEXT destroyDebug = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(DFL_HANDLE(Session)->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyDebug != NULL)
            destroyDebug(DFL_HANDLE(Session)->instance, DFL_HANDLE(Session)->messenger, NULL);
    }
    vkDestroyInstance(DFL_HANDLE(Session)->instance, NULL);
    free(*pSession);
}

bool dflSessionIsLegal(DflSession session)
{
    if(((struct DflSession_T*)session)->flags & DFL_ACTION_IS_LEGAL)
        return true;

    return false;
}
