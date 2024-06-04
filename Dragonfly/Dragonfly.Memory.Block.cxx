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

#include <vector>
#include <array>
#include <optional>
#include <mutex>

namespace DflMem = Dfl::Memory;
namespace DflHW = Dfl::Hardware;
namespace DflGen = Dfl::Generics;

using DflMemType = DflHW::Device::MemoryType;

template <DflMemType type>
using DflDevMemory = DflHW::Device::Characteristics::Memory<type>;

static std::mutex     INT_GLB_MemoryMutex;

// CODE 

auto DflMem::Block::Alloc(const VkBuffer& buffer) 
-> std::optional< std::array<uint64_t, 2> > 
{
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(
        this->pInfo->Device.GetDevice(),
        buffer,
        &requirements);

    uint64_t& size{ requirements.size };
    uint64_t  depth{ 0 };
    uint64_t  position{ 0 };
    uint64_t  offset{ 0 };
    auto      pNode{ &this->MemoryLayout };

    if ( *pNode < size ) { return std::nullopt; }

    INT_GLB_MemoryMutex.lock();
    // The memory allocation method used is the buddy system
    // Since buffers need to be aligned in a position multiple of the alignment
    // requirement, instead of dividing the node by 2, we give some extra space
    // to one of the nodes and some less space to the other, so that the nodes
    // represent the actual place the buffer will sit in in memory
    while ( *pNode > size &&
            ( *pNode / 2 ) + ( *pNode/2 % requirements.alignment ) > size )
    {
        if ( !pNode->HasBranch(0) ) 
        {
            pNode->MakeBranch(*pNode/2 + (*pNode / 2 % requirements.alignment)); 
            pNode->MakeBranch(*pNode/2 - (*pNode / 2 % requirements.alignment));
        }

        *pNode = *pNode - size;

        // if both nodes can accomodate the buffer, we pick the one with the 
        // smaller size, so we will reduce fragmentation
        if ( (*pNode)[0] > size && 
             ( (*pNode)[0].GetNodeValue() <= (*pNode)[1].GetNodeValue() ||
               (*pNode)[1] < size ) ) 
        {
            pNode = &(*pNode)[0];
        } else {
            pNode = &(*pNode)[1];
            offset += *pNode/2 - (*pNode / 2 % requirements.alignment);
            position |= 1;
        }

        position <<= 1;
        depth++;
    }

    *pNode = *pNode - size;

    vkBindBufferMemory(
        this->pInfo->Device.GetDevice(),
        buffer,
        this->Memory,
        (offset) - (offset % requirements.alignment));
    INT_GLB_MemoryMutex.unlock();

    return std::array<uint64_t, 2>({ depth, position });
};

void DflMem::Block::Free(
    const std::array<uint64_t, 2>& memoryID,
    const uint64_t                 size) 
{
    // we bring the ID of the memory allocation to the "front"
    const uint64_t& bitShiftAmount{ memoryID[0] };
    const uint64_t& ID{ memoryID[1] };

    uint64_t position{ ID << (63 - bitShiftAmount + 1) };
    int64_t  depth{ static_cast<int64_t>(bitShiftAmount) };
    auto     pNode{ &this->MemoryLayout };

    INT_GLB_MemoryMutex.lock();
    while (depth >= 0) 
    {
        *pNode = *pNode + size;

        depth--;

        // position & ( 1 << 63 ) checks if the most significant
        // bit is either 0 or 1, hence whether the algorithm should
        // follow the tree down to node 0 or 1 respectively
        if ( position & ( static_cast<uint64_t>(1) << 63 ) &&
             depth >= 0 ) 
        {
            pNode = &(*pNode)[1];
        } else if ( depth >= 0 ) {
            pNode = &(*pNode)[0];
        }
    }
    INT_GLB_MemoryMutex.unlock();
}

static inline DflHW::Device::Queue INT_GetQueue(
    const VkDevice&                                          device,
    const std::vector<Dfl::Hardware::Device::Queue::Family>& families,
          std::vector<uint32_t>&                             leastClaimedQueue,
          std::vector<uint32_t>&                             areFamiliesUsed) 
{
    VkQueue  queue{ nullptr };
    uint32_t familyIndex{ 0 };
    uint32_t amountOfClaimedGoal{ 0 };

    while (familyIndex <= families.size()) 
    {
        if (familyIndex == families.size()) 
        {
            amountOfClaimedGoal++;
            familyIndex = 0;
            continue;
        }

        if (!(families[familyIndex].QueueType & Dfl::Hardware::Device::Queue::Type::Transfer)) 
        {
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
        if (leastClaimedQueue[familyIndex] >= families[familyIndex].QueueCount) 
        {
            leastClaimedQueue[familyIndex] = 0;
        }

        vkGetDeviceQueue(
            device,
            familyIndex,
            leastClaimedQueue[familyIndex],
            &queue);
        leastClaimedQueue[familyIndex]++;
        break;
    }

    areFamiliesUsed[familyIndex]++;

    return { queue, familyIndex, leastClaimedQueue[familyIndex] - 1 };
};

static inline VkCommandPool INT_GetCmdPool(
    const VkDevice&       hGPU,
    const VkDeviceMemory& memory,
    const uint32_t&       queueFamilyIndex) 
{
    const VkCommandPoolCreateInfo cmdPoolInfo{
        .sType{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO },
        .pNext{ nullptr },
        .flags{ VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT },
        .queueFamilyIndex{ queueFamilyIndex }
    };
    VkCommandPool cmdPool{ nullptr };
    if ( vkCreateCommandPool(
            hGPU,
            &cmdPoolInfo,
            nullptr,
            &cmdPool) != VK_SUCCESS ) 
    {
        vkFreeMemory(
            hGPU,
            memory,
            nullptr);
        throw Dfl::Error::HandleCreation(
                L"Unable to create command pool for memory block",
                L"INT_GetCmdPool");
    }

    return cmdPool;
}

static DflMem::Block::Handles INT_GetMemory(
    const VkDevice&                                     gpu,
    const std::vector<DflHW::Device::Queue::Family>&    families,
    const uint64_t                                      maxAllocs,
          uint64_t&                                     totAllocs,
    const uint64_t                                      memorySize,
    const std::vector<DflDevMemory<DflMemType::Local>>& memories,
          std::vector<uint64_t>&                        usedMemories,
          std::vector<uint32_t>                         leastClaimedQueues,
          std::vector<uint32_t>&                        areFamiliesUsed) {
    if (totAllocs + 2 > maxAllocs) 
    {
        throw Dfl::Error::Limit(
                L"Maximum GPU memory allocations reached",
                L"INT_GetMemory");
    }

    std::optional<uint32_t> mainMemoryIndex{ std::nullopt };

    for (uint32_t i{0}; i < memories.size(); i++) 
    {
        for(auto property : memories[i].MemProperties) 
        {
            // host invisible memory is best suited as main memory to store data in
            if ((!property.IsHostVisible && !property.IsHostCached && !property.IsHostCoherent) &&
                  memories[i] - usedMemories[i] > memorySize &&
                 !mainMemoryIndex.has_value() ) 
            {
                mainMemoryIndex = property.TypeIndex;
                usedMemories[i] += memorySize;
                continue;
            }

            
        }

        if( mainMemoryIndex.has_value() ) { break; }
    }

    // if the above check fails, just pick the heap with the most available memory, no additional
    // requirements
    if (mainMemoryIndex == std::nullopt) 
    {
        for (uint32_t i{ 0 }; i < memories.size(); i++) 
        {
            if (memories[i] - usedMemories[i] > memorySize) 
            {
                mainMemoryIndex = memories[i].MemProperties[0].TypeIndex;
                usedMemories[i] += memorySize;
                break;
            }
        }
    }

    VkDeviceMemory mainMemory{ nullptr };
    VkMemoryAllocateInfo memInfo{
        .sType{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO },
        .pNext{ nullptr },
        .allocationSize{ memorySize },
        .memoryTypeIndex{ mainMemoryIndex.value() }
    };
    if ( vkAllocateMemory(
                    gpu,
                    &memInfo,
                    nullptr,
                    &mainMemory) != VK_SUCCESS ) 
    {
        throw Dfl::Error::HandleCreation(
                L"Unable to create memory handle",
                L"INT_GetMemory");
    }

    totAllocs++;

    std::vector<uint32_t> usedFamilies{ };
    for (uint32_t i{ 0 }; i < areFamiliesUsed.size(); i++) 
    {
        if (areFamiliesUsed[i] > 0) 
        {
            usedFamilies.push_back(i);
        }
    }

    DflHW::Device::Queue queue{ INT_GetQueue(
                                    gpu,
                                    families,
                                    leastClaimedQueues,
                                    areFamiliesUsed) };

    return { mainMemory, queue, INT_GetCmdPool(
                                    gpu,
                                    mainMemory,
                                    queue.FamilyIndex) };
};

DflMem::Block::Block(const Info& info)
: pInfo( new DflMem::Block::Info(info) ),
  Memory( INT_GetMemory(
                info.Device.GetDevice(),
                info.Device.GetQueueFamilies(),
                info.Device.GetCharacteristics().MaxAllocations,
                info.Device.GetTracker().Allocations,
                info.Size,
                info.Device.GetCharacteristics().LocalHeaps,
                info.Device.GetTracker().UsedLocalMemoryHeaps,
                info.Device.GetTracker().LeastClaimedQueue,
                info.Device.GetTracker().AreFamiliesUsed) ),
  MemoryLayout( DflGen::BinaryTree<uint32_t>(info.Size) )
{
}


DflMem::Block::~Block() {
    INT_GLB_MemoryMutex.lock();
    vkDeviceWaitIdle(this->pInfo->Device.GetDevice());

    if (this->Memory.hCmdPool != nullptr) 
    {
        vkDestroyCommandPool(
            this->pInfo->Device.GetDevice(),
            this->Memory.hCmdPool,
            nullptr);
    }

    if (this->Memory.hMemory != nullptr) 
    {
        vkFreeMemory(
            this->pInfo->Device.GetDevice(),
            this->Memory,
            nullptr);
    }

    INT_GLB_MemoryMutex.unlock();
}