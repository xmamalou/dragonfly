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

#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vulkan/vulkan.h>

export module Dragonfly.Hardware.Session;

import Dragonfly.Generics;
import Dragonfly.Observer;
//import Dragonfly.Graphics;

namespace DflOb = Dfl::Observer;

namespace Dfl { namespace Graphics { class Renderer; } }

namespace Dfl{
    namespace Hardware{
        export struct SessionInfo{
            const std::string_view AppName{ "Dragonfly App" };
            const uint32_t         AppVersion{ 0 };

            const bool             DoDebug{ false };
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

        struct Processor{
            const uint32_t Count; // amount of processors in the machine
            const uint64_t Speed; // speed in Hz
            // ----
            Processor(uint32_t count, uint64_t speed) : Count(count), Speed(speed) {};
        };

        /*class DebugProcess : public DflOb::WindowProcess{
            DflOb::Window* pWindow{ nullptr };
        public:
            void operator () (DflOb::WindowProcessArgs& args);
            void Destroy();
        };*/

        struct SessionHandles {
            VkInstance                    hInstance{ nullptr };
            VkDebugUtilsMessengerEXT      hDbMessenger{ nullptr };
            std::vector<VkPhysicalDevice> hDevices{}; 

                                          operator VkInstance() const { return this->hInstance; }
        };

        export class Session{
            const std::unique_ptr<const SessionInfo> GeneralInfo{ };

            const SessionHandles                     Instance{ };

            const Processor                          CPU{ 0, 0 };
            const uint64_t                           Memory{ 0 }; // memory size in B
                  SessionError                       Error{ SessionError::Success };
        public:
            DFL_API                       DFL_CALL Session(const SessionInfo& info);
            DFL_API                       DFL_CALL ~Session();

                    const SessionError             GetErrorCode() const noexcept { return this->Error; }
                    
                    const SessionInfo              GetInfo() const noexcept { return *this->GeneralInfo; }
                    const Processor                GetCPU() const noexcept { return this->CPU; }
                    const uint64_t                 GetMemory() const noexcept { return this->Memory; }
                    const uint64_t                 GetDeviceCount() const noexcept { return this->Instance.hDevices.size(); }

            friend class
                          Device;

            friend class
                          Dfl::Graphics::Renderer;
        };
    }
}

