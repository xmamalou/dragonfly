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
#include <thread>
#include <coroutine>

#include <vulkan/vulkan.h>

module Dragonfly.Memory.Buffer;

import Dragonfly.Generics;

namespace DflMem = Dfl::Memory;
namespace DflGen = Dfl::Generics;

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
        throw DflMem::Buffer::Error::VkBufferAllocError;
    }

    return buff;
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
        throw DflMem::Buffer::Error::VkBufferEventCreationError;
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
        throw DflMem::Buffer::Error::VkBufferCmdBufferAllocError;
    }

    return cmdBuff;
}

static inline DflMem::Buffer::Handles INT_GetBufferHandles(
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
        event = isStageVisible ? INT_GetEvent(hGPU) : nullptr;
    }
    catch (DflMem::Buffer::Error e) {
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
                event,
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
        throw DflMem::Buffer::Error::VkBufferCmdRecordingError;
    }
    
    uint64_t remainingSize{ size };
    uint64_t offset{ 0 };

    while ( remainingSize > 0 && dstOffset + 4*offset < dstSize ) {
        vkCmdUpdateBuffer(
            cmdBuff,
            stageBuff,
            0,
            remainingSize > DflMem::Block::StageMemorySize ? DflMem::Block::StageMemorySize : remainingSize,
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
            .size{ remainingSize > DflMem::Block::StageMemorySize ? DflMem::Block::StageMemorySize : remainingSize }
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
            .size{ DflMem::Block::StageMemorySize > dstSize - (dstOffset + sizeof(uint32_t) * offset) ?
                                                       dstSize - (dstOffset + sizeof(uint32_t) * offset) 
                                                     : DflMem::Block::StageMemorySize }
        };

        vkCmdCopyBuffer(
            cmdBuff,
            stageBuff,
            dstBuff,
            1,
            &copyRegion);

        remainingSize = remainingSize > dstSize - (dstOffset + sizeof(uint32_t) * offset) ?
                            0
                         : (remainingSize > DflMem::Block::StageMemorySize ?
                                remainingSize - DflMem::Block::StageMemorySize
                              : 0 );
        offset += copyRegion.size/sizeof(uint32_t);
    }

    if (vkEndCommandBuffer(
            cmdBuff) != VK_SUCCESS) {
        throw DflMem::Buffer::Error::VkBufferCmdRecordingError;
    }
}

DflMem::Buffer::Buffer(const Info& info)
try : pInfo( new Info(info) ),
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
        this->ErrorCode = Error::VkMemoryAllocationError;
    }

    VkFence fence{ this->pInfo->pMemoryBlock->pInfo->pDevice->GetFence(
                        this->pInfo->pMemoryBlock->TransferQueue.FamilyIndex,
                        this->pInfo->pMemoryBlock->TransferQueue.Index)};
    if (fence == nullptr) {
        this->ErrorCode = Error::VkBufferFenceCreationError;
    } else {
        this->QueueAvailableFence = fence;
    }
}
catch (Error e) { this->ErrorCode = e; }

DflMem::Buffer::~Buffer() {
    vkDeviceWaitIdle(this->pInfo->pMemoryBlock->pInfo->pDevice->GPU);

    if (this->Buffers.hTransferCmdBuff != nullptr) {
        vkFreeCommandBuffers(
            this->pInfo->pMemoryBlock->pInfo->pDevice->GPU,
            this->pInfo->pMemoryBlock->hCmdPool,
            1,
            &this->Buffers.hTransferCmdBuff);
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

const DflGen::Job<DflMem::Buffer::Error> DflMem::Buffer::Write(
                                                            const void*    pData,
                                                            const uint64_t size,
                                                            const uint64_t offset) const noexcept {
    const VkDevice& device = this->pInfo->pMemoryBlock->pInfo->pDevice->GPU;
    
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

    co_await DflGen::Job<Error>::Awaitable(
                device,
                this->QueueAvailableFence);

    vkResetFences(
        device,
        1,
        &this->QueueAvailableFence);

    if (vkQueueSubmit(
            this->pInfo->pMemoryBlock->TransferQueue,
            1,
            &subInfo,
            this->QueueAvailableFence) != VK_SUCCESS) {
        co_return Error::VkBufferWriteError;
    }

    co_await DflGen::Job<Error>::Awaitable(
        device,
        this->QueueAvailableFence);

    while (vkGetFenceStatus(device, this->QueueAvailableFence) != VK_SUCCESS) { 
        std::this_thread::sleep_for(std::chrono::nanoseconds(10)); 
    }
    vkResetCommandBuffer(
        this->Buffers.hTransferCmdBuff,
        0);

    co_return Error::Success;
}