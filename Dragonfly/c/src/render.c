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
int                             isSDLOn; // is SDL initialized? 
static SDL_Window*              window; // Window information
static short int                windowUse; // Whether to use a window or not

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
static int      dflVulkSurfaceMaking(); // make Vulkan surface

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
    int*                                    favoured // favoured physical device index
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
    
    isSDLOn = 1;

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
}

int dflDisplayCounting()
{
    switch(isSDLOn){
    case 0:
        if(SDL_Init(SDL_INIT_VIDEO) < 0)
            return -DFL_SDLV_INIT_ERROR;
        dflDisplayCount = SDL_GetNumVideoDisplays();
        SDL_Quit();
        break;
    default:
        dflDisplayCount = SDL_GetNumVideoDisplays();
        break;
    }

    return dflDisplayCount;
}

int dflDisplayInfoGetting(int displayNumber, int info){
    if(displayNumber > dflDisplayCount)
        return -DFL_WRONG_NUMBER_GIVEN_ERROR;

    switch(isSDLOn){
    case 0:
        if(SDL_Init(SDL_INIT_VIDEO) < 0)
            return -DFL_SDLV_INIT_ERROR;
        switch(info){
        case 0:
            SDL_GetDisplayMode(displayNumber, 0, &dflWidth);
            break;
        case 1:
            SDL_GetDisplayMode(displayNumber, 1, &dflHeight);
            break;
        case 2:
            SDL_GetDisplayMode(displayNumber, 5, &dflRefreshRate);
            break;
        default:
            SDL_Quit();
            return -DFL_WRONG_NUMBER_GIVEN_ERROR;
        }
        SDL_Quit();
        break;
    default:
        switch(info){
        case 0:
            SDL_GetDisplayMode(displayNumber, 0, &dflWidth);
            break;
        case 1:
            SDL_GetDisplayMode(displayNumber, 1, &dflHeight);
            break;
        case 2:
            SDL_GetDisplayMode(displayNumber, 5, &dflRefreshRate);
            break;
        default:
            return -DFL_WRONG_NUMBER_GIVEN_ERROR;
        }
        break;
    }

    switch(info){
        case 0:
            if(dflWidth.w > 0)
                return dflWidth.w;
            else
                return 1080;
        case 1:
            if(dflHeight.h > 0) 
                return dflHeight.h;
            else
                return 720;
        case 2:
            if(dflRefreshRate.refresh_rate > 0)
                return dflRefreshRate.refresh_rate;
            else
                return 60;
    }
}

// VULKAN FUNCTIONS
int dflVulkanIniting(short int useWindow)
{
    windowUse = useWindow;

    int error = 0;

    if(error = dflVulkInstanceMaking())
        return error;
    #ifndef DFL_NO_DEBUG 
        if(error = dflVulkLayersMaking())
            return error;
        if(error = dflVulkDebugMessengerMaking())
            return error;
    #endif
    switch(windowUse){
    case 1:
        if(error = dflVulkSurfaceMaking())
            return error;
        break;
    };
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
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "Dragonfly",
        .engineVersion = VK_MAKE_VERSION(0, 0, 4),
        .apiVersion = VK_API_VERSION_1_3
    }; 

    if(windowUse)
        appInfo.pApplicationName = "windowless";
    else 
        appInfo.pApplicationName = dflWindowName;

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
    if(error = dflVulkPhysicalDeviceRanking(&index))
        return error;

    dflRealGPU = physicalDevs[index];
    
    error = dflVulkQueueFamilyPicking(dflRealGPU);
    
    float graphicsPriority = 0.9f;
    float computePriority = 1.0f;
    float presentPriority = 0.8f;
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
        .pQueuePriorities = &computePriority
    };
    
    VkDeviceQueueCreateInfo queuePresentInfo = {};
    if(QFamIndices.graphics != QFamIndices.present)
    {
        queuePresentInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queuePresentInfo.queueFamilyIndex = QFamIndices.present;
        queuePresentInfo.queueCount = 1;
        queuePresentInfo.pQueuePriorities = &presentPriority;
    }

    VkDeviceQueueCreateInfo* queueInfo = malloc(2 * sizeof(VkDeviceQueueCreateInfo));
    queueInfo[0] = queueGraphicsInfo;
    queueInfo[1] = queueComputeInfo;

    int isSame = (QFamIndices.graphics == QFamIndices.present);
    if(!isSame){
        VkDeviceQueueCreateInfo* temp_info = realloc(queueInfo, 3 * sizeof(VkDeviceQueueCreateInfo));
        queueInfo = temp_info;
        queueInfo[2] = queuePresentInfo;
    };

    VkDeviceCreateInfo devInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
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

    switch(isSame){
    case 1:
        devInfo.queueCreateInfoCount = 2;
        break; 
    case 0:
        devInfo.queueCreateInfoCount = 3;
        break;
    }

    if(vkCreateDevice(dflRealGPU, &devInfo, NULL, &dflGPU))
        return DFL_VULKDEVICE_ERROR;

    vkGetDeviceQueue(dflGPU, QFamIndices.graphics, 0, &dflGraphicsQueue);
    vkGetDeviceQueue(dflGPU, QFamIndices.compute, 0, &dflComputeQueue);
    vkGetDeviceQueue(dflGPU, QFamIndices.present, 0, &dflPresentQueue);

    return DFL_SUCCESS;
}

static int dflVulkSurfaceMaking()
{
    if(!SDL_Vulkan_CreateSurface(window, dflVulkInstance, &dflSurface)) 
        return DFL_VULKSURFACE_ERROR;

    return DFL_SUCCESS;
}

// VULKAN SUBSUBFUNCTIONS
static int dflVulkExtensionSupplying()
{
    unsigned int extensionCount = 0;

    switch(windowUse){
    case 1: 
        if(SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, NULL) == SDL_FALSE)
            return DFL_SDL_VULKEXTENS_ENUM_ERROR;
        dflSetExtending(&dflVulkExtensions, extensionCount);
        if(SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, dflVulkExtensions.data) == SDL_FALSE)
            return DFL_SDL_VULKEXTENS_ERROR;
        break;
    }
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

static int dflVulkPhysicalDeviceRanking(int* favoured)
{
    int score = 0;
    int error = 0;
    *favoured = -1;

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

        if(QFamIndices.graphics == QFamIndices.present)
            curr_score += 5000;

        if(score < curr_score)
        {
            score = curr_score;
            *favoured = i;
        }
    }

    if(*favoured < 0)
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
    QFamIndices.compute = -1;

    switch(windowUse){
    case 1: 
        QFamIndices.present = -1;
        break;
    case 0:
        QFamIndices.present = DFL_DUMMY_VALUE;
        break;
    }


    for(int i = 0; i < queueFamilyCount; i++)
    {
        if(queueFamilies[i].queueCount > 0 && (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            QFamIndices.graphics = i;

            // Graphics queue may also be presentation queue
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDev, i, dflSurface, &presentSupport);
            if(presentSupport && windowUse)
                QFamIndices.present = i;

            continue;
        }

        if(queueFamilies[i].queueCount > 0 && (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            QFamIndices.compute = i;
            continue;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDev, i, dflSurface, &presentSupport);
        if(queueFamilies[i].queueCount > 0 && windowUse && presentSupport)
        {
            QFamIndices.present = i;
            printf("Present queue family found\n");
            continue;
        }
    }

    if(QFamIndices.compute < 0 || QFamIndices.graphics < 0 || QFamIndices.present < 0)
        return DFL_VULKQFAM_INCAPABLE_ERROR;
    
    return DFL_SUCCESS;
}