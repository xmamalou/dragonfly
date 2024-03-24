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

#include <array>
#include <string>
#include <mutex>
#include <vector>

#include <vulkan/vulkan.h>

export module Dragonfly.Hardware.Device;

import Dragonfly.Generics;

import Dragonfly.Hardware.Session;
import Dragonfly.Observer.Window;

namespace DflOb  = Dfl::Observer;
namespace DflGen = Dfl::Generics;

namespace Dfl { namespace Hardware { class Device; } }
namespace Dfl { namespace Graphics { class Renderer; } }

namespace Dfl {
    namespace Memory { class Block; class Buffer; }

    namespace Hardware {
        export class Device {
        public:
            enum class RenderOptions : unsigned int {
                Raytracing = 1,
            };

            struct Info {
                const Session* pSession{ }; // the session the device belongs to
                const uint32_t                    DeviceIndex{ 0 }; // the index of the device in the session's device list

                const uint32_t                    RenderersNumber{ 1 };
                const DflGen::BitFlag             RenderOptions{ 0 };
                const uint32_t                    SimulationsNumber{ 1 };
            };

            enum class [[ nodiscard ]] Error {
                Success = 0,
                // errors
                NullHandleError = -0x1001,
                VkDeviceInitError = -0x4202,
                VkDeviceNoSuchExtensionError = -0x4203,
                VkDeviceLostError = -0x4204,
                VkDeviceInvalidSession = -0x4205,
                VkNoAvailableQueuesError = -0x4701,
                VkInsufficientQueuesError = -0x4703,
                // warning
                VkInsufficientQueuesWarning = 0x4702,
            };

            enum class MemoryType : unsigned int {
                Local,
                Shared
            };

            template < MemoryType type >
            struct Memory {
                struct Properties {
                    uint32_t     TypeIndex{ 0 };

                    bool         IsHostVisible{ false };
                    bool         IsHostCoherent{ false };
                    bool         IsHostCached{ false };
                };

                VkDeviceSize            Size{ 0 };
                uint32_t                HeapIndex{ 0 };

                std::vector<Properties> MemProperties{ };

                operator VkDeviceSize() const { return this->Size; }
            };

            struct Queue {
                enum class Type : unsigned int {
                    Graphics = 1,
                    Compute = 2,
                    Transfer = 4,
                };

                struct Family {
                    uint32_t             Index{ 0 };
                    uint32_t             QueueCount{ 0 };
                    DflGen::BitFlag      QueueType{ 0 };
                };

                const VkQueue  hQueue{ nullptr };
                const uint32_t FamilyIndex{ 0 };
                const uint32_t Index{ 0 };

                operator VkQueue() const { return this->hQueue; }
            };

            struct Fence {
                const VkFence  hFence{ nullptr };
                const uint32_t QueueFamilyIndex{ 0 };
                const uint32_t QueueIndex{ 0 };

                operator VkFence () { return this->hFence; }
            };

            struct Characteristics {
                const std::string                             Name{ "Placeholder GPU Name" };

                const std::vector<Memory<MemoryType::Local>>  LocalHeaps{ };
                const std::vector<Memory<MemoryType::Shared>> SharedHeaps{ };

                const std::array<uint32_t, 2>                 MaxViewport{ {0, 0} };
                const std::array<uint32_t, 2>                 MaxSampleCount{ {0, 0} }; // {colour, depth}

                const std::array<uint32_t, 3>                 MaxGroups{ {0, 0, 0} }; // max amount of groups the device supports
                const uint64_t                                MaxAllocations{ 0 };

                const uint64_t                                MaxDrawIndirectCount{ 0 };
            };

            struct Tracker {
                uint64_t                    Allocations{ 0 };
                uint64_t                    IndirectDraws{ 0 };

                std::vector<uint32_t>       LeastClaimedQueue{ };
                std::vector<uint32_t>       AreFamiliesUsed{ };

                std::vector<uint64_t>       UsedLocalMemoryHeaps{ }; // size is the amount of heaps
                std::vector<uint64_t>       UsedSharedMemoryHeaps{ }; // size is the amount of heaps

                std::vector<Fence>          Fences{ };
            };

            struct Handles {
                const VkDevice                    hDevice{ nullptr };
                const VkPhysicalDevice            hPhysicalDevice{ nullptr };
                const std::vector<Queue::Family>  Families{ };
                const bool                        WithTimelineSems{ false };
                const bool                        WithRTX{ false };

                operator VkDevice() const { return this->hDevice; }
                operator VkPhysicalDevice() const { return this->hPhysicalDevice; }
            };
        
        private:
            const std::unique_ptr<const Info>             pInfo{ };
            const std::unique_ptr<const Characteristics>  pCharacteristics{ };
            const std::unique_ptr<      Tracker>          pTracker{ };
            const Handles                                 GPU{ };

                  Error                                   ErrorCode{ Error::Success };
            
            DFL_API const VkFence               DFL_CALL  GetFence(
                                                            const uint32_t queueFamilyIndex,
                                                            const uint32_t queueIndex) const;
        
        public:
            DFL_API                       DFL_CALL  Device(const Info& info);
            DFL_API                       DFL_CALL  ~Device();

                    const Error                     GetErrorCode() const noexcept { return this->ErrorCode; }
                    const Info                      GetInfo() const noexcept { return *this->pInfo; }
                    const Characteristics           GetCharacteristics() const noexcept { return *this->pCharacteristics; }
            
            friend class
                          Dfl::Graphics::Renderer;
            friend class
                          Dfl::Memory::Block;
            friend class 
                          Dfl::Memory::Buffer;
        };
    }
}

module :private;

