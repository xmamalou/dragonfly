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

module Dragonfly.Hardware.Memory;

namespace DflHW = Dfl::Hardware;

static inline DflHW::Queue INT_GetQueue(
    const VkDevice&                                device,
    const std::vector<Dfl::Hardware::QueueFamily>& families,
          uint32_t* const                          pLeastClaimedQueue,
          std::vector<bool>&                       areFamiliesUsed) {
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
        if (pLeastClaimedQueue[familyIndex] >= families[familyIndex].QueueCount) {
            pLeastClaimedQueue[familyIndex] = 0;
        }

        vkGetDeviceQueue(
            device,
            familyIndex,
            pLeastClaimedQueue[familyIndex],
            &queue
        );
        pLeastClaimedQueue[familyIndex]++;
        break;
    }

    areFamiliesUsed[familyIndex] = true;

    return { queue, familyIndex };
};

static inline VkEvent INT_GetEvent(const VkDevice& hGPU) {
    const VkEventCreateInfo eventInfo{
        .sType{ VK_STRUCTURE_TYPE_EVENT_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ 0 }
    };
    VkEvent  event{ nullptr };
    VkResult error{ VK_SUCCESS };
    if ((error = vkCreateEvent(
        hGPU,
        &eventInfo,
        nullptr,
        &event)) != VK_SUCCESS) {
        throw DflHW::BufferError::VkBufferEventCreationError;
    }

    return event;
}

static VkCommandPool INT_GetCmdPool(
    const VkDevice& hGPU,
    const uint32_t  queueFamilyIndex) {
          VkCommandPool           cmdPool{ nullptr };
    const VkCommandPoolCreateInfo cmdPoolInfo{
        .sType{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ 0 },
        .queueFamilyIndex{ queueFamilyIndex }
    };
    if (vkCreateCommandPool(
                hGPU,
                &cmdPoolInfo,
                nullptr,
                &cmdPool
            ) != VK_SUCCESS) {
       throw DflHW::MemoryError::VkMemoryCmdPoolError;
    }

    return cmdPool;
}

static VkCommandBuffer INT_GetCmdBuffer(
    const VkDevice&      hGPU,
    const VkCommandPool& hCmdPool) {
          VkCommandBuffer             cmdBuff{ nullptr };
    const VkCommandBufferAllocateInfo cmdBuffInfo{
        .sType{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO },
        .pNext{ nullptr },
        .commandPool{ hCmdPool },
        .level{ VK_COMMAND_BUFFER_LEVEL_PRIMARY },
        .commandBufferCount{ 1 }
    };
    if (vkAllocateCommandBuffers(
            hGPU,
            &cmdBuffInfo,
            &cmdBuff) != VK_SUCCESS) {
        throw DflHW::BufferError::VkBufferSemaphoreCreationError;
    }

    return cmdBuff;
}

static DflHW::MemoryBlock INT_GetMemory(
    const VkDevice&                                                   gpu,
    const uint64_t&                                                   maxAllocs,
          uint64_t&                                                   totAllocs,
    const uint64_t&                                                   memorySize,
    const std::vector<DflHW::DeviceMemory<DflHW::MemoryType::Local>>& memories,
    const std::vector<DflHW::QueueFamily>                             families,
          uint32_t* const                                             pLeastClaimedQueue,
          std::vector<bool>&                                          areFamiliesUsed) {
    if (totAllocs + 2 > maxAllocs) {
        throw DflHW::MemoryError::VkMemoryMaxAllocsReachedError;
    }

    std::optional<uint32_t> mainMemoryIndex{ std::nullopt };
    std::optional<uint32_t> stageMemoryIndex{ std::nullopt };
    uint64_t                oldMemorySize{ 0 };

    for (auto& memory : memories) {
        if ( !memory.IsHostVisible && !memory.IsHostCached && !memory.IsHostCoherent ) {
            mainMemoryIndex = memory.Size + DflHW::StageMemorySize > oldMemorySize ? 
                                    memory.HeapIndex : mainMemoryIndex;
        }
        
        if (memory.IsHostVisible || memory.IsHostCached || memory.IsHostCoherent) {
            stageMemoryIndex = memory.Size + DflHW::StageMemorySize > oldMemorySize ?
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
                        ( memories[mainMemoryIndex.value()].Size > memorySize + DflHW::StageMemorySize ?
                            memorySize + DflHW::StageMemorySize :
                            memories[mainMemoryIndex.value()].Size ) :
                        ( memories[mainMemoryIndex.value()].Size > memorySize ? 
                            memorySize :
                            memories[mainMemoryIndex.value()].Size );

    VkDeviceMemory mainMemory{ nullptr };
    VkResult error{ VK_SUCCESS };
    VkMemoryAllocateInfo memInfo{
        .sType{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO },
        .pNext{ nullptr },
        .allocationSize{ mainMemoryIndex.value() == stageMemoryIndex.value() ?
                                oldMemorySize - DflHW::StageMemorySize : 
                                oldMemorySize },
        .memoryTypeIndex{ mainMemoryIndex.value() }
    };
    if ((error = vkAllocateMemory(
        gpu,
        &memInfo,
        nullptr,
        &mainMemory)) != VK_SUCCESS) {
        throw DflHW::MemoryError::VkMemoryAllocationError;
    }

    VkDeviceMemory stageMemory{ nullptr };
    memInfo.allocationSize = DflHW::StageMemorySize;
    memInfo.memoryTypeIndex = stageMemoryIndex.value();
    if ((error = vkAllocateMemory(
        gpu,
        &memInfo,
        nullptr,
        &stageMemory)) != VK_SUCCESS) {
        vkFreeMemory(
            gpu,
            mainMemory,
            nullptr
        );
        throw DflHW::MemoryError::VkMemoryAllocationError;
    }

    totAllocs += 2;

    std::vector<uint32_t> usedFamilies{ };
    for (uint32_t i{ 0 }; i < areFamiliesUsed.size(); i++) {
        if (areFamiliesUsed[i]) {
            usedFamilies.push_back(i);
        }
    }

    const VkBufferCreateInfo bufInfo{
        .sType{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ 0 },
        .size{ DflHW::StageMemorySize },
        .usage{ VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT },
        .sharingMode{ VK_SHARING_MODE_EXCLUSIVE }
    };

    VkBuffer stageBuff{ nullptr };
    if ( (error = vkCreateBuffer(
                    gpu,
                    &bufInfo,
                    nullptr,
                    &stageBuff) ) != VK_SUCCESS) {
        throw DflHW::MemoryError::VkMemoryStageBuffError;
    }
    if ( (error = vkBindBufferMemory(
                        gpu,
                        stageBuff,
                        stageMemory,
                        0) ) != VK_SUCCESS) {
        throw DflHW::MemoryError::VkMemoryStageBuffError;
    }
    void* stageData{ nullptr };
    if (memories[stageMemoryIndex.value()].IsHostVisible) {
        if ( (error = vkMapMemory(
                            gpu,
                            stageMemory,
                            0,
                            DflHW::StageMemorySize,
                            0,
                            &stageData) ) != VK_SUCCESS ) {
            throw DflHW::MemoryError::VkMemoryMapError;
        }
    }

    DflHW::Queue queue = INT_GetQueue(
                            gpu,
                            families,
                            pLeastClaimedQueue,
                            areFamiliesUsed);

    VkCommandPool cmdPool = INT_GetCmdPool(
                                gpu,
                                queue.FamilyIndex);

    return { mainMemory, 
             stageMemory, stageBuff, memories[stageMemoryIndex.value()].IsHostVisible, stageData,
             queue, cmdPool,
             INT_GetCmdBuffer(
                gpu,
                cmdPool),
             INT_GetEvent(gpu),
             mainMemoryIndex.value() == stageMemoryIndex.value() ?
                        oldMemorySize - DflHW::StageMemorySize :
                        oldMemorySize };
 };

DflHW::Memory::Memory(const MemoryInfo& info)
try : pGPU(info.pDevice),
      MaxAllocsAllowed(info.pDevice->pCharacteristics->MaxAllocations),
      MemoryPool(INT_GetMemory(
          info.pDevice->GPU,
          info.pDevice->pCharacteristics->MaxAllocations,
          info.pDevice->pTracker->Allocations,
          info.Size,
          info.pDevice->pCharacteristics->LocalHeaps,
          info.pDevice->GPU.Families,
          info.pDevice->GPU.pLeastClaimedQueue.get(),
          info.pDevice->pTracker->AreFamiliesUsed)) {
}
catch (DflHW::MemoryError e) { this->Error = MemoryError::VkMemoryAllocationError; };

DflHW::Memory::~Memory() {
    if ( this->MemoryPool.hCmdPool != nullptr ) {
        vkDeviceWaitIdle(this->pGPU->GPU);
        vkDestroyCommandPool(
            this->pGPU->GPU,
            this->MemoryPool.hCmdPool,
            nullptr
        );
    }

    if ( this->MemoryPool.hMemory != nullptr ) {
        vkDeviceWaitIdle(this->pGPU->GPU);
        vkFreeMemory(
            this->pGPU->GPU,
            this->MemoryPool.hMemory,
            nullptr
        );
    }

    if (this->MemoryPool.hStageTransferEvent != nullptr) {
        vkDeviceWaitIdle(this->pGPU->GPU);
        vkDestroyEvent(
            this->pGPU->GPU,
            this->MemoryPool.hStageTransferEvent,
            nullptr
        );
    }

    if (this->MemoryPool.hStageBuffer != nullptr) {
        vkDeviceWaitIdle(this->pGPU->GPU);
        vkDestroyBuffer(
            this->pGPU->GPU,
            this->MemoryPool.hStageBuffer,
            nullptr
        );
    }

    if (this->MemoryPool.hStageMemory != nullptr) {
        vkDeviceWaitIdle(this->pGPU->GPU);
        vkFreeMemory(
            this->pGPU->GPU,
            this->MemoryPool.hStageMemory,
            nullptr
        );
    }
}

static inline VkBuffer INT_GetBuffer(
    const VkDevice&          hGPU,
    const uint64_t&          size,
    const uint32_t&          flags,
    const std::vector<bool>& areFamiliesUsed) {
    std::vector<uint32_t> usedFamilies{ };
    for (uint32_t i{ 0 }; i < areFamiliesUsed.size(); i++) {
        if ( areFamiliesUsed[i] ) { 
            usedFamilies.push_back(i); }
    }

    const VkBufferCreateInfo bufInfo{
        .sType{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ 0 },
        .size{ size },
        .usage{ VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                flags },
        .sharingMode{ ( usedFamilies.size() <= 1 ) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT },
        .queueFamilyIndexCount{ static_cast<uint32_t>(usedFamilies.size()) },
        .pQueueFamilyIndices{ usedFamilies.data() }
    };

    VkBuffer buff{ nullptr };
    VkResult error{ VK_SUCCESS };
    if ( ( error = vkCreateBuffer(
                            hGPU,
                            &bufInfo,
                            nullptr,
                            &buff
                         ) ) != VK_SUCCESS) {
        throw DflHW::BufferError::VkBufferAllocError;
    }

    return buff;
}

static inline VkSemaphore INT_GetSemaphore(const VkDevice& hGPU) {
    const VkSemaphoreCreateInfo semInfo{
        .sType{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ 0 }
    };

    VkSemaphore sem{ nullptr };
    VkResult error{ VK_SUCCESS };
    if ((error = vkCreateSemaphore(
        hGPU,
        &semInfo,
        nullptr,
        &sem)) != VK_SUCCESS) {
        throw DflHW::BufferError::VkBufferSemaphoreCreationError;
    }

    return sem;
}

static VkCommandBuffer INT_GetCmdBuffer(
    const VkDevice&      hGPU,
    const VkCommandPool& hCmdPool,
    const VkSemaphore&   hTransferSemaphore,
    const uint64_t&      bufferSize,
    const VkBuffer&      stageBuffer,
    const VkBuffer       destinationBuffer) {
    const VkCommandBufferAllocateInfo cmdBuffInfo{
        .sType{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO },
        .pNext{ nullptr },
        .commandPool{ hCmdPool },
        .level{ VK_COMMAND_BUFFER_LEVEL_PRIMARY },
        .commandBufferCount{ 1 } 
    };
    VkCommandBuffer cmdBuff{ nullptr};
    VkResult        error{ VK_SUCCESS };
    if ( ( error = vkAllocateCommandBuffers(
                        hGPU,
                        &cmdBuffInfo,
                        &cmdBuff) ) != VK_SUCCESS ) {
        throw DflHW::BufferError::VkBufferSemaphoreCreationError;
    }

    const VkCommandBufferBeginInfo cmdBuffBeginInfo{
        .sType{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO },
        .pNext{ nullptr },
        .flags{ },
        .pInheritanceInfo{ }
    };
    if ( ( error = vkBeginCommandBuffer(
                        cmdBuff,
                        &cmdBuffBeginInfo) ) != VK_SUCCESS ) {
        throw DflHW::BufferError::VkBufferCmdRecordingError;
    }
    //const uint32_t times{ bufferSize/DflHW::StageMemorySize + 1 };

    return cmdBuff;
}

static inline DflHW::BufferHandles INT_GetBufferHandles(
    const VkDevice&          hGPU,
    const VkCommandPool&     hPool,
    const uint64_t&          size,
    const uint32_t&          flags,
    const std::vector<bool>& areFamiliesUsed,
    const VkBuffer&          stageBuffer) {
    const VkBuffer buffer = INT_GetBuffer(
                                hGPU,
                                size, 
                                flags,
                                areFamiliesUsed);

    const VkSemaphore sem = INT_GetSemaphore(hGPU);
    
    return { buffer, sem, INT_GetCmdBuffer(
                            hGPU,
                            hPool,
                            sem,
                            size,
                            stageBuffer,
                            buffer) };
}

DflHW::Buffer::Buffer(const BufferInfo& info)
try : pInfo( new DflHW::BufferInfo(info) ),
      Buffers( INT_GetBufferHandles(
                    info.MemoryBlock->pGPU->GPU,
                    info.MemoryBlock->MemoryPool.hCmdPool,
                    info.Size,
                    info.Options.GetValue(),
                    info.MemoryBlock->pGPU->pTracker->AreFamiliesUsed,
                    info.MemoryBlock->MemoryPool.hStageBuffer) ) {}
catch ( DflHW::BufferError e ) { this->Error = e; }

DflHW::Buffer::~Buffer() {
    if( this->Buffers.hFromStageToMainCmd != nullptr ) {
        vkDeviceWaitIdle(this->pInfo->MemoryBlock->pGPU->GPU);
        vkFreeCommandBuffers(
            this->pInfo->MemoryBlock->pGPU->GPU,
            this->pInfo->MemoryBlock->MemoryPool.hCmdPool,
            1,
            &this->Buffers.hFromStageToMainCmd);
    }

    if (this->Buffers.hTransferDoneSem != nullptr) {
        vkDeviceWaitIdle(this->pInfo->MemoryBlock->pGPU->GPU);
        vkDestroySemaphore(
            this->pInfo->MemoryBlock->pGPU->GPU,
            this->Buffers.hTransferDoneSem,
            nullptr
        );
    }

    if (this->Buffers.hBuffer != nullptr) {
        vkDeviceWaitIdle(this->pInfo->MemoryBlock->pGPU->GPU);
        vkDestroyBuffer(
            this->pInfo->MemoryBlock->pGPU->GPU,
            this->Buffers.hBuffer,
            nullptr
        );
    }
}