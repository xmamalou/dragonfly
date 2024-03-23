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

module;

#include <array>
#include <iostream>
#include <vector>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winternl.h>
#include <powerbase.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

module Dragonfly.Hardware.Session;

import Dragonfly.Observer;

namespace DflHW = Dfl::Hardware;
namespace DflOb = Dfl::Observer;

// PROCESSOR_POWER_INFORMATION definition

typedef struct _PROCESSOR_POWER_INFORMATION {
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, * PPROCESSOR_POWER_INFORMATION;

//

static inline uint32_t INT_GetProcessorCount(){
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    return sysInfo.dwNumberOfProcessors;
};

static inline uint64_t INT_GetProcessorSpeed(const uint32_t& processorCount) {
    
    std::vector<PROCESSOR_POWER_INFORMATION> cpuInfos;
    cpuInfos.resize(processorCount);
    
    ULONG size{ processorCount*sizeof(PROCESSOR_POWER_INFORMATION) };
    
    NTSTATUS status{ CallNtPowerInformation(
                        ProcessorInformation,
                        nullptr,
                        0,
                        cpuInfos.data(),
                        size) };

    if (status != 0) {
        return 0;
    }

    return cpuInfos[0].CurrentMhz;
}

static inline DflHW::Session::Processor INT_GetProcessor() {
    const uint32_t processorCount{ INT_GetProcessorCount() };

    return { processorCount, INT_GetProcessorSpeed(processorCount) };
}

static inline uint64_t INT_GetMemory() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    GlobalMemoryStatusEx(&memInfo);
    return ( memInfo.ullTotalPhys / (1024*1024) );
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

static DflHW::Session::Error INT_InitDebugger(
    const VkInstance& instance,
    VkDebugUtilsMessengerEXT& debugger){
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
        return DflHW::Session::Error::VkDebuggerCreationError;
    }
    if (createDebug(
        instance,
        &debugInfo,
        NULL,
        &debugger) != VK_SUCCESS) {
        return DflHW::Session::Error::VkDebuggerCreationError;
    }

    return DflHW::Session::Error::Success;
}

static DflHW::Session::Error INT_LoadDevices(
    const VkInstance& instance, 
    std::vector<VkPhysicalDevice>& devices){
    uint32_t deviceCount{ 0 };
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
        return DflHW::Session::Error::VkNoDevicesError;
    devices.resize(deviceCount); // we now reserve space for both the physical GPUs and the ones Dragonfly understands
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    return DflHW::Session::Error::Success;
}

const std::array<const char*, 1> expectedLayers{
    "VK_LAYER_KHRONOS_validation"
 };

//

static DflHW::Session::Handles INT_InitSession(const DflHW::Session::Info& info){
    const VkApplicationInfo appInfo = {
        .sType{ VK_STRUCTURE_TYPE_APPLICATION_INFO },
        .pNext{ nullptr },
        .pApplicationName{ info.AppName.data() },
        .applicationVersion{ info.AppVersion },
        .pEngineName{ "Dragonfly" },
        .apiVersion{ VK_API_VERSION_1_3 }
    };

    std::vector<const char*> extensions{
        VK_KHR_SURFACE_EXTENSION_NAME,
        "VK_KHR_win32_surface",
        VK_KHR_DISPLAY_EXTENSION_NAME
    };

    if ( info.DoDebug == true ){
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        uint32_t count{ 0 };
        std::vector<VkLayerProperties> layerProperties{ };
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        if (count == 0){
            throw DflHW::Session::Error::VkNoLayersError;
        }
        layerProperties.resize(count);
        vkEnumerateInstanceLayerProperties(&count, layerProperties.data());

        // check desired layer availability
        for (auto& layer : expectedLayers)
        {
            for (uint32_t i{ 0 }; i < count; i++)
            {
                if (strcmp(layerProperties[i].layerName, layer) == 0) {
                    break;
                }
                if ( i == count - 1) {
                    throw DflHW::Session::Error::VkNoExpectedLayersError;
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
        .ppEnabledLayerNames{ info.DoDebug == true ? expectedLayers.data() : nullptr },
        .enabledExtensionCount{ static_cast<uint32_t>(extensions.size()) },
        .ppEnabledExtensionNames{ extensions.data() },
    };

    VkInstance instance{ nullptr };
    switch (const VkResult vkError{ vkCreateInstance(
                                   &instInfo,
                                   nullptr,
                                   &instance) }) {
    case VK_SUCCESS:
        break;
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        throw DflHW::Session::Error::VkBadDriverError;
    default:
        throw DflHW::Session::Error::VkInstanceCreationError;
    }

    VkDebugUtilsMessengerEXT dbMessenger{ nullptr };
    DflHW::Session::Error err{ DflHW::Session::Error::Success };
    if (info.DoDebug == true) {
        err = INT_InitDebugger(instance, dbMessenger);
    }

    if (err != DflHW::Session::Error::Success) {
        vkDestroyInstance(instance, nullptr);
        throw err;
    }

    std::vector<VkPhysicalDevice> devices{};
    err = INT_LoadDevices(instance, devices);
    if (err != DflHW::Session::Error::Success) {
        vkDestroyInstance(instance, nullptr);
        throw err;
    }

    return { instance, dbMessenger, devices };
}

DflHW::Session::Session(const Info& info) 
try : GeneralInfo(new Info(info)), 
      CPU( INT_GetProcessor() ),
      Memory( INT_GetMemory() ),
      Instance( INT_InitSession(info)) { }
catch (Error e) { this->ErrorCode = e; }

DflHW::Session::~Session()
{
    if (this->Instance.hDbMessenger != nullptr){ // that means debugging was enabled.
        const PFN_vkDestroyDebugUtilsMessengerEXT destroyDebug{ 
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                                                    this->Instance, 
                                                    "vkDestroyDebugUtilsMessengerEXT") };
        if ( destroyDebug != nullptr ){
            destroyDebug(
                this->Instance, 
                this->Instance.hDbMessenger, 
                nullptr);
        }
    }

    if( this->Instance.hInstance != nullptr ){
        vkDestroyInstance(this->Instance, nullptr);
    }
}

inline const std::string DflHW::Session::GetDeviceName(const uint32_t& index) const noexcept {
    if(index >= this->Instance.hDevices.size()) {
        return "N/A";
    }
    
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(this->Instance.hDevices[index], &props);
    return props.deviceName;
}