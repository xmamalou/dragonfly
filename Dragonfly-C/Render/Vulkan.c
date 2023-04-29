#include "Vulkan.h"

#include <stdlib.h>
#include <stdio.h>

struct DflSession_T {
    struct DflSessionInfo   info;
    VkInstance              instance;
    VkDevice                GPU;

    VkPhysicalDevice* devices;
    int               deviceCount;
    int               choice;

    // Session flags

    // GPU flags

    int doLowPerf : 1;
    int doUnderwork : 1;
    int doAbuseMemory : 1;
};

inline static struct DflSession_T* DflSessionAllocHIDN();
inline static struct DflSession_T* DflSessionAllocHIDN() {
    return calloc(1, sizeof(struct DflSession_T));
}

// VULKAN INIT FUNCTIONS

static int DflSessionVulkanInstanceInitHIDN(struct DflSessionInfo* info, VkInstance* instance);
static int DflSessionVulkanInstanceInitHIDN(struct DflSessionInfo* info, VkInstance* instance) {
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
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = 0,
        .ppEnabledExtensionNames = NULL, // TODO: Add extensions 
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = NULL // TODO: Add layers
    };
    if (vkCreateInstance(&instInfo, NULL, instance) != VK_SUCCESS)
        return DFL_VULKAN_NOT_INITIALIZED;

    return DFL_SUCCESS;
}

DflSession dflSessionInit(struct DflSessionInfo* info, int sessionCrit, int GPUCrit)
{
    struct DflSession_T* session = DflSessionAllocHIDN();
    session->info = *info;
    if(DflSessionVulkanInstanceInitHIDN(&info, &(session->instance)) != DFL_SUCCESS)
        return NULL;

    vkEnumeratePhysicalDevices(session->instance, &session->deviceCount, NULL);
    session->devices = calloc(session->deviceCount, sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(session->instance, &session->deviceCount, session->devices);

    switch (session->deviceCount)
    {
    case 0:
        return NULL;
    case 1:
        VkDeviceCreateInfo deviceInfo = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = NULL,
            .flags = NULL
        };
        VkPhysicalDeviceProperties* props = calloc(1, sizeof(VkPhysicalDeviceProperties));
        vkGetPhysicalDeviceProperties(session->devices, props);
        break;
    default:
        // TODO: Rank devices and select one based on given criteria
        break;
    }

    return (DflSession)session;
}

void dflSessionEnd(DflSession* session)
{
    vkDestroyInstance(((struct DflSession_T*)*session)->instance, NULL);

    free(*session);
}