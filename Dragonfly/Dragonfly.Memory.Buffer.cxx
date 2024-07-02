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

#include "Dragonfly.Memory.Buffer.hxx"
#include <vector>

#include "Dragonfly.Memory.Block.hxx"

namespace DflMem = Dfl::Memory;
namespace DflGen = Dfl::Generics;

static inline VkBuffer INT_GetBuffer(
    const VkDevice&              hGPU,
    const uint32_t               transferFamilyIndex,
    const std::vector<uint32_t>& familyIndices,
    const uint64_t&              size,
    const uint32_t&              flags) 
{
    std::vector<uint32_t> indices{ familyIndices };
    if (familyIndices.empty() 
        || (std::find(familyIndices.begin(), familyIndices.end() + 1,
            transferFamilyIndex) == familyIndices.end() + 1) )
    {
        indices.push_back(transferFamilyIndex);
    }

    const VkBufferCreateInfo bufInfo{
        .sType{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ 0 },
        .size{ size },
        .usage{ VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                flags },
        .sharingMode{ indices.size() == 1
                       ? VK_SHARING_MODE_EXCLUSIVE 
                       : VK_SHARING_MODE_CONCURRENT },
        .queueFamilyIndexCount{ static_cast<uint32_t>(indices.size()) },
        .pQueueFamilyIndices{ indices.data() }
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

static inline VkImage INT_GetImage(
    const VkDevice&              hGPU,
    const uint32_t               transferFamilyIndex,
    const std::vector<uint32_t>& familyIndices,
    const std::array<
            uint32_t, 3>&        size,
    const uint32_t&              flags,
    const uint32_t               mipLevels,
    const uint32_t               layers,
    const uint32_t               samples) 
{
    std::vector<uint32_t> indices{ familyIndices };
    if (familyIndices.empty() 
        || (std::find(familyIndices.begin(), familyIndices.end() + 1,
            transferFamilyIndex) == familyIndices.end() + 1))
    {
        indices.push_back(transferFamilyIndex);
    }

    const VkImageCreateInfo imgInfo{
        .sType{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ 0 },
        .imageType{ size[1] == 0 
                    ? VK_IMAGE_TYPE_1D
                    : ( size[2] == 0
                        ? VK_IMAGE_TYPE_2D
                        : VK_IMAGE_TYPE_3D )},
        .format{ VK_FORMAT_R8G8B8A8_SNORM },
        .extent{ VkExtent3D{ 
                    .width{ size[0] },
                    .height{ size[1] },
                    .depth{ size[2] } } },
        .mipLevels{ mipLevels },
        .arrayLayers{ layers },
        .samples{ static_cast<VkSampleCountFlagBits>(samples) },
        .tiling{ VK_IMAGE_TILING_OPTIMAL },
        .usage{ VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                flags },
        .sharingMode{ familyIndices.empty()
                      ? VK_SHARING_MODE_EXCLUSIVE 
                      : VK_SHARING_MODE_CONCURRENT },
        .queueFamilyIndexCount{ static_cast<uint32_t>(familyIndices.size()) },
        .pQueueFamilyIndices{ familyIndices.data() },
        .initialLayout{ VK_IMAGE_LAYOUT_UNDEFINED }
    };

    VkImage img{ nullptr };
    if ( vkCreateImage(
            hGPU,
            &imgInfo,
            nullptr,
            &img) != VK_SUCCESS ) 
    {
        throw Dfl::Error::HandleCreation(
                L"Unable to create handle for image",
                L"INT_GetImage");
    }

    return img;
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

static inline auto INT_GetBufferHandles(
    const Dfl::Hardware::Device& gpu,
    const uint32_t               transferFamilyIndex,
    const VkCommandPool&         hPool,
    const std::vector<uint32_t>  familyIndices,
    const uint64_t&              size,
    const uint32_t&              flags,
    const VkBuffer&              stageBuffer,
    const bool&                  isStageVisible) 
-> DflMem::Buffer< DflMem::StorageType::Buffer >::Handles
{
    const VkBuffer buffer{ INT_GetBuffer(
                                gpu.GetDevice(),
                                transferFamilyIndex,
                                familyIndices,
                                size,
                                flags) };
    
    VkEvent event{ nullptr };
    VkCommandBuffer cmdBuffer{ nullptr};
    
    try {
        event = isStageVisible ? INT_GetEvent(gpu.GetDevice()) : nullptr;

        cmdBuffer = INT_GetCmdBuffer(
                        gpu.GetDevice(),
                        hPool,
                        isStageVisible);
    } catch (Dfl::Error::HandleCreation& e) {
        vkDeviceWaitIdle(gpu.GetDevice());
        
        vkDestroyBuffer(
            gpu.GetDevice(),
            buffer,
            nullptr);

        if (event != nullptr) 
        {
            vkDestroyEvent(
                gpu.GetDevice(),
                event,
                nullptr);
        }

        throw;
    }

    return { buffer,  
             event, cmdBuffer };
}

static inline auto INT_GetImageHandles(
    const Dfl::Hardware::Device& gpu,
    const uint32_t               transferFamilyIndex,
    const std::vector<uint32_t>  familyIndices,
    const VkCommandPool&         hPool,
    const std::array<
            uint32_t, 3>&        size,
    const uint32_t&              mipLevels,
    const uint32_t&              layers,
    const uint32_t&              samples,
    const uint32_t&              flags,
    const VkBuffer&              stageBuffer,
    const bool&                  isStageVisible)
-> DflMem::Buffer< DflMem::StorageType::Image >::Handles
{
    const VkImage image{ INT_GetImage(
                                gpu.GetDevice(),
                                transferFamilyIndex,
                                familyIndices,
                                size,
                                flags,
                                mipLevels,
                                layers,
                                samples) };
    
    VkEvent event{ nullptr };
    VkCommandBuffer cmdBuffer{ nullptr};
    
    try {
        event = isStageVisible ? INT_GetEvent(gpu.GetDevice()) : nullptr;

        cmdBuffer = INT_GetCmdBuffer(
                        gpu.GetDevice(),
                        hPool,
                        isStageVisible);
    } catch (Dfl::Error::HandleCreation& e) {
        vkDeviceWaitIdle(gpu.GetDevice());
        
        vkDestroyImage(
            gpu.GetDevice(),
            image,
            nullptr);

        if (event != nullptr) 
        {
            vkDestroyEvent(
                gpu.GetDevice(),
                event,
                nullptr);
        }

        throw;
    }

    return { image, VK_IMAGE_LAYOUT_UNDEFINED,  
             event, cmdBuffer };
}

inline bool DflMem::Buffer< DflMem::StorageType::Buffer >::RecordWriteBufferCommand(
    const VkCommandBuffer& cmdBuff,
    const VkBuffer&        stageBuff,
    const VkBuffer&        dstBuff,
    const uint64_t         dstOffset,
    const uint64_t         dstSize,
    const void*            pData,
    const uint64_t         sourceSize,
    const uint64_t         sourceOffset)
{
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
        while ( remainingSize > 0
                && currentDstOffset < dstSize ) 
        {
            vkCmdUpdateBuffer(
                cmdBuff,
                stageBuff,
                0,
                remainingSize > DflHW::Device::StageMemory 
                    ? DflHW::Device::StageMemory 
                    : remainingSize,
                &((char*)pData)[currentSourceOffset]);

            currentSourceOffset += (remainingSize > DflHW::Device::StageMemory 
                                        ? DflHW::Device::StageMemory 
                                        : remainingSize)/sizeof(char);

            const VkBufferMemoryBarrier stageBuffBarrier{
                .sType{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER },
                .pNext{ nullptr },
                .srcAccessMask{ VK_ACCESS_TRANSFER_WRITE_BIT },
                .dstAccessMask{ VK_ACCESS_TRANSFER_READ_BIT },
                .srcQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
                .dstQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
                .buffer{ stageBuff },
                .offset{ 0 },
                .size{ remainingSize > DflHW::Device::StageMemory 
                            ? DflHW::Device::StageMemory
                            : remainingSize }
            };
            vkCmdPipelineBarrier(
                cmdBuff,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                1, &stageBuffBarrier,
                0, nullptr);

            const VkBufferCopy copyRegion{
                .srcOffset{ 0 },
                .dstOffset{ sizeof(char)*currentDstOffset },
                .size{ DflHW::Device::StageMemory > dstSize - sizeof(char) * currentDstOffset
                        ? dstSize - sizeof(char) * currentDstOffset
                        : DflHW::Device::StageMemory }
            };
            vkCmdCopyBuffer(
                cmdBuff,
                stageBuff,
                dstBuff,
                1,
                &copyRegion);

            currentDstOffset += copyRegion.size/sizeof(char);
            remainingSize -= remainingSize > DflHW::Device::StageMemory
                                ? DflHW::Device::StageMemory
                                : remainingSize;
        }
    }

    if (vkEndCommandBuffer(
            cmdBuff) != VK_SUCCESS) {
        return !VK_SUCCESS;
    }

    return VK_SUCCESS;
}

inline bool DflMem::Buffer< DflMem::StorageType::Buffer >::RecordReadBufferCommand(
    const VkCommandBuffer& cmdBuff,
    const VkEvent&         transferEvent,
    const VkBuffer&        stageBuff,
    const VkBuffer&        sourceBuff,
    const uint64_t         sourceSize,
    const uint64_t         sourceOffset)
{
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
        while ( sizeof(char) * currentSourceOffset < sourceSize ) {
            const VkBufferMemoryBarrier cpuCopyFromStageBarrier{
                .sType{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER },
                .pNext{ nullptr },
                .srcAccessMask{ VK_ACCESS_HOST_READ_BIT },
                .dstAccessMask{ VK_ACCESS_TRANSFER_WRITE_BIT },
                .srcQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
                .dstQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
                .buffer{ stageBuff },
                .offset{ 0 },
                .size{  DflHW::Device::StageMemory }
            };
            vkCmdWaitEvents(
                cmdBuff,
                1,
                &transferEvent,
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0, nullptr,
                1, &cpuCopyFromStageBarrier,
                0, nullptr);

            const VkBufferCopy copyRegion{
                .srcOffset{ currentSourceOffset },
                .dstOffset{ 0 },
                .size{  DflHW::Device::StageMemory > sourceSize - sizeof(char) * currentSourceOffset
                        ? sourceSize - sizeof(char) * currentSourceOffset
                        : DflHW::Device::StageMemory }
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

DflMem::Buffer< DflMem::StorageType::Buffer >::Buffer(const Info& info)
: pInfo( new Info(info) ),
  Buffers( INT_GetBufferHandles(
                info.MemoryBlock.GetDevice(),
                info.MemoryBlock.GetQueue().FamilyIndex,
                info.MemoryBlock.GetCmdPool(),
                info.AccessingQueueFamilies,
                info.Size,
                info.Options.GetValue(),
                info.MemoryBlock.GetDevice().GetStageBuffer(),
                info.MemoryBlock.GetDevice().GetStageMap() == nullptr ?
                    false :
                    true) ),
  MemoryLayoutID( this->pInfo->MemoryBlock.Alloc(this->Buffers.hBuffer).value() ),
  QueueAvailableFence( this->pInfo->MemoryBlock.GetDevice().GetFence(
                            this->pInfo->MemoryBlock.GetQueue().FamilyIndex,
                            this->pInfo->MemoryBlock.GetQueue().Index)) {}

DflMem::Buffer< DflMem::StorageType::Buffer >::~Buffer() {
    vkDeviceWaitIdle(this->pInfo->MemoryBlock.GetDevice().GetDevice());

    vkFreeCommandBuffers(
        this->pInfo->MemoryBlock.GetDevice().GetDevice(),
        this->pInfo->MemoryBlock.GetCmdPool(),
        1,
        &this->Buffers.hTransferCmdBuff);

    vkDestroyEvent(
        this->pInfo->MemoryBlock.GetDevice().GetDevice(),
        this->Buffers.hCPUTransferDone,
        nullptr);

    this->pInfo->MemoryBlock.Free(this->MemoryLayoutID, this->Buffers.hBuffer);

    vkDestroyBuffer(
        this->pInfo->MemoryBlock.GetDevice().GetDevice(),
        this->Buffers.hBuffer,
        nullptr);
}

inline bool DflMem::Buffer< DflMem::StorageType::Image >::RecordWriteImageCommand(
    const VkCommandBuffer&         cmdBuff,
    const VkBuffer&                stageBuff,
    const VkBuffer&                intermBuff,
    const VkImage&                 dstImage,
    const uint32_t                 dstMipLevels,
    const Dfl::Generics::BitFlag&  dstAspectFlag,
    const uint32_t                 dstArrayLayer,
    const std::array<int32_t, 3>&  dstOffset,
    const std::array<uint32_t, 3>& dstSize,
    const VkFilter                 dstFilter,
    const void*                    pData,
    const uint64_t                 sourceSize,
    const uint64_t                 sourceOffset)
{
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
    
    // first, we copy to the intermediate buffer
    // and then we copy to the image
    {
        uint64_t remainingSize{ dstSize[0] *
                                (dstSize[1] == 0 ? 1 : dstSize[1]) *
                                (dstSize[2] == 0 ? 1 : dstSize[2]) };
        uint64_t currentSourceOffset{ sourceOffset };
        uint64_t currentOffset{ 0 };
        while ( remainingSize > 0 
                && sizeof(char) * currentOffset < Dfl::Hardware::Device::IntermediateMemory)
        {
            vkCmdUpdateBuffer(
                cmdBuff,
                stageBuff,
                0,
                remainingSize > DflHW::Device::StageMemory 
                    ? DflHW::Device::StageMemory 
                    : remainingSize,
                &((char*)pData)[currentSourceOffset]);

            currentSourceOffset += (remainingSize > DflHW::Device::StageMemory 
                                        ? DflHW::Device::StageMemory 
                                        : remainingSize)/sizeof(char);

            const VkBufferMemoryBarrier stageBuffBarrier{
                .sType{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER },
                .pNext{ nullptr },
                .srcAccessMask{ VK_ACCESS_TRANSFER_WRITE_BIT },
                .dstAccessMask{ VK_ACCESS_TRANSFER_READ_BIT },
                .srcQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
                .dstQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
                .buffer{ stageBuff },
                .offset{ 0 },
                .size{ remainingSize > DflHW::Device::StageMemory 
                            ? DflHW::Device::StageMemory
                            : remainingSize }
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
                .dstOffset{ sizeof(char)*currentOffset },
                .size{ DflHW::Device::StageMemory > DflHW::Device::IntermediateMemory - sizeof(char) * currentOffset
                        ? DflHW::Device::IntermediateMemory - sizeof(char) * currentOffset
                        : DflHW::Device::StageMemory }
            };
            vkCmdCopyBuffer(
                cmdBuff,
                stageBuff,
                intermBuff,
                1,
                &copyRegion);

            remainingSize -= remainingSize > DflHW::Device::StageMemory 
                                    ? DflHW::Device::StageMemory
                                    : remainingSize;
            currentOffset += copyRegion.size/sizeof(char);
        }

        const VkBufferMemoryBarrier intermBuffBarrier{
                .sType{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER },
                .pNext{ nullptr },
                .srcAccessMask{ VK_ACCESS_TRANSFER_WRITE_BIT },
                .dstAccessMask{ VK_ACCESS_TRANSFER_READ_BIT },
                .srcQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
                .dstQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
                .buffer{ intermBuff },
                .offset{ 0 },
                .size{ dstSize[0] *
                       (dstSize[1] == 0 ? 1 : dstSize[1]) *
                       (dstSize[2] == 0 ? 1 : dstSize[2]) }
        };
        vkCmdPipelineBarrier(
            cmdBuff,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0,
            nullptr,
            1,
            &intermBuffBarrier,
            0,
            nullptr);

        const VkBufferImageCopy copyRegion{
            .bufferRowLength{ 0 },
            .bufferImageHeight{ 0 },
            .bufferOffset{ 0 },
            .imageSubresource{
                .aspectMask{ dstAspectFlag.GetValue() },
                .mipLevel{ 0 },
                .baseArrayLayer{ dstArrayLayer },
                .layerCount{ 1 } },
            .imageOffset{ VkOffset3D{ dstOffset[0], dstOffset[1], dstOffset[2] } },
            .imageExtent{ VkExtent3D{ dstSize[0], dstSize[1], dstSize[2] } }
        };
        vkCmdCopyBufferToImage(
            cmdBuff,
            intermBuff,
            dstImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &copyRegion);
    }

    // we then create mips of the copied data
    for( uint32_t i{ 1 }; i < dstMipLevels; i++)
    { 
        const VkImageMemoryBarrier imageBarrier{
            .sType{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER },
            .pNext{ nullptr },
            .srcAccessMask{ VK_ACCESS_TRANSFER_WRITE_BIT },
            .dstAccessMask{ VK_ACCESS_TRANSFER_READ_BIT },
            .oldLayout{ VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL },
            .newLayout{ VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL },
            .srcQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
            .dstQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
            .image{ dstImage },
            .subresourceRange{
                .aspectMask{ dstAspectFlag.GetValue() },
                .baseMipLevel{ i - 1 },
                .levelCount{ 1 },
                .baseArrayLayer{ dstArrayLayer },
                .layerCount{ 1 } },
        };
        vkCmdPipelineBarrier(
            cmdBuff,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imageBarrier );
            
        const VkImageBlit imgBlit{
            .srcSubresource{
                .aspectMask{ dstAspectFlag.GetValue() },
                .mipLevel{ i - 1 },
                .baseArrayLayer{ dstArrayLayer },
                .layerCount{ 1 }},
            .srcOffsets{
                VkOffset3D{ 0, 0, 0 },
                VkOffset3D{ static_cast<int>(dstSize[0] >> (i - 1)),
                            dstSize[1] == 0 ? 0 : static_cast<int>(dstSize[1] >> (i - 1)),
                            dstSize[2] == 0 ? 0 : static_cast<int>(dstSize[2] >> (i - 1)) }},
            .dstSubresource{
                .aspectMask{ dstAspectFlag.GetValue() },
                .mipLevel{ i },
                .baseArrayLayer{ dstArrayLayer },
                .layerCount{ 1 }},
            .dstOffsets{
                VkOffset3D{ 0, 0, 0 },
                VkOffset3D{ static_cast<int>(dstSize[0] >> i),
                            dstSize[1] == 0 ? 0 : static_cast<int>(dstSize[1] >> i),
                            dstSize[2] == 0 ? 0 : static_cast<int>(dstSize[2] >> i) } }};
        vkCmdBlitImage(
            cmdBuff,
            dstImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &imgBlit,
            dstFilter);
    }

    if (vkEndCommandBuffer(
            cmdBuff) != VK_SUCCESS) 
    {
        return !VK_SUCCESS;
    }

    return VK_SUCCESS;
}

inline bool DflMem::Buffer< DflMem::StorageType::Image >::RecordReadImageCommand(
    const VkCommandBuffer&         cmdBuff,
    const VkEvent&                 transferEvent,
    const VkBuffer&                stageBuff,
    const VkBuffer&                intermBuff,
    const VkImage&                 sourceImage,
    const std::array<
            uint32_t, 3>&          sourceSize,
    const std::array<
            int32_t, 3>&           sourceOffset,
    const Dfl::Generics::BitFlag&  sourceAspectFlag,
    const uint32_t                 sourceArrayLayer)
{
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
        const VkBufferImageCopy copyRegion{
            .bufferRowLength{ 0 },
            .bufferImageHeight{ 0 },
            .bufferOffset{ 0 },
            .imageSubresource{
                .aspectMask{ sourceAspectFlag.GetValue() },
                .mipLevel{ 0 },
                .baseArrayLayer{ sourceArrayLayer },
                .layerCount{ 1 } },
            .imageOffset{ VkOffset3D{ sourceOffset[0], sourceOffset[1], sourceOffset[2] } },
            .imageExtent{ VkExtent3D{ sourceSize[0], sourceSize[1], sourceSize[2] } }
        };
        vkCmdCopyImageToBuffer(
            cmdBuff,
            sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            intermBuff,
            1, &copyRegion);

        const VkBufferMemoryBarrier intermBuffBarrier{
                .sType{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER },
                .pNext{ nullptr },
                .srcAccessMask{ VK_ACCESS_TRANSFER_WRITE_BIT },
                .dstAccessMask{ VK_ACCESS_TRANSFER_READ_BIT },
                .srcQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
                .dstQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
                .buffer{ intermBuff },
                .offset{ 0 },
                .size{ sourceSize[0] *
                       (sourceSize[1] == 0 ? 1 : sourceSize[1]) *
                       (sourceSize[2] == 0 ? 1 : sourceSize[2]) }
        };
        vkCmdPipelineBarrier(
            cmdBuff,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            1, &intermBuffBarrier,
            0, nullptr);

        uint32_t currentOffset{ 0 };
        while (sizeof(char) * currentOffset < sourceSize[0] *
                                               (sourceSize[1] == 0 ? 1 : sourceSize[1]) *
                                               (sourceSize[2] == 0 ? 1 : sourceSize[2])) {
            const VkBufferMemoryBarrier cpuCopyFromStageBarrier{
                .sType{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER },
                .pNext{ nullptr },
                .srcAccessMask{ VK_ACCESS_HOST_READ_BIT },
                .dstAccessMask{ VK_ACCESS_TRANSFER_WRITE_BIT },
                .srcQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
                .dstQueueFamilyIndex{ VK_QUEUE_FAMILY_IGNORED },
                .buffer{ stageBuff },
                .offset{ 0 },
                .size{  DflHW::Device::StageMemory }
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
                .srcOffset{ currentOffset },
                .dstOffset{ 0 },
                .size{  DflHW::Device::StageMemory > DflHW::Device::IntermediateMemory - sizeof(char) * currentOffset
                        ? DflHW::Device::IntermediateMemory - sizeof(char) * currentOffset
                        : DflHW::Device::StageMemory }
            };
            vkCmdCopyBuffer(
                cmdBuff,
                intermBuff,
                stageBuff,
                1,
                &copyRegion);

            currentOffset += copyRegion.size/sizeof(char);

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

DflMem::Buffer< DflMem::StorageType::Image >::Buffer(const Info& info)
: pInfo( new Info(info) ),
  Buffers( INT_GetImageHandles(
                info.MemoryBlock.GetDevice(),
                info.MemoryBlock.GetQueue().FamilyIndex,
                info.AccessingQueues,
                info.MemoryBlock.GetCmdPool(),
                info.Size,
                info.MipLevels,
                info.Layers,
                info.Samples,
                info.Options.GetValue(),
                info.MemoryBlock.GetDevice().GetStageBuffer(),
                info.MemoryBlock.GetDevice().GetStageMap() == nullptr ?
                    false :
                    true) ),
  MemoryLayoutID( this->pInfo->MemoryBlock.Alloc(this->Buffers.hImage).value() ),
  QueueAvailableFence( this->pInfo->MemoryBlock.GetDevice().GetFence(
                            this->pInfo->MemoryBlock.GetQueue().FamilyIndex,
                            this->pInfo->MemoryBlock.GetQueue().Index)) {}

DflMem::Buffer< DflMem::StorageType::Image >::~Buffer() {
    vkDeviceWaitIdle(this->pInfo->MemoryBlock.GetDevice().GetDevice());

    vkFreeCommandBuffers(
        this->pInfo->MemoryBlock.GetDevice().GetDevice(),
        this->pInfo->MemoryBlock.GetCmdPool(),
        1,
        &this->Buffers.hTransferCmdBuff);

    vkDestroyEvent(
        this->pInfo->MemoryBlock.GetDevice().GetDevice(),
        this->Buffers.hCPUTransferDone,
        nullptr);

    this->pInfo->MemoryBlock.Free(this->MemoryLayoutID, this->Buffers.hImage);

    vkDestroyImage(
        this->pInfo->MemoryBlock.GetDevice().GetDevice(),
        this->Buffers.hImage,
        nullptr);
}