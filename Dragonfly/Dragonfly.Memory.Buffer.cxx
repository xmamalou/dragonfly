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

#include <algorithm>
#include <vector>
#include <optional>
#include <thread>
#include <iostream>

#include <vulkan/vulkan.h>

module Dragonfly.Memory.Buffer;

import Debugging;

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

static inline VkFence INT_GetFence(const VkDevice& hGPU) {
    const VkFenceCreateInfo fenceInfo{
        .sType{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ 0 }
    };

    VkFence fence{ nullptr };
    VkResult error{ VK_SUCCESS };
    if ((error = vkCreateFence(
                    hGPU,
                    &fenceInfo,
                    nullptr,
                    &fence)) != VK_SUCCESS) {
        throw DflMem::BufferError::VkBufferFenceCreationError;
    }

    return fence;
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
    const bool&          isStageVisible) {
    VkCommandBufferAllocateInfo cmdInfo{
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
        &cmdInfo,
        &cmdBuff)) != VK_SUCCESS) {
        throw DflMem::BufferError::VkBufferCmdBufferAllocError;
    }

    return cmdBuff;
}

static inline DflMem::BufferHandles INT_GetBufferHandles(
    const VkDevice&              hGPU,
    const VkCommandPool&         hPool,
    const uint64_t&              size,
    const uint32_t&              flags,
    const std::vector<uint32_t>& areFamiliesUsed,
    const VkBuffer&              stageBuffer,
    const bool&                  isStageVisible) {
    const VkBuffer buffer = INT_GetBuffer(
        hGPU,
        size,
        flags,
        areFamiliesUsed);
    
    VkFence fence{ nullptr };
    VkEvent event{ nullptr };
    try {
        fence = INT_GetFence(hGPU);
        event = isStageVisible ? INT_GetEvent(hGPU) : nullptr;
    }
    catch (DflMem::BufferError e) {
        vkDeviceWaitIdle(hGPU);
        vkDestroyBuffer(
            hGPU,
            buffer,
            nullptr);
        
        if (fence != nullptr) {
            vkDestroyFence(
                hGPU,
                fence,
                nullptr);
        }
    }

    auto cmdBuffer = INT_GetCmdBuffer(
                        hGPU,
                        hPool,
                        isStageVisible);

    return { buffer, 
                fence, event,
                cmdBuffer };
}

static inline void INT_RecordWriteCommand(
    const VkCommandBuffer& cmdBuff,
    const VkBuffer&        stageBuff,
    const VkBuffer&        dstBuff,
    const uint64_t         dstOffset,
    const uint64_t         dstSize,
    const void*            pData,
    const uint64_t         size) {
    VkCommandBufferBeginInfo cmdInfo{
        .sType{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO },
        .pNext{ nullptr },
        .flags{ 0 },
        .pInheritanceInfo{ nullptr }
    };
    if (vkBeginCommandBuffer(
            cmdBuff,
            &cmdInfo) != VK_SUCCESS) {
        throw DflMem::BufferError::VkBufferCmdRecordingError;
    }
    
    uint64_t remainingSize{ size };
    uint64_t offset{ 0 };

    while ( remainingSize > 0 && dstOffset + 4*offset < dstSize ) {
        vkCmdUpdateBuffer(
            cmdBuff,
            stageBuff,
            0,
            remainingSize > DflMem::StageMemorySize ? DflMem::StageMemorySize : remainingSize,
            &((uint32_t*)pData)[offset]);

        VkBufferMemoryBarrier buffBarrier{
            .sType{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER },
            .pNext{ nullptr },
            .srcAccessMask{ VK_ACCESS_TRANSFER_WRITE_BIT },
            .dstAccessMask{ VK_ACCESS_TRANSFER_READ_BIT },
            .srcQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
            .dstQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
            .buffer{ stageBuff },
            .offset{ 0 },
            .size{ remainingSize > DflMem::StageMemorySize ? DflMem::StageMemorySize : remainingSize }
        };

        vkCmdPipelineBarrier(
            cmdBuff,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            1,
            &buffBarrier,
            0,
            nullptr);

        VkBufferCopy copyRegion{
            .srcOffset{ 0 },
            .dstOffset{ dstOffset + sizeof(uint32_t)*offset },
            .size{ DflMem::StageMemorySize > dstSize - (dstOffset + sizeof(uint32_t) * offset) ?
                                               dstSize - (dstOffset + sizeof(uint32_t) * offset) 
                                             : DflMem::StageMemorySize }
        };

        vkCmdCopyBuffer(
            cmdBuff,
            stageBuff,
            dstBuff,
            1,
            &copyRegion);

        remainingSize = remainingSize > dstSize - (dstOffset + sizeof(uint32_t) * offset) ?
                            0
                         : (remainingSize > DflMem::StageMemorySize ? 
                                remainingSize - DflMem::StageMemorySize 
                              : 0 );
        offset += copyRegion.size/sizeof(uint32_t);
    }

    if (vkEndCommandBuffer(
            cmdBuff) != VK_SUCCESS) {
        throw DflMem::BufferError::VkBufferCmdRecordingError;
    }
}

DflMem::Buffer::Buffer(const BufferInfo& info)
try : pInfo( new DflMem::BufferInfo(info) ),
      Buffers( INT_GetBufferHandles(
                    info.pMemoryBlock->pInfo->pDevice->GPU,
                    info.pMemoryBlock->hCmdPool,
                    info.Size,
                    info.Options.GetValue(),
                    info.pMemoryBlock->pInfo->pDevice->pTracker->AreFamiliesUsed,
                    info.pMemoryBlock->pHandles->hStageBuffer,
                    info.pMemoryBlock->pHandles->IsStageVisible) ) {
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

    if (this->Buffers.hTransferCmdBuff != nullptr) {
        vkFreeCommandBuffers(
            this->pInfo->pMemoryBlock->pInfo->pDevice->GPU,
            this->pInfo->pMemoryBlock->hCmdPool,
            1,
            &this->Buffers.hTransferCmdBuff);
    }

    if (this->Buffers.hTransferDoneFence != nullptr) {
        vkDestroyFence(
            this->pInfo->pMemoryBlock->pInfo->pDevice->GPU,
            this->Buffers.hTransferDoneFence,
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
}

const DflMem::BufferError DflMem::Buffer::Write(
                                            const void*    pData,
                                            const uint64_t size,
                                            const uint64_t offset) const noexcept {
    INT_RecordWriteCommand(
        this->Buffers.hTransferCmdBuff,
        this->pInfo->pMemoryBlock->pHandles->hStageBuffer,
        this->Buffers.hBuffer,
        offset,
        this->pInfo->Size,
        pData,
        size);

    VkSubmitInfo subInfo{
        .sType{ VK_STRUCTURE_TYPE_SUBMIT_INFO },
        .pNext{ nullptr },
        .waitSemaphoreCount{ 0 },
        .pWaitSemaphores{ nullptr },
        .pWaitDstStageMask{ nullptr },
        .commandBufferCount{ 1 },
        .pCommandBuffers{ &this->Buffers.hTransferCmdBuff },
        .signalSemaphoreCount{ 0 },
        .pSignalSemaphores{ nullptr }
    };

    vkQueueWaitIdle(this->pInfo->pMemoryBlock->TransferQueue);
    if (vkQueueSubmit(
            this->pInfo->pMemoryBlock->TransferQueue,
            1,
            &subInfo,
            this->Buffers.hTransferDoneFence) != VK_SUCCESS) {
        return BufferError::VkBufferWriteError;
    }

    while (vkGetFenceStatus(
            this->pInfo->pMemoryBlock->pInfo->pDevice->GPU,
            this->Buffers.hTransferDoneFence) != VK_SUCCESS) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(10));
    }

    vkResetFences(
        this->pInfo->pMemoryBlock->pInfo->pDevice->GPU,
        1,
        &this->Buffers.hTransferDoneFence);

    vkResetCommandBuffer(
        this->Buffers.hTransferCmdBuff,
        0);
}