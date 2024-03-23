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
        export class Session{
        public:
            struct Info {
                const std::string_view AppName{ "Dragonfly App" };
                const uint32_t         AppVersion{ 0 };

                const bool             DoDebug{ false };
            };

            enum class [[nodiscard]] Error {
                Success = 0,
                // errors
                VkInstanceCreationError = -0x4101,
                VkBadDriverError = -0x4102,
                VkNoDevicesError = -0x4201,
                VkNoLayersError = -0x4301,
                VkNoExpectedLayersError = -0x4302,
                VkDebuggerCreationError = -0x4402,
            };

            struct Processor {
                const uint32_t Count{ 0 }; // amount of processors in the machine
                const uint64_t Speed{ 0 }; // speed in MHz
            };

            struct Handles {
                VkInstance                    hInstance{ nullptr };
                VkDebugUtilsMessengerEXT      hDbMessenger{ nullptr };
                std::vector<VkPhysicalDevice> hDevices{};

                operator VkInstance() const { return this->hInstance; }
            };

        private:
            const std::unique_ptr<const Info> GeneralInfo{ };

            const Handles                     Instance{ };

            const Processor                          CPU{ 0, 0 };
            const uint64_t                           Memory{ 0 }; // memory size in MB
                  Error                              ErrorCode{ Error::Success };

        public:
            DFL_API                          DFL_CALL Session(const Info& info);
            DFL_API                          DFL_CALL ~Session();

                           const Error                GetErrorCode() const noexcept { return this->ErrorCode; }
                    
                           const Processor            GetCPU() const noexcept { return this->CPU; }
                           const uint64_t             GetMemory() const noexcept { return this->Memory; }
                           const uint64_t             GetDeviceCount() const noexcept { return this->Instance.hDevices.size(); }
            DFL_API inline const std::string DFL_CALL GetDeviceName(const uint32_t& index) const noexcept;

            friend class
                          Device;

            friend class
                          Dfl::Graphics::Renderer;
        };
    }
}

