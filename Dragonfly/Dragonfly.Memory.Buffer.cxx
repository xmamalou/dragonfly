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

#include "Dragonfly.h"

#include <vector>
#include <optional>

#include <vulkan/vulkan.h>

module Dragonfly.Memory.Buffer;

namespace DflMem = Dfl::Memory;

static inline VkBuffer INT_GetBuffer(
    const VkDevice&              hGPU,
    const uint64_t&              size,
    const uint32_t&              flags,
    const std::vector<uint32_t>& areFamiliesUsed) {
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
        .size{ size },
        .usage{ VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                flags },
        .sharingMode{ (usedFamilies.size() <= 1) ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT },
        .queueFamilyIndexCount{ static_cast<uint32_t>(usedFamilies.size()) },
        .pQueueFamilyIndices{ usedFamilies.data() }
    };

    VkBuffer buff{ nullptr };
    VkResult error{ VK_SUCCESS };
    if ((error = vkCreateBuffer(
        hGPU,
        &bufInfo,
        nullptr,
        &buff
    )) != VK_SUCCESS) {
        throw DflMem::BufferError::VkBufferAllocError;
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
        throw DflMem::BufferError::VkBufferSemaphoreCreationError;
    }

    return sem;
}

static inline VkEvent INT_GetEvent(const VkDevice& hGPU) {
    const VkEventCreateInfo eventInfo{
        .sType{ VK_STRUCTURE_TYPE_EVENT_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ 0 }
    };
    VkEvent event{ nullptr };
    VkResult error{ VK_SUCCESS };
    if ((error = vkCreateEvent(
                    hGPU,
                    &eventInfo,
                    nullptr,
                    &event)) != VK_SUCCESS) {
        throw DflMem::BufferError::VkBufferEventCreationError;
    }

    return event;
}

static inline VkCommandBuffer INT_GetCmdBuffer(
    const VkDevice&      hGPU,
    const VkCommandPool& hCmdPool,
    const VkSemaphore&   hDoneSemaphore,
    const VkEvent&       hWaitEvent,
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
    VkCommandBuffer cmdBuff{ nullptr };
    VkResult        error{ VK_SUCCESS };
    if ((error = vkAllocateCommandBuffers(
        hGPU,
        &cmdBuffInfo,
        &cmdBuff)) != VK_SUCCESS) {
        throw DflMem::BufferError::VkBufferSemaphoreCreationError;
    }

    const VkCommandBufferBeginInfo cmdBuffBeginInfo{
        .sType{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO },
        .pNext{ nullptr },
        .flags{ },
        .pInheritanceInfo{ }
    };
    if ((error = vkBeginCommandBuffer(
        cmdBuff,
        &cmdBuffBeginInfo)) != VK_SUCCESS) {
        throw DflMem::BufferError::VkBufferCmdRecordingError;
    }
    const uint64_t times{ bufferSize/DflMem::StageMemorySize + 1 };

    return cmdBuff;
}

static inline DflMem::BufferHandles INT_GetBufferHandles(
    const VkDevice&              hGPU,
    const VkCommandPool&         hPool,
    const uint64_t&              size,
    const uint32_t&              flags,
    const std::vector<uint32_t>& areFamiliesUsed,
    const VkBuffer& stageBuffer) {
    const VkBuffer buffer = INT_GetBuffer(
        hGPU,
        size,
        flags,
        areFamiliesUsed);
    
    VkSemaphore sem{ nullptr };
    VkEvent     event{ nullptr };
    try {
        sem = INT_GetSemaphore(hGPU);
        event = INT_GetEvent(hGPU);
    }
    catch (DflMem::BufferError e) {
        vkDeviceWaitIdle(hGPU);
        vkDestroyBuffer(
            hGPU,
            buffer,
            nullptr);
        
        if (sem != nullptr) {
            vkDestroySemaphore(
                hGPU,
                sem,
                nullptr);
        }
    }

    return { buffer, 
                sem, event,
                INT_GetCmdBuffer(
                            hGPU,
                            hPool,
                            sem,
                            event,
                            size,
                            stageBuffer,
                            buffer), nullptr };
}

DflMem::Buffer::Buffer(const BufferInfo& info)
try : pInfo( new DflMem::BufferInfo(info) ),
      Buffers( INT_GetBufferHandles(
                    info.pMemoryBlock->pInfo->pDevice->GPU,
                    info.pMemoryBlock->hCmdPool,
                    info.Size,
                    info.Options.GetValue(),
                    info.pMemoryBlock->pInfo->pDevice->pTracker->AreFamiliesUsed,
                    info.pMemoryBlock->pHandles->hStageBuffer) ) {
    std::optional<std::array<uint64_t, 2>> id;
    if ( (id = this->pInfo->pMemoryBlock->Alloc(this->Buffers.hBuffer)).has_value() ) {
        this->MemoryLayoutID = id.value();
    } else {
        this->Error = BufferError::VkMemoryAllocationError;
    }
}
catch (DflMem::BufferError e) { this->Error = e; }

DflMem::Buffer::~Buffer() {
    vkDeviceWaitIdle(this->pInfo->pMemoryBlock->pInfo->pDevice->GPU);
    this->pInfo->pMemoryBlock->pInfo->pDevice->pUsageMutex->lock();
    if (this->Buffers.hFromStageToMainCmd != nullptr) {
        vkFreeCommandBuffers(
            this->pInfo->pMemoryBlock->pInfo->pDevice->GPU,
            this->pInfo->pMemoryBlock->hCmdPool,
            1,
            &this->Buffers.hFromStageToMainCmd);
    }

    if (this->Buffers.hTransferDoneSem != nullptr) {
        vkDestroySemaphore(
            this->pInfo->pMemoryBlock->pInfo->pDevice->GPU,
            this->Buffers.hTransferDoneSem,
            nullptr);
    }

    if (this->Buffers.hCPUTransferDone != nullptr) {
        vkDestroyEvent(
            this->pInfo->pMemoryBlock->pInfo->pDevice->GPU,
            this->Buffers.hCPUTransferDone,
            nullptr);
    }

    this->pInfo->pMemoryBlock->Free(this->MemoryLayoutID, this->pInfo->Size);

    if (this->Buffers.hBuffer != nullptr) {
        vkDestroyBuffer(
            this->pInfo->pMemoryBlock->pInfo->pDevice->GPU,
            this->Buffers.hBuffer,
            nullptr);
    }
    this->pInfo->pMemoryBlock->pInfo->pDevice->pUsageMutex->unlock();
}