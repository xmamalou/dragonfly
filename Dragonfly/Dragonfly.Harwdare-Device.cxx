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

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <optional>
#include <string>

#include <iostream>
#include <thread>

module Dragonfly.Hardware:Device;

import Dragonfly.Observer;

namespace DflOb = Dfl::Observer;
namespace DflHW = Dfl::Hardware;

// INTERNAL FOR INIT NODE

inline bool _DoesSupportSRGB(VkColorSpaceKHR& colorSpace, const std::vector<VkSurfaceFormatKHR>& formats)
{
    for (auto& format : formats)
    {
        colorSpace = format.colorSpace;
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB)
            return true;
    }

    VkColorSpaceKHR test = (colorSpace = formats[0].colorSpace);

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

inline VkExtent2D _MakeExtent(const std::array<uint32_t, 2>& resolution, const VkSurfaceCapabilitiesKHR& capabs)
{
    VkExtent2D extent{};

    uint32_t minHeight{ max(capabs.minImageExtent.height, resolution[0]) };
    uint32_t minWidth{ max(capabs.minImageExtent.width, resolution[1]) };

    extent.height = min(minHeight, capabs.maxImageExtent.height);
    extent.width = min(minWidth, capabs.maxImageExtent.width);

    return extent;
};

DflHW::DeviceError _CreateSwapchain(DflHW::SharedRenderResources& resources)
{
    VkSwapchainCreateInfoKHR swapchainInfo{};
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

    auto error{ vkCreateSwapchainKHR(resources.GPU, &swapchainInfo, nullptr, &resources.Swapchain) };
    switch (error)
    {
    case VK_SUCCESS:
        break;
    case VK_ERROR_SURFACE_LOST_KHR:
        return DflHW::DeviceError::VkSwapchainSurfaceLostError;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return DflHW::DeviceError::VkSwapchainWindowUnavailableError;
    default:
        return DflHW::DeviceError::VkSwapchainInitError;
    }

    return DflHW::DeviceError::Success;
};

//

void* _FailNode(DflHW::SharedRenderResources& resources, DflHW::DeviceError& error)
{
    std::cout << "";
    return _FailNode;
};

void* _LoopNode(DflHW::SharedRenderResources& resources, DflHW::DeviceError& error)
{
    std::cout << "";
    return _LoopNode;
};

void* _InitNode(DflHW::SharedRenderResources& resources, DflHW::DeviceError& error)
{
    VkCommandPoolCreateInfo cmdPoolInfo{};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.pNext = nullptr;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdPoolInfo.queueFamilyIndex = resources.AssignedQueue.FamilyIndex;

    resources.pDeviceMutex->lock();
    if (vkCreateCommandPool(resources.GPU, &cmdPoolInfo, nullptr, &resources.CmdPool) != VK_SUCCESS)
    {
        error = DflHW::DeviceError::VkComPoolInitError;
        resources.pDeviceMutex->unlock();
        return _FailNode;
    }
    //this->pMutex->unlock();

    error = _CreateSwapchain(resources);
    if (error != DflHW::DeviceError::Success)
    {
        resources.pDeviceMutex->unlock();
        return _FailNode;
    }

    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(resources.GPU, resources.Swapchain, &imageCount, nullptr);
    resources.SwapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(resources.GPU, resources.Swapchain, &imageCount, resources.SwapchainImages.data());

    resources.pDeviceMutex->unlock();

    return _LoopNode;
};

//

DflHW::RenderingSurface::RenderingSurface()
{
    this->pRenderNode = _InitNode;
}

void DflHW::RenderingSurface::operator() (DflOb::WindowProcessArgs& args)
{
    DflHW::RenderNode node{ (DflHW::RenderNode)this->pRenderNode(*this->pSharedResources, this->Error) };
    this->pRenderNode = node;
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

// Internal for Device constructor

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
};

void DflHW::_OrganizeData(Dfl::Hardware::Device& device)
{
    VkPhysicalDeviceProperties devProps;
    vkGetPhysicalDeviceProperties(device.Info.VkPhysicalGPU, &devProps);
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(device.Info.VkPhysicalGPU, &memProps);
    VkPhysicalDeviceFeatures devFeats;
    vkGetPhysicalDeviceFeatures(device.Info.VkPhysicalGPU, &devFeats);

    device.Name = devProps.deviceName;

    int localHeapCount{ 0 };
    int sharedHeapCount{ 0 };

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

    device.MaxGroups = { devProps.limits.maxComputeWorkGroupCount[0], devProps.limits.maxComputeWorkGroupCount[1], devProps.limits.maxComputeWorkGroupCount[2] };
};

//

DflHW::Device::Device(const GPUInfo& info) : Info(info) {
    if (this->Info.VkPhysicalGPU != nullptr)
        _OrganizeData(*this);
};

DflHW::Device::~Device(){
    this->UsageMutex.lock();
    for (auto& surface : this->Surfaces)
        vkDestroySurfaceKHR(this->Info.VkInstance, surface.pSharedResources->Surface, nullptr);

    if (this->VkGPU != nullptr)
    {
        vkDeviceWaitIdle(this->VkGPU);
        vkDestroyDevice(this->VkGPU, nullptr);
    }
    this->UsageMutex.unlock();
};

// internal for InitDevice

DflHW::DeviceError DflHW::_GetRenderResources(
    DflHW::SharedRenderResources& resources,
    const DflHW::Device& device,
    const DflOb::Window& window)
{
    if (window.WindowHandle() != nullptr) // ensures the window is valid
    {
        VkWin32SurfaceCreateInfoKHR surfaceInfo = {
            .sType{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR },
            .pNext{ nullptr },
            .flags{ 0 },
            .hwnd{ window.WindowHandle() }
        };
        if (vkCreateWin32SurfaceKHR(device.Info.VkInstance, &surfaceInfo, nullptr, &resources.Surface) != VK_SUCCESS)
            return DflHW::DeviceError::VkSurfaceCreationError;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.Info.VkPhysicalGPU, resources.Surface, &resources.Capabilities);

        uint32_t count{ 0 };
        vkGetPhysicalDeviceSurfaceFormatsKHR(device.Info.VkPhysicalGPU, resources.Surface, &count, nullptr);
        if (count == 0)
            return DflHW::DeviceError::VkNoSurfaceFormatsError;
        resources.Formats.resize(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device.Info.VkPhysicalGPU, resources.Surface, &count, resources.Formats.data());

        vkGetPhysicalDeviceSurfacePresentModesKHR(device.Info.VkPhysicalGPU, resources.Surface, &count, nullptr);
        if (count == 0)
            return DflHW::DeviceError::VkNoSurfacePresentModesError;
        resources.PresentModes.resize(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device.Info.VkPhysicalGPU, resources.Surface, &count, resources.PresentModes.data());
    }
    else
        return DflHW::DeviceError::InvalidWindowHandleError;

    return DflHW::DeviceError::Success;
};

std::vector<float> _QueuePriorities(uint32_t size)
{
    std::vector<float> priorities(size);
    for (auto& priority : priorities)
        priority = 1.0;

    return priorities;
};

DflHW::DeviceError DflHW::_GetQueues(
    std::vector<VkDeviceQueueCreateInfo>& infos,
    Device& device)
{
    std::vector<VkQueueFamilyProperties> props;
    uint32_t queueFamilies{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(device.Info.VkPhysicalGPU, &queueFamilies, nullptr);
    if (queueFamilies == 0)
        return DflHW::DeviceError::VkNoAvailableQueuesError;
    props.resize(queueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(device.Info.VkPhysicalGPU, &queueFamilies, props.data());

    uint32_t graphicsQueues{ static_cast<uint32_t>(device.Surfaces.size()) };
    uint32_t simulationQueues{ static_cast<uint32_t>(device.Simulations.size()) };
    uint32_t transferQueues{ 1 };
    // ^ Why this one? This queue is going to be used for transfer operations.
    for (uint32_t i = 0; i < queueFamilies; i++)
    {
        QueueFamily family; // this is used for caching

        uint32_t untakenQueues = props[i].queueCount;

        VkDeviceQueueCreateInfo info = {};
        info.queueCount = 0;
        if ((props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (graphicsQueues > 0))
        {
            family.QueueType |= static_cast<int>(DflHW::QueueType::Graphics);

            VkBool32 canPresent{ false };
            int forNextQueue{ 0 };
            // ^ this is the number of surfaces (and subsequently queues) that need to be presented but 
            // the current queue family is unable to do such a thing

            for (auto& surface : device.Surfaces)
            {
                vkGetPhysicalDeviceSurfaceSupportKHR(device.Info.VkPhysicalGPU, i, surface.pSharedResources->Surface, &canPresent);

                if (canPresent == false)
                    forNextQueue++;
                else
                    family.CanPresent = true;
            }

            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.queueFamilyIndex = i;
            info.queueCount += (untakenQueues >= graphicsQueues) ? (graphicsQueues - forNextQueue) : untakenQueues;

            untakenQueues -= info.queueCount;
            graphicsQueues -= info.queueCount;
        }

        if (untakenQueues <= 0)
        {
            family.QueueCount = info.queueCount;
            family.Index = i;
            device.QueueFamilies.push_back(family);
            infos.push_back(info);
            continue;
        }

        if ((props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && (simulationQueues > 0))
        {
            uint32_t queueCount{ (untakenQueues >= simulationQueues) ? simulationQueues : untakenQueues };

            family.QueueType |= static_cast<int>(DflHW::QueueType::Compute);

            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.queueFamilyIndex = i;
            info.queueCount += queueCount;

            untakenQueues -= queueCount;
            simulationQueues -= queueCount;
        }

        if (untakenQueues <= 0)
        {
            family.QueueCount = info.queueCount;
            family.Index = i;
            device.QueueFamilies.push_back(family);
            infos.push_back(info);
            continue;
        }

        if ((props[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && (transferQueues > 0))
        {
            family.QueueType |= static_cast<int>(DflHW::QueueType::Transfer);

            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.queueFamilyIndex = i;
            info.queueCount += transferQueues;

            transferQueues = 0;

            untakenQueues--;
        }

        if (untakenQueues <= 0)
        {
            family.QueueCount = info.queueCount;
            family.Index = i;
            device.QueueFamilies.push_back(family);
            infos.push_back(info);
            continue;
        }

        if (info.queueCount != 0)
            infos.push_back(info);

        family.QueueCount = info.queueCount;
        family.Index = i;
        // ^ we only consider that a queue Family has as many queues as we told it we want. This will also help 
        // with requesting the actual queue handles themselves later

        if (family.QueueCount != 0)
            device.QueueFamilies.push_back(family);

        if ((graphicsQueues + simulationQueues) <= 0)
            break;
    }

    // since Dragonfly always checks first whether a family supports graphics, we can assume that if we have
    // not filled all the graphics queues, then we cannot use the device
    if (graphicsQueues > 0)
        return DflHW::DeviceError::VkInsufficientQueuesError;

    return DflHW::DeviceError::Success;
};

//

DflHW::DeviceError DflHW::Device::InitDevice()
{
    if (this->Info.VkInstance == nullptr)
        return DeviceError::VkDeviceNullptrGiven;

    // TODO: Implement ranking system
    if (this->Info.VkPhysicalGPU == nullptr)
        return DeviceError::VkDeviceNullptrGiven;

    std::vector<const char*> extensions{};

    this->Surfaces.resize(this->Info.pDstWindows.size());
    this->Simulations.resize(this->Info.PhysicsSimsNumber);

    for (uint32_t i = 0; i < this->Info.pDstWindows.size(); i++){
        this->Surfaces[i].pSharedResources = std::make_unique<DflHW::SharedRenderResources>();
        this->Surfaces[i].pSharedResources->pDeviceMutex = &this->UsageMutex;
        auto& resources = this->Surfaces[i].pSharedResources;
        auto error = _GetRenderResources(*resources, *this, *this->Info.pDstWindows[i]);
        if (error < DflHW::DeviceError::Success)
            return error;
        resources->AssociatedWindow = this->Info.pDstWindows[i];
    }

    VkDeviceCreateInfo deviceInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    std::vector<VkDeviceQueueCreateInfo> queueInfo;
    auto error = _GetQueues(queueInfo, *this);
    if (error < DflHW::DeviceError::Success)
        return error;

    std::vector<std::vector<float>> queuePriorities(queueInfo.size());
    for (auto& queue : queueInfo)
    {
        queuePriorities[&queue - queueInfo.data()] = _QueuePriorities(queue.queueCount);
        queue.pQueuePriorities = queuePriorities[&queue - queueInfo.data()].data();
    }

    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfo.size());
    deviceInfo.pQueueCreateInfos = queueInfo.data();

    if (this->Info.EnableOnscreenRendering)
        extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    if (this->Info.EnableRaytracing)
    {
        extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        extensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        extensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
    }

    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    deviceInfo.ppEnabledExtensionNames = (extensions.empty()) ? nullptr : extensions.data();

    VkResult vkError = vkCreateDevice(
        this->Info.VkPhysicalGPU,
        &deviceInfo,
        nullptr,
        &this->VkGPU);
    switch (vkError)
    {
    case VK_SUCCESS:
        break;
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return DeviceError::VkDeviceNoSuchExtensionError;
    case VK_ERROR_DEVICE_LOST:
        return DeviceError::VkDeviceLostError;
    default:
        return DeviceError::VkDeviceInitError;
    }

    uint32_t i = 0;
    for (auto& surface : this->Surfaces)
    {
        QueueFamily& family = this->QueueFamilies[i];
        if (family.QueueType & static_cast<int>(DflHW::QueueType::Graphics))
        {

            vkGetDeviceQueue(this->VkGPU, family.Index, family.UsedQueues, &surface.pSharedResources->AssignedQueue.hQueue);
            surface.pSharedResources->AssignedQueue.FamilyIndex = family.Index;
            surface.pSharedResources->GPU = this->VkGPU;
        }
        else
        {
            i++;
            continue;
        }
        family.UsedQueues++;
        if (family.UsedQueues >= family.QueueCount)
            i++;
    }

    for (auto& sim : this->Simulations)
    {
        QueueFamily& family = this->QueueFamilies[i];
        if (family.QueueType & static_cast<int>(DflHW::QueueType::Compute))
        {
            vkGetDeviceQueue(this->VkGPU, family.Index, family.UsedQueues, &sim.AssignedQueue.hQueue);
            sim.AssignedQueue.FamilyIndex = family.Index;
        }
        else
        {
            i++;
            continue;
        }
        family.UsedQueues++;
        if (family.UsedQueues >= family.QueueCount)
            i++;
    }

    while (this->TransferQueue.hQueue == VK_NULL_HANDLE)
    {
        QueueFamily& family = this->QueueFamilies[i];
        if (family.QueueType & static_cast<int>(DflHW::QueueType::Transfer))
        {
            vkGetDeviceQueue(this->VkGPU, family.Index, family.UsedQueues, &this->TransferQueue.hQueue);
            this->TransferQueue.FamilyIndex = family.Index;
        }
        else
            i++;
    }

    return DflHW::DeviceError::Success;
};

DflHW::DeviceError DflHW::CreateRenderer(DflHW::Device& device, DflOb::Window& window)
{
    std::optional<uint32_t> surfaceIndex;
    for (auto& surface : device.Surfaces)
    {
        if (surface.pSharedResources->AssociatedWindow == &window)
        {
            surfaceIndex = &surface - &device.Surfaces[0];
            break;
        }
    }
    if (surfaceIndex.has_value() != true)
        return DflHW::DeviceError::VkWindowNotAssociatedError;

    auto& surface = device.Surfaces[surfaceIndex.value()];
    DflHW::DeviceError error = DflHW::DeviceError::ThreadNotReadyWarning;

    DflOb::PushProcess(surface, window);
    while (surface.Error == DflHW::DeviceError::ThreadNotReadyWarning)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    error = surface.Error;

    return error;
};