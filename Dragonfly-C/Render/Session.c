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

// SESSION

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

// DEVICES

/* -------------------- *
 *   INTERNAL           *
 * -------------------- */

static float* _dflDeviceQueuePrioritiesSet(int count);
static float* _dflDeviceQueuePrioritiesSet(int count) {
    float* priorities = calloc(count, sizeof(float));
    if (priorities == NULL)
        return NULL;
    for (int i = 0; i < count; i++)
    { // hyperbolic dropoff
        priorities[i] = 1.0f / ((float)i + 1.0f);
    }
    return priorities;
}

static VkDeviceQueueCreateInfo* _dflDeviceQueueCreateInfoSet(bool allQueues, struct DflDevice_T* pDevice, DflSession hSession);
static VkDeviceQueueCreateInfo* _dflDeviceQueueCreateInfoSet(bool allQueues, struct DflDevice_T* pDevice, DflSession hSession)
{
    VkDeviceQueueCreateInfo* infos = calloc(pDevice->queueFamilyCount, sizeof(VkDeviceQueueCreateInfo));
    if (infos == NULL) {
        return NULL;
    }

    // we are initializing queue families not queues themselves, so the properties
    // stored in the DflDevice_T struct are not useful here
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pDevice->physDevice, &count, NULL);
    if (count == 0) {
        DFL_SESSION->error = DFL_VULKAN_QUEUES_NO_PROPERTIES_ERROR;
        return NULL;
    }
    VkQueueFamilyProperties* props = calloc(count, sizeof(VkQueueFamilyProperties));
    if (props == NULL)
    {
        DFL_SESSION->error = DFL_GENERIC_OOM_ERROR;
        return NULL;
    }
    vkGetPhysicalDeviceQueueFamilyProperties(pDevice->physDevice, &count, props);

    float priority[] = { 1.0f };
    for (int i = 0; i < pDevice->queueFamilyCount; i++) {
        infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        infos[i].pNext = NULL;
        infos[i].flags = 0;
        infos[i].queueFamilyIndex = i;
        infos[i].queueCount = (allQueues == true) ? props[i].queueCount : 1;
        infos[i].pQueuePriorities = (allQueues == true) ? _dflDeviceQueuePrioritiesSet(props[i].queueCount) : priority;
    }

    return infos;
}

inline static struct DflDevice_T* _dflDeviceAlloc();
static struct DflDevice_T* _dflDeviceAlloc() {
    return calloc(1, sizeof(struct DflDevice_T));
}

// just a helper function that fills GPU specific information in DflSession_T. When ranking devices,
// Dragonfly will always sort each checked device using this function, unless the previous device
// was already ranked higher. This will help avoid calling this function too many times.
static void _dflDeviceOrganiseData(struct DflDevice_T* pDevice);
static void _dflDeviceOrganiseData(struct DflDevice_T* pDevice)
{
    VkDeviceCreateInfo deviceInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0
    };
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(pDevice->physDevice, &props);
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(pDevice->physDevice, &memProps);
    VkPhysicalDeviceFeatures feats;
    vkGetPhysicalDeviceFeatures(pDevice->physDevice, &feats);

    STRCPY(&pDevice->name, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE, &props.deviceName);

    for (int i = 0; i < memProps.memoryHeapCount; i++)
    {
        if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            pDevice->localHeaps++;
            if (pDevice->localHeaps > DFL_MAX_ITEM_COUNT)
            {
                pDevice->localHeaps--;
                continue;
            }
            pDevice->localMem[pDevice->localHeaps - 1].size = memProps.memoryHeaps[i].size;
            pDevice->localMem[pDevice->localHeaps - 1].heapIndex = memProps.memoryTypes[i].heapIndex;

            if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                pDevice->localMem[pDevice->localHeaps - 1].isHostVisible = true;
            else
                pDevice->localMem[pDevice->localHeaps - 1].isHostVisible = false;
        }
        else
        {
            pDevice->nonLocalHeaps++;
            if (pDevice->nonLocalHeaps > DFL_MAX_ITEM_COUNT)
            {
                pDevice->nonLocalHeaps--;
                continue;
            }
            pDevice->nonLocalMem[pDevice->nonLocalHeaps - 1].size = memProps.memoryHeaps[i].size;
            pDevice->nonLocalMem[pDevice->nonLocalHeaps - 1].heapIndex = memProps.memoryTypes[i].heapIndex;

            if (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
                pDevice->nonLocalMem[pDevice->nonLocalHeaps - 1].isHostVisible = true;
            else
                pDevice->nonLocalMem[pDevice->nonLocalHeaps - 1].isHostVisible = false;

        }
    }

    pDevice->maxDim1D = props.limits.maxImageDimension1D;
    pDevice->maxDim2D = props.limits.maxImageDimension2D;
    pDevice->maxDim3D = props.limits.maxImageDimension3D;

    pDevice->canDoGeomShade = feats.geometryShader;
    pDevice->canDoTessShade = feats.tessellationShader;
}

static int _dflDeviceQueuesGet(VkSurfaceKHR* surface, struct DflDevice_T* pDevice);
static int _dflDeviceQueuesGet(VkSurfaceKHR* surface, struct DflDevice_T* pDevice)
{
    for (int i = 0; i <= 3; i++)
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

    pDevice->canPresent = false;
    pDevice->queueFamilyCount = queueCount;

    pDevice->queues.count[DFL_QUEUE_TYPE_PRESENTATION] = 0;
    pDevice->queues.count[DFL_QUEUE_TYPE_GRAPHICS] = 0;
    pDevice->queues.count[DFL_QUEUE_TYPE_COMPUTE] = 0;
    pDevice->queues.count[DFL_QUEUE_TYPE_TRANSFER] = 0;

    for (int i = 0; i < queueCount; i++)
    {
        if ((surface != NULL) && (pDevice->canPresent == false))
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

    if (((pDevice->canPresent == true) && (pDevice->queues.count[DFL_QUEUE_TYPE_PRESENTATION] <= 0)) || pDevice->queues.count[DFL_QUEUE_TYPE_GRAPHICS] <= 0 || pDevice->queues.count[DFL_QUEUE_TYPE_COMPUTE] <= 0 || pDevice->queues.count[DFL_QUEUE_TYPE_TRANSFER] <= 0)
        return DFL_VULKAN_QUEUE_ERROR;

    return DFL_SUCCESS;
}

static void _dflDeviceRank(struct DflDevice_T* pDevice);
       void _dflDeviceRank(struct DflDevice_T* pDevice)
{
    pDevice->rank = 0;

    if (pDevice->canPresent)
       pDevice->rank += 1000;

    if (pDevice->canDoGeomShade)
       pDevice->rank += 1000;

    if (pDevice->canDoTessShade)
       pDevice->rank += 500;

    for(int i = 0; i < pDevice->localHeaps; i++)
       pDevice->rank += 2*(pDevice->localMem[i].size)/1000000; // 1MB = 1 point. With this, we make sure that memory doesn't overshadow other features. However, local memory is twice as important as non-local memory.
    for(int i = 0; i < pDevice->nonLocalHeaps; i++)
       pDevice->rank += (pDevice->nonLocalMem[i].size)/1000000; // 1MB = 1 point

    pDevice->rank += pDevice->maxDim1D/1000;
    pDevice->rank += 2*pDevice->maxDim2D/1000;
    pDevice->rank += 3*pDevice->maxDim3D/1000;
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

    if (!(pInfo->sessionCriteria & DFL_SESSION_CRITERIA_ONLY_OFFSCREEN))
        session->monitors = dflMonitorsGet(&session->monitorCount);

    return (DflSession)session;
}

/* -------------------- *
 *       CHANGE         *
 * -------------------- */

void dflSessionLoadDevices(DflSession hSession)
{
    VkPhysicalDevice* physDevices = NULL;
    DFL_SESSION->deviceCount = 0;
    vkEnumeratePhysicalDevices(DFL_SESSION->instance, &DFL_SESSION->deviceCount, NULL);
    if (DFL_SESSION->deviceCount <= 0)
    {
        DFL_SESSION->error = DFL_VULKAN_DEVICE_ERROR;
        return;
    }
    physDevices = calloc(DFL_SESSION->deviceCount, sizeof(VkPhysicalDevice));
    if (physDevices == NULL)
    {
        DFL_SESSION->error = DFL_GENERIC_OOM_ERROR;
        return;
    }
    vkEnumeratePhysicalDevices(DFL_SESSION->instance, &DFL_SESSION->deviceCount, physDevices);
    if (physDevices == NULL)
    {
        DFL_SESSION->error = DFL_GENERIC_OOM_ERROR;
        return;
    }

    DFL_SESSION->devices = calloc(DFL_SESSION->deviceCount, sizeof(struct DflDevice_T));
    if (DFL_SESSION->devices == NULL)
    {
        DFL_SESSION->error = DFL_GENERIC_OOM_ERROR;
        return;
    }

    VkSurfaceKHR surface = NULL;
    for (int i = 0; i < DFL_MAX_ITEM_COUNT; i++)
    {
        if (DFL_SESSION->windows[i] == NULL)
            continue;
        surface = ((struct DflWindow_T*)DFL_SESSION->windows[i])->surface;
    }

    for (int i = 0; i < DFL_SESSION->deviceCount; i++)
    {
        DFL_SESSION->devices[i].physDevice = physDevices[i];
        _dflDeviceOrganiseData(&DFL_SESSION->devices[i]);
        DFL_SESSION->error = _dflDeviceQueuesGet(&surface, &DFL_SESSION->devices[i]);
        if (DFL_SESSION->error != DFL_SUCCESS)
            return;
        _dflDeviceRank(&DFL_SESSION->devices[i]);
    }
}

void dflSessionInitDevice(int GPUCriteria, int deviceIndex, DflSession hSession)
{
    // device creation
    if (deviceIndex >= DFL_SESSION->deviceCount) // TODO: Negative numbers tell Dragonfly to choose the device based on a rating system (which will also depend on the criteria).
    {
        DFL_SESSION->error = DFL_GENERIC_OUT_OF_BOUNDS_ERROR;
        return;
    }

    if (deviceIndex < 0)
    {
        uint32_t oldRank = 0;
        for(int i = 0; i < DFL_SESSION->deviceCount; i++)
        {
            if (DFL_SESSION->devices[i].rank >= oldRank)
            {
                deviceIndex = i;
                oldRank = DFL_SESSION->devices[i].rank;
            }
        }
    }

    VkDeviceCreateInfo deviceInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = DFL_SESSION->devices[deviceIndex].queueFamilyCount,
        .pQueueCreateInfos = _dflDeviceQueueCreateInfoSet((GPUCriteria & DFL_GPU_CRITERIA_ALL_QUEUES) ? true : false, DFL_SESSION->devices + deviceIndex, hSession),
    };

    if (deviceInfo.pQueueCreateInfos == NULL)
    {
        DFL_SESSION->error = DFL_GENERIC_OOM_ERROR;
        return;
    }

    if (DFL_SESSION->devices[deviceIndex].canPresent == true)
    {
        const char* extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        deviceInfo.enabledExtensionCount = 1;
        deviceInfo.ppEnabledExtensionNames = extensions;
    }
    else
    {
        deviceInfo.enabledExtensionCount = 0;
        deviceInfo.ppEnabledExtensionNames = NULL;
    }

    if (vkCreateDevice(DFL_SESSION->devices[deviceIndex].physDevice, &deviceInfo, NULL, &(DFL_SESSION->devices[deviceIndex].device)) != VK_SUCCESS)
    {
        DFL_SESSION->error = DFL_VULKAN_DEVICE_ERROR;
        return;
    }

    for (int i = 0; i < 4; i++)
    {
        if (GPUCriteria & DFL_GPU_CRITERIA_ALL_QUEUES)
        {
            DFL_SESSION->devices[deviceIndex].queues.handles[i] = calloc(DFL_SESSION->devices[deviceIndex].queues.count[i], sizeof(VkQueue));
            if (DFL_SESSION->devices[deviceIndex].queues.handles[i] == NULL)
            {
                DFL_SESSION->error = DFL_GENERIC_OOM_ERROR;
                return;
            }
            for (int j = 0; j < DFL_SESSION->devices[deviceIndex].queues.count[i]; j++)
                vkGetDeviceQueue(DFL_SESSION->devices[deviceIndex].device, DFL_SESSION->devices[deviceIndex].queues.index[i], j, DFL_SESSION->devices[deviceIndex].queues.handles[i][j]);
        }
        else 
        {
            DFL_SESSION->devices[deviceIndex].queues.handles[i] = calloc(1, sizeof(VkQueue));
            if (DFL_SESSION->devices[deviceIndex].queues.handles[i] == NULL)
            {
                DFL_SESSION->error = DFL_GENERIC_OOM_ERROR;
                return;
            }
            vkGetDeviceQueue(DFL_SESSION->devices[deviceIndex].device, DFL_SESSION->devices[deviceIndex].queues.index[i], 0, DFL_SESSION->devices[deviceIndex].queues.handles[i]);
        }

        VkCommandPoolCreateInfo comPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = DFL_SESSION->devices[deviceIndex].queues.index[i],
        };

        if (vkCreateCommandPool(DFL_SESSION->devices[deviceIndex].device, &comPoolInfo, NULL, &DFL_SESSION->devices[deviceIndex].queues.comPool[i]) != VK_SUCCESS)
        {
            DFL_SESSION->error = DFL_VULKAN_QUEUES_COMPOOL_ALLOC_ERROR;
            return;
        }
    }
}

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

inline uint32_t dflSessionDeviceCountGet(DflSession hSession)
{
    return DFL_SESSION->deviceCount;
}

const char* dflSessionDeviceNameGet(int deviceIndex, DflSession hSession)
{
    return DFL_SESSION->devices[deviceIndex].name;
}

inline uint32_t dflSessionDeviceRankGet(int deviceIndex, DflSession hSession)
{
    return DFL_SESSION->devices[deviceIndex].rank;
}

int dflSessionErrorGet(DflSession hSession)
{
    return DFL_SESSION->error;
}

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void dflSessionTerminateDevice(uint32_t deviceIndex, DflSession hSession)
{
    if (deviceIndex >= DFL_SESSION->deviceCount)
    {
        DFL_SESSION->error = DFL_GENERIC_OUT_OF_BOUNDS_ERROR;
        return;
    }

    for (int i = 0; i < 4; i++)
    {
        vkDestroyCommandPool(DFL_SESSION->devices[deviceIndex].device, DFL_SESSION->devices[deviceIndex].queues.comPool[i], NULL);
        free(DFL_SESSION->devices[deviceIndex].queues.handles[i]);
    }
    vkDestroyDevice(DFL_SESSION->devices[deviceIndex].device, NULL);
};

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