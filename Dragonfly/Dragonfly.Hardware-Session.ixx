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

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <map>
#include <optional>

#include <vulkan/vulkan.h>

#include "Dragonfly.h"

export module Dragonfly.Hardware:Session;

import Dragonfly.Observer;
import Dragonfly.Graphics;

namespace DflOb = Dfl::Observer;

namespace Dfl
{
    namespace Hardware
    {
        export struct SessionInfo
        {
            std::string_view AppName{ "Dragonfly App" };
            uint32_t         AppVersion{ 0 };

            bool DoDebug{ false };
            bool EnableOnscreenRendering{ true }; // if set to false, disables onscreen rendering globally
            bool EnableRaytracing{ false }; // if set to false, disables raytracing globally
        };
        export struct GPUInfo
        {
            bool EnableOnscreenRendering{ true }; // if set to false, disables onscreen rendering for this device
            bool EnableRaytracing{ false }; // if set to false, disables raytracing for this device

            std::optional<uint32_t> DeviceIndex{ 0 }; // set to null for Dragonfly to pick the strongest one (based on a ranking system). By default, first device is picked.

            std::vector<DflOb::Window*> pDstWindows{ {} }; // where will the device render onto 

            uint32_t  PhysicsSimsNumber{ 1 }; // how many simulations the user wants to run on the device. By default, Dragonfly assumes 1.
        };

        export struct GPURenderpassInfo
        {

        };

        export enum class [[nodiscard]] SessionError
        {
            // errors
            Success = 0,
            VkInstanceCreationError = -0x4101,
            VkBadDriverError = -0x4102,
            VkNoDevicesError = -0x4201,
            VkDeviceInitError = -0x4202,
            VkDeviceNoSuchExtensionError = -0x4203,
            VkDeviceLostError = -0x4204,
            VkDeviceIndexOutOfRangeError = -0x4205,
            VkNoLayersError = -0x4301,
            VkNoExpectedLayersError = -0x4302,
            VkDebuggerCreationError = -0x4402,
            VkWindowNotAssociatedError = -0x4601,
            InvalidWindowHandleError = -0x4602,
            VkSurfaceCreationError = -0x4603,
            VkNoSurfaceFormatsError = -0x4604,
            VkNoSurfacePresentModesError = -0x4605,
            VkNoAvailableQueuesError = -0x4701,
            VkComPoolInitError = -0x4702,
            VkSwapchainInitError = -0x4801,
            VkSwapchainSurfaceLostError = -0x4802,
            VkSwapchainWindowUnavailableError = -0x4803,
            // warnings
            ThreadNotReadyWarning = 0x1001,
            VkAlreadyInitDeviceWarning = 0x4201,
            VkUnknownQueueTypeWarning = 0x4701,
            VkInsufficientQueuesWarning = 0x4702,
        };

        export struct MonitorInfo
        {
            std::tuple<uint32_t, uint32_t> Resolution;
            std::tuple<uint32_t, uint32_t> Position; // position of the specific monitor in the screen space
            std::tuple<uint32_t, uint32_t> WorkArea; // how much of the specific moitor is usable

            std::string Name;

            uint32_t    Rate;

            uint32_t    Depth;
            std::tuple<uint32_t, uint32_t, uint32_t> DepthPerPixel;
        };

        struct Processor
        {
            uint32_t Count; // amount of processors in the machine
            uint64_t Speed; // speed in Hz
        };

        /* ================================ *
         *             DEVICES              *
         * ================================ */

        enum class QueueType
        {
            Graphics = 1,
            Simulation = 2, // Simulation = Compute + Transfer. We consider those two essential for a queue that does sims.
            ComputeOnly = 4
        };

        struct Queue
        {
            VkQueue  hQueue{ nullptr };
            uint32_t FamilyIndex{ 0 };
        };

        enum class RenderingState
        {
            Init,
            Loop,
            Fail,
            Dead
        };

        struct SharedRenderResources
        {
            VkDevice GPU{ nullptr };
            Queue    AssignedQueue;
            
            VkCommandPool  CmdPool{ nullptr };
            // ^ why is this here, even though command pools are per family? The reason is that command pools need to be
            // used only by the thread that created them. Hence, it is not safe to allocate one command pool per family, but rather
            // per thread.
            VkSurfaceKHR   Surface{ nullptr };
            VkSwapchainKHR Swapchain{ nullptr };

            std::vector<VkImage> SwapchainImages{ };

            VkSurfaceCapabilitiesKHR        Capabilities;
            std::vector<VkSurfaceFormatKHR> Formats;
            std::vector<VkPresentModeKHR>   PresentModes;

            DflOb::Window* AssociatedWindow{ nullptr };
        };

        class RenderingSurface : public DflOb::WindowProcess
        {
            std::unique_ptr<SharedRenderResources> pSharedResources;
            std::mutex* pMutex;

            RenderingState State{ RenderingState::Init };
            SessionError   Error{ SessionError::ThreadNotReadyWarning };
        public:
            void operator () (DflOb::WindowProcessArgs& args);
            void Destroy();

            friend struct Device;
            friend class  Session;
            friend SessionError _GetRenderResources(SharedRenderResources& Resources, const Device& device, const VkInstance& instance, const DflOb::Window& window);
            friend SessionError _GetQueues(std::vector<VkDeviceQueueCreateInfo>& infos, Device& device);
            friend SessionError _CreateSwapchain(RenderingSurface& surface);
            friend DFL_API SessionError DFL_CALL CreateRenderer(Session& session, uint32_t deviceIndex, DflOb::Window& window);
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

        struct Device
        {
            GPUInfo                 Info{ };
            VkPhysicalDevice VkPhysicalGPU{ nullptr };
            VkDevice               VkGPU{ nullptr };

            std::vector<QueueFamily> QueueFamilies{ };

            std::string Name{ "Placeholder GPU Name" };

            std::vector<DeviceMemory<MemoryType::Local>> LocalHeaps{ };
            std::vector<DeviceMemory<MemoryType::Shared>> SharedHeaps{ };

            std::vector<RenderingSurface> Surfaces{ }; // the surfaces the device is told to render to. Not equivalent to Vulkan surfaces (may concern offscreen surfaces)
            std::vector<PhysicsSim>            Simulations{ };

            std::vector<VkDisplayKHR> Displays{ }; // the displays the device is connected to. Filled only if device is activated.

            std::mutex* pUsageMutex{ nullptr };
        };

        class DebugProcess : public DflOb::WindowProcess
        {
            DflOb::Window* pWindow{ nullptr };
        public:
            void operator () (DflOb::WindowProcessArgs& args);
            void Destroy();
        };

        /// <summary>
        /// The `Session` class concerns objects that represent
        /// the machine and its hardware
        /// </summary>
        export class Session
        {
            SessionInfo GeneralInfo;

            VkInstance                               VkInstance;
            VkDebugUtilsMessengerEXT VkDbMessenger;

            Processor CPU;

            uint64_t Memory; // memory size in B

            std::vector<Device>   Devices;
            std::mutex                   DeviceMutex;
            // ^ the above concerns all the devices that are available on the machine. That means that if the machine has, for example, 2 initialized GPUs, 
            // and GPU A is used by thread 1, if thread 2 attempts to use GPU B, even if it's unused, it will be blocked until thread 1 is done with GPU A. 
            // This may seem like an issue, but it is not that problematic, as there's not many personal computers that have more than one GPU. 
            // For now, this is how it will be done. Later, I may add a more robust/complex system that allows for multiple GPUs to be used at the same time.

            DebugProcess        WindowDebugging;
        public:
            DFL_API DFL_CALL Session(const SessionInfo& info);
            DFL_API DFL_CALL ~Session();

            /// <summary>
            /// Initialize Vulkan
            /// </summary>
            /// <returns>
            /// A `SessionError` error code that could be any of the following:
            /// <para>- Success, on success</para>
            /// </returns>
            DFL_API SessionError DFL_CALL InitVulkan();

            /// <summary>
            /// Initialize a device
            /// </summary>
            /// <param name="info">The GPUInfo struct that contains the information needed to initialize the device</param>
            /// <returns>
            /// A `SessionError` error code that could be any of the following:
            /// <para>- Success, on success</para>
            /// </returns>
            DFL_API SessionError DFL_CALL InitDevice(const GPUInfo& info);

            /// <summary>
            /// Get the amount of devices available on the machine, initalized or not.
            /// </summary>
            uint64_t DeviceCount() const
            {
                return this->Devices.size();
            };

            std::string_view DeviceName(uint32_t deviceIndex) const
            {
                return std::string_view(this->Devices[deviceIndex].Name);
            };

            friend SessionError _LoadDevices(Session& session);
            friend DFL_API SessionError DFL_CALL CreateRenderer(Session& session, uint32_t deviceIndex, DflOb::Window& window);
        };

        export DFL_API SessionError DFL_CALL CreateRenderer(Session& session, uint32_t deviceIndex, DflOb::Window& window);
    }
}

