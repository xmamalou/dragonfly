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

namespace DflOb = Dfl::Observer;
namespace DflHW = Dfl::Hardware;

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

            memory[heapCount].IsHostVisible = 
                props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? true : false;

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

            memory[heapCount].IsHostVisible =
                props.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ? true : false;

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
        if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT){
            localHeapCount++;
        }else{
            sharedHeapCount++;
        }
    }

    std::vector<DflHW::DeviceMemory<DflHW::MemoryType::Local>> localHeaps(localHeapCount);
    std::vector<DflHW::DeviceMemory<DflHW::MemoryType::Shared>> sharedHeaps(sharedHeapCount);
    INT_OrganizeMemory(localHeaps, memProps);
    INT_OrganizeMemory(sharedHeaps, memProps);

    return {
        std::string(devProps.deviceName),
        localHeaps, sharedHeaps,
        { devProps.limits.maxComputeWorkGroupCount[0], devProps.limits.maxComputeWorkGroupCount[1], devProps.limits.maxComputeWorkGroupCount[2] } };
};

static inline std::vector<DflHW::QueueFamily> INT_OrganizeQueues(
    const VkPhysicalDevice&                device) {
    std::vector<DflHW::QueueFamily> queueFamilies;
    std::vector<VkQueueFamilyProperties> props;
    uint32_t queueFamilyCount{ 0 };
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

const std::array< const char*, 4 > extensions{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
};

//

static DflHW::DeviceHandles INT_InitDevice(
    const VkInstance&                      instance,
    const VkPhysicalDevice&                physDevice,
    const Dfl::BitFlag&                    renderOptions,
    const uint32_t                         rendererNum,
    const uint32_t                         simNum){
    if (instance == nullptr)
        throw DflHW::DeviceError::VkDeviceInvalidSession;

    if (physDevice == nullptr) {
        throw DflHW::DeviceError::VkDeviceInvalidSession;
    }

    std::vector<DflHW::QueueFamily> queueFamilies = INT_OrganizeQueues(physDevice);

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

    const VkDeviceCreateInfo deviceInfo = {
       .sType{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO },
       .pNext{ nullptr },
       .flags{ 0 },
       .queueCreateInfoCount{ static_cast<uint32_t>(queueInfo.size()) },
       .pQueueCreateInfos{ queueInfo.data() },
       .enabledLayerCount{ 0 },
       .ppEnabledLayerNames{ nullptr },
       .enabledExtensionCount{ rendererNum > 1 ? (
                               renderOptions & DflHW::RenderOptions::Raytracing ?
                                    static_cast<uint32_t>(extensions.size()) : 1) : 0 },
       .ppEnabledExtensionNames{ extensions.data() },
    };

    VkDevice gpu{ nullptr };
    switch (VkResult vkError = vkCreateDevice(
        physDevice,
        &deviceInfo,
        nullptr,
        &gpu)) {
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

    std::vector<DflHW::Queue> usedQueues{ rendererNum + simNum + 1 };

    return { gpu, queueFamilies, usedQueues };
};

//

DflHW::Device::Device(const DeviceInfo& info) 
try : pInfo( new DeviceInfo(info) ), 
      pCharacteristics( new DeviceCharacteristics( INT_OrganizeData(
                                                     info.DeviceIndex < info.phSession->Instance.hDevices.size() ?
                                                         info.phSession->Instance.hDevices[info.DeviceIndex] 
                                                       : nullptr) ) ),
      hPhysicalDevice(info.DeviceIndex < info.phSession->Instance.hDevices.size() ?
                        info.phSession->Instance.hDevices[info.DeviceIndex]
                      : nullptr),
      GPU( INT_InitDevice(
                info.phSession->Instance,
                info.DeviceIndex < info.phSession->Instance.hDevices.size() ?
                    info.phSession->Instance.hDevices[info.DeviceIndex]
                    : nullptr,
                info.RenderOptions,
                info.RenderersNumber,
                info.SimulationsNumber) ) { }
catch (DeviceError e) { this->Error = e; }


DflHW::Device::~Device(){
    this->UsageMutex.lock();
    if ( this->GPU.hDevice != nullptr ){
        vkDeviceWaitIdle(this->GPU.hDevice);
        vkDestroyDevice(this->GPU.hDevice, nullptr);
    }
    this->UsageMutex.unlock();
};

// internal for InitDevice

