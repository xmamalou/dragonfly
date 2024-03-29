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
#include <mutex>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

module Dragonfly.Hardware.Device;

import Dragonfly.Observer;

namespace DflOb  = Dfl::Observer;
namespace DflHW  = Dfl::Hardware;
namespace DflGen = Dfl::Generics;

// GLOBAL MUTEX

static std::mutex INT_GLB_DeviceMutex;

// Internal for Device constructor

template <DflHW::Device::MemoryType type>
static inline void INT_OrganizeMemory(
          std::vector<DflHW::Device::Memory<type>>& memory, 
    const VkPhysicalDeviceMemoryProperties          props);

template <>
static inline void INT_OrganizeMemory(
          std::vector<DflHW::Device::Memory<DflHW::Device::MemoryType::Local>>& memory,
    const VkPhysicalDeviceMemoryProperties                                      props) {
    constexpr auto type = DflHW::Device::MemoryType::Local;

    uint32_t heapCount{ 0 };
    for (uint32_t i{ 0 }; i < props.memoryHeapCount; i++){
        if ( props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT ){
            memory[heapCount].Size = props.memoryHeaps[i].size;
            memory[heapCount].HeapIndex = i;

            DflHW::Device::Memory<type>::Properties dflProps{ };
            for (uint32_t j{ 0 }; j < VK_MAX_MEMORY_TYPES; j++) {
                if (props.memoryTypes[j].heapIndex != i) {
                    continue;
                }

                dflProps.TypeIndex = j;
                dflProps.IsHostVisible =
                    props.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? true : false;
                dflProps.IsHostCoherent =
                    props.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ? true : false;
                dflProps.IsHostCached =
                    props.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT ? true : false;

                memory[heapCount].MemProperties.push_back(dflProps);
            }

            heapCount++;
        }
    }
};

template <>
static inline void INT_OrganizeMemory(
          std::vector<DflHW::Device::Memory<DflHW::Device::MemoryType::Shared>>& memory, 
    const VkPhysicalDeviceMemoryProperties                                       props) {
    constexpr auto type = DflHW::Device::MemoryType::Shared;
    
    uint32_t heapCount{ 0 };
    for (uint32_t i{ 0 }; i < props.memoryHeapCount; i++) {
        if ( !(props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ) {
            memory[heapCount].Size = props.memoryHeaps[i].size;
            memory[heapCount].HeapIndex = props.memoryTypes[i].heapIndex;

            DflHW::Device::Memory<type>::Properties dflProps{ };
            for (uint32_t j{ 0 }; j < VK_MAX_MEMORY_TYPES; j++ ) { 
                if (props.memoryTypes[j].heapIndex != i) {
                    continue;
                }

                dflProps.TypeIndex = j;
                dflProps.IsHostVisible =
                    props.memoryTypes[j].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? true : false;

                memory[heapCount].MemProperties.push_back(dflProps);
            }

            heapCount++;
        }
    }
};

static DflHW::Device::Characteristics INT_OrganizeData(const VkPhysicalDevice& device){
    if (device == nullptr) {
        throw DflHW::Device::Error::NullHandleError;
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

    std::vector<DflHW::Device::Memory<DflHW::Device::MemoryType::Local>> localHeaps(localHeapCount);
    std::vector<DflHW::Device::Memory<DflHW::Device::MemoryType::Shared>> sharedHeaps(sharedHeapCount);
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

static inline std::vector<DflHW::Device::Queue::Family> INT_OrganizeQueues(
    const VkPhysicalDevice& device) {
    std::vector<DflHW::Device::Queue::Family> queueFamilies;
    std::vector<VkQueueFamilyProperties>      props;
    uint32_t                                  queueFamilyCount{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &queueFamilyCount,
        nullptr);
    if (queueFamilyCount == 0) {
        throw DflHW::Device::Error::VkNoAvailableQueuesError;
    }
    props.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &queueFamilyCount,
        props.data());

    uint32_t             index{ 0 };
    uint32_t             queueCount{ 0 };
    DflGen::BitFlag      queueType{ 0 };

    for (uint32_t i{ 0 }; i < queueFamilyCount; i++) {
        if (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
            vkGetPhysicalDeviceWin32PresentationSupportKHR(
                device,
                i)) {
            queueType |= DflHW::Device::Queue::Type::Graphics;
        }

        if (props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            queueType |= DflHW::Device::Queue::Type::Compute;
        }

        if (props[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            queueType |= DflHW::Device::Queue::Type::Transfer;
        }

        queueCount = props[i].queueCount;
        index = i;

        if(queueType != 0 && queueCount != 0){
            queueFamilies.push_back( {index, queueCount, queueType} );
        }
    }

    
    return queueFamilies;
}

static DflHW::Device::Error INT_GetQueues(
          std::vector<VkDeviceQueueCreateInfo>&      infos,
    const uint32_t                                   rendererNum,
    const uint32_t                                   simNum,
    const std::vector<DflHW::Device::Queue::Family>& families) {
    uint32_t leftQueues{ rendererNum + simNum + 1 };

    for (auto& family : families) {
        uint32_t usedQueues{ 0 };

        if (family.QueueType & DflHW::Device::Queue::Type::Graphics) {
            usedQueues = family.QueueCount > rendererNum ?
                rendererNum : family.QueueCount;
        }

        if (usedQueues >= family.QueueCount) {
            continue;
        }

        if (family.QueueType & DflHW::Device::Queue::Type::Compute) {
            usedQueues += family.QueueCount - usedQueues > simNum ?
                simNum : family.QueueCount - usedQueues;
        }

        if (usedQueues >= family.QueueCount) {
            continue;
        }

        if (family.QueueType & DflHW::Device::Queue::Type::Transfer) {
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
        return DflHW::Device::Error::VkInsufficientQueuesError;
    }

    return DflHW::Device::Error::Success;
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

static DflHW::Device::Handles INT_InitDevice(
    const VkInstance&                      instance,
    const VkPhysicalDevice&                physDevice,
    const DflGen::BitFlag&                 renderOptions,
    const uint32_t                         rendererNum,
    const uint32_t                         simNum){
    if (instance == nullptr)
        throw DflHW::Device::Error::VkDeviceInvalidSession;

    if (physDevice == nullptr) {
        throw DflHW::Device::Error::VkDeviceInvalidSession;
    }

    std::vector<DflHW::Device::Queue::Family> queueFamilies{ INT_OrganizeQueues(physDevice) };

    std::vector<VkDeviceQueueCreateInfo> queueInfo;
    DflHW::Device::Error error{ 0 };
    if ( ( error = INT_GetQueues(
                    queueInfo,
                    rendererNum,
                    simNum,
                    queueFamilies) ) < DflHW::Device::Error::Success ) {
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
                                        ( doesRTXExist && renderOptions & DflHW::Device::RenderOptions::Raytracing ? 
                                                static_cast<uint32_t>(extensionsWithRT.size()) :
                                                static_cast<uint32_t>(extensionsNoRT.size()) ) :
                                        0 ) :
                                    ( rendererNum > 0 ? 
                                        ( doesRTXExist && renderOptions & DflHW::Device::RenderOptions::Raytracing ?
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
        throw DflHW::Device::Error::VkDeviceNoSuchExtensionError;
    case VK_ERROR_DEVICE_LOST:
        throw DflHW::Device::Error::VkDeviceLostError;
    default:
        throw DflHW::Device::Error::VkDeviceInitError;
    }

    for (uint32_t i{ 0 }; i < queueInfo.size(); i++) {
        delete[] priorities[i];
    }

    delete[] priorities;

    return { gpu, physDevice, queueFamilies, doTimelineSemsExist, doesRTXExist };
};

//

const VkFence DflHW::Device::GetFence(
                    const uint32_t queueFamilyIndex,
                    const uint32_t queueIndex) const {
    for (auto fence : this->pTracker->Fences) {
        if (fence.QueueFamilyIndex == queueFamilyIndex &&
            fence.QueueIndex == queueIndex) {
            return fence;
        }
    }

    VkFenceCreateInfo fenceInfo{
        .sType{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ VK_FENCE_CREATE_SIGNALED_BIT }
    };
    VkFence fence{ nullptr };
    VkResult error{ VK_SUCCESS };
    if ((error = vkCreateFence(
            this->GPU,
            &fenceInfo,
            nullptr,
            &fence)) != VK_SUCCESS) {
        return nullptr;
    }
    this->pTracker->Fences.push_back( { fence, queueFamilyIndex, queueIndex } );

    return fence;
}

DflHW::Device::Device(const Info& info) 
try : pInfo( new Info(info) ), 
      pCharacteristics( new Characteristics( INT_OrganizeData(
                                                     info.DeviceIndex < info.pSession->Instance.hDevices.size() ?
                                                         info.pSession->Instance.hDevices[info.DeviceIndex] 
                                                       : nullptr) ) ),
      pTracker( new Tracker() ),
      GPU( INT_InitDevice(
                info.pSession->Instance,
                info.DeviceIndex < info.pSession->Instance.hDevices.size() ?
                    info.pSession->Instance.hDevices[info.DeviceIndex]
                    : nullptr,
                info.RenderOptions,
                info.RenderersNumber,
                info.SimulationsNumber) ) {
    this->pTracker->LeastClaimedQueue.resize(this->GPU.Families.size());
    this->pTracker->AreFamiliesUsed.resize(this->GPU.Families.size()); 
    
    this->pTracker->UsedLocalMemoryHeaps.resize(this->pCharacteristics->LocalHeaps.size());
    this->pTracker->UsedSharedMemoryHeaps.resize(this->pCharacteristics->SharedHeaps.size());
}
catch (Error e) { this->ErrorCode = e; }


DflHW::Device::~Device(){
    INT_GLB_DeviceMutex.lock();
    for (auto& fence : this->pTracker->Fences) {
        if (fence.hFence != nullptr) {
            vkDestroyFence(
                this->GPU,
                fence,
                nullptr
            );
        }
    }

    if ( this->GPU.hDevice != nullptr ){
        vkDeviceWaitIdle(this->GPU);
        vkDestroyDevice(this->GPU, nullptr);
    }
    INT_GLB_DeviceMutex.unlock();
};