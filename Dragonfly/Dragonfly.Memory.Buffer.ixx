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

#include <memory>
#include <array>

#include <vulkan/vulkan.h>

export module Dragonfly.Memory.Buffer;

import Dragonfly.Generics;
import Dragonfly.Memory.Block;

namespace DflGen = Dfl::Generics;

namespace Dfl {
    namespace Memory {
        export class Buffer {
        public:
            enum class [[ nodiscard ]] Error {
                Success = 0,
                VkMemoryAllocationError = -0x4902,
                VkBufferAllocError = -0x4A01,
                VkBufferFenceCreationError = -0x4A02,
                VkBufferCmdBufferAllocError = -0x4A03,
                VkBufferCmdRecordingError = -0x4A04,
                VkBufferEventCreationError = -0x4A05,
                VkBufferWriteError = -0x4A06,
            };

            enum class UsageOptions {
                VertexBuffer = 0x80,
                IndirectRendering = 0x100,
            };

            struct Info {
                Block* pMemoryBlock{ nullptr };
                const uint64_t                            Size{ 0 };

                const DflGen::BitFlag                     Options{ 0 };
            };

            struct Handles {
                const VkBuffer        hBuffer{ nullptr };

                // CmdBuffers

                const VkEvent         hCPUTransferDone{ nullptr };
                const VkCommandBuffer hTransferCmdBuff{ nullptr };

                operator const VkBuffer() { return this->hBuffer; }
            };
        
        private:
            const std::unique_ptr<const Info> pInfo{ nullptr };

            const Handles                     Buffers{ };

                  std::array<uint64_t, 2>     MemoryLayoutID{ 0, 0 };
                  VkFence                     QueueAvailableFence{ nullptr };

                  Error                       ErrorCode{ Error::Success };
        
        public:
            DFL_API                          DFL_CALL Buffer(const Info& info);
            DFL_API                          DFL_CALL ~Buffer();

            DFL_API const DflGen::Job<Error> DFL_CALL Write(
                                                        const void*    pData,
                                                        const uint64_t size,
                                                        const uint64_t offset) const noexcept;

                    const Error                       GetErrorCode() const noexcept { return this->ErrorCode; }
        };
    }
}