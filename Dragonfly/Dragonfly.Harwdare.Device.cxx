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

#include <optional>
#include <string>
#include <iostream>
#include <thread>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

module Dragonfly.Hardware.Device;

import Dragonfly.Observer;

namespace DflOb  = Dfl::Observer;
namespace DflHW  = Dfl::Hardware;
namespace DflGen = Dfl::Generics;

// Internal for Device constructor

template <DflHW::MemoryType type>
static inline void INT_OrganizeMemory(
          std::vector<DflHW::DeviceMemory<type>>& memory, 
    const VkPhysicalDeviceMemoryProperties        props);

template <>
static inline void INT_OrganizeMemory(
          std::vector<DflHW::DeviceMemory<DflHW::MemoryType::Local>>& memory,
    const VkPhysicalDeviceMemoryProperties                            props) {
    uint32_t heapCount{ 0 };
    for (uint32_t i{ 0 }; i < props.memoryHeapCount; i++){
        if ( props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ){
            memory[heapCount].Size = props.memoryHeaps[i].size;
            memory[heapCount].HeapIndex = props.memoryTypes[i].heapIndex;

            for (uint32_t j{ 0 }; j < VK_MAX_MEMORY_TYPES; j++) {
                if (props.memoryTypes[j].heapIndex != i) {
                    continue;
                }
                
                memory[heapCount].IsHostVisible = 
                    props.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? true : false;
                memory[heapCount].IsHostCoherent =
                    props.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? true : false;
                memory[heapCount].IsHostCached = 
                    props.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ? true : false;
            }

            heapCount++;
        }
    }
};

template <>
static inline void INT_OrganizeMemory(
          std::vector<DflHW::DeviceMemory<DflHW::MemoryType::Shared>>& memory, 
    const VkPhysicalDeviceMemoryProperties                             props) {
    uint32_t heapCount{ 0 };
    for (uint32_t i{ 0 }; i < props.memoryHeapCount; i++) {
        if ( !(props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ) {
            memory[heapCount].Size = props.memoryHeaps[i].size;
            memory[heapCount].HeapIndex = props.memoryTypes[i].heapIndex;

            for (uint32_t j{ 0 }; j < VK_MAX_MEMORY_TYPES; j++ ) { 
                if (props.memoryTypes[j].heapIndex != i) {
                    continue;
                }

                memory[heapCount].IsHostVisible =
                    props.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? true : false;
            }

            heapCount++;
        }
    }
};

static DflHW::DeviceCharacteristics INT_OrganizeData(const VkPhysicalDevice& device){
    if (device == nullptr) {
        throw DflHW::DeviceError::NullHandleError;
    }

    //VkDevice& gpu = device.Info.pSession->Device
    VkPhysicalDeviceProperties devProps;
    vkGetPhysicalDeviceProperties(device, &devProps);
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(device, &memProps);
    VkPhysicalDeviceFeatures devFeats;
    vkGetPhysicalDeviceFeatures(device, &devFeats);

    int localHeapCount{ 0 };
    int sharedHeapCount{ 0 };

    for (auto heap : memProps.memoryHeaps){
        if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT &&
            heap.size != 0){
            localHeapCount++;
        }else if (heap.size != 0){
            sharedHeapCount++;
        }
    }

    std::vector<DflHW::DeviceMemory<DflHW::MemoryType::Local>> localHeaps(localHeapCount);
    std::vector<DflHW::DeviceMemory<DflHW::MemoryType::Shared>> sharedHeaps(sharedHeapCount);
    INT_OrganizeMemory(localHeaps, memProps);
    INT_OrganizeMemory(sharedHeaps, memProps);

    uint32_t currentCheck{ 0x40 }; // represents VkSampleCountFlagBits elements
    uint32_t maxColourSamples{ 0 };
    uint32_t maxDepthSamples{ 0 };
    for (uint32_t i = 0; i < 7; i++) {
        if (devProps.limits.framebufferColorSampleCounts & currentCheck) {
            maxColourSamples = currentCheck;
            break;
        }
        currentCheck >>= 1;
    }
    currentCheck = 0x40;
    for (uint32_t i = 0; i < 7; i++) {
        if (devProps.limits.framebufferDepthSampleCounts & currentCheck) {
            maxDepthSamples = currentCheck;
            break;
        }
        currentCheck >>= 1;
    }

    return {
        std::string(devProps.deviceName),
        localHeaps, sharedHeaps,
        { devProps.limits.maxViewportDimensions[0], devProps.limits.maxViewportDimensions[1] },
        { maxColourSamples, maxDepthSamples },
        { devProps.limits.maxComputeWorkGroupCount[0], devProps.limits.maxComputeWorkGroupCount[1], devProps.limits.maxComputeWorkGroupCount[2] },
        devProps.limits.maxMemoryAllocationCount,
        devProps.limits.maxDrawIndirectCount };
};

static inline std::vector<DflHW::QueueFamily> INT_OrganizeQueues(
    const VkPhysicalDevice&                device) {
    std::vector<DflHW::QueueFamily>      queueFamilies;
    std::vector<VkQueueFamilyProperties> props;
    uint32_t                             queueFamilyCount{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &queueFamilyCount,
        nullptr);
    if (queueFamilyCount == 0) {
        throw DflHW::DeviceError::VkNoAvailableQueuesError;
    }
    props.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &queueFamilyCount,
        props.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        DflHW::QueueFamily family;

        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
            vkGetPhysicalDeviceWin32PresentationSupportKHR(
                device,
                i)) {
            family.QueueType |= DflHW::QueueType::Graphics;
        }

        if (props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            family.QueueType |= DflHW::QueueType::Compute;
        }

        if (props[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            family.QueueType |= DflHW::QueueType::Transfer;
        }

        family.QueueCount = props[i].queueCount;
        family.Index = i;

        if(family.QueueType != 0){
            queueFamilies.push_back(family);
        }
    }

    return queueFamilies;
}

static DflHW::DeviceError INT_GetQueues(
          std::vector<VkDeviceQueueCreateInfo>& infos,
    const uint32_t                              rendererNum,
    const uint32_t                              simNum,
    const std::vector<DflHW::QueueFamily>&      families) {
    uint32_t leftQueues{ rendererNum + simNum + 1 };

    for (auto& family : families) {
        uint32_t usedQueues{ 0 };

        if (family.QueueType & DflHW::QueueType::Graphics) {
            usedQueues = family.QueueCount > rendererNum ?
                rendererNum : family.QueueCount;
        }

        if (usedQueues >= family.QueueCount) {
            continue;
        }

        if (family.QueueType & DflHW::QueueType::Compute) {
            usedQueues += family.QueueCount - usedQueues > simNum ?
                simNum : family.QueueCount - usedQueues;
        }

        if (usedQueues >= family.QueueCount) {
            continue;
        }

        if (family.QueueType & DflHW::QueueType::Transfer) {
            usedQueues += family.QueueCount - usedQueues > 1 ?
                1 : family.QueueCount - usedQueues;
        }

        VkDeviceQueueCreateInfo info = {
            .sType{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO },
            .pNext{ nullptr },
            .flags{ 0 },
            .queueFamilyIndex{ family.Index },
            .queueCount{ usedQueues },
        };
        infos.push_back(info);

        if( (leftQueues -= usedQueues) <= 0){ break; }
    }

    if (infos.size() == 0) {
        return DflHW::DeviceError::VkInsufficientQueuesError;
    }

    return DflHW::DeviceError::Success;
};

static const std::array< const char*, 2 > extensionsNoRT{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME
};

static const std::array< const char*, 5 > extensionsWithRT{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME
};

//

static DflHW::DeviceHandles INT_InitDevice(
    const VkInstance&                      instance,
    const VkPhysicalDevice&                physDevice,
    const DflGen::BitFlag&                 renderOptions,
    const uint32_t                         rendererNum,
    const uint32_t                         simNum){
    if (instance == nullptr)
        throw DflHW::DeviceError::VkDeviceInvalidSession;

    if (physDevice == nullptr) {
        throw DflHW::DeviceError::VkDeviceInvalidSession;
    }

    std::vector<DflHW::QueueFamily> queueFamilies{ INT_OrganizeQueues(physDevice) };

    std::vector<VkDeviceQueueCreateInfo> queueInfo;
    DflHW::DeviceError error{ 0 };
    if ( ( error = INT_GetQueues(
                    queueInfo,
                    rendererNum,
                    simNum,
                    queueFamilies) ) < DflHW::DeviceError::Success ) {
        throw error;
    }

    float** const priorities = new float* [queueInfo.size()];
    for (uint32_t i{ 0 }; i < queueInfo.size(); i++) {
        priorities[i] = new float [queueInfo[i].queueCount];
        for (uint32_t j{ 0 }; j < queueInfo[i].queueCount; j++) {
            priorities[i][j] = 1.0f;
        }
        queueInfo[i].pQueuePriorities = priorities[i];
    }
    
    uint32_t extensionCount{ 0 };
    vkEnumerateDeviceExtensionProperties(
        physDevice,
        nullptr,
        &extensionCount,
        nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(
            physDevice,
            nullptr,
            &extensionCount,
            extensions.data());

    bool doTimelineSemsExist{ false };
    bool doesRTXExist{ false };

    for( auto& extension : extensions ) {
        if (!strcmp(extension.extensionName, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)) {
            doTimelineSemsExist = true;
        }

        if (!strcmp(extension.extensionName, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)) {
            doesRTXExist = true;
        }
    }

    const VkDeviceCreateInfo deviceInfo{
       .sType{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO },
       .pNext{ nullptr },
       .flags{ 0 },
       .queueCreateInfoCount{ static_cast<uint32_t>(queueInfo.size()) },
       .pQueueCreateInfos{ queueInfo.data() },
       .enabledLayerCount{ 0 },
       .ppEnabledLayerNames{ nullptr },
       .enabledExtensionCount{ doTimelineSemsExist ?
                                    ( rendererNum > 0 ? 
                                        ( doesRTXExist && renderOptions & DflHW::RenderOptions::Raytracing ? 
                                                static_cast<uint32_t>(extensionsWithRT.size()) :
                                                static_cast<uint32_t>(extensionsNoRT.size()) ) :
                                        0 ) :
                                    ( rendererNum > 0 ? 
                                        ( doesRTXExist && renderOptions & DflHW::RenderOptions::Raytracing ?
                                                    static_cast<uint32_t>(extensionsWithRT.size()) - 1 :
                                                    static_cast<uint32_t>(extensionsNoRT.size()) - 1 ) :
                                        0 ) },
       .ppEnabledExtensionNames{ doesRTXExist ?
                                    extensionsWithRT.data() : 
                                    extensionsNoRT.data() },
    };

    VkDevice gpu{ nullptr };
    switch (VkResult vkError{ vkCreateDevice(
        physDevice,
        &deviceInfo,
        nullptr,
        &gpu) }) {
    case VK_SUCCESS:
        break;
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        throw DflHW::DeviceError::VkDeviceNoSuchExtensionError;
    case VK_ERROR_DEVICE_LOST:
        throw DflHW::DeviceError::VkDeviceLostError;
    default:
        throw DflHW::DeviceError::VkDeviceInitError;
    }

    for (uint32_t i{ 0 }; i < queueInfo.size(); i++) {
        delete[] priorities[i];
    }

    delete[] priorities;

    return { gpu, queueFamilies, doTimelineSemsExist, doesRTXExist };
};

//

DflHW::Device::Device(const DeviceInfo& info) 
try : pInfo( new DeviceInfo(info) ), 
      pCharacteristics( new DeviceCharacteristics( INT_OrganizeData(
                                                     info.DeviceIndex < info.pSession->Instance.hDevices.size() ?
                                                         info.pSession->Instance.hDevices[info.DeviceIndex] 
                                                       : nullptr) ) ),
      pTracker( new DeviceTracker() ),
      hPhysicalDevice(info.DeviceIndex < info.pSession->Instance.hDevices.size() ?
                        info.pSession->Instance.hDevices[info.DeviceIndex]
                      : nullptr),
      GPU( INT_InitDevice(
                info.pSession->Instance,
                info.DeviceIndex < info.pSession->Instance.hDevices.size() ?
                    info.pSession->Instance.hDevices[info.DeviceIndex]
                    : nullptr,
                info.RenderOptions,
                info.RenderersNumber,
                info.SimulationsNumber) ),
      pUsageMutex( new std::mutex() ) {
    this->pTracker->LeastClaimedQueue.resize(this->GPU.Families.size());
    this->pTracker->AreFamiliesUsed.resize(this->GPU.Families.size()); }
catch (DeviceError e) { this->Error = e; }


DflHW::Device::~Device(){
    this->pUsageMutex->lock();
    if ( this->GPU.hDevice != nullptr ){
        vkDeviceWaitIdle(this->GPU.hDevice);
        vkDestroyDevice(this->GPU.hDevice, nullptr);
    }
    this->pUsageMutex->unlock();
};