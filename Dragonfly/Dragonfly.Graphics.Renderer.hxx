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

#include <vector>
#include <memory>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "Dragonfly.h"

#include "Dragonfly.Hardware.Device.hxx"

namespace Dfl {
    namespace UI { class Window; }

    // Dragonfly.Graphics
    namespace Graphics {
        // Dragonfly.Graphics.Renderer
        class Renderer {
        public:
            struct Info {
                      DflHW::Device&   AssocDevice;
                const Dfl::UI::Window& AssocWindow;

                bool                   DoVsync{ true };
                uint32_t               Rate{ 60 };
            };

            struct Handles {
                const VkSurfaceKHR                    hSurface{ nullptr };
                const DflHW::Device::Queue            AssignedQueue{ };

                const VkSwapchainKHR                  hSwapchain{ nullptr };
                const std::vector<VkImage>            hSwapchainImages{ };
                const VkCommandPool                   hCmdPool{ nullptr };
                // ^ why is this here, even though command pools are per family? The reason is that command pools need to be
                // used only by the thread that created them. Hence, it is not safe to allocate one command pool per family, but rather
                // per thread.

                operator VkSwapchainKHR() { return this->hSwapchain; }
            };

            struct Characteristics {
                const std::array< uint32_t, 2>        TargetRes{ 0, 0 };

                const VkSurfaceCapabilitiesKHR        Capabilities{ 0 };
                const std::vector<VkSurfaceFormatKHR> Formats;
                const std::vector<VkPresentModeKHR>   PresentModes;
            };

            enum class State {
                Initialize,
                Loop,
                Fail
            };

            DFL_API static constexpr uint32_t   DefaultRate{ 60 };

        protected:
            const std::shared_ptr<const Info>            pInfo;
            const Handles                                Swapchain;
            const std::shared_ptr<const Characteristics> pCharacteristics;
            const VkFence                                QueueFence{ nullptr };

                  State                                  CurrentState{ State::Initialize };
        public:
            DFL_API DFL_CALL Renderer(const Info& info);
            DFL_API DFL_CALL Renderer(Renderer&& oldRenderer);

            DFL_API DFL_CALL ~Renderer();

            DFL_API       
            void  
            DFL_CALL Cycle();
        };
    }
    namespace DflGr = Dfl::Graphics;
}