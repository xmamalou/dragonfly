module;

#include <iostream>
#include <vector>

#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

module Dragonfly.Hardware:Session;

namespace DflHW = Dfl::Hardware;

DflHW::Session::Session(const SessionInfo& info)
{
    this->GeneralInfo = info;

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    this->CPU.Count = sysInfo.dwNumberOfProcessors;
}

DflHW::Session::~Session()
{
    for (auto device : this->Devices)
    {
        for (auto surface : device.Surfaces)
            vkDestroySurfaceKHR(this->VkInstance, surface.surface, nullptr);

        if (device.VkGPU != nullptr)
        {
            vkDeviceWaitIdle(device.VkGPU);
            vkDestroyDevice(device.VkGPU, nullptr);
        }
    }

    if (this->VkDbMessenger != nullptr) // that means debugging was enabled.
    {
        PFN_vkDestroyDebugUtilsMessengerEXT destroyDebug = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(this->VkInstance, "vkDestroyDebugUtilsMessengerEXT");
        if (destroyDebug != nullptr)
            destroyDebug(this->VkInstance, this->VkDbMessenger, nullptr);
    }
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
    if (!(this->GeneralInfo.Criteria & static_cast<uint32_t>(SessionCriteria::OnlyOffscreen)))
    {
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        extensions.push_back("VK_KHR_win32_surface");
    }

    std::vector<const char*> expectedLayers = { "VK_LAYER_KHRONOS_validation" };
    if (this->GeneralInfo.Criteria & static_cast<uint32_t>(SessionCriteria::DoDebug))
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

    instInfo.enabledLayerCount = (this->GeneralInfo.Criteria & static_cast<uint32_t>(SessionCriteria::DoDebug)) ? expectedLayers.size() : 0;
    instInfo.ppEnabledLayerNames = (this->GeneralInfo.Criteria & static_cast<uint32_t>(SessionCriteria::DoDebug)) ? expectedLayers.data() : nullptr;

    instInfo.enabledExtensionCount = extensions.size();
    instInfo.ppEnabledExtensionNames = extensions.data();

    if (vkCreateInstance(&instInfo, nullptr, &this->VkInstance))
        SessionError::VkInstanceCreationError;

    SessionError err = SessionError::Success;
    if (this->GeneralInfo.Criteria & static_cast<uint32_t>(SessionCriteria::DoDebug))
        err = _InitDebugger(this->VkInstance, this->VkDbMessenger);

    return err;
}

// internal for LoadDevices

void DflHW::Session::OrganizeData(Device& device)
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

    localHeapCount = 0;
    sharedHeapCount = 0;
    for (uint32_t i = 0; i < memProps.memoryHeapCount; i++)
    {
        if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            device.LocalHeaps[localHeapCount].Size = memProps.memoryHeaps[i].size;
            device.LocalHeaps[localHeapCount].HeapIndex = memProps.memoryTypes[i].heapIndex;

            (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ? device.LocalHeaps[localHeapCount].IsHostVisible = true : device.LocalHeaps[localHeapCount].IsHostVisible = false;

            localHeapCount++;
        }
        else
        {
            device.SharedHeaps[sharedHeapCount].Size = memProps.memoryHeaps[i].size;
            device.SharedHeaps[sharedHeapCount].HeapIndex = memProps.memoryTypes[i].heapIndex;

            (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ? device.SharedHeaps[sharedHeapCount].IsHostVisible = true : device.SharedHeaps[sharedHeapCount].IsHostVisible = false;

            sharedHeapCount++;
        }
    }
}

//

DflHW::SessionError DflHW::Session::LoadDevices()
{
    uint32_t deviceCount;
    std::vector<VkPhysicalDevice> devices;
    vkEnumeratePhysicalDevices(this->VkInstance, &deviceCount, nullptr);
    if (deviceCount == 0)
        return SessionError::VkNoDevicesError;
    devices.resize(deviceCount);
    this->Devices.resize(deviceCount); // we now reserve space for both the physical GPUs and the ones Dragonfly understands
    vkEnumeratePhysicalDevices(this->VkInstance, &deviceCount, devices.data());

    for (uint32_t i = 0; i < deviceCount; i++)
    {
        this->Devices[i].VkPhysicalGPU = devices[i];

        this->OrganizeData(this->Devices[i]);
    }

    return SessionError::Success;
}

// internal for InitDevice

std::vector<float> _QueuePriorities(uint32_t size)
{
    std::vector<float> priorities(size);
    for (uint32_t i = 0; i < priorities.size(); i++)
        priorities[i] = 1.0f;

    return priorities;
}

DflHW::SessionError DflHW::Session::GetQueues(std::vector<VkDeviceQueueCreateInfo>& infos, Device& device)
{
    std::vector<VkQueueFamilyProperties> props;
    uint32_t queueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device.VkPhysicalGPU, &queueFamilies, nullptr);
    if (queueFamilies == 0)
        return DflHW::SessionError::VkNoAvailableQueuesError;
    props.resize(queueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(device.VkPhysicalGPU, &queueFamilies, props.data());
    
    uint32_t requiredQueues = device.Surfaces.size() + device.Simulations.size() + 1;
    // ^ Why + 1? If the user doesn't want any sims or any rendering, we assume that they may, at least
    // want to use the device for some kind of computational work.
    for (uint32_t i = 0; i < queueFamilies; i++)
    {
        QueueFamily family; // this is used for caching

        VkDeviceQueueCreateInfo info = {};
        if ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (device.Surfaces.size() != 0))
        {
            family.QueueType &= static_cast<int>(DflHW::QueueType::Graphics);

            VkBool32 canPresent = false;
            uint32_t forNextQueue = 0; 
            // ^ this is the number of surfaces (and subsequently queues) that need to be presented but 
            // the current queue family is unable to do such a thing

            for (auto surface : device.Surfaces)
            {
                if (surface.surface != nullptr) // some rendering surfaces may not be for presentation!
                    vkGetPhysicalDeviceSurfaceSupportKHR(device.VkPhysicalGPU, i, surface.surface, &canPresent);
                else
                    canPresent == true;
                    // ^ why? We are essentially telling Dragonfly that the presentation capability of 
                    // this queue family doesn't matter, so we don't want to push the surface in 
                    // question to the next queue
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
                forNextQueue -= device.Simulations.size() + 1;
                family.QueueType &= static_cast<int>(DflHW::QueueType::Simulation);
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
            info.queueCount = (props[i].queueCount >= (requiredQueues - forNextQueue - device.Simulations.size())) ? (requiredQueues - forNextQueue - 1) : props[i].queueCount;
            info.pQueuePriorities = _QueuePriorities(info.queueCount).data();

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
            info.queueCount = (props[i].queueCount >= device.Simulations.size()) ? device.Simulations.size() : props[i].queueCount;
            info.pQueuePriorities = _QueuePriorities(info.queueCount).data();

            requiredQueues -= info.queueCount;

            infos.push_back(info);
        }

        family.QueueCount = info.queueCount; 
        // ^ we only consider that a queue Family has as many queues as we told it we want. This will also help 
        // with requesting the actual queue handles themselves later

        if (requiredQueues <= 0)
            break;
    }

    if (requiredQueues > 0)
        return DflHW::SessionError::VkInsufficientQueuesError;

    return DflHW::SessionError::Success;
}

//

DflHW::SessionError DflHW::Session::InitDevice(const GPUInfo& info)
{
    if (this->Devices.size() == 0)
        return SessionError::VkNoDevicesError;

    if (this->Devices[info.DeviceIndex].VkGPU != nullptr)
        return SessionError::VkAlreadyInitDeviceWarning;

    // TODO: Implement ranking system

    std::vector<const char*> extensions;

    this->Devices[info.DeviceIndex].Surfaces.resize(info.pDstWindows.size());

    this->Devices[info.DeviceIndex].Surfaces.resize(info.pDstWindows.size());

    for (uint32_t i = 0; i < info.pDstWindows.size(); i++)
    {
        if (info.pDstWindows[i]->WindowHandle() != nullptr) // it is assumed all windows with a HWND are for onscreen rendering, even if not visible
        {
            VkWin32SurfaceCreateInfoKHR surfaceInfo = {
                .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .hwnd = info.pDstWindows[i]->WindowHandle()
            };
            if (vkCreateWin32SurfaceKHR(
                this->VkInstance,
                &surfaceInfo,
                nullptr,
                &this->Devices[info.DeviceIndex].Surfaces[i].surface) != VK_SUCCESS)
                return DflHW::SessionError::VkSurfaceCreationError;
        }
        this->Devices[info.DeviceIndex].Surfaces[i].associatedWindow = info.pDstWindows[i];
    }

    VkDeviceCreateInfo deviceInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    std::vector<VkDeviceQueueCreateInfo> queueInfo;
    auto error = GetQueues(queueInfo, this->Devices[info.DeviceIndex]);
    if (error < DflHW::SessionError::Success)
        return error;

    deviceInfo.queueCreateInfoCount = queueInfo.size();
    deviceInfo.pQueueCreateInfos = queueInfo.data();
    if (this->GeneralInfo.Criteria & static_cast<int>(DflHW::SessionCriteria::OnlyOffscreen))
    {
        deviceInfo.enabledExtensionCount = 0;
    }
    else
    {
        deviceInfo.enabledExtensionCount = 1;
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        deviceInfo.ppEnabledExtensionNames = extensions.data();
    }

    if (vkCreateDevice(
        this->Devices[this->DeviceInfo.DeviceIndex].VkPhysicalGPU,
        &deviceInfo,
        nullptr,
        &this->Devices[this->DeviceInfo.DeviceIndex].VkGPU) != VK_SUCCESS)
        return DflHW::SessionError::VkDeviceInitError;

    // we will now find which queue families support graphics (and presentation, if applicable)

    return DflHW::SessionError::Success;
}
