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
    int                         queueFamilyCount;
    struct DflQueueCollection_T queues;
    VkPhysicalDevice            physDevice;

    DflSession                  session;

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

    bool canPresent;

    // error
    int error;
};

struct DflSession_T {
    struct DflSessionInfo       info;
    VkInstance                  instance;
    VkDebugUtilsMessengerEXT    messenger;

    int                         deviceCount;

    VkSurfaceKHR  surface; // this exists purely to make device creation work, because it requires to check a surface for presentation support.

    /* -------------------- *
     *  FLAGS               *
     * -------------------- */

     int flags;

     // error 
     int error;
};

/* -------------------- *
 *   INTERNAL           *
 * -------------------- */

static float* _dflSessionQueuePrioritiesSet(int count);
static float* _dflSessionQueuePrioritiesSet(int count) {
    float* priorities = calloc(count, sizeof(float));
    for (int i = 0; i < count; i++) { // exponential dropoff
        priorities[i] = 1.0f/((float)i + 1.0f);
    }
    return priorities;
}

static VkDeviceQueueCreateInfo* _dflSessionQueueCreateInfoSet(struct DflDevice_T* pDevice);
static VkDeviceQueueCreateInfo* _dflSessionQueueCreateInfoSet(struct DflDevice_T* pDevice) 
{
    VkDeviceQueueCreateInfo* infos = calloc(pDevice->queueFamilyCount, sizeof(VkDeviceQueueCreateInfo));
    if (infos == NULL) {
        return NULL;
    }
    
    int count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pDevice->physDevice, &count, NULL);
    if (count == 0) {
        return NULL;
    }
    VkQueueFamilyProperties* props = calloc(count, sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(pDevice->physDevice, &count, props);

    for (int i = 0; i < pDevice->queueFamilyCount; i++) {
        infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        infos[i].pNext = NULL;
        infos[i].flags = 0;
        infos[i].queueFamilyIndex = i;
        infos[i].queueCount = props[i].queueCount;
        infos[i].pQueuePriorities = _dflSessionQueuePrioritiesSet(props[i].queueCount);
    }

    return infos;
}

inline static void _dflSessionLegalize(DflSession* pSession);
inline static void _dflSessionLegalize(DflSession* pSession) {
    DFL_HANDLE(Session)->flags |= DFL_ACTION_IS_LEGAL;
}

inline static void _dflSessionIllegalize(DflSession* pSession);
inline static void _dflSessionIllegalize(DflSession* pSession) {
    DFL_HANDLE(Session)->flags &= ~DFL_ACTION_IS_LEGAL;
}

inline static struct DflSession_T* _dflSessionAlloc();
inline static struct DflSession_T* _dflSessionAlloc() {
    return calloc(1, sizeof(struct DflSession_T));
}

inline static struct DflDevice_T* _dflDeviceAlloc();
inline static struct DflDevice_T* _dflDeviceAlloc() {
    return calloc(1, sizeof(struct DflDevice_T));
}

static VKAPI_ATTR VkBool32 VKAPI_CALL _dflDebugCLBK(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* data, void* userData);
static VKAPI_ATTR VkBool32 VKAPI_CALL _dflDebugCLBK(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* data, void* userData)
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

#ifdef _WIN32 // Windows demands an extra extension for surfaces to work.
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
            return DFL_GENERIC_OOM_ERROR;
        extensions = dummy;
        extensions[extensionCount - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, NULL);
        if (!layerCount)
        {
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

// just a helper function that fills GPU specific information in DflSession_T. When ranking devices,
// Dragonfly will always sort each checked device using this function, unless the previous device
// was already ranked higher. This will help avoid calling this function too many times.
static int _dflSessionDeviceOrganiseData(struct DflDevice_T* device);
static int _dflSessionDeviceOrganiseData(struct DflDevice_T* device)
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

static int _dflSessionDeviceQueuesGet(VkSurfaceKHR* surface, struct DflDevice_T* pDevice);
static int _dflSessionDeviceQueuesGet(VkSurfaceKHR* surface, struct DflDevice_T* pDevice)
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

    pDevice->queueFamilyCount = queueCount;

    pDevice->queues.count[DFL_QUEUE_TYPE_PRESENTATION] = 0;
    pDevice->queues.count[DFL_QUEUE_TYPE_GRAPHICS] = 0;
    pDevice->queues.count[DFL_QUEUE_TYPE_COMPUTE] = 0;
    pDevice->queues.count[DFL_QUEUE_TYPE_TRANSFER] = 0;

    for (int i = 0; i < queueCount; i++)
    {
        if (surface != NULL)
        {
            vkGetPhysicalDeviceSurfaceSupportKHR(pDevice->physDevice, i, *surface, &pDevice->canPresent);
            if (pDevice->canPresent)
            {
                pDevice->queues.index[DFL_QUEUE_TYPE_PRESENTATION] = i;
                pDevice->queues.count[DFL_QUEUE_TYPE_PRESENTATION] = queueProps[i].queueCount;
            }
        }

        if (queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            pDevice->queues.index[DFL_QUEUE_TYPE_GRAPHICS] = i;
            pDevice->queues.count[DFL_QUEUE_TYPE_GRAPHICS] = queueProps[i].queueCount;
        }
        if (queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            pDevice->queues.index[DFL_QUEUE_TYPE_COMPUTE] = i;
            pDevice->queues.count[DFL_QUEUE_TYPE_COMPUTE] = queueProps[i].queueCount;
        }
        if (queueProps[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            pDevice->queues.index[DFL_QUEUE_TYPE_TRANSFER] = i;
            pDevice->queues.count[DFL_QUEUE_TYPE_TRANSFER] = queueProps[i].queueCount;
        }
    }

    if(pDevice->queues.count[DFL_QUEUE_TYPE_PRESENTATION] <= 0 || pDevice->queues.count[DFL_QUEUE_TYPE_GRAPHICS] <= 0 || pDevice->queues.count[DFL_QUEUE_TYPE_COMPUTE] <= 0 || pDevice->queues.count[DFL_QUEUE_TYPE_TRANSFER] <= 0)
        return DFL_VULKAN_QUEUE_ERROR;

    return DFL_SUCCESS;
} 

static int _dflWindowBindToSession(DflWindow* pWindow, DflSession* pSession);
static int _dflWindowBindToSession(DflWindow* pWindow, DflSession* pSession)
{
    if (!(DFL_HANDLE(Session)->flags & DFL_ACTION_IS_LEGAL))
        return DFL_GENERIC_ILLEGAL_ACTION_ERROR;

    if ((dflWindowSessionGet(*pWindow) != NULL))
        return DFL_SUCCESS;

    _dflSessionLegalize(pSession);
    _dflWindowSessionSet(NULL, *pSession, pWindow);
    GLFWwindow* window = _dflWindowHandleGet(*pWindow, *pSession);
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(DFL_HANDLE(Session)->instance, window, NULL, &surface) != VK_SUCCESS)
        return DFL_VULKAN_SURFACE_ERROR;
    DFL_HANDLE(Session)->surface = surface; // This exists simply for the sake of the _dflSessionDeviceQueuesGet function.
    _dflWindowSessionSet(surface, *pSession, pWindow);
    _dflSessionIllegalize(pSession);

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
            .appName = "Dragonfly-App",
            .appVersion = VK_MAKE_VERSION(1, 0, 0),
            .sessionCriteria = NULL
        };
        session->info = info;
    }
    else
        session->info = *pInfo;

    session->error = _dflSessionVulkanInstanceInit(&(session->info), &(session->instance), &(session->messenger), session->info.sessionCriteria);

    return (DflSession)session;
}

DflWindow dflWindowInit(DflWindowInfo* pWindowInfo, DflSession* pSession)
{
    DflWindow window = _dflWindowCreate(pWindowInfo);

    if (dflWindowErrorGet(window) == DFL_SUCCESS)
    {
        _dflSessionLegalize(pSession);
        _dflWindowErrorSet(_dflWindowBindToSession(&window, pSession), &window);
        _dflSessionIllegalize(pSession);
    }
    
    return window;
}

DflDevice dflDeviceInit(int GPUCriteria, int choice, DflDevice* pDevices, DflSession* pSession)
{
    struct DflDevice_T* device = _dflDeviceAlloc();

    // device creation
    if ((choice < 0) || (choice >= DFL_HANDLE(Session)->deviceCount)) // TODO: Negative numbers tell Dragonfly to choose the device based on a rating system (which will also depend on the criteria).
    {
        device->error = DFL_GENERIC_OUT_OF_BOUNDS_ERROR;
        return (DflDevice)device;
    }

    device = DFL_HANDLE_ARRAY(Device, choice);
    VkDeviceCreateInfo deviceInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = DFL_HANDLE_ARRAY(Device, choice)->queueFamilyCount,
        .pQueueCreateInfos = _dflSessionQueueCreateInfoSet(DFL_HANDLE_ARRAY(Device, choice)),
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL,
    };

    if(vkCreateDevice(((struct DflDevice_T*)device)->physDevice, &deviceInfo, NULL, &((struct DflDevice_T*)device)->device) != VK_SUCCESS)
    {
        device->error = DFL_VULKAN_DEVICE_ERROR;
        return (DflDevice)device;
    }

    device->session = *pSession;

    return (DflDevice)device;
}

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

DflDevice* dflSessionDevicesGet(int* pCount, DflSession* pSession)
{
    VkPhysicalDevice* physDevices = NULL;
    *pCount = 0;
    vkEnumeratePhysicalDevices(DFL_HANDLE(Session)->instance, pCount, NULL);
    physDevices = calloc(*pCount, sizeof(VkPhysicalDevice));
    if (physDevices == NULL)
        return NULL;
    vkEnumeratePhysicalDevices(DFL_HANDLE(Session)->instance, pCount, physDevices);
    if(physDevices == NULL)
        return NULL;

    struct DflDevice_T** devices = calloc(*pCount, sizeof(struct DflDevice_T*));
    DFL_HANDLE(Session)->deviceCount = *pCount;
    if (devices == NULL)
        return NULL;

    for (int i = 0; i < *pCount; i++)
    {
        devices[i] = calloc(1, sizeof(struct DflDevice_T));
        if (devices[i] == NULL)
            return NULL;
        devices[i]->physDevice = physDevices[i];
        if (_dflSessionDeviceOrganiseData(devices[i]) != DFL_SUCCESS)
            return NULL;
        if (_dflSessionDeviceQueuesGet(&(DFL_HANDLE(Session)->surface), devices[i]) != DFL_SUCCESS)
            return NULL;
    }

    return (DflDevice*)devices;
}

int dflSessionErrorGet(DflSession session)
{
    return ((struct DflSession_T*)session)->error;
}

bool dflDeviceCanPresentGet(DflDevice device)
{
    return ((struct DflDevice_T*)device)->canPresent;
}

const char* dflDeviceNameGet(DflDevice device)
{
    return ((struct DflDevice_T*)device)->name;
}

int dflDeviceErrorGet(DflDevice device)
{
    return ((struct DflDevice_T*)device)->error;
}

bool _dflSessionIsLegalGet(DflSession session)
{
    if (((struct DflSession_T*)session)->flags & DFL_ACTION_IS_LEGAL)
        return true;

    return false;
}

VkInstance _dflSessionInstanceGet(DflSession session)
{
    if(((struct DflSession_T*)session)->flags & DFL_ACTION_IS_LEGAL)
        return ((struct DflSession_T*)session)->instance;
}

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void dflSessionDestroy(DflSession* pSession)
{
    if (DFL_HANDLE(Session)->messenger != NULL) // that means debugging was enabled.
    {
        PFN_vkDestroyDebugUtilsMessengerEXT destroyDebug = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(DFL_HANDLE(Session)->instance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyDebug != NULL)
            destroyDebug(DFL_HANDLE(Session)->instance, DFL_HANDLE(Session)->messenger, NULL);
    }
    vkDestroyInstance(DFL_HANDLE(Session)->instance, NULL);
    free(*pSession);
}

void dflDeviceTerminate(DflDevice* pDevice, DflSession* pSession)
{
    if(*pSession != DFL_HANDLE(Device)->session) // the device doesn't belong to the session and cannot be destroyed by it.
        return;

    if (DFL_HANDLE(Device)->device == NULL)
        return; // since devices in Dragonfly *can* be uninitialized, it's prudent, I think, to check for that.

    vkDestroyDevice(DFL_HANDLE(Device)->device, NULL);
}

void dflWindowTerminate(DflWindow* pWindow, DflSession* pSession)
{
    if(*pSession != dflWindowSessionGet(*pWindow)) // the window doesn't belong to the session and cannot be destroyed by it.
        return;

    _dflSessionLegalize(pSession);
    _dflWindowDestroy(pWindow);
    _dflSessionIllegalize(pSession);
}