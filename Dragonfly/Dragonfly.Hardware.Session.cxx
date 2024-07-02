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

#include "Dragonfly.Hardware.Session.hxx"

#include <iostream>
#include <vector>
#include <thread>
#include <array>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winternl.h>
#include <powerbase.h>

#include "Dragonfly.hxx"

namespace DflHW = Dfl::Hardware;

//

static inline uint32_t INT_GetProcessorCount() 
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwNumberOfProcessors;
}

static inline uint64_t INT_GetProcessorSpeed(const uint32_t& processorCount) 
{
    typedef struct _PROCESSOR_POWER_INFORMATION {
        ULONG Number;
        ULONG MaxMhz;
        ULONG CurrentMhz;
        ULONG MhzLimit;
        ULONG MaxIdleState;
        ULONG CurrentIdleState;
    } PROCESSOR_POWER_INFORMATION;
    std::vector<PROCESSOR_POWER_INFORMATION> cpuInfos(processorCount);
    
    {
        ULONG size{ 
                processorCount* static_cast<ULONG>(sizeof(PROCESSOR_POWER_INFORMATION)) };
        if (NTSTATUS status{ CallNtPowerInformation(
                            ProcessorInformation,
                            nullptr,
                            0,
                            cpuInfos.data(),
                            size) }; 
            status != 0) {
            return 0;
        }
    }

    return cpuInfos[0].CurrentMhz;
}

static inline auto INT_GetProcessor() 
-> DflHW::Session::Characteristics::Processor
{
    const uint32_t processorCount{ INT_GetProcessorCount() };

    return { processorCount, INT_GetProcessorSpeed(processorCount) };
}

static inline uint64_t INT_GetMemory()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    GlobalMemoryStatusEx(&memInfo);
    return ( memInfo.ullTotalPhys / Dfl::Mega );
}

static inline DflHW::Session::Characteristics INT_GetCharacteristics() 
{
    return { INT_GetProcessor(), INT_GetMemory() };
}

// internal for InitVulkan

static VKAPI_ATTR VkBool32 VKAPI_CALL INT_dflDebugCLBK(
    VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
    VkDebugUtilsMessageTypeFlagsEXT             type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* userData) 
{
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::cout << "\n\x1b[31mVULKAN ENCOUNTERED AN ERROR\n===ERROR===\x1b[0m\n <<" << data->pMessage << "\n";
        return VK_FALSE;
    }
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cout << "\n\x1b[33mVULKAN WARNS\n===WARNING===\x1b[0m\n <<" << data->pMessage << "\n";
        return VK_FALSE;
    }
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        std::cout << "\n\x1b[95mVULKAN INFORMS\n===INFO===\x1b[0m\n <<" << data->pMessage << "\n";
        return VK_FALSE;
    }

    return VK_FALSE;
}

static VkDebugUtilsMessengerEXT INT_InitDebugger(const VkInstance& instance)
{
    const VkDebugUtilsMessengerCreateInfoEXT debugInfo = {
        .sType{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT },
        .pNext{ nullptr },
        .flags{ 0 },
        .messageSeverity{
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT },
        .messageType{ VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT },
        .pfnUserCallback{ INT_dflDebugCLBK },
        .pUserData{ nullptr }
    };

    const PFN_vkCreateDebugUtilsMessengerEXT createDebug{
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT") };
    if (createDebug == nullptr) {
        vkDestroyInstance(
            instance,
            nullptr);
        throw Dfl::Error::HandleCreation(
                L"Unable to get required function handle for debugger",
                L"INT_InitDebugger");
    }

    VkDebugUtilsMessengerEXT debugger{ nullptr };
    if (createDebug(
            instance,
            &debugInfo,
            NULL,
            &debugger) != VK_SUCCESS) {
        vkDestroyInstance(
            instance,
            nullptr);
        throw Dfl::Error::HandleCreation(
                L"Unable to create debugger",
                L"INT_InitDebugger");
    }

    return debugger;
}

static std::vector<VkPhysicalDevice> INT_LoadDevices(const VkInstance& instance) 
{
    uint32_t deviceCount{ 0 };
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) 
    {
        throw Dfl::Error::NoData(
                L"Unable to find Vulkan compatible devices",
                L"INT_LoadDevice");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    return devices;
}

//

static DflHW::Session::Handles INT_InitSession(const DflHW::Session::Info& info)
{
    const VkApplicationInfo appInfo{
        .sType{ VK_STRUCTURE_TYPE_APPLICATION_INFO },
        .pNext{ nullptr },
        .pApplicationName{ info.AppName.data() },
        .applicationVersion{ info.AppVersion },
        .pEngineName{ "Dragonfly" },
        .apiVersion{ VK_API_VERSION_1_3 }
    };

    constexpr std::array<const char*, 4> extensions{
        VK_KHR_SURFACE_EXTENSION_NAME,
        "VK_KHR_win32_surface",
        VK_KHR_DISPLAY_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
    constexpr std::array<const char*, 1> expectedLayers{
        "VK_LAYER_KHRONOS_validation"
    };

    if ( info.DoDebug == true )
    {
        uint32_t count{ 0 };
        std::vector<VkLayerProperties> layerProperties{ };
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        if (count == 0)
        {
            throw Dfl::Error::NoData(
                    L"No layers found in machine",
                    L"INT_InitSession");
        }
        layerProperties.resize(count);
        vkEnumerateInstanceLayerProperties(&count, layerProperties.data());

        // check desired layer availability
        for (auto& layer : expectedLayers)
        {
            for ( auto pProperty{ layerProperties.begin() }; 
                  pProperty != layerProperties.end(); 
                  pProperty++ )
            {
                if ( strcmp(pProperty->layerName, layer) == 0 ) 
                {
                    break;
                }
                if ( pProperty == layerProperties.end() - 1 ) 
                {
                    throw Dfl::Error::NoData(
                            L"The requested layers are not present",
                            L"INT_InitSession");
                }
            }
        }
    }

    const VkInstanceCreateInfo instInfo = {
        .sType{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ 0 },
        .pApplicationInfo{ &appInfo },
        .enabledLayerCount{ info.DoDebug == true ? static_cast<uint32_t>(expectedLayers.size()) : 0 },
        .ppEnabledLayerNames{ expectedLayers.data() },
        .enabledExtensionCount{ static_cast<uint32_t>(extensions.size()) - (info.DoDebug == true ? 0 : 1) },
        .ppEnabledExtensionNames{ extensions.data() },
    };

    VkInstance instance{ nullptr };
    switch (const VkResult vkError{ vkCreateInstance(
                                       &instInfo,
                                       nullptr,
                                       &instance) }) 
    {
    case VK_SUCCESS:
        break;
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        throw Dfl::Error::HandleCreation(
                L"The Vulkan driver is incompatible",
                L"INT_InitSession");
    default:
        throw Dfl::Error::HandleCreation(
                L"Unable to create Vulkan instance",
                L"INT_InitSession");
    }

    return { instance, 
             info.DoDebug ? INT_InitDebugger(instance) : nullptr, 
             INT_LoadDevices(instance) };
}

DflHW::Session::Session(const Info& info) 
: pInfo( new Info(info) ), 
  pCharacteristics( new Characteristics(INT_GetCharacteristics()) ),
  Instance( INT_InitSession(info) ) { }

DflHW::Session::~Session()
{
    if (this->Instance.hDbMessenger != nullptr) // that means debugging was enabled.
    { 
        if ( const PFN_vkDestroyDebugUtilsMessengerEXT destroyDebug{
                (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                                                        this->Instance,
                                                        "vkDestroyDebugUtilsMessengerEXT") }; 
             destroyDebug != nullptr )
        {
            destroyDebug(
                this->Instance, 
                this->Instance.hDbMessenger, 
                nullptr);
        }
    }

    vkDestroyInstance(this->Instance, nullptr);
}

inline const std::string DflHW::Session::GetDeviceName(const uint32_t& index) const noexcept {
    if(index >= this->Instance.hDevices.size()) 
    {
        return "N/A";
    }
    
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(this->Instance.hDevices[index], &props);
    return props.deviceName;
}