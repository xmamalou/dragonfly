#include <string>
#include <vector>
#include <memory>

#include <vulkan/vulkan.h>

#include "Dragonfly.h"

export module Dragonfly.Hardware:Session;

import Dragonfly.Observer;

namespace DflOb = Dfl::Observer;

namespace Dfl
{
    namespace Hardware
    {
        export enum class SessionCriteria
        {
            None = 0,
            DoDebug = 1,
            OnlyOffscreen = 2 // disable onscreen capabilities for all devices
        };
        export enum class GPUCriteria
        {
            None = 0,
        };

        export struct SessionInfo
        {
            std::string_view AppName{ "Dragonfly App" };
            uint32_t         AppVersion{ 0 };
            uint32_t         Criteria{ 0 };
        };
        export struct GPUInfo
        {
            bool      ChooseOnRank = false; // whether to pick the strongest one (based on a ranking system). By default, Dragonfly will initialize the device on DeviceIndex instead

            uint32_t  DeviceIndex = 0;
            uint32_t  Criteria = 0;
            std::vector<DflOb::Window*> pDstWindows = {}; // where will the device render onto - by default, Dragonfly assumes no surfaces

            uint32_t  PhysicsSimsNumber = 1; // how many simulations the user wants to run on the device. By default, Dragonfly assumes 1.
        };

        export enum class [[nodiscard]] SessionError
        {
            // errors
            Success = 0,
            VkInstanceCreationError = -0x4101,
            VkNoDevicesError = -0x4201,
            VkDeviceInitError = -0x4202,
            VkNoLayersError = -0x4301,
            VkNoExpectedLayersError = -0x4302,
            VkDebuggerCreationError = -0x4402,
            VkSurfaceCreationError = -0x4601,
            VkNoAvailableQueuesError = -0x4701,
            VkInsufficientQueuesError = -0x4702,
            // warnings
            VkAlreadyInitDeviceWarning = 0x1001,
            VkUnknownQueueTypeWarning = 0x4701
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

        struct RenderingSurface
        {
            VkSurfaceKHR   surface; // not required to be filled. If not filled, it's implied it's offscreen surface
            DflOb::Window* associatedWindow;
            Queue          assignedQueue;
        };

        struct PhysicsSim
        {

        };

        struct QueueFamily
        {
            VkCommandPool ComPool;
            uint32_t      QueueCount;
            uint32_t      QueueType;
            bool          CanPresent;
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

            std::vector<RenderingSurface> Surfaces; // the surfaces the device is told to render to.
            std::vector<PhysicsSim>       Simulations;
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

            std::vector<Device>      Devices;

            void OrganizeData(Device& device); // for LoadDevices

            SessionError GetQueues(std::vector<VkDeviceQueueCreateInfo>& infos, Device& device);
        public:
            DFL_API DFL_CALL Session(const SessionInfo& info);
            DFL_API DFL_CALL ~Session();

            /**
            * \brief Initialize Vulkan
            *
            * \returns `SessionError::Success` on success.
            * \returns `SessionError::VkInstanceCreationError if Vulkan couldn't create an instance.
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
            *
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
        };
    }
}