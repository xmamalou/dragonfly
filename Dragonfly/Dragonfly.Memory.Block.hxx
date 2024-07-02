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
#include <optional>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "Dragonfly.h"

#include "Dragonfly.Generics.hxx"
#include "Dragonfly.Hardware.Device.hxx"

namespace Dfl { 
    namespace Memory {
        // Dragonfly.Memory.Block
        class Block {
        public:
            struct Info {
                      DflHW::Device& Device;
                const uint64_t       Size{ 0 }; // in B
            };

            struct Handles {
                const VkDeviceMemory       hMemory{ nullptr };
                const uint64_t             HeapIndex{ 0 };

                const DflHW::Device::Queue TransferQueue{ };
                const VkCommandPool        hCmdPool{ nullptr };

                operator VkDeviceMemory () const { return this->hMemory; }
            };

        protected:
            const std::unique_ptr<const Info>    pInfo{ nullptr };
            const Handles                        Memory{};
                  DflGen::BinaryTree<uint64_t>   MemoryLayout;
        public: 
            DFL_API DFL_CALL Block(const Info& info);
            DFL_API DFL_CALL ~Block();

                  DflHW::Device&        GetDevice() const noexcept { 
                                            return this->pInfo->Device; }
            const DflHW::Device::Queue& GetQueue() const noexcept {
                                            return this->Memory.TransferQueue; }
            const VkCommandPool         GetCmdPool() const noexcept {
                                            return this->Memory.hCmdPool; }

            template< Dfl::Generics::VulkanStorage T >
                  auto                  Alloc(const T& buffer) noexcept
                  -> std::optional< std::array<uint64_t, 2> >;
            template< Dfl::Generics::VulkanStorage T >
                  void                  Free(
                                            const std::array<uint64_t, 2>&  memoryID,
                                            const T&                        buffer) noexcept;
        };
   }
}

template< Dfl::Generics::VulkanStorage T >
auto Dfl::Memory::Block::Alloc(const T& buffer) noexcept
-> std::optional< std::array<uint64_t, 2> > 
{
    VkMemoryRequirements requirements;
    if constexpr ( Dfl::Generics::SameType<T, VkBuffer> )
    { 
        vkGetBufferMemoryRequirements(
            this->GetDevice().GetDevice(),
            buffer,
            &requirements);
    }
    else {
        vkGetImageMemoryRequirements(
            this->GetDevice().GetDevice(),
            buffer,
            &requirements);
    }

    uint64_t& size{ requirements.size };
    uint64_t  depth{ 0 };
    uint64_t  position{ 0 };
    uint64_t  offset{ 0 };
    auto      pNode{ &(this->MemoryLayout) };

    if ( *pNode < size ) { return std::nullopt; }

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
            pNode->MakeBranch(*pNode/2 + (*pNode / 2 % requirements.alignment))
                  .MakeBranch(*pNode/2 - (*pNode / 2 % requirements.alignment)); 
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

    if constexpr ( Dfl::Generics::SameType<T, VkBuffer> )
    { 
        vkBindBufferMemory(
            this->GetDevice().GetDevice(),
            buffer,
            this->Memory,
            (offset) - (offset % requirements.alignment));
    }
    else {
        vkBindImageMemory(
            this->GetDevice().GetDevice(),
            buffer,
            this->Memory,
            (offset) - (offset % requirements.alignment));
    }

    return std::array<uint64_t, 2>({ depth, position });
};

template< Dfl::Generics::VulkanStorage T >
void  Dfl::Memory::Block::Free(
    const std::array<uint64_t, 2>& memoryID,
    const T&                       buffer) noexcept
{
    // we bring the ID of the memory allocation to the "front"
    const uint64_t& bitShiftAmount{ memoryID[0] };
    const uint64_t& ID{ memoryID[1] };

    uint64_t position{ ID << (63 - bitShiftAmount + 1) };
    int64_t  depth{ static_cast<int64_t>(bitShiftAmount) };
    auto     pNode{ &(this->MemoryLayout) };

    VkMemoryRequirements requirements;
    if constexpr ( Dfl::Generics::SameType<T, VkBuffer> )
    { 
        vkGetBufferMemoryRequirements(
            this->GetDevice().GetDevice(),
            buffer,
            &requirements);
    }
    else {
        vkGetImageMemoryRequirements(
            this->GetDevice().GetDevice(),
            buffer,
            &requirements);
    }

    while (depth >= 0) 
    {
        *pNode = *pNode + requirements.size;

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
}