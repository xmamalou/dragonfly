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
#include <optional>

#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

module Dragonfly.Hardware:Session;

namespace DflHW = Dfl::Hardware;
namespace DflOb = Dfl::Observer;

void DflHW::DebugProcess::operator() (DflOb::WindowProcessArgs& args)
{
    
};

void DflHW::DebugProcess::Destroy ()
{

};

inline bool _DoesSupportSRGB(VkColorSpaceKHR& colorSpace, const std::vector<VkSurfaceFormatKHR>& formats)
{
    for (auto& format : formats)
    {
        colorSpace = format.colorSpace;
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB)
            return true;
    }

    colorSpace = formats[0].colorSpace;

    return false;
};

inline bool _DoesSupportMailbox(const std::vector<VkPresentModeKHR>& modes)
{
    for (auto& mode : modes)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return true;
    }

    return false;
};

inline VkExtent2D _MakeExtent(const std::tuple<uint32_t, uint32_t>& resolution, const VkSurfaceCapabilitiesKHR& capabs)
{
    VkExtent2D extent;

    uint32_t minHeight = max(capabs.minImageExtent.height, std::get<1>(resolution));
    uint32_t minWidth = max(capabs.minImageExtent.width, std::get<0>(resolution));

    extent.height = min(minHeight, capabs.maxImageExtent.height);
    extent.width = min(minWidth, capabs.maxImageExtent.width);

    return extent;
}

DflHW::SessionError _CreateSwapchain(DflHW::SharedRenderResources& resources)
{
    VkSwapchainCreateInfoKHR swapchainInfo;
    VkColorSpaceKHR colorSpace;
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.pNext = nullptr;
    swapchainInfo.flags = 0;
    swapchainInfo.oldSwapchain = resources.Swapchain;
    swapchainInfo.surface = resources.Surface;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageFormat = (_DoesSupportSRGB(colorSpace, resources.Formats)) ? VK_FORMAT_B8G8R8A8_SRGB : resources.Formats[0].format;
    swapchainInfo.imageColorSpace = colorSpace;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = (_DoesSupportMailbox(resources.PresentModes)) ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
    swapchainInfo.imageExtent = _MakeExtent(resources.AssociatedWindow->Resolution(), resources.Capabilities);
    swapchainInfo.minImageCount = resources.Capabilities.minImageCount + 1;
    swapchainInfo.preTransform = resources.Capabilities.currentTransform;

    auto error = vkCreateSwapchainKHR(resources.GPU, &swapchainInfo, nullptr, &resources.Swapchain);
    switch (error)
    {
    case VK_SUCCESS:
        break;
    case VK_ERROR_SURFACE_LOST_KHR:
        return DflHW::SessionError::VkSwapchainSurfaceLostError;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return DflHW::SessionError::VkSwapchainWindowUnavailableError;
    default:
        return DflHW::SessionError::VkSwapchainInitError;
    }

    return DflHW::SessionError::Success;
};

void DflHW::RenderingSurface::operator() (DflOb::WindowProcessArgs& args)
{
    uint32_t imageCount = 0;
    switch (this->State)
    {
    case DflHW::RenderingState::Init:
        VkCommandPoolCreateInfo cmdPoolInfo;
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.pNext = nullptr;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmdPoolInfo.queueFamilyIndex = this->pSharedResources->AssignedQueue.FamilyIndex;

        this->pSharedResources->pDeviceMutex->lock();
        if (vkCreateCommandPool(this->pSharedResources->GPU, &cmdPoolInfo, nullptr, &(this->pSharedResources->CmdPool)) != VK_SUCCESS)
        {
            this->Error = DflHW::SessionError::VkComPoolInitError;
            this->State = DflHW::RenderingState::Fail;
            this->pSharedResources->pDeviceMutex->unlock();
            break;
        }
        //this->pMutex->unlock();
        
        this->Error = _CreateSwapchain(*this->pSharedResources);
        if(this->Error != DflHW::SessionError::Success)
        {
            this->State = DflHW::RenderingState::Fail;
            this->pSharedResources->pDeviceMutex->unlock();
            break;
        }
       
        vkGetSwapchainImagesKHR(this->pSharedResources->GPU, this->pSharedResources->Swapchain, &imageCount, nullptr);
        this->pSharedResources->SwapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(this->pSharedResources->GPU, this->pSharedResources->Swapchain, &imageCount, this->pSharedResources->SwapchainImages.data());

        this->State = DflHW::RenderingState::Loop;
        this->pSharedResources->pDeviceMutex->unlock();
        break;

    case DflHW::RenderingState::Fail:
        this->pSharedResources->pDeviceMutex->lock();
        if (this->pSharedResources->Swapchain != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(this->pSharedResources->GPU);
            vkDestroySwapchainKHR(this->pSharedResources->GPU, this->pSharedResources->Swapchain, nullptr);
        }
        if(this->pSharedResources->CmdPool != VK_NULL_HANDLE)
            vkDestroyCommandPool(this->pSharedResources->GPU, this->pSharedResources->CmdPool, nullptr);
        this->pSharedResources->pDeviceMutex->unlock();
        this->State = DflHW::RenderingState::Dead;
        break;

    case DflHW::RenderingState::Dead:
        break;

    case DflHW::RenderingState::Loop:
        std::cout << "";
        break;
    }
};

void DflHW::RenderingSurface::Destroy()
{
    this->pSharedResources->pDeviceMutex->lock();
    if (this->pSharedResources->Swapchain != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(this->pSharedResources->GPU);
        vkDestroySwapchainKHR(this->pSharedResources->GPU, this->pSharedResources->Swapchain, nullptr);
    }
    if (this->pSharedResources->CmdPool != VK_NULL_HANDLE)
        vkDestroyCommandPool(this->pSharedResources->GPU, this->pSharedResources->CmdPool, nullptr);
    this->pSharedResources->pDeviceMutex->unlock();
};

DflHW::Session::Session(const SessionInfo& info) : GeneralInfo(info)
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    this->CPU.Count = sysInfo.dwNumberOfProcessors;
}

DflHW::Session::~Session()
{
    for (auto& device : this->Devices)
    {
        this->DeviceMutex.lock();
        for (auto& surface : device.Surfaces)
            vkDestroySurfaceKHR(this->VkInstance, surface.pSharedResources->Surface, nullptr);

        if (device.VkGPU != nullptr)
        {
            vkDeviceWaitIdle(device.VkGPU);
            vkDestroyDevice(device.VkGPU, nullptr);
        }
        this->DeviceMutex.unlock();
    }

    if (this->VkDbMessenger != nullptr) // that means debugging was enabled.
    {
        PFN_vkDestroyDebugUtilsMessengerEXT destroyDebug = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->VkInstance, "vkDestroyDebugUtilsMessengerEXT");
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
    VkDebugUtilsMessengerCreateInfoEXT debugInfo;
    debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugInfo.pNext = nullptr;
    debugInfo.flags = 0;
    debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugInfo.pfnUserCallback = _dflDebugCLBK;
    debugInfo.pUserData = nullptr;

    PFN_vkCreateDebugUtilsMessengerEXT createDebug = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (createDebug == nullptr)
        return DflHW::SessionError::VkDebuggerCreationError;

    if (createDebug(instance, &debugInfo, NULL, &debugger) != VK_SUCCESS)
        return DflHW::SessionError::VkDebuggerCreationError;

    return DflHW::SessionError::Success;
}

// internal for _LoadDevices

template <DflHW::MemoryType type>
inline void _OrganizeMemory(std::vector<DflHW::DeviceMemory<type>>& memory, const VkPhysicalDeviceMemoryProperties props)
{
    uint32_t heapCount = 0;
    for (uint32_t i = 0; i < props.memoryHeapCount; i++)
    {
        if (((props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) && (type == DflHW::MemoryType::Local)) || (!(props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) && (type == DflHW::MemoryType::Shared)))
        {
            memory[heapCount].Size = props.memoryHeaps[i].size;
            memory[heapCount].HeapIndex = props.memoryTypes[i].heapIndex;

            memory[heapCount].IsHostVisible = (props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ? true : false;

            heapCount++;
        }
    }
}

void _OrganizeData(Dfl::Hardware::Device& device)
{
    VkPhysicalDeviceProperties devProps;
    vkGetPhysicalDeviceProperties(device.VkPhysicalGPU, &devProps);
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(device.VkPhysicalGPU, &memProps);
    VkPhysicalDeviceFeatures devFeats;
    vkGetPhysicalDeviceFeatures(device.VkPhysicalGPU, &devFeats);

    device.Name = devProps.deviceName;

    int localHeapCount = 0;
    int sharedHeapCount = 0;

    for (auto heap : memProps.memoryHeaps)
    {
        if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            localHeapCount++;
        else
            sharedHeapCount++;
    }

    device.LocalHeaps.resize(localHeapCount);
    device.SharedHeaps.resize(sharedHeapCount);

    _OrganizeMemory(device.LocalHeaps, memProps);
    _OrganizeMemory(device.SharedHeaps, memProps);

    device.MaxGroups = std::make_tuple(devProps.limits.maxComputeWorkGroupCount[0], devProps.limits.maxComputeWorkGroupCount[1], devProps.limits.maxComputeWorkGroupCount[2]);
}

//

DflHW::SessionError DflHW::_LoadDevices(Session& session)
{
    uint32_t deviceCount;
    std::vector<VkPhysicalDevice> devices;
    vkEnumeratePhysicalDevices(session.VkInstance, &deviceCount, nullptr);
    if (deviceCount == 0)
        return SessionError::VkNoDevicesError;
    devices.resize(deviceCount);
    session.Devices.resize(deviceCount); // we now reserve space for both the physical GPUs and the ones Dragonfly understands
    vkEnumeratePhysicalDevices(session.VkInstance, &deviceCount, devices.data());

    for (uint32_t i = 0; i < deviceCount; i++)
    {
        session.Devices[i].VkPhysicalGPU = devices[i];

        _OrganizeData(session.Devices[i]);
    }

    return SessionError::Success;
}

//

DflHW::SessionError DflHW::Session::InitVulkan()
{
    VkApplicationInfo appInfo =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = this->GeneralInfo.AppName.data(),
        .applicationVersion = this->GeneralInfo.AppVersion,
        .pEngineName = "Dragonfly",
        .apiVersion = VK_API_VERSION_1_3
    };

    VkInstanceCreateInfo instInfo =
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &appInfo
    };

    std::vector<const char*> extensions;
    if (this->GeneralInfo.EnableOnscreenRendering == true)
    {
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        extensions.push_back("VK_KHR_win32_surface");
        extensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
    }

    std::vector<const char*> expectedLayers = { "VK_LAYER_KHRONOS_validation" };
    if (this->GeneralInfo.DoDebug == true)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

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
            for (uint32_t i = 0; i < count; i++)
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

    VkResult vkError = vkCreateInstance(&instInfo, nullptr, &this->VkInstance);
    switch (vkError)
    {
    case VK_SUCCESS:
        break;
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return SessionError::VkBadDriverError;
    default:
        return SessionError::VkInstanceCreationError;
    }

    SessionError err = SessionError::Success;
    if (this->GeneralInfo.DoDebug == true)
        err = _InitDebugger(this->VkInstance, this->VkDbMessenger);

    if (err != SessionError::Success)
        vkDestroyInstance(this->VkInstance, nullptr);

    err = _LoadDevices(*this);
    if (err != SessionError::Success)
        vkDestroyInstance(this->VkInstance, nullptr);

    return err;
}

// internal for InitDevice

DflHW::SessionError DflHW::_GetRenderResources(
    DflHW::SharedRenderResources& resources, 
    const DflHW::Device& device, 
    const VkInstance& instance,
    const DflOb::Window& window)
{
    if (window.WindowHandle() != nullptr) // ensures the window is valid
    {
        VkWin32SurfaceCreateInfoKHR surfaceInfo = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .hwnd = window.WindowHandle()
        };
        if (vkCreateWin32SurfaceKHR(instance, &surfaceInfo, nullptr, &resources.Surface) != VK_SUCCESS)
            return DflHW::SessionError::VkSurfaceCreationError;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.VkPhysicalGPU, resources.Surface, &resources.Capabilities);

        uint32_t  count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device.VkPhysicalGPU, resources.Surface, &count, nullptr);
        if (count == 0)
            return DflHW::SessionError::VkNoSurfaceFormatsError;
        resources.Formats.resize(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device.VkPhysicalGPU, resources.Surface, &count, resources.Formats.data());

        vkGetPhysicalDeviceSurfacePresentModesKHR(device.VkPhysicalGPU, resources.Surface, &count, nullptr);
        if (count == 0)
            return DflHW::SessionError::VkNoSurfacePresentModesError;
        resources.PresentModes.resize(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device.VkPhysicalGPU, resources.Surface, &count, resources.PresentModes.data());
    }
    else
        return DflHW::SessionError::InvalidWindowHandleError;

    return DflHW::SessionError::Success;
}

std::vector<float> _QueuePriorities(uint32_t size)
{
    std::vector<float> priorities(size);
    for (auto& priority : priorities)
        priority = 1.0;

    return priorities;
}

DflHW::SessionError DflHW::_GetQueues(
    std::vector<VkDeviceQueueCreateInfo>& infos, 
    Device& device)
{
    std::vector<VkQueueFamilyProperties> props;
    uint32_t queueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device.VkPhysicalGPU, &queueFamilies, nullptr);
    if (queueFamilies == 0)
        return DflHW::SessionError::VkNoAvailableQueuesError;
    props.resize(queueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(device.VkPhysicalGPU, &queueFamilies, props.data());
    
    uint32_t requiredQueues = static_cast<uint32_t>(device.Surfaces.size()) + static_cast<uint32_t>(device.Simulations.size()) + 1;
    // ^ Why + 1? If the user doesn't want any sims or any rendering, we assume that they may, at least
    // want to use the device for some kind of computational work.
    for (uint32_t i = 0; i < queueFamilies; i++)
    {
        QueueFamily family; // this is used for caching

        VkDeviceQueueCreateInfo info = {};
        if ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (device.Surfaces.size() != 0))
        {
            family.QueueType |= static_cast<int>(DflHW::QueueType::Graphics);

            VkBool32 canPresent = false;
            int forNextQueue = 0; 
            // ^ this is the number of surfaces (and subsequently queues) that need to be presented but 
            // the current queue family is unable to do such a thing

            for (auto& surface : device.Surfaces)
            {
                vkGetPhysicalDeviceSurfaceSupportKHR(device.VkPhysicalGPU, i, surface.pSharedResources->Surface, &canPresent);

                if (canPresent == false)
                    forNextQueue++;
                else
                    family.CanPresent = true;
            }

            // a lot of times, graphics queues also support computations, so we gotta check for this too
            // since computational queues are going to be used for simulations, transfer capabilities are 
            // also a requirement
            if ((props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && (props[i].queueFlags & VK_QUEUE_TRANSFER_BIT))
            {
                forNextQueue -= static_cast<uint32_t>(device.Simulations.size()) + 1;
                family.QueueType |= static_cast<int>(DflHW::QueueType::Simulation);
            }
            // ^ Instead of declaring another variable, we do the trick of passing the info of the availability of a computation queue 
            // down by reducing the surfaces that aren't supported for presentation by the current queue. 
            // Dragonfly does not use the amount of "passed over" surfaces explicitly for any other reason than to know 
            // how many queues there are left to claim, so there's not really an issue using this "hack"

            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.queueFamilyIndex = i;
                // v why requiredQueues - SimulationsSize? Because we do not implicitly assume that the queue family also supports computations
                // If it does, then forNextQueue is reduced by SimulationsSize, so:
                // (n - p) - (m - p) = n - m, in other words, we request all requiredQueues (minus the actual forNextQueues)
                // This also works even when p > m, as seen above.
            info.queueCount = (static_cast<int>(props[i].queueCount) >= (requiredQueues - forNextQueue - static_cast<int>(device.Simulations.size()))) ? (requiredQueues - forNextQueue - 1) : props[i].queueCount;

            requiredQueues -= info.queueCount;

            infos.push_back(info);
        }
        else if ((props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && (props[i].queueFlags & VK_QUEUE_TRANSFER_BIT))
        {
            // if the above "failed" (the family doesn't support graphics), then we got to check if it's an exclusively 
            // computational family
            // since such queues are going to be used for simulations, transfer capabilities are a requirement

            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.queueFamilyIndex = i;
            info.queueCount = (props[i].queueCount >= static_cast<uint32_t>(device.Simulations.size())) ? static_cast<uint32_t>(device.Simulations.size()) : props[i].queueCount;

            requiredQueues -= info.queueCount;

            infos.push_back(info);
        }

        family.QueueCount = info.queueCount;
        family.Index = i;
        // ^ we only consider that a queue Family has as many queues as we told it we want. This will also help 
        // with requesting the actual queue handles themselves later

        device.QueueFamilies.push_back(family);

        if (requiredQueues <= 0)
            break;
    }

    if (requiredQueues > 0)
        return DflHW::SessionError::VkInsufficientQueuesWarning;

    return DflHW::SessionError::Success;
}

//

DflHW::SessionError DflHW::Session::InitDevice(const GPUInfo& info)
{
    // TODO: Implement ranking system
    if (!info.DeviceIndex.has_value())
        return SessionError::VkDeviceIndexOutOfRangeError;

    if (info.DeviceIndex.value() > this->Devices.size())
        return SessionError::VkDeviceIndexOutOfRangeError;

    Device& device = this->Devices[info.DeviceIndex.value()];
    if (this->Devices.size() == 0)
        return SessionError::VkNoDevicesError;

    if (device.VkGPU != nullptr)
        return SessionError::VkAlreadyInitDeviceWarning;

    

    device.Info = info;
    device.pUsageMutex = &this->DeviceMutex;

    std::vector<const char*> extensions;

    device.Surfaces.resize(info.pDstWindows.size());
    device.Simulations.resize(info.PhysicsSimsNumber);

    for (uint32_t i = 0; i < info.pDstWindows.size(); i++)
    {
        device.Surfaces[i].pSharedResources = std::unique_ptr<DflHW::SharedRenderResources>::unique_ptr(new DflHW::SharedRenderResources);
        device.Surfaces[i].pSharedResources->pDeviceMutex = &this->DeviceMutex;
        auto& resources = device.Surfaces[i].pSharedResources;
        auto error = _GetRenderResources(*resources, device, this->VkInstance, *info.pDstWindows[i]);
        if(error < DflHW::SessionError::Success)
            return error;
        resources->AssociatedWindow = info.pDstWindows[i];
    }

    VkDeviceCreateInfo deviceInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    std::vector<VkDeviceQueueCreateInfo> queueInfo;
    auto error = _GetQueues(queueInfo, device);
    if (error < DflHW::SessionError::Success)
        return error;

    std::vector<std::vector<float>> queuePriorities(queueInfo.size());
    for (auto& queue : queueInfo)
    {
        queuePriorities[&queue - queueInfo.data()] = _QueuePriorities(queue.queueCount);
        queue.pQueuePriorities = queuePriorities[&queue - queueInfo.data()].data();
    }

    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfo.size());
    deviceInfo.pQueueCreateInfos = queueInfo.data();

    if ((this->GeneralInfo.EnableOnscreenRendering) && (device.Info.EnableOnscreenRendering))
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    if ((this->GeneralInfo.EnableRaytracing) && (device.Info.EnableRaytracing))
    {
        extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        extensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        extensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
    }

    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    deviceInfo.ppEnabledExtensionNames = (extensions.empty()) ? nullptr : extensions.data();

    VkResult vkError = vkCreateDevice(
        device.VkPhysicalGPU,
        &deviceInfo,
        nullptr,
        &device.VkGPU);
    switch (vkError)
    {
    case VK_SUCCESS:
        break;
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return SessionError::VkDeviceNoSuchExtensionError;
    case VK_ERROR_DEVICE_LOST:
        return SessionError::VkDeviceLostError;
    default:
        return SessionError::VkDeviceInitError;
    }

    uint32_t i = 0;
    for (auto& surface : device.Surfaces)
    {
        QueueFamily& family = device.QueueFamilies[i];
        vkGetDeviceQueue(device.VkGPU, family.Index, family.UsedQueues, &surface.pSharedResources->AssignedQueue.hQueue);
        surface.pSharedResources->AssignedQueue.FamilyIndex = family.Index;
        family.UsedQueues++;
        if(family.UsedQueues >= family.QueueCount)
            i++;
        surface.pSharedResources->GPU = device.VkGPU;
    }

    return DflHW::SessionError::Success;
}

Dfl::Hardware::SessionError Dfl::Hardware::CreateRenderer(Session& session, uint32_t deviceIndex, DflOb::Window& window)
{
    std::optional<uint32_t> surfaceIndex = std::nullopt;
    for (auto& surface : session.Devices[deviceIndex].Surfaces)
    {
        if (surface.pSharedResources->AssociatedWindow == &window)
        {
            surfaceIndex = &surface - &session.Devices[deviceIndex].Surfaces[0];
            break;
        }
    }
    if (surfaceIndex.has_value() == false)
        return DflHW::SessionError::VkWindowNotAssociatedError;

    auto& surface = session.Devices[deviceIndex].Surfaces[surfaceIndex.value()];
    DflHW::SessionError error = DflHW::SessionError::ThreadNotReadyWarning;

    DflOb::PushProcess(surface, window);
    while (surface.Error == DflHW::SessionError::ThreadNotReadyWarning)
        Sleep(50);
    error = surface.Error;

    return error;
};