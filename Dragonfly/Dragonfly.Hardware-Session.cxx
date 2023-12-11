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

#include <iostream>
#include <vector>
#include <thread>

#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

module Dragonfly.Hardware:Session;

import Dragonfly.Observer;

namespace DflHW = Dfl::Hardware;
namespace DflOb = Dfl::Observer;

DflHW::Session::Session(const SessionInfo& info) : GeneralInfo(info)
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    this->CPU.Count = sysInfo.dwNumberOfProcessors;
}

DflHW::Session::~Session()
{
    if (this->VkDbMessenger != nullptr) // that means debugging was enabled.
    {
        PFN_vkDestroyDebugUtilsMessengerEXT destroyDebug{ (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->VkInstance, "vkDestroyDebugUtilsMessengerEXT") };
        if (destroyDebug != nullptr)
            destroyDebug(this->VkInstance, this->VkDbMessenger, nullptr);
    }

    if(this->VkInstance != nullptr)
        vkDestroyInstance(this->VkInstance, nullptr);
}

// internal for InitVulkan

static VKAPI_ATTR VkBool32 VKAPI_CALL _dflDebugCLBK(
    VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
    VkDebugUtilsMessageTypeFlagsEXT             type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* userData)
{
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        std::cout << "\n\x1b[31mVULKAN ENCOUNTERED AN ERROR\n===ERROR===\x1b[0m\n <<" << data->pMessage << "\n";
        return VK_FALSE;
    }
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        std::cout << "\n\x1b[33mVULKAN WARNS\n===WARNING===\x1b[0m\n <<" << data->pMessage << "\n";
        return VK_FALSE;
    }
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        std::cout << "\n\x1b[95mVULKAN INFORMS\n===INFO===\x1b[0m\n <<" << data->pMessage << "\n";
        return VK_FALSE;
    }

    return VK_FALSE;
}

static DflHW::SessionError _InitDebugger(
    const VkInstance& instance,
    VkDebugUtilsMessengerEXT& debugger)
{
    VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
    debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugInfo.pNext = nullptr;
    debugInfo.flags = 0;
    debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugInfo.pfnUserCallback = _dflDebugCLBK;
    debugInfo.pUserData = nullptr;

    PFN_vkCreateDebugUtilsMessengerEXT createDebug{ (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT") };
    if (createDebug == nullptr)
        return DflHW::SessionError::VkDebuggerCreationError;

    if (createDebug(instance, &debugInfo, NULL, &debugger) != VK_SUCCESS)
        return DflHW::SessionError::VkDebuggerCreationError;

    return DflHW::SessionError::Success;
}

DflHW::SessionError DflHW::_LoadDevices(Session& session)
{
    uint32_t deviceCount{ 0 };
    vkEnumeratePhysicalDevices(session.VkInstance, &deviceCount, nullptr);
    if (deviceCount == 0)
        return SessionError::VkNoDevicesError;
    session.Devices.resize(deviceCount); // we now reserve space for both the physical GPUs and the ones Dragonfly understands
    vkEnumeratePhysicalDevices(session.VkInstance, &deviceCount, session.Devices.data());

    return SessionError::Success;
}

//

DflHW::SessionError DflHW::Session::InitSession()
{
    VkApplicationInfo appInfo =
    {
        .sType{ VK_STRUCTURE_TYPE_APPLICATION_INFO },
        .pNext{ nullptr },
        .pApplicationName{ this->GeneralInfo.AppName.data() },
        .applicationVersion{ this->GeneralInfo.AppVersion },
        .pEngineName{ "Dragonfly" },
        .apiVersion{ VK_API_VERSION_1_3 }
    };

    VkInstanceCreateInfo instInfo =
    {
        .sType{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ 0 },
        .pApplicationInfo{ &appInfo }
    };

    std::vector<const char*> extensions;
    if (this->GeneralInfo.EnableOnscreenRendering == true)
    {
        extensions.push_back(static_cast<const char*>(VK_KHR_SURFACE_EXTENSION_NAME));
        extensions.push_back(static_cast<const char*>("VK_KHR_win32_surface"));
        extensions.push_back(static_cast<const char*>(VK_KHR_DISPLAY_EXTENSION_NAME));
    }

    std::vector<const char*> expectedLayers{ "VK_LAYER_KHRONOS_validation" };
    if (this->GeneralInfo.DoDebug == true)
    {
        extensions.push_back(static_cast<const char*>(VK_EXT_DEBUG_UTILS_EXTENSION_NAME));

        uint32_t count;
        std::vector<VkLayerProperties> layerProperties;
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        if (count == 0)
            return Dfl::Hardware::SessionError::VkNoLayersError;
        layerProperties.resize(count);
        vkEnumerateInstanceLayerProperties(&count, layerProperties.data());

        // check desired layer availability
        for (const char* layer : expectedLayers)
        {
            for (uint32_t i{ 0 }; i < count; i++)
            {
                if (strcmp(layerProperties[i].layerName, layer) == 0)
                    break;
                if (++i == count)
                    return SessionError::VkNoExpectedLayersError;
            }
        }
    }

    instInfo.enabledLayerCount = (this->GeneralInfo.DoDebug == true) ? static_cast<uint32_t>(expectedLayers.size()) : 0;
    instInfo.ppEnabledLayerNames = (this->GeneralInfo.DoDebug == true) ? expectedLayers.data() : nullptr;

    instInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instInfo.ppEnabledExtensionNames = extensions.data();

    VkResult vkError{ vkCreateInstance(&instInfo, nullptr, &this->VkInstance) };
    switch (vkError)
    {
    case VK_SUCCESS:
        break;
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return SessionError::VkBadDriverError;
    default:
        return SessionError::VkInstanceCreationError;
    }

    SessionError err{ SessionError::Success };
    if (this->GeneralInfo.DoDebug == true)
        err = _InitDebugger(this->VkInstance, this->VkDbMessenger);

    if (err != SessionError::Success)
        vkDestroyInstance(this->VkInstance, nullptr);

    err = _LoadDevices(*this);
    if (err != SessionError::Success)
        vkDestroyInstance(this->VkInstance, nullptr);

    return err;
}

