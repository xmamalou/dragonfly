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
#include <vector>
#include <thread>
#include <chrono>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>


module Dragonfly.Graphics.Renderer;

import Dragonfly.Generics;
import Dragonfly.Observer.Window;

namespace DflHW = Dfl::Hardware;
namespace DflGr = Dfl::Graphics;
namespace DflOb = Dfl::Observer;

// INTERNAL FOR INIT NODE

static inline bool INT_DoesSupportSRGB(VkColorSpaceKHR& colorSpace, const std::vector<VkSurfaceFormatKHR>& formats) {
    for (auto& format : formats) {
        colorSpace = format.colorSpace;
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB) {
            return true;
        }
    }

    VkColorSpaceKHR test = (colorSpace = formats[0].colorSpace);

    return false;
};

static inline bool INT_DoesSupportMailbox(const std::vector<VkPresentModeKHR>& modes) {
    for (auto& mode : modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return true;
        }
    }

    return false;
};

static inline VkExtent2D INT_MakeExtent(const std::array<uint32_t, 2>& resolution, const VkSurfaceCapabilitiesKHR& capabs) {
    VkExtent2D extent{};

    uint32_t minHeight{ max(capabs.minImageExtent.height, resolution[0]) };
    uint32_t minWidth{ max(capabs.minImageExtent.width, resolution[1]) };

    extent.height = min(minHeight, capabs.maxImageExtent.height);
    extent.width = min(minWidth, capabs.maxImageExtent.width);

    return extent;
};

static inline DflGr::RendererError INT_CreateSwapchain(
    const VkDevice&                        device,
    const bool&                            doVsync,
    const VkSurfaceKHR&                    surface,
    const std::array< uint32_t, 2 >&       targetRes,
    const VkSurfaceCapabilitiesKHR&        capabs,
    const std::vector<VkSurfaceFormatKHR>& formats,
    const std::vector<VkPresentModeKHR>&   modes,
    const VkSwapchainKHR&                  oldSwapchain,
          VkSwapchainKHR&                  newSwapchain) {
    VkColorSpaceKHR colorSpace;
    const VkSwapchainCreateInfoKHR swapchainInfo = {
        .sType{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR },
        .pNext{ nullptr },
        .flags{ 0 },
        .surface{ surface },
        .minImageCount{ capabs.minImageCount + 1 },
        .imageFormat{ INT_DoesSupportSRGB(colorSpace, formats) ?
                        VK_FORMAT_B8G8R8A8_SRGB 
                      : formats[0].format },
        .imageColorSpace{ colorSpace },
        .imageExtent{ INT_MakeExtent(targetRes, capabs) },
        .imageArrayLayers{ 1 },
        .imageUsage{ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT },
        .imageSharingMode{ VK_SHARING_MODE_EXCLUSIVE },
        .queueFamilyIndexCount{ 0 },
        .pQueueFamilyIndices{ nullptr },
        .preTransform{ capabs.currentTransform },
        .compositeAlpha{ VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR },
        .presentMode{ INT_DoesSupportMailbox(modes) && doVsync ?
                        VK_PRESENT_MODE_MAILBOX_KHR 
                      : VK_PRESENT_MODE_FIFO_KHR },
        .clipped{ VK_TRUE },
        .oldSwapchain{ oldSwapchain }
    };

    switch (auto error{ vkCreateSwapchainKHR(
                            device,
                            &swapchainInfo,
                            nullptr,
                            &newSwapchain) }) {
    case VK_SUCCESS:
        break;
    case VK_ERROR_SURFACE_LOST_KHR:
        return DflGr::RendererError::VkSwapchainSurfaceLostError;
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return DflGr::RendererError::VkSwapchainWindowUnavailableError;
    default:
        return DflGr::RendererError::VkSwapchainInitError;
    }

    return DflGr::RendererError::Success;
};

//

static inline void INT_FailNode(
                const VkDevice&             pDevice,
                      DflGr::Swapchain&     swapchain,
                      DflGr::RendererError& error) {
    std::cout << "";
};

static inline void INT_LoopNode(
                  const VkDevice&             pDevice,
                        DflGr::Swapchain&     swapchain,
                        DflGr::RendererError& error) {
    std::cout << "";
};

static inline void* INT_InitNode(
                  const VkDevice&             device,
                  const bool&                 doVsync,
                        DflGr::Swapchain&     swapchain,
                        DflGr::RendererError& error) {
    const VkCommandPoolCreateInfo cmdPoolInfo = {
        .sType{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO },
        .pNext{ nullptr},
        .flags{ VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT },
        .queueFamilyIndex{ swapchain.AssignedQueue.FamilyIndex }
    };

    if (vkCreateCommandPool(
        device,
        &cmdPoolInfo,
        nullptr,
        &swapchain.hCmdPool) != VK_SUCCESS) {
        error = DflGr::RendererError::VkComPoolInitError;
        return INT_FailNode;
    }
    //this->pMutex->unlock();

    VkSwapchainKHR newSwapchain{ nullptr };
    if ((error = INT_CreateSwapchain(
                    device,
                    doVsync,
                    swapchain.hSurface,
                    swapchain.TargetRes,
                    swapchain.Capabilities,
                    swapchain.Formats,
                    swapchain.PresentModes,
                    swapchain.hSwapchain,
                    newSwapchain)) != DflGr::RendererError::Success) {
        return INT_FailNode;
    }
    swapchain.hSwapchain = newSwapchain;

    uint32_t imageCount{ 0 };
    vkGetSwapchainImagesKHR(
        device,
        swapchain,
        &imageCount,
        nullptr);
    swapchain.hSwapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(
        device,
        swapchain,
        &imageCount,
        swapchain.hSwapchainImages.data());
};

//

// Internal for constructor

static Dfl::Hardware::Queue INT_GetQueue(
    const VkDevice&                                device,
    const std::vector<Dfl::Hardware::QueueFamily>& families,
          std::vector<uint32_t>&                   areFamiliesUsed,
          std::vector<uint32_t>&                   leastClaimedQueue) {
    VkQueue  queue{ nullptr };
    uint32_t familyIndex{ 0 };
    uint32_t amountOfClaimedGoal{ 0 };
    while (familyIndex <= families.size()) {
        if (familyIndex == families.size()) {
            amountOfClaimedGoal++;
            familyIndex = 0;
            continue;
        }

        if (!(families[familyIndex].QueueType & Dfl::Hardware::QueueType::Graphics)) {
            familyIndex++;
            continue;
        }

        /*
        * The way Dragonfly searches for queues is this:
        * If it finds a queue family of the type it wants, it first
        * checks what value pFamilyQueue[i], where i the family index,
        * has. This value is, essentially, the index of the queue that has
        * been claimed the least. If the value exceeds (or is equal to)
        * the number of queues in the family, it resets it to 0.
        * This is a quick way to ensure that work is distributed evenly
        * between queues and that no queue is overworked.
        * 
        * Note that this does not check whether the queues of the appropriate
        * type indeed exist, since this condition should be verified already
        * from device creation.
        */
        if (leastClaimedQueue[familyIndex] >= families[familyIndex].QueueCount) {
            leastClaimedQueue[familyIndex] = 0;
        }

        vkGetDeviceQueue(
            device,
            familyIndex,
            leastClaimedQueue[familyIndex],
            &queue
        );
        leastClaimedQueue[familyIndex]++;
        break;
    }

    areFamiliesUsed[familyIndex]++;

    return { queue, familyIndex };
};

static DflGr::Swapchain INT_GetRenderResources(
    const VkInstance&                              instance,
    const VkPhysicalDevice&                        hPhysDevice,
    const VkDevice&                                hDevice,
    const HWND&                                    hWindow,
    const std::array< uint32_t, 2>&                resolution,
    const std::vector<Dfl::Hardware::QueueFamily>& families,
          std::vector<uint32_t>&                   leastClaimedQueue,
          std::vector<uint32_t>&                   areFamiliesUsed) {
    if (hPhysDevice == nullptr)
        throw DflGr::RendererError::NullHandleError;

    VkSurfaceKHR surface;
    if (hWindow == nullptr) {
        throw DflGr::RendererError::NullHandleError;
    }
    const VkWin32SurfaceCreateInfoKHR surfaceinfo = {
        .sType{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR },
        .pNext{ nullptr },
        .flags{ 0 },
        .hwnd{ hWindow }
    };
    if (vkCreateWin32SurfaceKHR(
            instance,
            &surfaceinfo, nullptr,
            &surface) != VK_SUCCESS) {
        throw DflGr::RendererError::VkSurfaceCreationError;
    }

    VkSurfaceCapabilitiesKHR capabs;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        hPhysDevice,
        surface,
        &capabs);

    uint32_t count{ 0 };
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        hPhysDevice,
        surface,
        &count,
        nullptr);
    if (count == 0)
        throw DflGr::RendererError::VkNoSurfaceFormatsError;
    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        hPhysDevice,
        surface,
        &count,
        formats.data());

    count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        hPhysDevice,
        surface,
        &count,
        nullptr);
    if (count == 0)
       throw DflGr::RendererError::VkNoSurfacePresentModesError;
    std::vector<VkPresentModeKHR> modes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        hPhysDevice,
        surface,
        &count,
        modes.data());


    return {
    nullptr, {},
    surface, resolution, capabs, formats, modes, INT_DoesSupportMailbox(modes),
    nullptr, INT_GetQueue(
                hDevice,
                families,
                areFamiliesUsed,
                leastClaimedQueue)
    };
};

// 

DflGr::Renderer::Renderer(const RendererInfo& info)
try : pInfo( new RendererInfo(info) ),
      pSwapchain(new Swapchain( INT_GetRenderResources(
                                    info.pAssocDevice->pInfo->pSession->Instance,
                                    info.pAssocDevice->hPhysicalDevice,
                                    info.pAssocDevice->GPU,
                                    info.pAssocWindow->GetHandle(),
                                    info.pAssocWindow->pInfo->Resolution,
                                    info.pAssocDevice->GPU.Families, 
                                    info.pAssocDevice->pTracker->LeastClaimedQueue,
                                    info.pAssocDevice->pTracker->AreFamiliesUsed) ) ) {
    
}
catch (DflGr::RendererError e) { this->Error = e; }

DflGr::Renderer::~Renderer() {
    auto& pDevice = this->pInfo->pAssocDevice;
    vkDeviceWaitIdle(pDevice->GPU);
    pDevice->pUsageMutex->lock();
    if (this->pSwapchain->hSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(
            pDevice->GPU,
            *this->pSwapchain,
            nullptr);
    }
    if (this->pSwapchain->hCmdPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(
            pDevice->GPU,
            this->pSwapchain->hCmdPool,
            nullptr);
    }

    if (this->pSwapchain->hSurface != nullptr) {
        vkDestroySurfaceKHR(
            this->pInfo->pAssocDevice->pInfo->pSession->Instance,
            this->pSwapchain->hSurface,
            nullptr);
    }
    pDevice->pUsageMutex->unlock();
}

void DflGr::Renderer::Cycle() {
    switch (this->State) {
    case RenderState::Initialize:
        INT_InitNode(
            this->pInfo->pAssocDevice->GPU,
            this->pInfo->pAssocWindow->pInfo->DoVsync,
            *this->pSwapchain,
            this->Error);
        this->State = ( this->Error < RendererError::Success ) ? RenderState::Fail : RenderState::Loop;
        break;
    case RenderState::Loop:
        printf("");
        break;
    case RenderState::Fail:
        printf("FAILURE\n");
        break;
    }
    if (!this->pInfo->pAssocWindow->pInfo->DoVsync || !this->pSwapchain->SupportsMailbox) {
        std::this_thread::sleep_for(std::chrono::microseconds(1000000/this->pInfo->pAssocWindow->pInfo->Rate)); }
};
