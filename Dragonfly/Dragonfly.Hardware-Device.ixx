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

#include <array>
#include <string>
#include <mutex>
#include <vector>

#include <vulkan/vulkan.h>

#include "Dragonfly.h"

export module Dragonfly.Hardware:Device;

import Dragonfly.Observer;

namespace DflOb  = Dfl::Observer;

namespace Dfl {
    namespace Hardware {
        export struct GPUInfo {
            bool                        EnableOnscreenRendering{ true }; // if set to false, disables onscreen rendering for THIS device
            bool                        EnableRaytracing{ false }; // if set to false, disables raytracing for THIS device

            VkInstance                  VkInstance{ nullptr }; // the Vulkan instance handle
            VkPhysicalDevice            VkPhysicalGPU{ nullptr }; // the Vulkan device handle

            std::vector<DflOb::Window*> pDstWindows{ }; // where will the device render onto 

            uint32_t                    PhysicsSimsNumber{ 0 };
        };

        export enum class [[nodiscard]] DeviceError {
            Success                           = 0,
            // errors
            VkDeviceInitError                 = -0x4202,
            VkDeviceNoSuchExtensionError      = -0x4203,
            VkDeviceLostError                 = -0x4204,
            VkDeviceNullptrGiven              = -0x4205,
            VkWindowNotAssociatedError        = -0x4601,
            InvalidWindowHandleError          = -0x4602,
            VkSurfaceCreationError            = -0x4603,
            VkNoSurfaceFormatsError           = -0x4604,
            VkNoSurfacePresentModesError      = -0x4605,
            VkNoAvailableQueuesError          = -0x4701,
            VkComPoolInitError                = -0x4702,
            VkInsufficientQueuesError         = -0x4703,
            VkSwapchainInitError              = -0x4801,
            VkSwapchainSurfaceLostError       = -0x4802,
            VkSwapchainWindowUnavailableError = -0x4803,
            // warnings
            ThreadNotReadyWarning             = 0x1001,
            VkAlreadyInitDeviceWarning        = 0x4201,
            VkUnknownQueueTypeWarning         = 0x4701,
            VkInsufficientQueuesWarning       = 0x4702,
        };

        enum class QueueType {
            Graphics = 1,
            Compute  = 2, // Simulation = Compute + Transfer. We consider those two essential for a queue that does sims.
            Transfer = 4
        };

        struct Queue {
            VkQueue  hQueue{ nullptr };
            uint32_t FamilyIndex{ 0 };
        };

        enum class RenderingState {
            Init,
            Loop,
            Fail,
            Dead
        };

        struct SharedRenderResources {
            VkDevice                        GPU{ nullptr };
            Queue                           AssignedQueue;

            VkCommandPool                   CmdPool{ nullptr };
            // ^ why is this here, even though command pools are per family? The reason is that command pools need to be
            // used only by the thread that created them. Hence, it is not safe to allocate one command pool per family, but rather
            // per thread.
            VkSurfaceKHR                    Surface{ nullptr };
            VkSwapchainKHR                  Swapchain{ nullptr };

            std::vector<VkImage>            SwapchainImages{ };

            VkSurfaceCapabilitiesKHR        Capabilities;
            std::vector<VkSurfaceFormatKHR> Formats;
            std::vector<VkPresentModeKHR>   PresentModes;

            DflOb::Window*                  AssociatedWindow{ nullptr };

            std::mutex*                     pDeviceMutex{ nullptr };
        };

        typedef void* (*RenderNode)(SharedRenderResources& resources, DeviceError& error);

        class RenderingSurface : public DflOb::WindowProcess
        {
            std::unique_ptr<SharedRenderResources> pSharedResources;

            RenderingState                         State{ RenderingState::Init };
            RenderNode                             pRenderNode{ nullptr };
            DeviceError                            Error{ DeviceError::ThreadNotReadyWarning };
        public:
                                         RenderingSurface();

                    void                 operator()(DflOb::WindowProcessArgs& args);
                    void                 Destroy();

            // friends 

            friend 
            class   Device;
            friend 
            class   Session;
            friend         
                    DeviceError          _GetRenderResources(SharedRenderResources& Resources, const Device& device, const DflOb::Window& window);
            friend         
                    DeviceError          _GetQueues(std::vector<VkDeviceQueueCreateInfo>& infos, Device& device);
            friend 
            DFL_API DeviceError DFL_CALL CreateRenderer(Device& device, DflOb::Window& window);
        };

        struct PhysicsSim
        {
            Queue AssignedQueue;
        };

        struct QueueFamily
        {
            uint32_t Index{ 0 };
            uint32_t QueueCount{ 0 };
            int      QueueType{ 0 };
            bool     CanPresent{ false };

            uint32_t UsedQueues{ 0 };
        };

        enum class MemoryType
        {
            Local,
            Shared
        };

        template <MemoryType type>
        struct DeviceMemory
        {
            VkDeviceSize Size{ 0 };
            uint32_t     HeapIndex{ 0 };
            bool         IsHostVisible{ false };
        };

        export class Device {
            GPUInfo                                       Info{ };
            VkDevice                                      VkGPU{ nullptr };

            std::vector<QueueFamily>                      QueueFamilies{ };

            std::string_view                              Name{ "Placeholder GPU Name" };

            std::vector<DeviceMemory<MemoryType::Local>>  LocalHeaps{ };
            std::vector<DeviceMemory<MemoryType::Shared>> SharedHeaps{ };

            std::vector<RenderingSurface>                 Surfaces{ }; // the surfaces the device is told to render to. Not equivalent to Vulkan surfaces (may concern offscreen surfaces)
            std::vector<PhysicsSim>                       Simulations{ };
            Queue                                         TransferQueue{ };

            std::vector<VkDisplayKHR>                     Displays{ }; // the displays the device is connected to. Filled only if device is activated.

            std::array<uint32_t, 3>                       MaxGroups{ {0, 0, 0} }; // max amount of groups the device supports

            std::mutex                                    UsageMutex;
        public:
            DFL_API                   DFL_CALL Device(const GPUInfo& info);
            DFL_API                   DFL_CALL ~Device();

            DFL_API       DeviceError DFL_CALL InitDevice();

                    const char*                DeviceName() const noexcept { return this->Name.data(); }

            friend 
                          void                 _OrganizeData(Device& device);
            friend 
                          DeviceError          _GetRenderResources(SharedRenderResources& Resources, const Device& device, const DflOb::Window& window);
            friend 
                          DeviceError          _GetQueues(std::vector<VkDeviceQueueCreateInfo>& infos, Device& device);
            friend 
            DFL_API       DeviceError DFL_CALL CreateRenderer(Device& device, DflOb::Window& window);
        };

        export DFL_API DeviceError DFL_CALL CreateRenderer(Device& device, DflOb::Window& window);
    }
}