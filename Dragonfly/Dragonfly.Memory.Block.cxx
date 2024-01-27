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

#include <vector>
#include <optional>

#include <vulkan/vulkan.h>

module Dragonfly.Memory.Block;

namespace DflMem = Dfl::Memory;
namespace DflHW = Dfl::Hardware;

static inline DflHW::Queue INT_GetQueue(
    const VkDevice&                                device,
    const std::vector<Dfl::Hardware::QueueFamily>& families,
          std::vector<uint32_t>&                   leastClaimedQueue,
          std::vector<uint32_t>&                   areFamiliesUsed) {
    VkQueue  queue{ nullptr };
    uint32_t familyIndex{ 0 };
    uint32_t amountOfClaimedGoal{ 0 };
    while (familyIndex <= families.size()) {
        if (familyIndex == families.size()) {
            amountOfClaimedGoal++;
            familyIndex = 0;
            continue;
        }

        if (!(families[familyIndex].QueueType & Dfl::Hardware::QueueType::Transfer)) {
            familyIndex++;
            continue;
        }

        /*
        * The way Dragonfly searches for queues is this:
        * If it finds a queue family of the type it wants, it first
        * checks what value pFamilyQueue[i], where i the family index,
        * has. This value is, essentially, the index of the queue that has
        * been claimed the least. If the value exceeds (or is equal to)
        * the number of queues in the family, it resets it to 0.
        * This is a quick way to ensure that work is distributed evenly
        * between queues and that no queue is overworked.
        *
        * Note that this does not check whether the queues of the appropriate
        * type indeed exist, since this condition should be verified already
        * from device creation.
        */
        if (leastClaimedQueue[familyIndex] >= families[familyIndex].QueueCount) {
            leastClaimedQueue[familyIndex] = 0;
        }

        vkGetDeviceQueue(
            device,
            familyIndex,
            leastClaimedQueue[familyIndex],
            &queue
        );
        leastClaimedQueue[familyIndex]++;
        break;
    }

    areFamiliesUsed[familyIndex]++;

    return { queue, familyIndex };
};

static inline VkCommandPool INT_GetCmdPool(
    const VkDevice& hGPU,
    const uint32_t& queueFamilyIndex) {
    const VkCommandPoolCreateInfo cmdPoolInfo{
        .sType{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT },
        .queueFamilyIndex{ queueFamilyIndex }
    };
    VkCommandPool cmdPool{ nullptr };
    VkResult      error{ VK_SUCCESS };
    if (( error = vkCreateCommandPool(
                        hGPU,
                        &cmdPoolInfo,
                        nullptr,
                        &cmdPool) ) != VK_SUCCESS) {
        throw DflMem::BlockError::VkMemoryCmdPoolError;
    }

    return cmdPool;
}

static DflMem::BlockHandles INT_GetMemory(
    const VkDevice&                                                   gpu,
    const uint64_t&                                                   maxAllocs,
          uint64_t&                                                   totAllocs,
    const uint64_t&                                                   memorySize,
    const std::vector<DflHW::DeviceMemory<DflHW::MemoryType::Local>>& memories,
          std::vector<uint32_t>&                                      areFamiliesUsed) {
    if (totAllocs + 2 > maxAllocs) {
        throw DflMem::BlockError::VkMemoryMaxAllocsReachedError;
    }

    std::optional<uint32_t> mainMemoryIndex{ std::nullopt };
    std::optional<uint32_t> stageMemoryIndex{ std::nullopt };
    uint64_t                oldMemorySize{ 0 };

    for (auto& memory : memories) {
        if (!memory.IsHostVisible && !memory.IsHostCached && !memory.IsHostCoherent) {
            mainMemoryIndex = memory.Size + DflMem::StageMemorySize > oldMemorySize ?
                memory.HeapIndex : mainMemoryIndex;
        }

        if (memory.IsHostVisible || memory.IsHostCached || memory.IsHostCoherent) {
            stageMemoryIndex = memory.Size + DflMem::StageMemorySize > oldMemorySize ?
                memory.HeapIndex : stageMemoryIndex;
        }
    }

    if (mainMemoryIndex == std::nullopt) {
        mainMemoryIndex = 0;
    }

    if (stageMemoryIndex == std::nullopt) {
        stageMemoryIndex = 0;
    }

    oldMemorySize = mainMemoryIndex.value() == stageMemoryIndex.value() ?
        (memories[mainMemoryIndex.value()].Size > memorySize + DflMem::StageMemorySize ?
            memorySize + DflMem::StageMemorySize :
            memories[mainMemoryIndex.value()].Size) :
        (memories[mainMemoryIndex.value()].Size > memorySize ?
            memorySize :
            memories[mainMemoryIndex.value()].Size);

    VkDeviceMemory mainMemory{ nullptr };
    VkResult error{ VK_SUCCESS };
    VkMemoryAllocateInfo memInfo{
        .sType{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO },
        .pNext{ nullptr },
        .allocationSize{ mainMemoryIndex.value() == stageMemoryIndex.value() ?
                                oldMemorySize - DflMem::StageMemorySize :
                                oldMemorySize },
        .memoryTypeIndex{ mainMemoryIndex.value() }
    };
    if ((error = vkAllocateMemory(
                    gpu,
                    &memInfo,
                    nullptr,
                    &mainMemory)) != VK_SUCCESS) {
        throw DflMem::BlockError::VkMemoryAllocationError;
    }

    VkDeviceMemory stageMemory{ nullptr };
    memInfo.allocationSize = DflMem::StageMemorySize;
    memInfo.memoryTypeIndex = stageMemoryIndex.value();
    if ( (error = vkAllocateMemory(
                        gpu,
                        &memInfo,
                        nullptr,
                        &stageMemory) ) != VK_SUCCESS) {
        vkFreeMemory(
            gpu,
            mainMemory,
            nullptr
        );
        throw DflMem::BlockError::VkMemoryAllocationError;
    }

    totAllocs += 2;

    std::vector<uint32_t> usedFamilies{ };
    for (uint32_t i{ 0 }; i < areFamiliesUsed.size(); i++) {
        if (areFamiliesUsed[i] > 0) {
            usedFamilies.push_back(i);
        }
    }

    const VkBufferCreateInfo bufInfo{
        .sType{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ 0 },
        .size{ DflMem::StageMemorySize },
        .usage{ VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT },
        .sharingMode{ VK_SHARING_MODE_EXCLUSIVE }
    };

    VkBuffer stageBuff{ nullptr };
    if ((error = vkCreateBuffer(
        gpu,
        &bufInfo,
        nullptr,
        &stageBuff)) != VK_SUCCESS) {
        throw DflMem::BlockError::VkMemoryStageBuffError;
    }
    if ((error = vkBindBufferMemory(
        gpu,
        stageBuff,
        stageMemory,
        0)) != VK_SUCCESS) {
        throw DflMem::BlockError::VkMemoryStageBuffError;
    }
    void* stageData{ nullptr };
    if (memories[stageMemoryIndex.value()].IsHostVisible) {
        if ((error = vkMapMemory(
            gpu,
            stageMemory,
            0,
            DflMem::StageMemorySize,
            0,
            &stageData)) != VK_SUCCESS) {
            throw DflMem::BlockError::VkMemoryMapError;
        }
    }

    return { mainMemory,
             stageMemory, stageBuff, memories[stageMemoryIndex.value()].IsHostVisible, stageData };
};

DflMem::Block::Block(const BlockInfo& info)
try : pInfo( new DflMem::BlockInfo(info) ),
      pHandles( new DflMem::BlockHandles( INT_GetMemory(
                                            info.pDevice->GPU,
                                            info.pDevice->pCharacteristics->MaxAllocations,
                                            info.pDevice->pTracker->Allocations,
                                            info.Size,
                                            info.pDevice->pCharacteristics->LocalHeaps,
                                            info.pDevice->pTracker->AreFamiliesUsed) ) ),
      TransferQueue( INT_GetQueue(
                        info.pDevice->GPU,
                        info.pDevice->GPU.Families,
                        info.pDevice->pTracker->LeastClaimedQueue, 
                        info.pDevice->pTracker->AreFamiliesUsed) ),
      pMutex( new std::mutex() ) {
    const VkCommandPoolCreateInfo cmdPoolInfo{
          .sType{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO },
          .pNext{ nullptr },
          .flags{ },
          .queueFamilyIndex{ this->TransferQueue.FamilyIndex }
    };
    VkResult      error{ VK_SUCCESS };
    if ((error = vkCreateCommandPool(
                    info.pDevice->GPU,
                    &cmdPoolInfo,
                    nullptr,
                    &this->hCmdPool)) != VK_SUCCESS) {
        vkDeviceWaitIdle(this->pInfo->pDevice->GPU);
        this->pInfo->pDevice->pUsageMutex->lock();
        if (this->pHandles->hMemory != nullptr) {
            vkFreeMemory(
                this->pInfo->pDevice->GPU,
                this->pHandles->hMemory,
                nullptr
            );
        }

        if (this->pHandles->hStageBuffer != nullptr) {
            vkDestroyBuffer(
                this->pInfo->pDevice->GPU,
                this->pHandles->hStageBuffer,
                nullptr
            );
        }

        if (this->pHandles->hStageMemory != nullptr) {
            vkFreeMemory(
                this->pInfo->pDevice->GPU,
                this->pHandles->hStageMemory,
                nullptr
            );
        }
        this->pInfo->pDevice->pUsageMutex->unlock();
        this->Error = DflMem::BlockError::VkMemoryCmdPoolError;
    }
}
catch (DflMem::BlockError e) { this->Error = BlockError::VkMemoryAllocationError; };

DflMem::Block::~Block() {
    this->pMutex->lock();
    vkDeviceWaitIdle(this->pInfo->pDevice->GPU);
    this->pInfo->pDevice->pUsageMutex->lock();
    if (this->hCmdPool != nullptr) {
        vkDestroyCommandPool(
            this->pInfo->pDevice->GPU,
            this->hCmdPool,
            nullptr
        );
    }

    if (this->pHandles->hMemory != nullptr) {
        vkFreeMemory(
            this->pInfo->pDevice->GPU,
            this->pHandles->hMemory,
            nullptr
        );
    }

    if (this->pHandles->hStageBuffer != nullptr) {
        vkDestroyBuffer(
            this->pInfo->pDevice->GPU,
            this->pHandles->hStageBuffer,
            nullptr
        );
    }

    if (this->pHandles->hStageMemory != nullptr) {
        vkFreeMemory(
            this->pInfo->pDevice->GPU,
            this->pHandles->hStageMemory,
            nullptr
        );
    }
    this->pInfo->pDevice->pUsageMutex->unlock();
    this->pMutex->unlock();
}