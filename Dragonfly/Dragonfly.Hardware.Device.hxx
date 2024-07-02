/*
   Copyright 2024 Christopher-Marios Mamaloukas

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

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <array>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "Dragonfly.h"

#include "Dragonfly.Generics.hxx"

namespace Dfl {
    namespace Graphics { class Renderer; }
    namespace Memory { class Block; }

    // Dragonfly.Hardware
    namespace Hardware {
        class Session;

        // Dragonfly.Hardware.Device
        class Device {
        public:
            enum class RenderOptions : unsigned int {
                Raytracing = 1,
            };

            struct Info {
                const Session&        Session; // the session the device belongs to
                const uint32_t        DeviceIndex{ 0 }; // the index of the device in the session's device list

                const uint32_t        RenderersNumber{ 1 };
                const DflGen::BitFlag RenderOptions{ 0 };
                const uint32_t        SimulationsNumber{ 1 };
            };

            enum class MemoryType : unsigned int {
                Local,
                Shared
            };

            struct Queue {
                enum class Type : unsigned int {
                    Graphics = 1,
                    Compute = 2,
                    Transfer = 4,
                };

                struct Family {
                    uint32_t             Index{ 0 };
                    uint32_t             QueueCount{ 0 };
                    DflGen::BitFlag      QueueType{ 0 };
                };

                const VkQueue  hQueue{ nullptr };
                const uint32_t FamilyIndex{ 0 };
                const uint32_t Index{ 0 };

                operator VkQueue() const { return this->hQueue; }
            };

            struct Fence {
                const VkFence  hFence{ nullptr };
                const uint32_t QueueFamilyIndex{ 0 };
                const uint32_t QueueIndex{ 0 };

                operator VkFence () { return this->hFence; }
            };

            struct Characteristics {
                const std::string                             Name{ "Placeholder GPU Name" };

                template < MemoryType type >
                struct Memory {
                    struct Properties {
                        uint32_t     TypeIndex{ 0 };

                        bool         IsHostVisible{ false };
                        bool         IsHostCoherent{ false };
                        bool         IsHostCached{ false };
                    };

                    VkDeviceSize            Size{ 0 };
                    uint32_t                HeapIndex{ 0 };

                    std::vector<Properties> MemProperties{ };

                    operator VkDeviceSize() const { return this->Size; }
                };
                const std::vector<Memory<MemoryType::Local>>  LocalHeaps{ };
                const std::vector<Memory<MemoryType::Shared>> SharedHeaps{ };

                const std::array<uint32_t, 2>                 MaxViewport{ {0, 0} };
                const std::array<uint32_t, 2>                 MaxSampleCount{ {0, 0} }; // {colour, depth}

                const std::array<uint32_t, 3>                 MaxGroups{ {0, 0, 0} }; // max amount of groups the device supports
                const uint64_t                                MaxAllocations{ 0 };

                const uint64_t                                MaxDrawIndirectCount{ 0 };
            
                const std::vector<VkExtensionProperties>      Extensions{ };
            
            };

            struct Tracker {
                uint64_t                           Allocations{ 0 };
                uint64_t                           IndirectDraws{ 0 };

                std::vector<
                    std::vector<uint32_t>>         QueueClaims{ };

                std::vector<uint64_t>              UsedLocalMemoryHeaps{ }; // size is the amount of heaps
                std::vector<uint64_t>              UsedSharedMemoryHeaps{ }; // size is the amount of heaps

                std::vector<Fence>                 Fences{ };

                VkDeviceMemory                     hStageMemory{ nullptr };
                VkBuffer                           hStageBuffer{ nullptr };

                VkDeviceMemory                     hIntermediateMem{ nullptr };
                VkBuffer                           hIntermediateBuffer{ nullptr };

                void*                              pStageMemoryMap{ nullptr };
            };

            struct Handles {
                const VkDevice                    hDevice{ nullptr };
                const VkPhysicalDevice            hPhysicalDevice{ nullptr };
                const std::vector<Queue::Family>  Families{ };

                operator VkDevice() const { return this->hDevice; }
                operator VkPhysicalDevice() const { return this->hPhysicalDevice; }
            };

            static constexpr uint64_t StageMemory{ 65536 }; // 64 KB of stage memory
            static constexpr uint64_t IntermediateMemory{ 829440 }; // 24 MB of intermediate memory for copying to images
            // roughly the size of a 4K image
        protected:
            const std::unique_ptr<const Info>             pInfo{ };
            const std::unique_ptr<const Characteristics>  pCharacteristics{ };

            const Handles                                 GPU{ };
            const std::unique_ptr<      Tracker>          pTracker{ };
                          
        public:
            DFL_API DFL_CALL Device(const Info& info);
            DFL_API DFL_CALL ~Device();

            //

            const Session&                     GetSession() const noexcept { 
                                                    return this->pInfo->Session; }
            const Characteristics&             GetCharacteristics() const noexcept { 
                                                    return *this->pCharacteristics; }
            const VkDevice                     GetDevice() const noexcept {
                                                    return this->GPU.hDevice; }
            const VkPhysicalDevice             GetPhysicalDevice() const noexcept {
                                                    return this->GPU.hPhysicalDevice; }
            const decltype(Handles::Families)& GetQueueFamilies() const noexcept {
                                                    return this->GPU.Families; }
            const VkDeviceMemory&              GetStageMemory() const noexcept {
                                                    return this->pTracker->hStageMemory; }
            const VkBuffer&                    GetStageBuffer() const noexcept {
                                                    return this->pTracker->hStageBuffer; }
            const void*                        GetStageMap() const noexcept {
                                                    return this->pTracker->pStageMemoryMap; }
            const VkDeviceMemory&              GetIntermediateMemory() const noexcept {
                                                    return this->pTracker->hIntermediateMem; }
            const VkBuffer&                    GetIntermediateBuffer() const noexcept {
                                                    return this->pTracker->hIntermediateBuffer; }
            DFL_API 
            const VkFence 
            DFL_CALL                           GetFence(
                                                    const uint32_t queueFamilyIndex,
                                                    const uint32_t queueIndex) const;
            DFL_API
            const Queue                       
            DFL_CALL                           BorrowQueue(Queue::Type type) noexcept;
            template< MemoryType type >
            const VkDeviceMemory               BorrowMemory(
                                                    uint64_t heapIndex,
                                                    bool     isHostVisible,
                                                    bool     isHostCached,
                                                    bool     isHostCoherent,
                                                    bool     hasAnyProperty,
                                                    uint64_t size) noexcept;
            
                  void                         ReturnQueue(Queue queue) noexcept {
                                                    this->pTracker->
                                                    QueueClaims[queue.FamilyIndex][queue.Index]--; };
            template< MemoryType type >
                  void                         ReturnMemory(
                                                    VkDeviceMemory memory,
                                                    uint64_t       heapIndex,
                                                    uint64_t       size) noexcept {
                                                    this->pTracker->Allocations--;
                                                    
                                                    if constexpr (type == MemoryType::Local) { this->pTracker->UsedLocalMemoryHeaps[heapIndex] += size; }
                                                    else { this->pTracker->UsedSharedMemoryHeaps[heapIndex] += size; }
                                                    
                                                    vkFreeMemory(this->GPU, memory, nullptr); };
        };
    }
    namespace DflHW = Dfl::Hardware;
}

template< Dfl::Hardware::Device::MemoryType type >
const VkDeviceMemory Dfl::Hardware::Device::BorrowMemory(
                                        uint64_t heapIndex,
                                        bool     isHostVisible,
                                        bool     isHostCached,
                                        bool     isHostCoherent,
                                        bool     hasAnyProperty,
                                        uint64_t size) noexcept
{
    if( heapIndex >= ( type == Device::MemoryType::Local 
                          ? this->pCharacteristics->LocalHeaps.size()
                          : this->pCharacteristics->SharedHeaps.size() ) )
    {
        return nullptr;
    }

    if (this->pTracker->Allocations + 1 > this->pCharacteristics->MaxAllocations)
    {
        return nullptr;
    }

    std::optional<uint32_t> typeIndex{ std::nullopt };

    if constexpr ( type == Device::MemoryType::Local )
    { 
        for(auto& property : this->pCharacteristics->LocalHeaps[heapIndex].MemProperties)
        {
                if (( ( (isHostVisible == property.IsHostVisible)
                         && (isHostCached == property.IsHostCached) 
                         && (isHostCoherent == property.IsHostCoherent) ) || hasAnyProperty ) 
                      && this->pCharacteristics->LocalHeaps[heapIndex] - this->pTracker->UsedLocalMemoryHeaps[heapIndex] > size ) 
                {
                    typeIndex = property.TypeIndex;
                    this->pTracker->UsedLocalMemoryHeaps[heapIndex] += size;
                    break;
                }
        }
    }
    else
    {
        for(auto& property : this->pCharacteristics->SharedHeaps[heapIndex].MemProperties)
        {
                if (( ( (isHostVisible == property.IsHostVisible)
                         && (isHostCached == property.IsHostCached) 
                         && (isHostCoherent == property.IsHostCoherent) ) || hasAnyProperty ) 
                      && this->pCharacteristics->SharedHeaps[heapIndex] - this->pTracker->UsedSharedMemoryHeaps[heapIndex] > size ) 
                {
                    typeIndex = property.TypeIndex;
                    this->pTracker->UsedSharedMemoryHeaps[heapIndex] += size;
                    break;
                }
        }
    }

    if (!typeIndex.has_value())
    {
        return nullptr;
    }

    VkDeviceMemory memory{ nullptr };
    VkMemoryAllocateInfo memInfo{
        .sType{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO },
        .pNext{ nullptr },
        .allocationSize{ size },
        .memoryTypeIndex{ typeIndex.value() }
    };
    if ( vkAllocateMemory(
                    this->GPU,
                    &memInfo,
                    nullptr,
                    &memory) != VK_SUCCESS ) 
    {
        return nullptr;
    }

    this->pTracker->Allocations++;
    return memory;
}