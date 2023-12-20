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
#include <vector>
#include <mutex>

#include <vulkan/vulkan.h>

export module Dragonfly.Graphics.Renderer;

import Dragonfly.Observer.Window;
import Dragonfly.Hardware.Device;

namespace DflOb = Dfl::Observer;
namespace DflHW = Dfl::Hardware;

namespace Dfl {
    namespace Graphics {
        export struct RendererInfo {
            DflHW::Device* pAssocDevice{ nullptr };
            DflOb::Window* pAssocWindow{ nullptr };
        };

        export enum class [[ nodiscard ]] RendererError {
            Success                           = 0,
            // error
            NullHandleError                   = -0x1001,
            VkWindowNotAssociatedError        = -0x4601,
            VkSurfaceCreationError            = -0x4602,
            VkNoSurfaceFormatsError           = -0x4603,
            VkNoSurfacePresentModesError      = -0x4604,
            VkComPoolInitError                = -0x4702,
            VkSwapchainInitError              = -0x4801,
            VkSwapchainSurfaceLostError       = -0x4802,
            VkSwapchainWindowUnavailableError = -0x4803,
            // warnings
            ThreadNotReadyWarning             = 0x1001,
            VkAlreadyInitDeviceWarning        = 0x4201,
            VkNoRenderingRequestedWarning     = 0x4202,
            VkUnknownQueueTypeWarning         = 0x4701,
        };

        struct Swapchain {
            VkSwapchainKHR                  hSwapchain{ nullptr };
            std::vector<VkImage>            hSwapchainImages{ };

            VkSurfaceKHR                    hSurface{ nullptr };
            std::array< uint32_t, 2>        TargetRes{ 0, 0 };

            VkSurfaceCapabilitiesKHR        Capabilities;
            std::vector<VkSurfaceFormatKHR> Formats;
            std::vector<VkPresentModeKHR>   PresentModes;

            VkCommandPool                   hCmdPool{ nullptr };
            // ^ why is this here, even though command pools are per family? The reason is that command pools need to be
            // used only by the thread that created them. Hence, it is not safe to allocate one command pool per family, but rather
            // per thread.

                                            operator VkSwapchainKHR() { return this->hSwapchain; }
        };

        struct RenderData {
            std::mutex*              pMutex;
            uint32_t                 QueueFamilyIndex;
        };

        typedef void* (*RenderNode)(
                            const VkDevice&      device,
                                  RenderData&&   additionalData,
                                  Swapchain&     swapchain,
                                  RendererError& error);

        export class Renderer : public DflOb::WindowProcess {
            const std::unique_ptr<const RendererInfo>    pInfo;
            const std::unique_ptr<Swapchain>             pSwapchain;

                  RenderNode                             pRenderNode{ nullptr };
                  RendererError                          Error{ RendererError::ThreadNotReadyWarning };
        public:
            DFL_API                     DFL_CALL Renderer(const RendererInfo& info);
            DFL_API                     DFL_CALL ~Renderer();

                    const RendererError          GetErrorCode() const noexcept { return this->Error; }

        protected:
            void operator()(const DflOb::WindowProcessArgs& args);
            void Destroy();
        };
    }
}