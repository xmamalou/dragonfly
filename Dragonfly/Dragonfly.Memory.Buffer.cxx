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

#include "Dragonfly.hxx"

#include <thread>

namespace DflMem = Dfl::Memory;
namespace DflGen = Dfl::Generics;

static inline VkBuffer INT_GetBuffer(
    const VkDevice&              hGPU,
    const uint64_t&              size,
    const uint32_t&              flags,
    const std::vector<uint32_t>& areFamiliesUsed) 
{
    std::vector<uint32_t> usedFamilies{ };
    for (uint32_t i{ 0 }; i < areFamiliesUsed.size(); i++) 
    {
        if (areFamiliesUsed[i]) 
        {
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
    if ( vkCreateBuffer(
            hGPU,
            &bufInfo,
            nullptr,
            &buff) != VK_SUCCESS ) 
    {
        throw Dfl::Error::HandleCreation(
                L"Unable to create handle for buffer",
                L"INT_GetBuffer");
    }

    return buff;
}

static inline VkEvent INT_GetEvent(const VkDevice& hGPU) 
{
    const VkEventCreateInfo eventInfo{
        .sType{ VK_STRUCTURE_TYPE_EVENT_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ 0 }
    };

    VkEvent event{ nullptr };
    if (VkResult error{ vkCreateEvent(
                            hGPU,
                            &eventInfo,
                            nullptr,
                            &event) };
        error != VK_SUCCESS) 
    {
        throw Dfl::Error::HandleCreation(
                L"Unable to get synchronization primitive for buffer",
                L"INT_GetEvent");
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
    if ( vkAllocateCommandBuffers(
            hGPU,
            &cmdInfo,
            &cmdBuff)  != VK_SUCCESS ) 
    {
        throw Dfl::Error::HandleCreation(
                L"Unable to create command buffers for buffer",
                L"INT_GetCmdBuffer");
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
    const bool&                  isStageVisible) 
{
    const VkBuffer buffer{ INT_GetBuffer(
                                hGPU,
                                size,
                                flags,
                                areFamiliesUsed) };
    
    VkEvent event{ nullptr };
    VkCommandBuffer cmdBuffer{ nullptr};
    
    try {
        event = isStageVisible ? INT_GetEvent(hGPU) : nullptr;

        cmdBuffer = INT_GetCmdBuffer(
                        hGPU,
                        hPool,
                        isStageVisible);
    } catch (Dfl::Error::HandleCreation& e) {
        vkDeviceWaitIdle(hGPU);
        
        vkDestroyBuffer(
            hGPU,
            buffer,
            nullptr);

        if (event != nullptr) 
        {
            vkDestroyEvent(
                hGPU,
                event,
                nullptr);
        }

        throw;
    }

    return { buffer, event, cmdBuffer };
}

static inline bool INT_RecordWriteCommand(
    const VkCommandBuffer& cmdBuff,
    const VkBuffer&        stageBuff,
    const VkBuffer&        dstBuff,
    const uint64_t         dstOffset,
    const uint64_t         dstSize,
    const void*            pData,
    const uint64_t         sourceSize,
    const uint64_t         sourceOffset) {
    {
        const VkCommandBufferBeginInfo cmdInfo{
            .sType{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO },
            .pNext{ nullptr },
            .flags{ 0 },
            .pInheritanceInfo{ nullptr }
        };
        if (vkBeginCommandBuffer(
                cmdBuff,
                &cmdInfo) != VK_SUCCESS) {
            return !VK_SUCCESS;
        }
    }
    
    {
        uint64_t remainingSize{ sourceSize };
        uint64_t currentSourceOffset{ sourceOffset };
        uint64_t currentDstOffset{ dstOffset };
        while ( remainingSize > 0 && 
                dstOffset + sizeof(char)*currentDstOffset < dstSize ) {
            vkCmdUpdateBuffer(
                cmdBuff,
                stageBuff,
                0,
                remainingSize > DflMem::Block::StageMemorySize ? DflMem::Block::StageMemorySize : remainingSize,
                &((char*)pData)[currentSourceOffset]);

            currentSourceOffset += (remainingSize > DflMem::Block::StageMemorySize ? DflMem::Block::StageMemorySize : remainingSize)/sizeof(char);

            const VkBufferMemoryBarrier stageBuffBarrier{
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
                &stageBuffBarrier,
                0,
                nullptr);

            const VkBufferCopy copyRegion{
                .srcOffset{ 0 },
                .dstOffset{ dstOffset + sizeof(char)*currentDstOffset },
                .size{ DflMem::Block::StageMemorySize > dstSize - sizeof(char) * currentDstOffset ?
                                                           dstSize - sizeof(char) * currentDstOffset
                                                         : DflMem::Block::StageMemorySize }
            };
            vkCmdCopyBuffer(
                cmdBuff,
                stageBuff,
                dstBuff,
                1,
                &copyRegion);

            remainingSize = remainingSize > dstSize - sizeof(char) * currentDstOffset ?
                                0
                                : (remainingSize > DflMem::Block::StageMemorySize ?
                                    remainingSize - DflMem::Block::StageMemorySize
                                    : 0);
            currentDstOffset += copyRegion.size/sizeof(char);
        }
    }

    if (vkEndCommandBuffer(
            cmdBuff) != VK_SUCCESS) {
        return !VK_SUCCESS;
    }

    return VK_SUCCESS;
}

static inline bool INT_RecordReadCommand(
    const VkCommandBuffer& cmdBuff,
    const VkEvent&         transferEvent,
    const VkBuffer&        stageBuff,
    const VkBuffer&        sourceBuff,
    const uint64_t         sourceSize,
    const uint64_t         sourceOffset) {
    {
        const VkCommandBufferBeginInfo cmdInfo{
            .sType{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO },
            .pNext{ nullptr },
            .flags{ 0 },
            .pInheritanceInfo{ nullptr }
        };
        if (vkBeginCommandBuffer(
                cmdBuff,
                &cmdInfo) != VK_SUCCESS) {
            return !VK_SUCCESS;
        }
    }

    {
        uint64_t currentSourceOffset{ sourceOffset };
        while (sourceOffset + sizeof(char) * currentSourceOffset < sourceSize) {
            const VkBufferMemoryBarrier cpuCopyFromStageBarrier{
                .sType{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER },
                .pNext{ nullptr },
                .srcAccessMask{ VK_ACCESS_HOST_READ_BIT },
                .dstAccessMask{ VK_ACCESS_TRANSFER_WRITE_BIT },
                .srcQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
                .dstQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
                .buffer{ stageBuff },
                .offset{ 0 },
                .size{ DflMem::Block::StageMemorySize }
            };
            vkCmdWaitEvents(
                cmdBuff,
                1,
                &transferEvent,
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                nullptr,
                1,
                &cpuCopyFromStageBarrier,
                0,
                nullptr);

            const VkBufferCopy copyRegion{
                .srcOffset{ currentSourceOffset },
                .dstOffset{ 0 },
                .size{ DflMem::Block::StageMemorySize > sourceSize - sizeof(char) * currentSourceOffset ?
                                                           sourceSize - sizeof(char) * currentSourceOffset
                                                         : DflMem::Block::StageMemorySize }
            };
            vkCmdCopyBuffer(
                cmdBuff,
                sourceBuff,
                stageBuff,
                1,
                &copyRegion);

            currentSourceOffset += copyRegion.size/sizeof(char);

            vkCmdSetEvent(
                cmdBuff,
                transferEvent,
                VK_PIPELINE_STAGE_TRANSFER_BIT);
        }
    }

    if (vkEndCommandBuffer(
            cmdBuff) != VK_SUCCESS) {
        return !VK_SUCCESS;
    }

    return VK_SUCCESS;
}

DflMem::Buffer::Buffer(const Info& info)
: pInfo( new Info(info) ),
  Buffers( INT_GetBufferHandles(
                info.MemoryBlock.GetDevice().GetDevice(),
                info.MemoryBlock.GetCmdPool(),
                info.Size,
                info.Options.GetValue(),
                info.MemoryBlock.GetDevice().GetTracker().AreFamiliesUsed,
                info.MemoryBlock.GetDevice().GetTracker().hStageBuffer,
                info.MemoryBlock.GetDevice().GetTracker().pStageMemoryMap == nullptr ?
                    false :
                    true) ),
  MemoryLayoutID( this->pInfo->MemoryBlock.Alloc(this->Buffers.hBuffer).value() ),
  QueueAvailableFence( this->pInfo->MemoryBlock.GetDevice().GetFence(
                            this->pInfo->MemoryBlock.GetQueue().FamilyIndex,
                            this->pInfo->MemoryBlock.GetQueue().Index)) {}

DflMem::Buffer::~Buffer() {
    vkDeviceWaitIdle(this->pInfo->MemoryBlock.GetDevice().GetDevice());

    if (this->Buffers.hTransferCmdBuff != nullptr) {
        vkFreeCommandBuffers(
            this->pInfo->MemoryBlock.GetDevice().GetDevice(),
            this->pInfo->MemoryBlock.GetCmdPool(),
            1,
            &this->Buffers.hTransferCmdBuff);
    }

    if (this->Buffers.hCPUTransferDone != nullptr) {
        vkDestroyEvent(
            this->pInfo->MemoryBlock.GetDevice().GetDevice(),
            this->Buffers.hCPUTransferDone,
            nullptr);
    }

    this->pInfo->MemoryBlock.Free(this->MemoryLayoutID, this->pInfo->Size);

    if (this->Buffers.hBuffer != nullptr) {
        vkDestroyBuffer(
            this->pInfo->MemoryBlock.GetDevice().GetDevice(),
            this->Buffers.hBuffer,
            nullptr);
    }
}

auto DflMem::Buffer::Write(
                    const void*    pData,
                    const uint64_t sourceSize,
                    const uint64_t sourceOffset,
                    const uint64_t dstOffset) const noexcept 
-> const DflGen::Job<Error>
{
    const VkDevice& device = this->pInfo->MemoryBlock.GetDevice().GetDevice();
    
    if( INT_RecordWriteCommand(
            this->Buffers.hTransferCmdBuff,
            this->pInfo->MemoryBlock.GetDevice().GetTracker().hStageBuffer,
            this->Buffers.hBuffer,
            dstOffset,
            this->pInfo->Size,
            pData,
            sourceSize,
            sourceOffset) != VK_SUCCESS ) {
        co_return Error::RecordError;      
    };

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
            this->pInfo->MemoryBlock.GetQueue(),
            1,
            &subInfo,
            this->QueueAvailableFence) != VK_SUCCESS) {
        co_return Error::WriteError;
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

auto DflMem::Buffer::Read(
                      void*     pDestination,
                const uint64_t& dstSize,
                const uint64_t& sourceOffset) const noexcept 
-> const DflGen::Job<Error>
{
    const DflHW::Device& gpu{ this->pInfo->MemoryBlock.GetDevice() };

    if (!gpu.pTracker->pStageMemoryMap) [[ unlikely ]] 
    {
        co_return Error::UnreadableError;
    }

    if( INT_RecordReadCommand(
            this->Buffers.hTransferCmdBuff,
            this->Buffers.hCPUTransferDone,
            gpu.pTracker->hStageBuffer,
            this->Buffers.hBuffer,
            this->pInfo->Size,
            sourceOffset) != VK_SUCCESS ) {
        co_return Error::RecordError;
    }

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
        gpu.GPU,
        this->QueueAvailableFence);

    vkResetFences(
        gpu.GPU,
        1,
        &this->QueueAvailableFence);

    if (vkQueueSubmit(
            this->pInfo->MemoryBlock.GetQueue(),
            1,
            &subInfo,
            this->QueueAvailableFence) != VK_SUCCESS) 
    {
        co_return Error::ReadError;
    }

    if (vkSetEvent(
            gpu.GPU,
            this->Buffers.hCPUTransferDone) != VK_SUCCESS) 
    {
        co_return Error::EventSetError;
    }

    {
        auto& stageMap{ gpu.pTracker->pStageMemoryMap };
        auto& stageSize{ Dfl::Memory::Block::StageMemorySize };

        uint64_t remainingSize{ dstSize };
        uint64_t currentOffset{ 0 };
        while (remainingSize > 0) 
        {
            while (vkGetEventStatus(
                        gpu.GPU,
                        this->Buffers.hCPUTransferDone) != VK_EVENT_SET) 
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            memcpy_s(
                pDestination,
                remainingSize > stageSize ? stageSize : remainingSize,
                stageMap,
                remainingSize > stageSize ? stageSize : remainingSize);
            if (vkSetEvent(
                    gpu.GPU,
                    this->Buffers.hCPUTransferDone) != VK_SUCCESS) 
            {
                co_return Error::EventSetError;
            }
            remainingSize -= remainingSize > stageSize ? stageSize : remainingSize;
            currentOffset += (remainingSize > stageSize ? stageSize : remainingSize)/sizeof(char);
        }
    }

    co_return Error::Success;
}