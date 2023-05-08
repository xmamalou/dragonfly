#include "Vulkan.h"

#include <stdlib.h>
#include <stdio.h>

struct DflLocalMem_T {
    uint64_t size;
    uint32_t heapIndex;
};

struct DflSession_T {
    struct DflSessionInfo   info;
    VkInstance              instance;
    VkDevice                GPU;

    VkPhysicalDevice* devices;
    int               deviceCount;
    int               choice;

    int error; // Error code

    // Session flags
    
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
    bool doUnderwork;
    bool doAbuseMemory;
};

inline static struct DflSession_T* DflSessionAllocHIDN();
inline static struct DflSession_T* DflSessionAllocHIDN() {
    return calloc(1, sizeof(struct DflSession_T));
}

// VULKAN INIT FUNCTIONS

static int DflSessionVulkanInstanceInitHIDN(struct DflSessionInfo* info, VkInstance* instance);
static int DflSessionVulkanInstanceInitHIDN(struct DflSessionInfo* info, VkInstance* instance) 
{
    if(info->debug == true)
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, NULL);
        if (!layerCount)
            return DFL_NO_LAYERS_FOUND;
        VkLayerProperties* layers = calloc(layerCount, sizeof(VkLayerProperties));
        vkEnumerateInstanceLayerProperties(&layerCount, layers);
    }


    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = "test",
        .applicationVersion = VK_MAKE_API_VERSION(0,0,1,0),
        .pEngineName = "Dragonfly",
        .engineVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .apiVersion = VK_API_VERSION_1_3
    };
    VkInstanceCreateInfo instInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL, // TODO: Add extensions 
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL
    };
    if (vkCreateInstance(&instInfo, NULL, instance) != VK_SUCCESS)
        return DFL_VULKAN_NOT_INITIALIZED;

    return DFL_SUCCESS;
}

static int DflSessionDeviceOrganiseDataHIDN(int GPUCrit, int deviceCount, struct DflSession_T** session);
static int DflSessionDeviceOrganiseDataHIDN(int GPUCrit, int deviceCount, struct DflSession_T** session)
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

DflSession dflSessionInit(struct DflSessionInfo* info, int sessionCrit, int GPUCrit)
{
    struct DflSession_T* session = DflSessionAllocHIDN();
    session->info = *info;
    if(DflSessionVulkanInstanceInitHIDN(&(session->info), &(session->instance)) != DFL_SUCCESS)
        return NULL;

    vkEnumeratePhysicalDevices(session->instance, &session->deviceCount, NULL);
    session->devices = calloc(session->deviceCount, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(session->instance, &session->deviceCount, session->devices);

    switch (session->deviceCount)
    {
    case 0:
        return NULL;
    case 1:
        if(DflSessionDeviceOrganiseDataHIDN(GPUCrit, 0, &session) != DFL_SUCCESS)
            return NULL;
        break;
    default:
        if (GPUCrit & DFL_GPU_CRITERIA_HASTY)
            if(DflSessionDeviceOrganiseDataHIDN(GPUCrit, 0, &session) != DFL_SUCCESS)
                return NULL;
        break;
    }

    printf("");

    return (DflSession)session;
}

void dflSessionEnd(DflSession* session)
{
    vkDestroyInstance(((struct DflSession_T*)*session)->instance, NULL);

    free(*session);
}