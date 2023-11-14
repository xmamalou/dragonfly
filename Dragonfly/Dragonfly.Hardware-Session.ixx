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

#include <iostream>
#include <string>
#include <vector>
#include <memory>

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
        export enum class GPUCriteria
        {
            None = 0,
        };

        export struct SessionInfo
        {
            std::string_view AppName{ "Dragonfly App" };
            uint32_t         AppVersion{ 0 };

            bool             DoDebug{ false };
            bool             EnableRTRendering{ true }; // enable real time rendering
            bool             EnableRaytracing{ false };
        };
        export struct GPUInfo
        {
            bool      ChooseOnRank{ false }; // whether to pick the strongest one (based on a ranking system). By default, Dragonfly will initialize the device on DeviceIndex instead

            uint32_t                    DeviceIndex{ 0 };
            uint32_t                    Criteria{ 0 };
            std::vector<DflOb::Window*> pDstWindows{ {} }; // where will the device render onto - by default, Dragonfly assumes no surfaces

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
            VkNoLayersError = -0x4301,
            VkNoExpectedLayersError = -0x4302,
            VkDebuggerCreationError = -0x4402,
            VkSurfaceCreationError = -0x4601,
            VkNoAvailableQueuesError = -0x4701,
            VkComPoolInitError = -0x4702,
            // warnings
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
            Simulation = 2 // Simulation = Compute + Transfer. We consider those two essential for a queue that does sims.
        };

        struct Queue
        {
            VkQueue  hQueue;
            uint32_t FamilyIndex;
        };

        class RenderingSurface : public DflOb::WindowProcess
        {
            VkSurfaceKHR   surface; // not required to be filled. If not filled, it's implied it's offscreen surface
            VkSwapchainKHR swapchain; // same

            DflOb::Window* associatedWindow;
            Queue          assignedQueue;

        public: 
            void operator () (DflOb::WindowProcessArgs& args);

            friend class Device;
            friend class Session;
            friend SessionError _GetQueues(std::vector<VkDeviceQueueCreateInfo>& infos, Device& device);
        };

        struct PhysicsSim
        {
            Queue assignedQueue;
        };

        struct QueueFamily
        {
            uint32_t      Index;
            uint32_t      QueueCount;
            int           QueueType{ 0 };
            bool          CanPresent{ false };
        };

        struct DeviceMemory
        {
            VkDeviceSize Size;
            uint32_t     HeapIndex;
            bool         IsHostVisible;
        };

        struct Device
        {
            VkPhysicalDevice VkPhysicalGPU;
            VkDevice         VkGPU;

            std::vector<QueueFamily> QueueFamilies;

            std::string Name;

            std::vector<DeviceMemory> LocalHeaps;
            std::vector<DeviceMemory> SharedHeaps;

            std::vector<RenderingSurface> Surfaces; // the surfaces the device is told to render to. Not equivalent to Vulkan surfaces (may concern offscreen surfaces)
            std::vector<PhysicsSim>       Simulations;

            std::vector<VkDisplayKHR> Displays; // the displays the device is connected to. Filled only if device is activated.

            friend class Session;
            friend SessionError _GetQueues(std::vector<VkDeviceQueueCreateInfo>& infos, Device& device);
        };

        class DebugProcess : public DflOb::WindowProcess
        {
            int            NotInit{ 0 };
            DflOb::Window* pWindow{ nullptr };

        public:
            void operator () (DflOb::WindowProcessArgs& args);
        };

        /**
        * \brief The `Session` class concerns objects that represent
        * the machine and its hardware
        */
        export class Session
        {
            SessionInfo        GeneralInfo;
            GPUInfo            DeviceInfo;

            VkInstance               VkInstance;
            VkDebugUtilsMessengerEXT VkDbMessenger;

            Processor CPU;

            uint64_t Memory; // memory size in B

            std::vector<Device> Devices;

            DebugProcess        WindowDebugging;

            void OrganizeData(Device& device); // for LoadDevices
        public:
            DFL_API DFL_CALL Session(const SessionInfo& info);
            DFL_API DFL_CALL ~Session();

            /**
            * \brief Initialize Vulkan
            *
            * \returns `SessionError::Success` on success.
            * \returns `SessionError::VkInstanceCreationError` if Vulkan couldn't create an instance.
            * \returns `SessionError::VkBadDriverError` if the Vulkan driver is bad or out of date.
            * \returns `SessionError::VkNoLayersError` if Vulkan didn't find any layers (applicable if `SessionCriteria::DoDebug` is set).
            * \returns `SessionError::VkNoExpectedLayersError` if Vulkan didn't find the desired layers (applicable if `SessionCriteria::DoDebug` is set).
            * \returns `SessionError::VkDebuggerCreationError` if Vulkan couldn't create the debug messenger (applicable if `SessionCriteria::DoDebug` is set).
            */
            DFL_API SessionError DFL_CALL InitVulkan();

            /**
            * \brief Gets the devices available to the machine into memory
            */
            DFL_API SessionError DFL_CALL LoadDevices();

            /**
            * \brief Initializes the device indicated by `DeviceIndex` in `info` based on other various
            * information in `info`
            *
            * \param `info`: Information about the GPU to be initialized
            *
            * \returns `SessionError::Success` on success.
            * \returns `SessionError::VkAlreadyInitDeviceWarning` if the desired device is already initialized.
            * \returns `SessionError::VkNoDevicesError` if there are no (Vulkan capable) devices in the system.
            * \returns `SessionError::VkDeviceInitError` if the device couldn't be initialized.
            */
            DFL_API SessionError DFL_CALL InitDevice(const GPUInfo& info);

            uint64_t DeviceCount() const
            {
                return this->Devices.size();
            };

            std::string_view DeviceName(uint32_t deviceIndex) const
            {
                return std::string_view(this->Devices[deviceIndex].Name);
            };

            friend DFL_API SessionError DFL_CALL CreateRenderer(Session& session, uint32_t deviceIndex, const DflOb::Window& window);
        };

        export DFL_API SessionError DFL_CALL CreateRenderer(Session& session, uint32_t deviceIndex, const DflOb::Window& window);
    }
}