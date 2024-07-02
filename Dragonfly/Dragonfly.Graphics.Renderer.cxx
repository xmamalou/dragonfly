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
#include "Dragonfly.Graphics.Renderer.hxx"

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "Dragonfly.Hardware.Session.hxx"
#include "Dragonfly.UI.Window.hxx"

namespace DflHW = Dfl::Hardware;
namespace DflGr = Dfl::Graphics;
namespace DflUI = Dfl::UI;

static DflGr::Renderer::Characteristics INT_GetCharacteristics(
    const VkPhysicalDevice&         hPhysDevice,
    const VkSurfaceKHR&             surface,
    const std::array< uint32_t, 2>& resolution) 
{
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
    {
        throw Dfl::Error::NoData(
                L"Unable to get formats of window",
                L"INT_GetCharacteristics");
    }
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
    { 
       throw Dfl::Error::NoData(
                L"Unable to get present modes of window",
                L"INT_GetCharacteristics");
    }
    std::vector<VkPresentModeKHR> modes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        hPhysDevice,
        surface,
        &count,
        modes.data());

    return { resolution, capabs, formats, modes };
}

static inline bool INT_DoesSupportSRGB(
          VkColorSpaceKHR&                 colorSpace, 
    const std::vector<VkSurfaceFormatKHR>& formats) 
{
    for (auto& format : formats) {
        colorSpace = format.colorSpace;
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB) {
            return true;
        }
    }

    return false;
};

static inline bool INT_DoesSupportMailbox(const std::vector<VkPresentModeKHR>& modes) 
{
    for (auto& mode : modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return true;
        }
    }

    return false;
};

static inline VkExtent2D INT_MakeExtent(
    const std::array<uint32_t, 2>&  resolution, 
    const VkSurfaceCapabilitiesKHR& capabs) 
{
    VkExtent2D extent{};

    uint32_t minHeight{ max(capabs.minImageExtent.height, resolution[0]) };
    uint32_t minWidth{ max(capabs.minImageExtent.width, resolution[1]) };

    extent.height = min(minHeight, capabs.maxImageExtent.height);
    extent.width = min(minWidth, capabs.maxImageExtent.width);

    return extent;
};

static inline VkSurfaceKHR INT_GetSurface(
    const VkInstance&                                        hInstance,
    const VkPhysicalDevice&                                  hPhysDevice,
    const HWND&                                              hWindow) 
{
    VkSurfaceKHR surface;
    const VkWin32SurfaceCreateInfoKHR surfaceinfo = {
        .sType{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR },
        .pNext{ nullptr },
        .flags{ 0 },
        .hwnd{ hWindow }
    };
    if (vkCreateWin32SurfaceKHR(
            hInstance,
            &surfaceinfo, nullptr,
            &surface) != VK_SUCCESS) {
        throw Dfl::Error::HandleCreation(
                L"Unable to create surface.",
                L"INT_GetSurface");
    }

    return surface;
};

static inline VkSwapchainKHR INT_GetSwapchain(
    const VkDevice&                        hDevice,
    const VkPhysicalDevice&                hPhysDevice,
    const bool&                            doVsync,
    const VkSurfaceKHR&                    surface,
    const std::array< uint32_t, 2 >&       targetRes,
    const VkSwapchainKHR&                  hOldSwapchain) 
{
    auto characteristics{ INT_GetCharacteristics(
                            hPhysDevice,
                            surface,
                            targetRes) };

    VkColorSpaceKHR colorSpace;
    const VkSwapchainCreateInfoKHR swapchainInfo = {
        .sType{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR },
        .pNext{ nullptr },
        .flags{ 0 },
        .surface{ surface },
        .minImageCount{ characteristics.Capabilities.minImageCount + 1 },
        .imageFormat{ INT_DoesSupportSRGB(
                        colorSpace, 
                        characteristics.Formats) 
                      ? VK_FORMAT_B8G8R8A8_SRGB 
                      : characteristics.Formats[0].format },
        .imageColorSpace{ colorSpace },
        .imageExtent{ INT_MakeExtent(
                        targetRes, 
                        characteristics.Capabilities) },
        .imageArrayLayers{ 1 },
        .imageUsage{ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT },
        .imageSharingMode{ VK_SHARING_MODE_EXCLUSIVE },
        .queueFamilyIndexCount{ 0 },
        .pQueueFamilyIndices{ nullptr },
        .preTransform{ characteristics.Capabilities.currentTransform },
        .compositeAlpha{ VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR },
        .presentMode{ INT_DoesSupportMailbox(characteristics.PresentModes) && doVsync
                      ? VK_PRESENT_MODE_MAILBOX_KHR 
                      : VK_PRESENT_MODE_FIFO_KHR },
        .clipped{ VK_TRUE },
        .oldSwapchain{ hOldSwapchain }
    };

    VkSwapchainKHR swapchain{ nullptr };
    switch (auto error{ vkCreateSwapchainKHR(
                            hDevice,
                            &swapchainInfo,
                            nullptr,
                            &swapchain) }) {
    case VK_SUCCESS:
        break;
    case VK_ERROR_SURFACE_LOST_KHR:
        throw Dfl::Error::HandleCreation(
                L"Window is not configured for Vulkan",
                L"INT_GetSwapchain");
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        throw Dfl::Error::HandleCreation(
                L"Window is being used by another application",
                L"INT_GetSwapchain");;
    default:
        throw Dfl::Error::HandleCreation(
                L"Unable to create swapchain for window",
                L"INT_GetSwapchain");;
    }

    return swapchain;
};

static VkCommandPool INT_GetCommandPool()
{

}

using DflQueueFams = Dfl::Hardware::Device::Queue::Family;

static DflGr::Renderer::Handles INT_GetHandles(
          DflHW::Device&             gpu,
    const HWND&                      hWindow,
    const bool&                      doVsync,
    const std::array< uint32_t, 2 >& targetRes,
    const VkSurfaceKHR&              oldSurface,
    const std::optional<
            DflHW::Device::Queue >&  oldQueue,
    const VkSwapchainKHR&            oldSwapchain,
    const VkCommandPool&             oldCommandPool) 
{
    auto surface{ oldSurface == nullptr 
                    ? INT_GetSurface(
                        gpu.GetSession().GetInstance(),
                        gpu.GetPhysicalDevice(),
                        hWindow)
                    : oldSurface };

    auto queue{ !oldQueue.has_value() 
                   ? gpu.BorrowQueue(DflHW::Device::Queue::Type::Graphics)
                   : oldQueue.value() };

    VkSwapchainKHR       swapchain{ nullptr };
    std::vector<VkImage> swapImages;
    VkCommandPool        comPool{ oldCommandPool == nullptr 
                                    ? nullptr
                                    : oldCommandPool };
    try {
        swapchain = INT_GetSwapchain(
                        gpu.GetDevice(),
                        gpu.GetPhysicalDevice(),
                        doVsync,
                        surface,
                        targetRes,
                        oldSwapchain);

        uint32_t swapImageCount{ 0 };
        vkGetSwapchainImagesKHR(
            gpu.GetDevice(),
            swapchain,
            &swapImageCount,
            nullptr);
        if (swapImageCount == 0) {
            throw Dfl::Error::NoData(
                    L"Unable to gather swapchain images",
                    L"INT_GetSwapchain");
        }
        swapImages.resize(swapImageCount);
        vkGetSwapchainImagesKHR(
            gpu.GetDevice(),
            swapchain,
            &swapImageCount,
            swapImages.data());
    }
    catch (Dfl::Error::Generic& err) {
        vkDestroySurfaceKHR(
            gpu.GetSession().GetInstance(),
            surface,
            nullptr);

        if (swapchain != nullptr) {
            vkDestroySwapchainKHR(
                gpu.GetDevice(),
                swapchain,
                nullptr);
        }

        throw;
    }

    return { surface, queue, swapchain, swapImages, comPool };
}

// Internal for constructor

DflGr::Renderer::Renderer(const Info& info)
: pInfo(new Info(info)),
  Swapchain( INT_GetHandles(
               info.AssocDevice,
               info.AssocWindow.GetHandle(),
               info.DoVsync,
               info.AssocWindow.GetRectangle<DflUI::Window::Rectangle::Resolution>(),
               nullptr,
               std::nullopt,
               nullptr,
               nullptr) ),
  pCharacteristics( new Characteristics( INT_GetCharacteristics(
                                            info.AssocDevice.GetPhysicalDevice(),
                                            this->Swapchain.hSurface,
                                            info.AssocWindow.GetRectangle<DflUI::Window::Rectangle::Resolution>()) ) ),
  QueueFence( this->pInfo->AssocDevice.GetFence(this->Swapchain.AssignedQueue.FamilyIndex,
                                                   this->Swapchain.AssignedQueue.Index) )
{
}

DflGr::Renderer::Renderer(Renderer&& oldRenderer) 
: pInfo( oldRenderer.pInfo.get() ),
  Swapchain( INT_GetHandles(
               this->pInfo->AssocDevice,
               this->pInfo->AssocWindow.GetHandle(),
               this->pInfo->DoVsync,
               this->pInfo->AssocWindow.GetRectangle<DflUI::Window::Rectangle::Resolution>(),
               oldRenderer.Swapchain.hSurface,
               oldRenderer.Swapchain.AssignedQueue,
               oldRenderer.Swapchain.hSwapchain,
               oldRenderer.Swapchain.hCmdPool) ),
  pCharacteristics( oldRenderer.pCharacteristics.get() ),
  QueueFence( oldRenderer.QueueFence )
{
}

DflGr::Renderer::~Renderer() {
    auto& device{ this->pInfo->AssocDevice };
    vkDeviceWaitIdle(device.GetDevice());

    vkDestroySwapchainKHR(
        device.GetDevice(),
        this->Swapchain.hSwapchain,
        nullptr);

    vkDestroyCommandPool(
        device.GetDevice(),
        this->Swapchain.hCmdPool,
        nullptr);

    vkDestroySurfaceKHR(
        device.GetSession().GetInstance(),
        this->Swapchain.hSurface,
        nullptr);

    this->pInfo->AssocDevice.ReturnQueue(this->Swapchain.AssignedQueue);
}

void DflGr::Renderer::Cycle() 
{
    switch (this->CurrentState) 
    {
    case State::Initialize:
        printf("INITIALIZATION\n");
        this->CurrentState = State::Loop;
        break;
    case State::Loop:
        printf("");
        break;
    default:
        break;
    }

    if (!this->pInfo->DoVsync || !INT_DoesSupportMailbox(this->pCharacteristics->PresentModes)) 
    {
        std::this_thread::sleep_for(std::chrono::microseconds(1000000/this->pInfo->Rate)); 
    }
};
