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

#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>

#include <vulkan/vulkan.h>

#include "Dragonfly.h"

export module Dragonfly.Hardware:Session;

import Dragonfly.Observer;
//import Dragonfly.Graphics;

namespace DflOb = Dfl::Observer;

namespace Dfl{
    namespace Hardware{
        export struct SessionInfo{
            std::string_view AppName{ "Dragonfly App" };
            uint32_t         AppVersion{ 0 };

            bool             DoDebug{ false };
            bool             EnableOnscreenRendering{ true }; // if set to false, disables onscreen rendering globally
            bool             EnableRaytracing{ false }; // if set to false, disables raytracing globally
        };

        export enum class [[nodiscard]] SessionError{
            Success                 = 0,
            // errors
            VkInstanceCreationError = -0x4101,
            VkBadDriverError        = -0x4102,
            VkNoDevicesError        = -0x4201,
            VkNoLayersError         = -0x4301,
            VkNoExpectedLayersError = -0x4302,
            VkDebuggerCreationError = -0x4402,
        };

        export struct DeviceHandles {
            VkInstance       Instance;
            VkPhysicalDevice GPU;
        };

        struct Processor{
            uint32_t Count; // amount of processors in the machine
            uint64_t Speed; // speed in Hz
        };

        /*class DebugProcess : public DflOb::WindowProcess{
            DflOb::Window* pWindow{ nullptr };
        public:
            void operator () (DflOb::WindowProcessArgs& args);
            void Destroy();
        };*/

        export class Session{
            SessionInfo                   GeneralInfo;

            VkInstance                    VkInstance;
            VkDebugUtilsMessengerEXT      VkDbMessenger;

            Processor                     CPU;

            uint64_t                      Memory; // memory size in B

            std::vector<VkPhysicalDevice> Devices;

            //DebugProcess                  WindowDebugging;
        public:
            DFL_API               DFL_CALL Session(const SessionInfo& info);
            DFL_API               DFL_CALL ~Session();

            DFL_API SessionError  DFL_CALL InitSession();

            DFL_API uint64_t      DFL_CALL DeviceCount() const noexcept { return this->Devices.size(); };
                    DeviceHandles          Device(uint32_t index) const noexcept { if(index < this->Devices.size()) return { this->VkInstance, this->Devices[index] }; else return { this->VkInstance, nullptr };};

            friend 
                    SessionError           _LoadDevices(Session& session);
        };
    }
}

