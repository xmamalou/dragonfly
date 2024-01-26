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

namespace Dfl { namespace Graphics { class Renderer; } }

namespace Dfl {
    namespace Hardware {
        export enum class RenderOptions : unsigned int{
            Raytracing = 1,
        };

        export struct DeviceInfo {
            const Session*                    pSession{ }; // the session the device belongs to
            const uint32_t                    DeviceIndex{ 0 }; // the index of the device in the session's device list

            const uint32_t                    RenderersNumber{ 1 };
            const Dfl::BitFlag                RenderOptions{ 0 };
            const uint32_t                    SimulationsNumber{ 1 };
        };

        export enum class [[ nodiscard ]] DeviceError {
            Success                           = 0,
            // errors
            NullHandleError                   = -0x1001,
            VkDeviceInitError                 = -0x4202,
            VkDeviceNoSuchExtensionError      = -0x4203,
            VkDeviceLostError                 = -0x4204,
            VkDeviceInvalidSession            = -0x4205,
            VkNoAvailableQueuesError          = -0x4701,
            VkInsufficientQueuesError         = -0x4703,
            // warning
            VkInsufficientQueuesWarning       = 0x4702,
        };

        export enum class MemoryType : unsigned int {
            Local,
            Shared
        };

        export template < MemoryType type >
        struct DeviceMemory {
            VkDeviceSize Size{ 0 };
            uint32_t     HeapIndex{ 0 };
            bool         IsHostVisible{ false };
            bool         IsHostCoherent{ false };
            bool         IsHostCached{ false };
        };

        export enum class QueueType : unsigned int {
            Graphics = 1,
            Compute  = 2,
            Transfer = 4,
        };

        export struct QueueFamily {
            uint32_t     Index{ 0 };
            uint32_t     QueueCount{ 0 };
            Dfl::BitFlag QueueType{ 0 };
        };

        export struct Queue {
            VkQueue  hQueue{ nullptr };
            uint32_t FamilyIndex{ 0 };
        };

        export struct DeviceCharacteristics {
            std::string                                   Name{ "Placeholder GPU Name" };

            std::vector<DeviceMemory<MemoryType::Local>>  LocalHeaps{ };
            std::vector<DeviceMemory<MemoryType::Shared>> SharedHeaps{ };

            std::array<uint32_t, 2>                       MaxViewport{ {0, 0} };
            std::array<uint32_t, 2>                       MaxSampleCount{ {0, 0} }; // {colour, depth}

            std::array<uint32_t, 3>                       MaxGroups{ {0, 0, 0} }; // max amount of groups the device supports
            uint64_t                                      MaxAllocations{ 0 };

            uint64_t                                      MaxDrawIndirectCount{ 0 };
        };

        struct DeviceTracker {
            uint64_t          Allocations{ 0 };
            uint64_t          IndirectDraws{ 0 };

            std::vector<bool> AreFamiliesUsed{ };
        };

        struct DeviceHandles {
            VkDevice                    hDevice{ nullptr };
            std::vector<QueueFamily>    Families{ };
            std::unique_ptr<uint32_t[]> pLeastClaimedQueue{ nullptr };
            bool                        WithTimelineSems{ false };
            bool                        WithRTX{ false };

                                        operator VkDevice() const { return this->hDevice; }
        };

        export class Device {
            const std::unique_ptr<const DeviceInfo>             pInfo{ };
            const std::unique_ptr<const DeviceCharacteristics>  pCharacteristics{ };
            const std::unique_ptr<      DeviceTracker>          pTracker{ };
            const VkPhysicalDevice                              hPhysicalDevice{ nullptr };
            const DeviceHandles                                 GPU{ };

                  DeviceError                                   Error{ DeviceError::Success };

                  std::mutex                                    UsageMutex;
        public:
            DFL_API                             DFL_CALL  Device(const DeviceInfo& info);
            DFL_API                             DFL_CALL  ~Device();

                    const DeviceError                     GetErrorCode() const noexcept { return this->Error; }
                    const DeviceInfo                      GetInfo() const noexcept { return *this->pInfo; }
                    const DeviceCharacteristics           GetCharacteristics() const noexcept { return *this->pCharacteristics; }
            
            friend class
                          Dfl::Graphics::Renderer;
            friend class
                          Memory;
            friend class 
                          Buffer;
        };
    }
}

module :private;

