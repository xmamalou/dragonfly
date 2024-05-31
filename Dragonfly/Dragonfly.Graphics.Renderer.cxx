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
#include "Dragonfly.hxx"

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

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
        throw DflGr::Renderer::Error::VkNoSurfaceFormatsError;
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
       throw DflGr::Renderer::Error::VkNoSurfacePresentModesError;
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

static inline VkExtent2D INT_MakeExtent(const std::array<uint32_t, 2>& resolution, const VkSurfaceCapabilitiesKHR& capabs) {
    VkExtent2D extent{};

    uint32_t minHeight{ max(capabs.minImageExtent.height, resolution[0]) };
    uint32_t minWidth{ max(capabs.minImageExtent.width, resolution[1]) };

    extent.height = min(minHeight, capabs.maxImageExtent.height);
    extent.width = min(minWidth, capabs.maxImageExtent.width);

    return extent;
};

static inline VkSwapchainKHR INT_CreateSwapchain(
    const VkDevice&                        device,
    const VkPhysicalDevice&                physDevice,
    const bool&                            doVsync,
    const VkSurfaceKHR&                    surface,
    const std::array< uint32_t, 2 >&       targetRes,
    const VkSwapchainKHR&                  oldSwapchain) 
{
    auto characteristics{ INT_GetCharacteristics(
                            physDevice,
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
        .oldSwapchain{ oldSwapchain }
    };

    VkSwapchainKHR swapchain{ nullptr };
    switch (auto error{ vkCreateSwapchainKHR(
                            device,
                            &swapchainInfo,
                            nullptr,
                            &swapchain) }) {
    case VK_SUCCESS:
        break;
    case VK_ERROR_SURFACE_LOST_KHR:
        throw Dfl::Error::HandleCreation(L"Window is not configured for Vulkan");
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        throw Dfl::Error::HandleCreation(L"Window is being used by another application");;
    default:
        throw Dfl::Error::HandleCreation(L"Unable to create swapchain for window");;
    }

    return swapchain;
};

static std::vector<VkImage> INT_GetImages() 
{
    
}

static VkCommandPool INT_GetCommandPool()
{

}

// Internal for constructor

static Dfl::Hardware::Device::Queue INT_GetQueue(
    const VkDevice&                                          device,
    const std::vector<Dfl::Hardware::Device::Queue::Family>& families,
          std::vector<uint32_t>&                             areFamiliesUsed,
          std::vector<uint32_t>&                             leastClaimedQueue) 
{
    VkQueue  queue{ nullptr };
    uint32_t familyIndex{ 0 };
    uint32_t amountOfClaimedGoal{ 0 };
    while (familyIndex <= families.size()) 
    {
        if (familyIndex == families.size()) 
        {
            amountOfClaimedGoal++;
            familyIndex = 0;
            continue;
        }

        if (!(families[familyIndex].QueueType & Dfl::Hardware::Device::Queue::Type::Graphics)) 
        {
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
        if (leastClaimedQueue[familyIndex] >= families[familyIndex].QueueCount) 
        {
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

    return { queue, familyIndex, leastClaimedQueue[familyIndex] - 1 };
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
        throw Dfl::Error::HandleCreation(L"Unable to create surface.");
    }

    return surface;
};

// 

DflGr::Renderer::Handles::Handles( const Info& info )
: hSurface( INT_GetSurface(
                info.AssocDevice.GetSession().GetInstance(),
                info.AssocDevice.GetPhysicalDevice(),
                info.AssocWindow.GetHandle()) ),
  AssignedQueue( INT_GetQueue(
                    info.AssocDevice.GetDevice(),
                    info.AssocDevice.GetQueueFamilies(),
                    info.AssocDevice.GetTracker().AreFamiliesUsed,
                    info.AssocDevice.GetTracker().LeastClaimedQueue) ),
  hSwapchain( INT_CreateSwapchain(
                info.AssocDevice.GetDevice(),
                info.AssocDevice.GetPhysicalDevice(),
                info.AssocWindow.pInfo->DoVsync,
                this->hSurface,
                info.AssocWindow.pInfo->Resolution,
                nullptr) ) {}

DflGr::Renderer::Renderer(const Info& info)
: pInfo(new Info(info)),
  Swapchain( Handles(info) ),
  pCharacteristics( new Characteristics( INT_GetCharacteristics(
                                            info.AssocDevice.GetPhysicalDevice(),
                                            this->Swapchain.hSurface,
                                            info.AssocWindow.pInfo->Resolution) ) ),
  QueueFence( this->pInfo->AssocDevice.GetFence(this->Swapchain.AssignedQueue.FamilyIndex,
                                                this->Swapchain.AssignedQueue.Index) )
{
}

DflGr::Renderer::~Renderer() {
    auto& device{ this->pInfo->AssocDevice };
    vkDeviceWaitIdle(device.GetDevice());

    if (this->Swapchain.hSwapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(
            device.GetDevice(),
            this->Swapchain.hSwapchain,
            nullptr);
    }
    if (this->Swapchain.hCmdPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(
            device.GetDevice(),
            this->Swapchain.hCmdPool,
            nullptr);
    }

    if (this->Swapchain.hSurface != nullptr) {
        vkDestroySurfaceKHR(
            device.GetSession().GetInstance(),
            this->Swapchain.hSurface,
            nullptr);
    }
}

void DflGr::Renderer::Cycle() {
    switch (this->CurrentState) 
    {
    case State::Initialize:
        printf("INITIALIZATION\n");
        this->CurrentState = State::Loop;
        break;
    case State::Loop:
        printf("");
        break;
    }

    if (!this->pInfo->AssocWindow.pInfo->DoVsync || !INT_DoesSupportMailbox(this->pCharacteristics->PresentModes)) 
    {
        std::this_thread::sleep_for(std::chrono::microseconds(1000000/this->pInfo->AssocWindow.pInfo->Rate)); 
    }
};
