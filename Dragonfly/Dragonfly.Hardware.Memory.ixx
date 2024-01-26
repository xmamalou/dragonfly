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
#include <mutex>

#include <vulkan/vulkan.hpp>

export module Dragonfly.Hardware.Memory;

import Dragonfly.Hardware.Device;
import Dragonfly.Generics;

namespace DflGr = Dfl::Graphics;

namespace Dfl{
    namespace Hardware{
        export struct MemoryInfo {
            const Device*  pDevice{ nullptr };
            const uint64_t Size{ 0 }; // in B
        };

        export enum class [[ nodiscard ]] MemoryError {
            Success                       = 0,
            VkComPoolInitError            = -0x4702,
            VkMemoryMaxAllocsReachedError = -0x4901,
            VkMemoryAllocationError       = -0x4902,
            VkMemoryStageBuffError        = -0x4903,
            VkMemoryMapError              = -0x4904,
            VkMemoryCmdPoolError          = -0x4905
        };

        struct MemoryBlock {
            const VkDeviceMemory       hMemory{ nullptr };

            const VkDeviceMemory       hStageMemory{ nullptr };
            const VkBuffer             hStageBuffer{ nullptr };
            const bool                 IsStageVisible{ false };
            const void*                StageMemoryMap{ nullptr };

            const Dfl::Hardware::Queue TransferQueue{ nullptr };
            const VkCommandPool        hCmdPool{ nullptr };
            const VkCommandBuffer      FromCPUToStageCmd{ nullptr };

            const VkEvent              hStageTransferEvent{ nullptr };
            const uint64_t             Size{ 0 };

                                 operator VkDeviceMemory () const { return this->hMemory; }
        };

        export constexpr uint64_t StageMemorySize = 1000; // in B

        export class Memory {
            const Device*              pGPU{ nullptr };
            const uint32_t             MaxAllocsAllowed{ 0 };

            const MemoryBlock          MemoryPool{ };
                    
                  std::mutex           Mutex;
                  MemoryError          Error{ MemoryError::Success };
        public:
            DFL_API                   DFL_CALL Memory(const MemoryInfo& info);
            DFL_API                   DFL_CALL ~Memory();

                    const MemoryError          GetErrorCode() const noexcept { return this->Error; }
            
            friend class 
                          Buffer;  
        };

        // BUFFERS

        export enum class [[ nodiscard ]] BufferError {
            Success                        = 0,
            VkBufferAllocError             = -0x4A01,
            VkBufferSemaphoreCreationError = -0x4A02,
            VkBufferCmdBufferAllocError    = -0x4A03,
            VkBufferCmdRecordingError      = -0x4A04,
            VkBufferEventCreationError     = -0x4A05
        };

        export enum class BufferUsageOptions {
            VertexBuffer      = 0x80,
            IndirectRendering = 0x100,
        };

        export struct BufferInfo {
            const Memory*                             MemoryBlock{ nullptr };
            const uint64_t                            Size{ 0 };

            const Dfl::BitFlag                        Options{ 0 };
        };

        struct BufferHandles {
            const VkBuffer        hBuffer{ nullptr };
            const VkSemaphore     hTransferDoneSem{ nullptr };
            const VkCommandBuffer hFromStageToMainCmd{ nullptr };
        };

        export class Buffer {
            const std::unique_ptr<const BufferInfo> pInfo{ nullptr };

            const BufferHandles                     Buffers{ };

                  BufferError                       Error{ BufferError::Success };
        public:
            DFL_API                   DFL_CALL Buffer(const BufferInfo& info);
            DFL_API                   DFL_CALL ~Buffer();

                    const BufferError          GetErrorCode() const noexcept { return this->Error; }
        };    
    }
}