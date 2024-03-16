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
#include <array>
#include <optional>
#include <mutex>

#include <vulkan/vulkan.h>

module Dragonfly.Memory.Block;

namespace DflMem = Dfl::Memory;
namespace DflHW = Dfl::Hardware;
namespace DflGen = Dfl::Generics;

// GLOBAL STAGE BUFFER STATE

       uint64_t       INT_GLB_ReferenceCount{ 0 }; // how many memory blocks there exist

       VkDeviceMemory INT_GLB_hStageMemory{ nullptr };
       VkDeviceSize   INT_GLB_StageMemorySize{ 0 };
       VkBuffer       INT_GLB_hStageBuffer{ nullptr };

       bool           INT_GLB_IsStageVisible{ false };
       void*          INT_GLB_pStageMemoryMap{ nullptr }; 

static std::mutex     INT_GLB_MemoryMutex;

// CODE 

std::optional<std::array<uint64_t, 2>> DflMem::Block::Alloc(const VkBuffer& buffer) {
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(
        this->pInfo->pDevice->GPU,
        buffer,
        &requirements);

    uint64_t& size{ requirements.size };
    uint64_t  depth{ 0 };
    uint64_t  position{ 0 };
    uint64_t  offset{ 0 };
    auto      pNode{ &this->MemoryLayout };

    if(*pNode < size) { return std::nullopt; }

    INT_GLB_MemoryMutex.lock();
    // The memory allocation method used is the buddy system
    // Since buffers need to be aligned in a position multiple of the alignment
    // requirement, instead of dividing the node by 2, we give some extra space
    // to one of the nodes and some less space to the other, so that the nodes
    // represent the actual place the buffer will sit in in memory
    while (*pNode > size &&
           (*pNode / 2) + (*pNode/2 % requirements.alignment) > size) {
        if (!pNode->HasBranch(0)) {
            pNode->MakeBranch(*pNode/2 + (*pNode / 2 % requirements.alignment)); 
            pNode->MakeBranch(*pNode/2 - (*pNode / 2 % requirements.alignment));
        }

        *pNode = *pNode - size;

        // if both nodes can accomodate the buffer, we pick the one with the 
        // smaller size, so we will reduce fragmentation
        if ((*pNode)[0] > size && 
            ((*pNode)[0].GetNodeValue() <= (*pNode)[1].GetNodeValue() ||
             (*pNode)[1] < size)) {
            pNode = &(*pNode)[0];
            
        } else {
            pNode = &(*pNode)[1];
            offset += *pNode/2 - (*pNode / 2 % requirements.alignment);
            position |= 1;
        }

        position << 1;
        depth++;
    }

    *pNode = *pNode - size;

    vkBindBufferMemory(
        this->pInfo->pDevice->GPU,
        buffer,
        this->pHandles->hMemory,
        (offset) - (offset % requirements.alignment));
    INT_GLB_MemoryMutex.unlock();

    return std::array<uint64_t, 2>({ depth, position });
};

void DflMem::Block::Free(
    const std::array<uint64_t, 2>& memoryID,
    const uint64_t                 size) {
    uint64_t position{ memoryID[1] << (64 - memoryID[0] + 1) };
    int64_t  depth{ static_cast<int64_t>(memoryID[0]) };
    auto     pNode{ &this->MemoryLayout };

    INT_GLB_MemoryMutex.lock();
    while (depth >= 0) {
        *pNode = *pNode + size;

        depth--;

        // if both nodes can accomodate the buffer, we pick the one with the 
        // smaller size, so we will reduce fragmentation
        if ( position & ( static_cast<uint64_t>(1) << 64 ) &&
             depth >= 0 ) {
            pNode = &(*pNode)[1];
        } else if ( depth >= 0 ) {
            pNode = &(*pNode)[0];
        }
    }
    INT_GLB_MemoryMutex.unlock();
}

static inline DflHW::Queue INT_GetQueue(
    const VkDevice&                                device,
    const std::vector<Dfl::Hardware::QueueFamily>& families,
          std::vector<uint32_t>&                   leastClaimedQueue,
          std::vector<uint32_t>&                   areFamiliesUsed) {
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
        if (leastClaimedQueue[familyIndex] >= families[familyIndex].QueueCount) {
            leastClaimedQueue[familyIndex] = 0;
        }

        vkGetDeviceQueue(
            device,
            familyIndex,
            leastClaimedQueue[familyIndex],
            &queue
        );
        leastClaimedQueue[familyIndex]++;
        break;
    }

    areFamiliesUsed[familyIndex]++;

    return { queue, familyIndex };
};

static inline VkCommandPool INT_GetCmdPool(
    const VkDevice& hGPU,
    const uint32_t& queueFamilyIndex) {
    const VkCommandPoolCreateInfo cmdPoolInfo{
        .sType{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT },
        .queueFamilyIndex{ queueFamilyIndex }
    };
    VkCommandPool cmdPool{ nullptr };
    VkResult      error{ VK_SUCCESS };
    if (( error = vkCreateCommandPool(
                        hGPU,
                        &cmdPoolInfo,
                        nullptr,
                        &cmdPool) ) != VK_SUCCESS) {
        throw DflMem::BlockError::VkMemoryCmdPoolError;
    }

    return cmdPool;
}

static DflMem::BlockHandles INT_GetMemory(
    const VkDevice&                                                   gpu,
    const uint64_t&                                                   maxAllocs,
          uint64_t&                                                   totAllocs,
    const uint64_t&                                                   memorySize,
    const std::vector<DflHW::DeviceMemory<DflHW::MemoryType::Local>>& memories,
          std::vector<uint32_t>&                                      usedMemories,
          std::vector<uint32_t>&                                      areFamiliesUsed) {
    if (totAllocs + 2 > maxAllocs) {
        throw DflMem::BlockError::VkMemoryMaxAllocsReachedError;
    }

    std::optional<uint32_t> mainMemoryIndex{ std::nullopt };
    std::optional<uint32_t> stageMemoryIndex{ std::nullopt };

    for (uint32_t i{0}; i < memories.size(); i++) {
        for(auto property : memories[i].Properties) {
            // host invisible memory is best suited as main memory to store data in
            if ((!property.IsHostVisible && !property.IsHostCached && !property.IsHostCoherent) &&
                  memories[i] - usedMemories[i] > memorySize &&
                 !mainMemoryIndex.has_value() ) {
                mainMemoryIndex = property.TypeIndex;
                usedMemories[i] += memorySize;
                continue;
            }

            // preferably, we want stage memory that is visible to the host 
            // Unlike main memory, the defauly stage memory size is simply a suggestion
            // and is not followed if it's a condition that cannot be met.
            // In other words, Dragonfly will prefer host visible memory over any kind of 
            // memory, even if there is more memory to claim from other heaps
            if ( (property.IsHostVisible || property.IsHostCached || property.IsHostCoherent) &&
                  !stageMemoryIndex.has_value() &&
                  INT_GLB_ReferenceCount == 0 ) {
                stageMemoryIndex = property.TypeIndex;
                INT_GLB_IsStageVisible = property.IsHostVisible;
                usedMemories[i] += ( INT_GLB_StageMemorySize =
                                        memories[i] - usedMemories[i] > DflMem::StageMemorySize ? DflMem::StageMemorySize : usedMemories[i] );
            }
        }

        if( mainMemoryIndex.has_value() && stageMemoryIndex.has_value() ) { break; }
    }

    // if the above check fails, just pick the heap with the most available memory, no additional
    // requirements
    if (mainMemoryIndex == std::nullopt) {
        for (uint32_t i{ 0 }; i < memories.size(); i++) {
            if (memories[i] - usedMemories[i] > memorySize) {
                mainMemoryIndex = memories[i].Properties[0].TypeIndex;
                usedMemories[i] += memorySize;
                break;
            }
        }
    }

    // likewise.
    if (stageMemoryIndex == std::nullopt && INT_GLB_ReferenceCount == 0) {
        for (uint32_t i{ 0 }; i < memories.size(); i++) {
            if (memories[i] - usedMemories[i] > DflMem::StageMemorySize) {
                stageMemoryIndex = memories[i].Properties[0].TypeIndex;
                INT_GLB_IsStageVisible = memories[i].Properties[0].IsHostVisible;
                usedMemories[i] += ( INT_GLB_StageMemorySize =
                                        memories[i] - usedMemories[i] > DflMem::StageMemorySize ? DflMem::StageMemorySize : usedMemories[i]);
                break;
            }
        }
    }

    VkDeviceMemory mainMemory{ nullptr };
    VkResult error{ VK_SUCCESS };
    VkMemoryAllocateInfo memInfo{
        .sType{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO },
        .pNext{ nullptr },
        .allocationSize{ memorySize },
        .memoryTypeIndex{ mainMemoryIndex.value() }
    };
    if ((error = vkAllocateMemory(
                    gpu,
                    &memInfo,
                    nullptr,
                    &mainMemory)) != VK_SUCCESS) {
        throw DflMem::BlockError::VkMemoryAllocationError;
    }

    totAllocs++;

    if( INT_GLB_ReferenceCount == 0 ) {
        // unlike main memory, if we cannot allocate stage memory, we will make due and just skip 
        // stage memory altogether
        if( INT_GLB_StageMemorySize > 0 ) {
            memInfo.allocationSize = INT_GLB_StageMemorySize;
            memInfo.memoryTypeIndex = stageMemoryIndex.value();
            if ( (error = vkAllocateMemory(
                                gpu,
                                &memInfo,
                                nullptr,
                                &INT_GLB_hStageMemory) ) != VK_SUCCESS) {
                vkFreeMemory(
                    gpu,
                    mainMemory,
                    nullptr
                );
                throw DflMem::BlockError::VkMemoryAllocationError;
            }
        }

        if( INT_GLB_StageMemorySize > 0 ) { totAllocs++; }
    }

    std::vector<uint32_t> usedFamilies{ };
    for (uint32_t i{ 0 }; i < areFamiliesUsed.size(); i++) {
        if (areFamiliesUsed[i] > 0) {
            usedFamilies.push_back(i);
        }
    }

    if(INT_GLB_ReferenceCount == 0) {
        if (INT_GLB_StageMemorySize > 0) {
            const VkBufferCreateInfo bufInfo{
                .sType{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO },
                .pNext{ nullptr },
                .flags{ 0 },
                .size{ INT_GLB_StageMemorySize },
                .usage{ VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT },
                .sharingMode{ VK_SHARING_MODE_EXCLUSIVE }
            };

            if ((error = vkCreateBuffer(
                gpu,
                &bufInfo,
                nullptr,
                &INT_GLB_hStageBuffer)) != VK_SUCCESS) {
                throw DflMem::BlockError::VkMemoryStageBuffError;
            }
            if ((error = vkBindBufferMemory(
                gpu,
                INT_GLB_hStageBuffer,
                INT_GLB_hStageMemory,
                0)) != VK_SUCCESS) {
                throw DflMem::BlockError::VkMemoryStageBuffError;
            }
        
            if ( INT_GLB_IsStageVisible ) {
                if ((error = vkMapMemory(
                    gpu,
                    INT_GLB_hStageMemory,
                    0,
                    DflMem::StageMemorySize,
                    0,
                    &INT_GLB_pStageMemoryMap)) != VK_SUCCESS) {
                    throw DflMem::BlockError::VkMemoryMapError;
                }
            }
        }
    }

    INT_GLB_ReferenceCount++;

    return { mainMemory,
             INT_GLB_hStageMemory, INT_GLB_StageMemorySize, INT_GLB_hStageBuffer, 
             INT_GLB_IsStageVisible, INT_GLB_pStageMemoryMap };
};

static inline DflGen::BinaryTree<uint32_t> INT_GetMemoryLayout(const uint32_t initialSize) {
    DflGen::BinaryTree<uint32_t> tree(initialSize);

    return tree;
}

DflMem::Block::Block(const BlockInfo& info)
try : pInfo( new DflMem::BlockInfo(info) ),
      pHandles( new DflMem::BlockHandles( INT_GetMemory(
                                            info.pDevice->GPU,
                                            info.pDevice->pCharacteristics->MaxAllocations,
                                            info.pDevice->pTracker->Allocations,
                                            info.Size,
                                            info.pDevice->pCharacteristics->LocalHeaps,
                                            info.pDevice->pTracker->UsedLocalMemoryHeaps,
                                            info.pDevice->pTracker->AreFamiliesUsed) ) ),
      MemoryLayout( INT_GetMemoryLayout(info.Size) ),
      TransferQueue( INT_GetQueue(
                        info.pDevice->GPU,
                        info.pDevice->GPU.Families,
                        info.pDevice->pTracker->LeastClaimedQueue, 
                        info.pDevice->pTracker->AreFamiliesUsed) ) {
    const VkCommandPoolCreateInfo cmdPoolInfo{
          .sType{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO },
          .pNext{ nullptr },
          .flags{ VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT },
          .queueFamilyIndex{ this->TransferQueue.FamilyIndex }
    };
    VkResult      error{ VK_SUCCESS };
    if ((error = vkCreateCommandPool(
                    info.pDevice->GPU,
                    &cmdPoolInfo,
                    nullptr,
                    &this->hCmdPool)) != VK_SUCCESS) {
        vkDeviceWaitIdle(this->pInfo->pDevice->GPU);

        if (this->pHandles->hMemory != nullptr) {
            vkFreeMemory(
                this->pInfo->pDevice->GPU,
                this->pHandles->hMemory,
                nullptr
            );
        }

        INT_GLB_ReferenceCount--;
        
        if(INT_GLB_ReferenceCount == 0) {
            if (INT_GLB_hStageBuffer != nullptr) {
                vkDestroyBuffer(
                    this->pInfo->pDevice->GPU,
                    INT_GLB_hStageBuffer,
                    nullptr
                );
            }

            if (INT_GLB_hStageMemory != nullptr) {
                vkFreeMemory(
                    this->pInfo->pDevice->GPU,
                    INT_GLB_hStageMemory,
                    nullptr
                );
            }
        }

        this->Error = DflMem::BlockError::VkMemoryCmdPoolError;
    }
}
catch (DflMem::BlockError e) { this->Error = BlockError::VkMemoryAllocationError; };

DflMem::Block::~Block() {
    INT_GLB_MemoryMutex.lock();
    vkDeviceWaitIdle(this->pInfo->pDevice->GPU);

    if (this->hCmdPool != nullptr) {
        vkDestroyCommandPool(
            this->pInfo->pDevice->GPU,
            this->hCmdPool,
            nullptr
        );
    }

    if (this->pHandles->hMemory != nullptr) {
        vkFreeMemory(
            this->pInfo->pDevice->GPU,
            this->pHandles->hMemory,
            nullptr
        );
    }

    INT_GLB_ReferenceCount--;

    if (INT_GLB_ReferenceCount == 0) {
        if (INT_GLB_hStageBuffer != nullptr) {
            vkDestroyBuffer(
                this->pInfo->pDevice->GPU,
                INT_GLB_hStageBuffer,
                nullptr
            );
        }

        if (INT_GLB_hStageMemory != nullptr) {
            vkFreeMemory(
                this->pInfo->pDevice->GPU,
                INT_GLB_hStageMemory,
                nullptr
            );
        }
    }

    INT_GLB_MemoryMutex.unlock();
}