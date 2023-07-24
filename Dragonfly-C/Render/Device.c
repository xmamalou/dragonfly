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
#include <stdio.h>

/* -------------------- *
 *   INTERNAL           *
 * -------------------- */

static float* _dflDeviceQueuePrioritiesSet(int count);
static float* _dflDeviceQueuePrioritiesSet(int count) {
    float* priorities = calloc(count, sizeof(float));
    if (priorities == NULL)
        return NULL;
    for (int i = 0; i < count; i++) { // exponential dropoff
        priorities[i] = 1.0f / ((float)i + 1.0f);
    }
    return priorities;
}

static VkDeviceQueueCreateInfo* _dflDeviceQueueCreateInfoSet(bool allQueues, struct DflDevice_T* pDevice);
static VkDeviceQueueCreateInfo* _dflDeviceQueueCreateInfoSet(bool allQueues, struct DflDevice_T* pDevice)
{
    VkDeviceQueueCreateInfo* infos = calloc(pDevice->queueFamilyCount, sizeof(VkDeviceQueueCreateInfo));
    if (infos == NULL) {
        return NULL;
    }

    // we are initializing queue families not queues themselves, so the properties
    // stored in the DflDevice_T struct are not useful here
    int count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pDevice->physDevice, &count, NULL);
    if (count == 0) {
        pDevice->error = DFL_VULKAN_QUEUES_NO_PROPERTIES_ERROR;
        return NULL;
    }
    VkQueueFamilyProperties* props = calloc(count, sizeof(VkQueueFamilyProperties));
    if (props == NULL)
    {
        pDevice->error = DFL_GENERIC_OOM_ERROR;
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
inline static struct DflDevice_T* _dflDeviceAlloc() {
    return calloc(1, sizeof(struct DflDevice_T));
}

// just a helper function that fills GPU specific information in DflSession_T. When ranking devices,
// Dragonfly will always sort each checked device using this function, unless the previous device
// was already ranked higher. This will help avoid calling this function too many times.
static int _dflDeviceOrganiseData(struct DflDevice_T* pDevice);
static int _dflDeviceOrganiseData(struct DflDevice_T* pDevice)
{
    VkDeviceCreateInfo deviceInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = NULL,
            .flags = NULL
    };
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(pDevice->physDevice, &props);
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(pDevice->physDevice, &memProps);
    VkPhysicalDeviceFeatures feats;
    vkGetPhysicalDeviceFeatures(pDevice->physDevice, &feats);

    strcpy_s(&pDevice->name, VK_MAX_PHYSICAL_DEVICE_NAME_SIZE, &props.deviceName);

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

    return DFL_SUCCESS;
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

    if (pDevice->queues.count[DFL_QUEUE_TYPE_PRESENTATION] <= 0 || pDevice->queues.count[DFL_QUEUE_TYPE_GRAPHICS] <= 0 || pDevice->queues.count[DFL_QUEUE_TYPE_COMPUTE] <= 0 || pDevice->queues.count[DFL_QUEUE_TYPE_TRANSFER] <= 0)
        return DFL_VULKAN_QUEUE_ERROR;

    return DFL_SUCCESS;
}

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

DflDevice dflDeviceInit(int GPUCriteria, int choice, DflDevice* pDevices, DflSession hSession)
{
    struct DflDevice_T* device = _dflDeviceAlloc();

    // device creation
    if ((choice < 0) || (choice >= DFL_HANDLE(Session)->deviceCount)) // TODO: Negative numbers tell Dragonfly to choose the device based on a rating system (which will also depend on the criteria).
    {
        device->error = DFL_GENERIC_OUT_OF_BOUNDS_ERROR;
        return (DflDevice)device;
    }

    device->canPresent = DFL_HANDLE_ARRAY(Device, choice)->canPresent;

    device = DFL_HANDLE_ARRAY(Device, choice);
    VkDeviceCreateInfo deviceInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .queueCreateInfoCount = DFL_HANDLE_ARRAY(Device, choice)->queueFamilyCount,
        .pQueueCreateInfos = _dflDeviceQueueCreateInfoSet((GPUCriteria & DFL_GPU_CRITERIA_ALL_QUEUES) ? true : false, DFL_HANDLE_ARRAY(Device, choice)),
    };

    if(device->canPresent == true)
    {
        deviceInfo.enabledExtensionCount = 1;
        deviceInfo.ppEnabledExtensionNames = (const char* const[]){VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    }
    else
    {
        deviceInfo.enabledExtensionCount = 0;
        deviceInfo.ppEnabledExtensionNames = NULL;
    }

    if (deviceInfo.pQueueCreateInfos == NULL)
        return (DflDevice)device;

    if (vkCreateDevice(((struct DflDevice_T*)device)->physDevice, &deviceInfo, NULL, &((struct DflDevice_T*)device)->device) != VK_SUCCESS)
    {
        device->error = DFL_VULKAN_DEVICE_ERROR;
        return (DflDevice)device;
    }

    for (int i = 0; i < 4; i++)
    {
        if (GPUCriteria & DFL_GPU_CRITERIA_ALL_QUEUES)
        {
            device->queues.handles[i] = calloc(device->queues.count[i], sizeof(VkQueue));
            if (device->queues.handles[i] == NULL)
            {
                device->error = DFL_GENERIC_OOM_ERROR;
                return (DflDevice)device;
            }
            for (int j = 0; j < device->queues.count[i]; j++)
                vkGetDeviceQueue(device->device, device->queues.index[i], j, &device->queues.handles[i][j]);
        }
        else {
            device->queues.handles[i] = calloc(1, sizeof(VkQueue));
            vkGetDeviceQueue(device->device, device->queues.index[i], 0, device->queues.handles[i]);
        }

        VkCommandPoolCreateInfo comPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = device->queues.index[i],
        };

        if (vkCreateCommandPool(device->device, &comPoolInfo, NULL, &device->queues.comPool[i]) != VK_SUCCESS)
        {
            device->error = DFL_VULKAN_QUEUES_COMPOOL_ALLOC_ERROR;
            return (DflDevice)device;
        }
    }

    device->session = hSession;
    for (int i = 0; i < DFL_MAX_ITEM_COUNT; i++)
    {
        if (DFL_HANDLE(Session)->devices[i] == NULL)
        {
            DFL_HANDLE(Session)->devices[i] = device;
            device->index = i;
            break;
        }
    }

    return (DflDevice)device;
}

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

DflDevice* dflSessionDevicesGet(int* pCount, DflSession hSession)
{
    VkPhysicalDevice* physDevices = NULL;
    *pCount = 0;
    vkEnumeratePhysicalDevices(DFL_SESSION->instance, pCount, NULL);
    physDevices = calloc(*pCount, sizeof(VkPhysicalDevice));
    if (physDevices == NULL)
        return NULL;
    vkEnumeratePhysicalDevices(DFL_SESSION->instance, pCount, physDevices);
    if (physDevices == NULL)
        return NULL;

    struct DflDevice_T** devices = calloc(*pCount, sizeof(struct DflDevice_T*));
    DFL_SESSION->deviceCount = *pCount;
    if (devices == NULL)
        return NULL;

    for (int i = 0; i < *pCount; i++)
    {
        devices[i] = calloc(1, sizeof(struct DflDevice_T));
        if (devices[i] == NULL)
            return NULL;
        devices[i]->physDevice = physDevices[i];
        if (_dflDeviceOrganiseData(devices[i]) != DFL_SUCCESS)
            return NULL;
        if (_dflDeviceQueuesGet(&(DFL_SESSION->surface), devices[i]) != DFL_SUCCESS)
            return NULL;
    }

    return (DflDevice*)devices;
}

bool dflDeviceCanPresentGet(DflDevice hDevice)
{
    return DFL_DEVICE->canPresent;
}

const char* dflDeviceNameGet(DflDevice hDevice)
{
    return DFL_DEVICE->name;
}

inline DflSession dflDeviceSessionGet(DflDevice hDevice)
{
    return DFL_DEVICE->session;
}

int dflDeviceErrorGet(DflDevice hDevice)
{
    return DFL_DEVICE->error;
}

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

// Similarly to dflWindowTerminate, the existence of hSession in the arguments is purely to remind the user to call 
// dflDeviceTerminate before dflSessionDestroy.
void dflDeviceTerminate(DflDevice hDevice, DflSession hSession)
{
    if (hSession != DFL_DEVICE->session)
        return; // but it's also good to not allow accidental mixups between sessions and their devices.

    for(int i = 0; i < 4; i++) // destroy command pools
        vkDestroyCommandPool(DFL_DEVICE->device, DFL_DEVICE->queues.comPool[i], NULL);

    if (DFL_DEVICE->device == NULL)
        return; // since devices in Dragonfly *can* be uninitialized, it's prudent, I think, to check for that.

    ((struct DflSession_T*)DFL_DEVICE->session)->devices[DFL_DEVICE->index] = NULL;

    vkDestroyDevice(DFL_DEVICE->device, NULL);
}