/*
   Copyright 2024 Christopher-Marios Mamaloukas

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

#pragma once

#include <string>
#include <memory>
#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "Dragonfly.h"

#include "Dragonfly.Generics.hxx"

namespace Dfl { 
    // Dragonfly.Hardware
    namespace Hardware {
        // Dragonfly.Hardware.Session
        class Session {
        public:
            struct Info {
                const std::string_view AppName{ "Dragonfly App" };
                const uint32_t         AppVersion{ 0 };

                const bool             DoDebug{ false };
            };

            struct Handles {
                const VkInstance                    hInstance{ nullptr };
                const VkDebugUtilsMessengerEXT      hDbMessenger{ nullptr };
                const std::vector<VkPhysicalDevice> hDevices{};

                operator VkInstance() const { return this->hInstance; }
            };

            struct Characteristics {
                const struct Processor {
                    const uint32_t Count{ 0 }; // amount of processors in the machine
                    const uint64_t Speed{ 0 }; // speed in MHz
                }               CPU{ 0, 0 };
                const uint64_t  Memory{ 0 };
            };

        protected:
            const std::unique_ptr<const Info>            pInfo{ };
            const std::unique_ptr<const Characteristics> pCharacteristics{ };

            const Handles                                Instance{ };
        public:
            DFL_API DFL_CALL Session(const Info& info);
            DFL_API DFL_CALL ~Session();

            //

            const VkInstance       GetInstance() const noexcept { 
                                        return this->Instance; }
            const uint64_t         GetDeviceCount() const noexcept { 
                                        return this->Instance.hDevices.size(); }
            DFL_API 
            inline 
            const std::string     
                          DFL_CALL GetDeviceName(const uint32_t& index) const noexcept;
            const VkPhysicalDevice GetDeviceHandle(const uint32_t& index) const noexcept { 
                                        return index >= this->Instance.hDevices.size() 
                                                    ? nullptr 
                                                    : this->Instance.hDevices[index]; }
            const Characteristics& GetCharacteristics() {
                                        return *this->pCharacteristics; }
        };

        template< typename T > 
        concept SessionLike = requires (T a, uint32_t index)
        {
            typename T::Info;
            typename T::Handles;
            typename T::Characteristics;

            { a.GetInstance() } noexcept 
                -> Dfl::Generics::Equal<VkInstance>;
            { a.GetDeviceCount() } noexcept
                -> Dfl::Generics::Equal<uint64_t>;
            { a.GetDeviceHandle(index) } noexcept 
                -> Dfl::Generics::Equal<VkPhysicalDevice>;
        };
    }
    namespace DflHW = Dfl::Hardware;
}