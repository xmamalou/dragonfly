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

#include <memory>
#include <array>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "Dragonfly.h"

#include "Dragonfly.Generics.hxx"
#include "Dragonfly.Hardware.Device.hxx"

namespace Dfl {
    // Dragonfly.Memory
    namespace Memory {
        class Block;

        enum class StorageType {
            Buffer,
            Image
        };

        // Dragonfly.Memory.Buffer
        template< StorageType type >
        class Buffer;

        template<>
        class Buffer< StorageType::Buffer > {
        public:
            struct Info {
                      Block&                MemoryBlock;

                const std::vector<uint32_t> AccessingQueueFamilies; // Families that will access the buffer, other than the memory block's one 
                const uint64_t              Size{ 0 }; // if type == Buffer, the 2 other dimensions are ignored

                const DflGen::BitFlag       Options{ 0 };
            };

            struct Handles {
                const VkBuffer        hBuffer{ nullptr };

                // CmdBuffers

                const VkEvent         hCPUTransferDone{ nullptr };
                const VkCommandBuffer hTransferCmdBuff{ nullptr };

                operator const VkBuffer() { return this->hBuffer; }
            };

            enum class Error {
                Success = 0,
                WriteError = -1,
                UnreadableError = -2,
                ReadError = -3,
                EventSetError = -4,
                RecordError = -5    
            };

        protected:
            const std::unique_ptr<const Info> pInfo{ nullptr };

            const Handles                     Buffers{ };

            const std::array<uint64_t, 2>     MemoryLayoutID{ 0, 0 };
            const VkFence                     QueueAvailableFence{ nullptr };

            DFL_API
            static inline 
                  bool
            DFL_CALL                          RecordWriteBufferCommand(
                                                const VkCommandBuffer& cmdBuff,
                                                const VkBuffer&        stageBuff,
                                                const VkBuffer&        dstBuff,
                                                const uint64_t         dstOffset,
                                                const uint64_t         dstSize,
                                                const void*            pData,
                                                const uint64_t         sourceSize,
                                                const uint64_t         sourceOffset);

            DFL_API
            static inline 
                  bool
            DFL_CALL                          RecordReadBufferCommand(
                                                const VkCommandBuffer& cmdBuff,
                                                const VkEvent&         transferEvent,
                                                const VkBuffer&        stageBuff,
                                                const VkBuffer&        sourceBuff,
                                                const uint64_t         sourceSize,
                                                const uint64_t         sourceOffset);
        public:
            DFL_API DFL_CALL Buffer(const Info& info);
            DFL_API DFL_CALL ~Buffer();

            template< Dfl::Generics::Complete T >
            const DflGen::Job<Error> Write(
                                        const T&       source,
                                        const uint64_t sourceOffset,
                                        const uint64_t dstOffset) const noexcept;
            template< Dfl::Generics::Complete T > 
            const DflGen::Job<Error> Read(
                                              T&        destination,
                                        const uint64_t& dstOffset,
                                        const uint64_t& sourceOffset) const noexcept;
        };

        template<>
        class Buffer< StorageType::Image > {
        public:
            enum class Aspect
            {
                None    = VK_IMAGE_ASPECT_NONE,
                Colour  = VK_IMAGE_ASPECT_COLOR_BIT, 
                Depth   = VK_IMAGE_ASPECT_DEPTH_BIT,
                Stencil = VK_IMAGE_ASPECT_STENCIL_BIT 
            };

            enum class Filter
            {
                Linear  = VK_FILTER_LINEAR,
                Cubic   = VK_FILTER_CUBIC_IMG,
                Nearest = VK_FILTER_NEAREST
            };

            struct Info {
                      Block&                MemoryBlock;

                const std::vector<uint32_t> AccessingQueues;
                
                const std::array<
                        uint32_t, 3>        Size{ 0, 0, 0 }; // if type == Buffer, the 2 other dimensions are ignored
                const uint32_t              MipLevels{ 1 }; // levels of detail of the image
                const uint32_t              Layers{ 1 }; // array layers of the image
                const uint32_t              Samples{ 1 }; // samples of the image per pixel

                const DflGen::BitFlag Options{ 0 };
            };

            struct Handles {
                const VkImage         hImage{ nullptr };
                      VkImageLayout   Layout{  };

                // CmdBuffers

                const VkEvent         hCPUTransferDone{ nullptr };
                const VkCommandBuffer hTransferCmdBuff{ nullptr };

                operator const VkImage() { return this->hImage; }
            };

            enum class Error {
                Success = 0,
                WriteError = -1,
                UnreadableError = -2,
                ReadError = -3,
                EventSetError = -4,
                RecordError = -5    
            };
        protected:
            const std::unique_ptr<const Info> pInfo{ nullptr };

            const Handles                     Buffers{ };

            const std::array<uint64_t, 2>     MemoryLayoutID{ 0, 0 };
            const VkFence                     QueueAvailableFence{ nullptr };

            DFL_API
            static inline 
                  bool
            DFL_CALL                          RecordWriteImageCommand(
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
                                                const uint64_t                 sourceOffset);
            DFL_API
            static inline 
                  bool
            DFL_CALL                          RecordReadImageCommand(
                                                const VkCommandBuffer&         cmdBuff,
                                                const VkEvent&                 transferEvent,
                                                const VkBuffer&                stageBuff,
                                                const VkBuffer&                intermBuff,
                                                const VkImage&                 sourceImage,
                                                const std::array<
                                                        uint32_t, 3>&          sourceSize,
                                                const std::array<
                                                         int32_t, 3>&          sourceOffset,
                                                const Dfl::Generics::BitFlag&  sourceAspectFlag,
                                                const uint32_t                 sourceArrayLayer);
        public:
            DFL_API DFL_CALL Buffer(const Info& info);
            DFL_API DFL_CALL ~Buffer();

            template< Dfl::Generics::Complete T >
            const DflGen::Job<Error> Write(
                                        const T&                       source,
                                        const uint64_t                 sourceOffset,
                                        const std::array<uint64_t, 3>& dstOffset,
                                        const Dfl::Generics::BitFlag&  dstAspectFlag,
                                        const Filter                   dstFilter) const noexcept;
            template< Dfl::Generics::Complete T > 
            const DflGen::Job<Error> Read(
                                              T&           destination,
                                        const uint64_t     dstOffset,
                                        const std::vector<
                                                uint64_t>& sourceOffset) const noexcept;
        };

        using GenericBuffer = Dfl::Memory::Buffer<Dfl::Memory::StorageType::Buffer>;
    }
    namespace DflMem = Dfl::Memory;
}

// TEMPLATE DEFINITIONS

// Dragonfly.Memory.Buffer

template< Dfl::Generics::Complete T > 
auto Dfl::Memory::Buffer< Dfl::Memory::StorageType::Buffer >::Write(
                    const T&       source,
                    const uint64_t sourceOffset,
                    const uint64_t dstOffset) const noexcept 
-> const DflGen::Job<Error>
{
    const VkDevice& device = this->pInfo->MemoryBlock.GetDevice().GetDevice();
    
    if( this->RecordWriteBufferCommand(
            this->Buffers.hTransferCmdBuff,
            this->pInfo->MemoryBlock.GetDevice().GetStageBuffer(),
            this->Buffers.hBuffer,
            dstOffset,
            this->pInfo->Size,
            &source,
            sizeof(T),
            sourceOffset) != VK_SUCCESS ) 
    {
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
            this->QueueAvailableFence) != VK_SUCCESS) 
    {
        co_return Error::WriteError;
    }

    co_await DflGen::Job<Error>::Awaitable(
        device,
        this->QueueAvailableFence);

    while (vkGetFenceStatus(device, this->QueueAvailableFence) != VK_SUCCESS) 
    { 
        std::this_thread::sleep_for(std::chrono::nanoseconds(10)); 
    }
    vkResetCommandBuffer(
        this->Buffers.hTransferCmdBuff,
        0);

    co_return Error::Success;
}

template< Dfl::Generics::Complete T >
auto Dfl::Memory::Buffer< Dfl::Memory::StorageType::Buffer >::Read(
                      T&        destination,
                const uint64_t& dstOffset,
                const uint64_t& sourceOffset) const noexcept 
-> const DflGen::Job<Error>
{
    const DflHW::Device& gpu{ this->pInfo->MemoryBlock.GetDevice() };

    if (gpu.GetStageMap() == nullptr) [[ unlikely ]] 
    {
        co_return Error::UnreadableError;
    }

    if( this->RecordReadBufferCommand(
            this->Buffers.hTransferCmdBuff,
            this->Buffers.hCPUTransferDone,
            gpu.GetStageBuffer(),
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
        gpu.GetDevice(),
        this->QueueAvailableFence);

    vkResetFences(
        gpu.GetDevice(),
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
            gpu.GetDevice(),
            this->Buffers.hCPUTransferDone) != VK_SUCCESS) 
    {
        co_return Error::EventSetError;
    }

    {
        auto& stageSize{ DflHW::Device::StageMemory };

        uint64_t remainingSize{ sizeof(T) };
        uint64_t currentOffset{ dstOffset };
        while (remainingSize > 0
               && currentOffset < sizeof(T) ) 
        {
            while (vkGetEventStatus(
                        gpu.GetDevice(),
                        this->Buffers.hCPUTransferDone) != VK_EVENT_SET) 
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            memcpy_s(
                (char*)&destination + currentOffset,
                remainingSize > stageSize ? stageSize : remainingSize,
                gpu.GetStageMap(),
                remainingSize > stageSize ? stageSize : remainingSize);
            if (vkSetEvent(
                    gpu.GetDevice(),
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

template< Dfl::Generics::Complete T > 
auto Dfl::Memory::Buffer< Dfl::Memory::StorageType::Image >::Write(
                    const T&                       source,
                    const uint64_t                 sourceOffset,
                    const std::array<uint64_t, 3>& dstOffset,
                    const Dfl::Generics::BitFlag&  dstAspectFlag,
                    const Filter                   dstFilter) const noexcept 
-> const DflGen::Job<Error>
{
    const VkDevice& device = this->pInfo->MemoryBlock.GetDevice().GetDevice();
    
    if( this->RecordWriteImageCommand(
            this->Buffers.hTransferCmdBuff,
            this->pInfo->MemoryBlock.GetDevice().GetStageBuffer(),
            this->Buffers.hImage,
            dstAspectFlag,
            dstOffset,
            this->pInfo->Size,
            &source,
            sizeof(T),
            sourceOffset) != VK_SUCCESS ) 
    {
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
            this->QueueAvailableFence) != VK_SUCCESS) 
    {
        co_return Error::WriteError;
    }

    co_await DflGen::Job<Error>::Awaitable(
        device,
        this->QueueAvailableFence);

    while (vkGetFenceStatus(device, this->QueueAvailableFence) != VK_SUCCESS) 
    { 
        std::this_thread::sleep_for(std::chrono::nanoseconds(10)); 
    }
    vkResetCommandBuffer(
        this->Buffers.hTransferCmdBuff,
        0);

    this->Buffers.Layout = dstLayout;
    co_return Error::Success;
}

template< Dfl::Generics::Complete T >
auto Dfl::Memory::Buffer< Dfl::Memory::StorageType::Image >::Read(
                      T&           destination,
                const uint64_t     dstOffset,
                const std::vector<
                        uint64_t>& sourceOffset) const noexcept 
-> const DflGen::Job<Error>
{
    const DflHW::Device& gpu{ this->pInfo->MemoryBlock.GetDevice() };

    if (gpu.GetStageMap() == nullptr) [[ unlikely ]] 
    {
        co_return Error::UnreadableError;
    }

    if( this->RecordReadImageCommand(
            this->Buffers.hTransferCmdBuff,
            this->Buffers.hCPUTransferDone,
            gpu.GetStageBuffer(),
            this->Buffers.hImage,
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
        gpu.GetDevice(),
        this->QueueAvailableFence);

    vkResetFences(
        gpu.GetDevice(),
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
            gpu.GetDevice(),
            this->Buffers.hCPUTransferDone) != VK_SUCCESS) 
    {
        co_return Error::EventSetError;
    }

    {
        auto& stageSize{ DflHW::Device::StageMemory };

        uint64_t remainingSize{ sizeof(T) };
        uint64_t currentOffset{ dstOffset };
        while (remainingSize > 0
                && currentOffset < sizeof(T) ) 
        {
            while (vkGetEventStatus(
                        gpu.GetDevice(),
                        this->Buffers.hCPUTransferDone) != VK_EVENT_SET) 
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            memcpy_s(
                (char*)&destination + currentOffset,
                remainingSize > stageSize ? stageSize : remainingSize,
                gpu.GetStageMap(),
                remainingSize > stageSize ? stageSize : remainingSize);
            if (vkSetEvent(
                    gpu.GetDevice(),
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