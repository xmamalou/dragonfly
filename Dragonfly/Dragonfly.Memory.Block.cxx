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
#include "Dragonfly.Memory.Block.hxx"

#include <vector>
#include <array>
#include <optional>
#include <mutex>

#include <vulkan/vulkan.h>

#include "Dragonfly.Generics.hxx"

namespace DflMem = Dfl::Memory;
namespace DflHW = Dfl::Hardware;
namespace DflGen = Dfl::Generics;

using DflMemType = DflHW::Device::MemoryType;

template <DflMemType type>
using DflDevMemory = DflHW::Device::Characteristics::Memory<type>;

// CODE 

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
          DflHW::Device&                                device,
    const uint64_t                                      memorySize) 
{
    VkDeviceMemory mainMemory{ nullptr };
    uint64_t       heapIndex{ 0 };
    for (; heapIndex < device.GetCharacteristics().LocalHeaps.size(); heapIndex++) 
    {
        mainMemory = device.BorrowMemory<DflHW::Device::MemoryType::Local>(
                            heapIndex,
                            false,
                            false,
                            false,
                            false,
                            memorySize);

        if( mainMemory != nullptr ) { break; }
    }

    // if the above check fails, it is first assumed that there wasn't a heap that had 
    // a type with those properties. Thus, the first available one is picked.
    if (mainMemory == nullptr) 
    {
        for (heapIndex = 0; heapIndex < device.GetCharacteristics().LocalHeaps.size(); heapIndex++) 
        {
            mainMemory = device.BorrowMemory<DflHW::Device::MemoryType::Local>(
                            heapIndex,
                            false,
                            false,
                            false,
                            true,
                            memorySize);
        }
    }

    if (mainMemory == nullptr) 
    {
        throw Dfl::Error::HandleCreation(
                L"Unable to create memory handle",
                L"INT_GetMemory");
    }

    const DflHW::Device::Queue queue{ device.BorrowQueue(DflHW::Device::Queue::Type::Transfer) };

    return { mainMemory, heapIndex, 
             queue, INT_GetCmdPool(
                        device.GetDevice(),
                        mainMemory,
                        queue.FamilyIndex) };
};

DflMem::Block::Block(const Info& info)
: pInfo( new DflMem::Block::Info(info) ),
  Memory( INT_GetMemory(
                info.Device,
                info.Size) ),
  MemoryLayout( DflGen::BinaryTree<uint32_t>(info.Size) )
{
}


DflMem::Block::~Block() {
    vkDeviceWaitIdle(this->pInfo->Device.GetDevice());

    vkDestroyCommandPool(
        this->pInfo->Device.GetDevice(),
        this->Memory.hCmdPool,
        nullptr);

    this->pInfo->Device.ReturnMemory<DflHW::Device::MemoryType::Local>(
                            this->Memory,
                            this->Memory.HeapIndex,
                            this->pInfo->Size);
}