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

#include <mutex>
#include <optional>
#include <memory>

#include <vulkan/vulkan.h>

export module Dragonfly.Memory.Block;

import Dragonfly.Hardware.Device;
import Dragonfly.Generics;

namespace DflHW = Dfl::Hardware;
namespace DflGen = Dfl::Generics;

namespace Dfl {
    namespace Memory {
        export struct BlockInfo {
            const DflHW::Device* pDevice{ nullptr };
            const uint64_t       Size{ 0 }; // in B
        };

        export enum class [[ nodiscard ]] BlockError {
            Success = 0,
            VkMemoryMaxAllocsReachedError = -0x4901,
            VkMemoryAllocationError = -0x4902,
            VkMemoryStageBuffError = -0x4903,
            VkMemoryMapError = -0x4904,
            VkMemoryCmdPoolError = -0x4905
        };

        struct BlockHandles {
            const VkDeviceMemory       hMemory{ nullptr };

            const VkDeviceMemory       hStageMemory{ nullptr };
            const VkDeviceSize         StageMemorySize{ 0 };
            const VkBuffer             hStageBuffer{ nullptr };

            const bool                 IsStageVisible{ false };
            const void*                StageMemoryMap{ nullptr };

            operator VkDeviceMemory () const { return this->hMemory; }
        };

        // NOTE: The following is only used as a suggestion by the allocator
        // Actual size of stage memory may vary. There may not even be a stage
        // memory if there's not enough memory to spare
        export constexpr uint64_t StageMemorySize{ 1024 }; // in B

        export class Block {
            const std::unique_ptr<const BlockInfo>    pInfo{ nullptr };
            const std::unique_ptr<const BlockHandles> pHandles{};
                  DflGen::BinaryTree<uint32_t>        MemoryLayout;

            const DflHW::Queue                        TransferQueue;
                  VkCommandPool                       hCmdPool;

            const std::unique_ptr<std::mutex>         pMutex;

                  BlockError                          Error{ BlockError::Success };
            
            DFL_API       std::optional<std::array<uint64_t, 2>> DFL_CALL Alloc(const VkBuffer& buffer);
            DFL_API       void                                   DFL_CALL Free(
                                                                            const std::array<uint64_t, 2>& memoryID,
                                                                            const uint64_t                 size);
        public:
            DFL_API                                              DFL_CALL Block(const BlockInfo& info);
            DFL_API                                              DFL_CALL ~Block();

                    const BlockError                                      GetErrorCode() const noexcept { return this->Error; }

            friend class
                          Buffer;
        };
    }
}