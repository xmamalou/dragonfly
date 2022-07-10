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

//#define DFL_NO_DEBUG

#include "../include/render.h"
#include <string.h>
#include <stdio.h>

// Variables
static SDL_DisplayMode*         display; // Display information
static SDL_Window*              window; // Window information

static VkPhysicalDevice*        physicalDevs; // Physical device information
static int                      physicalDevCount; // Number of physical devices
static struct DflVulkQFamIndices {
    int                         graphics; // graphics queue index
    int                         present; // present queue index
    int                         compute; // compute queue index
}                               QFamIndices; // Vulkan queue indices

// SUBFUNCTIONS - INITIALIZATIONS
static int      dflVulkInstanceMaking(); // make Vulkan instance
#ifndef DFL_NO_DEBUG
    static int  dflVulkLayersMaking(); // push Vulkan layers
    static int  dflVulkDebugMessengerMaking(); // make Vulkan debug messenger
#endif
static int      dflVulkDeviceMaking(); // make Vulkan device (logical device)

// SUBSUBFUNCTIONS - INITIALIZATIONS
static int                                  dflVulkExtensionSupplying(); // supply Vulkan extensions
#ifndef DFL_NO_DEBUG
    static int                              dflVulkValLayersVerifying(); // verify Vulkan validation layers
    static VKAPI_ATTR VkBool32 VKAPI_CALL   dflVulkBugCallbacking( // callback for Vulkan validation layers
        VkDebugUtilsMessageSeverityFlagBitsEXT      severity, // message severity 
        VkDebugUtilsMessageTypeFlagsEXT             type, // message type
        const VkDebugUtilsMessengerCallbackDataEXT* data, // message data
        void*                                       userdata 
        );
    static VkResult                         dflVulkDebugExtCreating( // create Vulkan debug extension
        VkDebugUtilsMessengerCreateInfoEXT*         info // Vulkan debug messenger create info
    );
    static void                             dflVulkDebugExtDestroying(); // delete Vulkan debug extension 
#endif 
static int                                  dflVulkPhysicalDevicePicking(); // pick physical device
static int                                  dflVulkPhysicalDeviceRanking( // rank physical device
    int                                     favoured // favoured physical device index
);
static int                                  dflVulkQueueFamilyPicking( // pick queue family
    VkPhysicalDevice                        physicalDev // physical device
); 

// !!! FUNCTION DEFINITIONS !!!
int dflWindowIniting(const char* windowName, int width, int height, int fullscreen)
{
    dflWindowName = windowName;
    
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
        return DFL_SDLV_INIT_ERROR;

    if(SDL_GetDesktopDisplayMode(0, display))
        return DFL_SDL_MONITOR_INFO_ERROR;

    if(!fullscreen)
        window = SDL_CreateWindow(windowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
    else 
        window = SDL_CreateWindow(windowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 2560, 1440, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);

    if(window == NULL)
        return DFL_SDL_WINDOW_INIT_ERROR;

    //dflSurface = SDL_GetWindowSurface(dflWindow)
    return DFL_SUCCESS;
}

void dflWindowKilling()
{
    //SDL_FreeSurface(dflSurface);
    SDL_DestroyWindow(window);
    SDL_Quit();

    free(display);
}

// VULKAN FUNCTIONS
int dflVulkanIniting()
{
    int error = 0;

    if(error = dflVulkInstanceMaking())
        return error;
    #ifndef DFL_NO_DEBUG 
        if(error = dflVulkLayersMaking())
            return error;
        if(error = dflVulkDebugMessengerMaking())
            return error;
    #endif
    if(error = dflVulkDeviceMaking())
        return error;

    return DFL_SUCCESS;
}

void dflVulkanKilling()
{ 
    vkDestroyDevice(dflGPU, NULL);
    #ifndef DFL_NO_DEBUG
        dflSetClearing(&dflVulkLayers);
        dflVulkDebugExtDestroying();
    #endif
    dflSetClearing(&dflVulkExtensions);
    vkDestroyInstance(dflVulkInstance, NULL);
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
        .apiVersion = VK_API_VERSION_1_0
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
        VkDebugUtilsMessengerCreateInfoEXT debugInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = 
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = 
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = dflVulkBugCallbacking,
            .pUserData = NULL
        };
        instanceInfo.enabledLayerCount = dflVulkLayers.size;
        instanceInfo.ppEnabledLayerNames = dflVulkLayers.data;
        instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugInfo;
    #else
        instanceInfo.enabledLayerCount = 0;
        instanceInfo.pNext = NULL;
    #endif

    if(vkCreateInstance(&instanceInfo, NULL, &dflVulkInstance) != VK_SUCCESS)
        return DFL_VULKINSTANCE_ERROR;

    return DFL_SUCCESS;
}

#ifndef DFL_NO_DEBUG
static int dflVulkLayersMaking()
{
    // we insert one layer here
    dflSetPushing(&dflVulkLayers, "VK_LAYER_KHRONOS_validation");

    int error = 0;

    if(error = dflVulkValLayersVerifying())
        return error;

    return DFL_SUCCESS;
}

static int dflVulkDebugMessengerMaking()
{
    VkDebugUtilsMessengerCreateInfoEXT messengerInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = dflVulkBugCallbacking,
        .pUserData = NULL
    };

    if(dflVulkDebugExtCreating(&messengerInfo) != VK_SUCCESS)
        return DFL_VULKDEBUG_MESSENGER_ERROR;
    
    return DFL_SUCCESS;
}
#endif

static int dflVulkDeviceMaking()
{
    int error = 0;
    int index = 0; 

    if(error = dflVulkPhysicalDevicePicking())
        return error;
    if(error = dflVulkPhysicalDeviceRanking(index))
        return error;

    dflRealGPU = physicalDevs[index];
    
    error = dflVulkQueueFamilyPicking(dflRealGPU);
    
    float graphicsPriority = 0.8f;
    float computePriority = 1.0f;
    VkDeviceQueueCreateInfo queueGraphicsInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = QFamIndices.graphics,
        .queueCount = 1,
        .pQueuePriorities = &graphicsPriority
    };
    VkDeviceQueueCreateInfo queueComputeInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = QFamIndices.compute,
        .queueCount = 1,
        .pQueuePriorities = &graphicsPriority
    };

    VkDeviceQueueCreateInfo queueInfo[2] = {queueGraphicsInfo, queueComputeInfo};

    VkDeviceCreateInfo devInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 2,
        .pQueueCreateInfos = queueInfo,
        .pEnabledFeatures = NULL,
        #ifndef DFL_NO_DEBUG
            .enabledLayerCount = dflVulkLayers.size,
            .ppEnabledLayerNames = dflVulkLayers.data,
        #else
            .enabledLayerCount = 0
        #endif
        .enabledExtensionCount = 0,
        .pNext = NULL,
    };

    if(vkCreateDevice(dflRealGPU, &devInfo, NULL, &dflGPU))
        return DFL_VULKDEVICE_ERROR;

    vkGetDeviceQueue(dflGPU, QFamIndices.graphics, 0, &dflGraphicsQueue);
    vkGetDeviceQueue(dflGPU, QFamIndices.compute, 0, &dflComputeQueue);

    return DFL_SUCCESS;
}
// VULKAN SUBSUBFUNCTIONS
static int dflVulkExtensionSupplying()
{
    unsigned int extensionCount = 0;
    if(SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, NULL) == SDL_FALSE)
        return DFL_SDL_VULKEXTENS_ENUM_ERROR;
    dflSetExtending(&dflVulkExtensions, extensionCount);
    if(SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, dflVulkExtensions.data) == SDL_FALSE)
        return DFL_SDL_VULKEXTENS_ERROR;

    #ifndef DFL_NO_DEBUG
        dflSetPushing(&dflVulkExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif

    return DFL_SUCCESS;
}

#ifndef DFL_NO_DEBUG
static int dflVulkValLayersVerifying()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    VkLayerProperties* layers = malloc(sizeof(VkLayerProperties) * layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers);

    short layerFound = 0;
    for(int j = 0; j < dflVulkLayers.size; j++)
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

static VKAPI_ATTR VkBool32 VKAPI_CALL dflVulkBugCallbacking(VkDebugUtilsMessageSeverityFlagBitsEXT severity, 
VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* data, void* userdata)
{
    if(severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        printf("Validation layer: %s\n", data->pMessage);

    return VK_FALSE;
}

static VkResult dflVulkDebugExtCreating(VkDebugUtilsMessengerCreateInfoEXT* info)
{
    PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(dflVulkInstance, "vkCreateDebugUtilsMessengerEXT");
    if(func != NULL)
        return func(dflVulkInstance, info, NULL, &dflDebugMessenger);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void dflVulkDebugExtDestroying()
{
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(dflVulkInstance, "vkDestroyDebugUtilsMessengerEXT");
    if(func != NULL)
        func(dflVulkInstance, dflDebugMessenger, NULL);
}
#endif

static int dflVulkPhysicalDevicePicking()
{
    vkEnumeratePhysicalDevices(dflVulkInstance, &physicalDevCount, NULL);
    if(physicalDevCount == 0)
        return DFL_VULKPHYSICAL_UNAVAILABLE_ERROR;
    physicalDevs = malloc(sizeof(VkLayerProperties) * physicalDevCount);
    vkEnumeratePhysicalDevices(dflVulkInstance, &physicalDevCount, physicalDevs);

    return DFL_SUCCESS;
}

static int dflVulkPhysicalDeviceRanking(int favoured)
{
    int score = 0;
    int error = 0;
    favoured = -1;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures   features;

    for(int i = 0; i < physicalDevCount; i++)
    {
        int curr_score = 0; 

        vkGetPhysicalDeviceProperties(physicalDevs[i], &properties);
        vkGetPhysicalDeviceFeatures(physicalDevs[i], &features);

        if(!features.geometryShader || (error = dflVulkQueueFamilyPicking(physicalDevs[i])))
            continue;

        if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            curr_score += 10000;

        curr_score += properties.limits.maxImageDimension2D;
        curr_score += properties.limits.maxImageDimension3D;

        if(score < curr_score)
        {
            score = curr_score;
            favoured = i;
        }  
    }

    if(favoured < 0)
    {
        switch(error){
            case DFL_SUCCESS:
                return DFL_VULKPHYSICAL_INCAPABLE_ERROR;
                break;
            default:
                return error;
                break;
        }
    }

    return DFL_SUCCESS;
}

static int dflVulkQueueFamilyPicking(VkPhysicalDevice physicalDev)
{
    unsigned int queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDev, &queueFamilyCount, NULL);
    if(!queueFamilyCount)
        return DFL_VULKQFAM_UNAVAILABLE_ERROR;
    VkQueueFamilyProperties* queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDev, &queueFamilyCount, queueFamilies);

    QFamIndices.graphics = -1;
    QFamIndices.present = -1;
    QFamIndices.compute = -1;

    for(int i = 0; i < queueFamilyCount; i++)
    {
        if(queueFamilies[i].queueCount > 0 && (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            QFamIndices.graphics = i;
            continue;
        }

        if(queueFamilies[i].queueCount > 0 && (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            QFamIndices.compute = i;
            continue;
        }
    }

    if(QFamIndices.compute < 0 || QFamIndices.graphics < 0)
        return DFL_VULKQFAM_INCAPABLE_ERROR;
    
    return DFL_SUCCESS;
}