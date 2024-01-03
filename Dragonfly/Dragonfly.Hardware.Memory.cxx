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

#include <vulkan/vulkan.h>

module Dragonfly.Hardware.Memory;

namespace DflHW = Dfl::Hardware;

static inline DflHW::Queue INT_GetQueue(
    const VkDevice& device,
    const std::vector<Dfl::Hardware::QueueFamily>& families,
    uint32_t* pLeastClaimedQueue) {
    VkQueue  queue{ nullptr };
    uint32_t familyIndex{ 0 };
    for (uint32_t i{ 0 }; i < families.size(); ++i) {
        if (!(families[i].QueueType & Dfl::Hardware::QueueType::Transfer)) {
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
        */
        if (pLeastClaimedQueue[i] >= families[i].QueueCount) {
            pLeastClaimedQueue[i] = 0;
        }

        familyIndex = i;
        vkGetDeviceQueue(
            device,
            i,
            pLeastClaimedQueue[i],
            &queue
        );
        pLeastClaimedQueue[i]++;
        break;
    }

    return { queue, familyIndex };
};

DflHW::Memory::Memory(const MemoryInfo& info) noexcept
    : PhysicalGPU(info.pDevice->hPhysicalDevice),
      GPU(info.pDevice->GPU),
      TransferQueue(INT_GetQueue(
          info.pDevice->GPU,
          info.pDevice->GPU.Families,
          info.pDevice->GPU.pLeastClaimedQueue.get())){
    
};