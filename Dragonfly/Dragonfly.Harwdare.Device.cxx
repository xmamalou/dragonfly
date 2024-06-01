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
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR

#include "Dragonfly.hxx"

#include <optional>
#include <string>
#include <iostream>
#include <thread>
#include <mutex>

#endif 
#include <vulkan/vulkan.h>

namespace DflHW  = Dfl::Hardware;
namespace DflGen = Dfl::Generics;

using DflMemType = DflHW::Device::MemoryType;

template <DflMemType type>
using DflDevMemory = DflHW::Device::Characteristics::Memory<type>;


// GLOBAL MUTEX

static std::mutex INT_GLB_DeviceMutex;

// Internal for Device constructor

template <DflMemType type>
static inline void INT_OrganizeMemory(
          std::vector<DflDevMemory<type>>& memory, 
    const VkPhysicalDeviceMemoryProperties props);

template <>
static inline void INT_OrganizeMemory(
          std::vector<DflDevMemory<DflMemType::Local>>& memory,
    const VkPhysicalDeviceMemoryProperties              props) 
{
    constexpr auto type{ DflMemType::Local };

    uint32_t heapCount{ 0 };
    for (uint32_t i{ 0 }; i < props.memoryHeapCount; i++)
    {
        if ( props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT )
        {
            memory[heapCount].Size = props.memoryHeaps[i].size;
            memory[heapCount].HeapIndex = i;

            DflDevMemory<type>::Properties dflProps{ };
            for (uint32_t j{ 0 }; j < VK_MAX_MEMORY_TYPES; j++) 
            {
                if (props.memoryTypes[j].heapIndex != i) 
                {
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
          std::vector<DflDevMemory<DflMemType::Shared>>& memory, 
    const VkPhysicalDeviceMemoryProperties               props) 
{
    constexpr auto type{ DflMemType::Shared };
    
    uint32_t heapCount{ 0 };
    for (uint32_t i{ 0 }; i < props.memoryHeapCount; i++) 
    {
        if ( !(props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ) 
        {
            memory[heapCount].Size = props.memoryHeaps[i].size;
            memory[heapCount].HeapIndex = props.memoryTypes[i].heapIndex;

            DflDevMemory<type>::Properties dflProps{ };
            for (uint32_t j{ 0 }; j < VK_MAX_MEMORY_TYPES; j++ ) 
            { 
                if (props.memoryTypes[j].heapIndex != i) 
                {
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

static DflHW::Device::Characteristics INT_OrganizeData(const VkPhysicalDevice& device)
{
    //VkDevice& gpu = device.Info.pSession->Device
    VkPhysicalDeviceProperties devProps;
    vkGetPhysicalDeviceProperties(device, &devProps);
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(device, &memProps);
    VkPhysicalDeviceFeatures devFeats;
    vkGetPhysicalDeviceFeatures(device, &devFeats);

    int localHeapCount{ 0 };
    int sharedHeapCount{ 0 };

    for (auto heap : memProps.memoryHeaps)
    {
        if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT &&
            heap.size != 0)
        {
            localHeapCount++;
        } else if (heap.size != 0) {
            sharedHeapCount++;
        }
    }

    std::vector<DflDevMemory<DflMemType::Local>> localHeaps(localHeapCount);
    std::vector<DflDevMemory<DflMemType::Shared>> sharedHeaps(sharedHeapCount);
    INT_OrganizeMemory(localHeaps, memProps);
    INT_OrganizeMemory(sharedHeaps, memProps);

    uint32_t currentCheck{ 0x40 }; // represents VkSampleCountFlagBits elements
    uint32_t maxColourSamples{ 0 };
    uint32_t maxDepthSamples{ 0 };
    for (uint32_t i = 0; i < 7; i++) 
    {
        if (devProps.limits.framebufferColorSampleCounts & currentCheck)
        {
            maxColourSamples = currentCheck;
            break;
        }
        currentCheck >>= 1;
    }
    currentCheck = 0x40;
    for (uint32_t i = 0; i < 7; i++) {
        if (devProps.limits.framebufferDepthSampleCounts & currentCheck) 
        {
            maxDepthSamples = currentCheck;
            break;
        }
        currentCheck >>= 1;
    }

    uint32_t extensionCount{ 0 };
    vkEnumerateDeviceExtensionProperties(
        device,
        nullptr,
        &extensionCount,
        nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(
        device,
        nullptr,
        &extensionCount,
        extensions.data());

    return {
        std::string(devProps.deviceName),
        localHeaps, sharedHeaps,
        { devProps.limits.maxViewportDimensions[0], devProps.limits.maxViewportDimensions[1] },
        { maxColourSamples, maxDepthSamples },
        { devProps.limits.maxComputeWorkGroupCount[0], devProps.limits.maxComputeWorkGroupCount[1], devProps.limits.maxComputeWorkGroupCount[2] },
        devProps.limits.maxMemoryAllocationCount,
        devProps.limits.maxDrawIndirectCount,
        extensions };
};

static inline auto INT_OrganizeQueues(const VkPhysicalDevice& device)
-> std::vector<DflHW::Device::Queue::Family>
{
    std::vector<DflHW::Device::Queue::Family> queueFamilies;
    std::vector<VkQueueFamilyProperties>      props;
    uint32_t                                  queueFamilyCount{ 0 };
    vkGetPhysicalDeviceQueueFamilyProperties(
        device,
        &queueFamilyCount,
        nullptr);
    if (queueFamilyCount == 0) {
        throw Dfl::Error::NoData(L"Unable to get device queues");
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

static auto INT_GetQueues(
    const uint32_t                                   rendererNum,
    const uint32_t                                   simNum,
    const std::vector<DflHW::Device::Queue::Family>& families) 
-> std::vector<VkDeviceQueueCreateInfo>
{
    uint32_t leftQueues{ rendererNum + simNum + 1 };

    std::vector<VkDeviceQueueCreateInfo> infos;

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
        throw Dfl::Error::NoData(L"Unable to find queues with appropriate specifications");
    }

    return infos;
};

//

static DflHW::Device::Handles INT_InitDevice(
    const VkInstance&                         instance,
    const VkPhysicalDevice&                   physDevice,
    const DflGen::BitFlag&                    renderOptions,
    const uint32_t                            rendererNum,
    const uint32_t                            simNum,
    const std::vector<VkExtensionProperties>& extensions)
{
    auto queueFamilies{ INT_OrganizeQueues(physDevice) };

    auto queueInfo{ INT_GetQueues( rendererNum,
                                   simNum,
                                   queueFamilies) };

    float** const priorities = new float* [queueInfo.size()];
    for (uint32_t i{ 0 }; i < queueInfo.size(); i++) 
    {
        priorities[i] = new float [queueInfo[i].queueCount];
        for (uint32_t j{ 0 }; j < queueInfo[i].queueCount; j++)
        {
            priorities[i][j] = 1.0f;
        }
        queueInfo[i].pQueuePriorities = priorities[i];
    }
    
    std::vector<const char*> desiredExtensions;
    desiredExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    for( auto& extension : extensions ) 
    {
        if (!strcmp(extension.extensionName, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)) 
        {
            desiredExtensions.push_back(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);
        }

        if (!strcmp(extension.extensionName, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)) 
        {
            desiredExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
            desiredExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
            desiredExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
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
       .enabledExtensionCount{ static_cast<uint32_t>(desiredExtensions.size()) },
       .ppEnabledExtensionNames{ desiredExtensions.data() },
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
        throw Dfl::Error::NoData(L"Unable to find the desired extensions on the device");
    case VK_ERROR_DEVICE_LOST:
        throw Dfl::Error::System(L"Unable to reach the device");
    default:
        throw Dfl::Error::HandleCreation(L"Unable to create logical device");
    }

    for (uint32_t i{ 0 }; i < queueInfo.size(); i++) {
        delete[] priorities[i];
    }

    delete[] priorities;

    return { gpu, physDevice, queueFamilies };
};

//

const VkDeviceMemory INT_GetStageMemory(
                        const VkDevice&                           gpu,
                        const std::vector<
                                DflDevMemory<DflMemType::Local>>& memories,
                              VkDeviceSize&                       stageSize,
                              VkBuffer&                           stageBuffer,
                              void*&                              stageMap,
                              std::vector<uint64_t>               usedMemories) 
{
    std::optional<uint32_t> stageMemoryIndex{ std::nullopt };
    bool isStageVisible{ false };

    // preferably, we want stage memory that is visible to the host 
    // Unlike main memory, the defauly stage memory size is simply a suggestion
    // and is not followed if it's a condition that cannot be met.
    // In other words, Dragonfly will prefer host visible memory over any kind of 
    // memory, even if there is more memory to claim from other heaps
    for (uint32_t i{0}; i < memories.size(); i++) 
    {
        for(auto property : memories[i].MemProperties) 
        {
            if ( (property.IsHostVisible || property.IsHostCached || property.IsHostCoherent) &&
                    !stageMemoryIndex.has_value() ) 
            {
                stageMemoryIndex = property.TypeIndex;
                isStageVisible = property.IsHostVisible;
                usedMemories[i] += Dfl::Memory::Block::StageMemorySize;
            }

            if( stageMemoryIndex.has_value() ) { break; }
        }
    }

    if (stageMemoryIndex == std::nullopt) {
        for (uint32_t i{ 0 }; i < memories.size(); i++) {
            if (memories[i] - usedMemories[i] > Dfl::Memory::Block::StageMemorySize) {
                stageMemoryIndex = memories[i].MemProperties[0].TypeIndex;
                isStageVisible = memories[i].MemProperties[0].IsHostVisible;
                usedMemories[i] += Dfl::Memory::Block::StageMemorySize;
                break;
            }
        }
    }

    VkDeviceMemory stageMemory{ nullptr };
    VkMemoryAllocateInfo memInfo{
        .sType{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO },
        .pNext{ nullptr },
        .allocationSize{ Dfl::Memory::Block::StageMemorySize },
        .memoryTypeIndex{ stageMemoryIndex.value() }
    };
    if ( vkAllocateMemory(
            gpu,
            &memInfo,
            nullptr,
            &stageMemory) != VK_SUCCESS ) 
    {
        throw Dfl::Error::HandleCreation(L"Unable to reserve stage memory");
    }

    const VkBufferCreateInfo bufInfo{
        .sType{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ 0 },
        .size{ Dfl::Memory::Block::StageMemorySize },
        .usage{ VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT },
        .sharingMode{ VK_SHARING_MODE_EXCLUSIVE }
    };

    if ( vkCreateBuffer(
            gpu,
            &bufInfo,
            nullptr,
            &stageBuffer) != VK_SUCCESS ) 
    {
        vkFreeMemory(
            gpu,
            stageMemory,
            nullptr);
        throw Dfl::Error::HandleCreation(L"Unable to create buffer for stage memory");
    }
    if ( vkBindBufferMemory(
            gpu,
            stageBuffer,
            stageMemory,
            0) != VK_SUCCESS ) 
    {
        vkDestroyBuffer(
            gpu,
            stageBuffer,
            nullptr);
        vkFreeMemory(
            gpu,
            stageMemory,
            nullptr);
        throw Dfl::Error::HandleCreation(L"Unable to bind memory to buffer");
    }
    
    if ( isStageVisible ) [[ likely ]] 
    {
        if (vkMapMemory(
                gpu,
                stageMemory,
                0,
                Dfl::Memory::Block::StageMemorySize,
                0,
                &stageMap) != VK_SUCCESS) 
        {
            vkDestroyBuffer(
                gpu,
                stageBuffer,
                nullptr);
            vkFreeMemory(
                gpu,
                stageMemory,
                nullptr);
            throw Dfl::Error::HandleCreation(L"Unable to map buffer");
        }
    }

    return stageMemory;
}

//

const VkFence DflHW::Device::GetFence(
                    const uint32_t queueFamilyIndex,
                    const uint32_t queueIndex) const 
{
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
: pInfo( new Info(info) ), 
  pCharacteristics( new Characteristics( 
                        INT_OrganizeData( info.Session.GetDeviceHandle(info.DeviceIndex) ) ) ),
  GPU( INT_InitDevice(
                info.Session.GetInstance(),
                info.Session.GetDeviceHandle(info.DeviceIndex),
                info.RenderOptions,
                info.RenderersNumber,
                info.SimulationsNumber,
                this->pCharacteristics->Extensions) ),
  pTracker( new Tracker() ) 
{
     this->pTracker->LeastClaimedQueue.resize(this->GPU.Families.size());
     this->pTracker->AreFamiliesUsed.resize(this->GPU.Families.size());
     this->pTracker->UsedLocalMemoryHeaps.resize(this->pCharacteristics->LocalHeaps.size());
     this->pTracker->UsedSharedMemoryHeaps.resize(this->pCharacteristics->SharedHeaps.size());

     try {
         this->pTracker->hStageMemory = INT_GetStageMemory(
                                            this->GPU,
                                            this->pCharacteristics->LocalHeaps,
                                            this->pTracker->StageMemorySize,
                                            this->pTracker->hStageBuffer,
                                            this->pTracker->pStageMemoryMap,
                                            this->pTracker->UsedLocalMemoryHeaps);
     } catch (Dfl::Error::HandleCreation& error) {
         vkDestroyDevice(
             this->GPU,
             nullptr);
         throw;
     }
}

DflHW::Device::~Device(){
    INT_GLB_DeviceMutex.lock();
    for (auto& fence : this->pTracker->Fences) 
    {
        if (fence.hFence != nullptr) 
        {
            vkDestroyFence(
                this->GPU,
                fence,
                nullptr
            );
        }
    }

    if (this->pTracker->hStageBuffer != nullptr) 
    {
        vkDestroyBuffer(
            this->GPU,
            this->pTracker->hStageBuffer,
            nullptr);
    }

    if (this->pTracker->hStageMemory != nullptr) 
    {
        vkFreeMemory(
            this->GPU,
            this->pTracker->hStageMemory,
            nullptr);
    }

    if ( this->GPU.hDevice != nullptr )
    {
        vkDeviceWaitIdle(this->GPU);
        vkDestroyDevice(this->GPU, nullptr);
    }
    INT_GLB_DeviceMutex.unlock();
};