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

#include <stdlib.h>

#include "../Internal.h"

#ifdef _WIN32
    #include <Windows.h>
    #include <uxtheme.h>
    #include <dwmapi.h>
    #pragma comment (lib, "Dwmapi")
    #define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include <GLFW/glfw3native.h>

/* -------------------- *
 *   INTERNAL           *
 * -------------------- */

// make the window
static GLFWwindow* _dflWindowCall(struct DflVec2D dim, const char* name, int mode, int* error);
static GLFWwindow* _dflWindowCall(struct DflVec2D dim, const char* name, int mode, int* error)
{
    /*
        The following are default values.
        Dragonfly checks if they aren't set and sets them to default values.

        TODO: Make them "smarter" (specifically the resolution ones), so that they can be set to the monitor's resolution.
    */

    if (name == NULL)
        name = "Dragonfly-App";

    if (dim.x == NULL || dim.y == NULL)
        dim = (struct DflVec2D){ 1920, 1080 };

    if (!glfwInit())
    {
        *error = DFL_GLFW_API_INIT_ERROR;
        return NULL;
    }

    switch (mode) {
    case DFL_WINDOWED:
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // There is no sane way I can think of to make a callback for remaking the swapchain
        //without touching platform specific window APIs, so I'm disabling the ability to resize by dragging.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        return glfwCreateWindow(dim.x, dim.y, name, NULL, NULL);
    case DFL_FULLSCREEN:
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        return glfwCreateWindow(dim.x, dim.y, name, glfwGetPrimaryMonitor(), NULL);
    case DFL_BORDERLESS:
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        return glfwCreateWindow(dim.x, dim.y, name, NULL, NULL);
    default:
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        return glfwCreateWindow(dim.x, dim.y, name, NULL, NULL);
    }
}

static int _dflWindowBindToSession(DflWindow hWindow, DflSession hSession);
static int _dflWindowBindToSession(DflWindow hWindow, DflSession hSession)
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(DFL_HANDLE(Session)->instance, DFL_HANDLE(Window)->handle, NULL, &surface) != VK_SUCCESS)
        return DFL_VULKAN_SURFACE_ERROR;
    DFL_HANDLE(Session)->surface = surface; // This exists simply for the sake of the _dflSessionDeviceQueuesGet function.
    DFL_HANDLE(Window)->surface = surface;

    return DFL_SUCCESS;
}

static VkSwapchainCreateInfoKHR _dflWindowSwapchainCreateInfoGet(DflWindow hWindow, DflDevice hDevice);
static VkSwapchainCreateInfoKHR _dflWindowSwapchainCreateInfoGet(DflWindow hWindow, DflDevice hDevice)
{
    if(DFL_HANDLE(Window)->info.layers == NULL)
        DFL_HANDLE(Window)->info.layers = 1;

    VkSwapchainCreateInfoKHR swapInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = DFL_HANDLE(Window)->surface,
        .flags = NULL,
        .minImageCount = DFL_HANDLE(Window)->imageCount,
        .imageExtent = { DFL_HANDLE(Window)->info.dim.x, DFL_HANDLE(Window)->info.dim.y },
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &DFL_HANDLE(Device)->queues.index[DFL_QUEUE_TYPE_PRESENTATION],
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .oldSwapchain = DFL_HANDLE(Window)->swapchain,
        .imageFormat = DFL_HANDLE(Window)->info.colorFormat,
        .imageColorSpace = DFL_HANDLE(Window)->colorSpace,
        .imageArrayLayers = DFL_HANDLE(Window)->info.layers,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .clipped = VK_TRUE,
    };

    if ((DFL_HANDLE(Window)->info.rate == 0) || (DFL_HANDLE(Window)->info.vsync == false))
        swapInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // unlimited fps
    else
        swapInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    // borderless windows will do alpha composition
    if (DFL_HANDLE(Window)->info.mode == DFL_BORDERLESS)
        swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    else
        swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    return swapInfo;
}

static void _dflWindowSwapchainImagesGet(DflWindow hWindow);
void        _dflWindowSwapchainImagesGet(DflWindow hWindow)
{
    vkGetSwapchainImagesKHR(((struct DflDevice_T*)DFL_WINDOW->device)->device, DFL_WINDOW->swapchain, &DFL_WINDOW->imageCount, NULL);
    if(DFL_WINDOW->images == NULL)
        DFL_WINDOW->images = malloc(sizeof(VkImage) * DFL_WINDOW->imageCount);
    else
    {
        void* tmp = realloc(DFL_WINDOW->images, sizeof(VkImage) * DFL_WINDOW->imageCount);
        DFL_WINDOW->images = (VkImage*)tmp;
    }

    if (DFL_WINDOW->images == NULL)
    {
        DFL_WINDOW->error = DFL_GENERIC_OOM_ERROR;
        return;
    }
    vkGetSwapchainImagesKHR(((struct DflDevice_T*)DFL_WINDOW->device)->device, DFL_WINDOW->swapchain, &DFL_WINDOW->imageCount, DFL_WINDOW->images);

    if (DFL_WINDOW->imageViews == NULL)
        DFL_WINDOW->imageViews = calloc(DFL_WINDOW->imageCount, sizeof(VkImageView));
    if (DFL_WINDOW->imageViews == NULL)
    {
        DFL_WINDOW->error = DFL_GENERIC_OOM_ERROR;
        return;
    }
}

static void _dflWindowSwapchainImageViewsCreate(DflWindow hWindow);
void        _dflWindowSwapchainImageViewsCreate(DflWindow hWindow)
{
    for (int i = 0; i < DFL_WINDOW->imageCount; i++)
    {
        VkImageViewCreateInfo imageViewInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = DFL_WINDOW->images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = DFL_WINDOW->info.colorFormat,
            .components = {
                /*
                * the swizzling component in the window's info is a bitfield
                * whose first bit says whether the red channel is used, the 
                * second whether the green channel is used, the third whether
                * the blue channel is used, and the fourth whether the alpha
                * channel is used. 
                * 
                * If the bit is set (= 1), then (1 & 1) = 1, so the channel is used. 
                * If the bit is not set (= 0), then (0 & 1) = 0, so the channel is not used.
                * 
                * Since, however, Vulkan uses the opposite convention, we need to use conditionals to swap the values.
                */
                .r = (DFL_WINDOW->info.swizzling & VK_COMPONENT_SWIZZLE_ZERO) ? VK_COMPONENT_SWIZZLE_IDENTITY : VK_COMPONENT_SWIZZLE_ZERO,
                .g = ((DFL_WINDOW->info.swizzling >> 1) & VK_COMPONENT_SWIZZLE_ZERO) ? VK_COMPONENT_SWIZZLE_IDENTITY : VK_COMPONENT_SWIZZLE_ZERO,
                .b = ((DFL_WINDOW->info.swizzling >> 2) & VK_COMPONENT_SWIZZLE_ZERO) ? VK_COMPONENT_SWIZZLE_IDENTITY : VK_COMPONENT_SWIZZLE_ZERO,
                .a = ((DFL_WINDOW->info.swizzling >> 3) & VK_COMPONENT_SWIZZLE_ZERO) ? VK_COMPONENT_SWIZZLE_IDENTITY : VK_COMPONENT_SWIZZLE_ZERO,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .layerCount = DFL_WINDOW->info.layers,
                .baseArrayLayer = 0,
            }
        };

        if (vkCreateImageView(((struct DflDevice_T*)DFL_WINDOW->device)->device, &imageViewInfo, NULL, &DFL_WINDOW->imageViews[i]) != VK_SUCCESS)
        {
            DFL_WINDOW->error = DFL_VULKAN_SURFACE_NO_VIEWS_ERROR;
            return;
        }
    }
}

static void _dflWindowSwapchainRecreate(DflWindow hWindow);
void        _dflWindowSwapchainRecreate(DflWindow hWindow)
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(((struct DflDevice_T*)DFL_WINDOW->device)->physDevice, DFL_WINDOW->surface, &surfaceCaps);

    // the below make sure that the resolution is within the bounds that are supported
    if ((DFL_WINDOW->info.dim.x > surfaceCaps.minImageExtent.width) || (DFL_WINDOW->info.dim.x == NULL))
        DFL_WINDOW->info.dim.x = surfaceCaps.minImageExtent.width;
    if ((DFL_WINDOW->info.dim.y > surfaceCaps.minImageExtent.height) || (DFL_WINDOW->info.dim.y == NULL))
        DFL_WINDOW->info.dim.y = surfaceCaps.minImageExtent.height;

    if ((DFL_WINDOW->info.dim.x < surfaceCaps.maxImageExtent.width) || (DFL_WINDOW->info.dim.x == NULL))
        DFL_WINDOW->info.dim.x = surfaceCaps.maxImageExtent.width;
    if ((DFL_WINDOW->info.dim.y < surfaceCaps.maxImageExtent.height) || (DFL_WINDOW->info.dim.y == NULL))
        DFL_WINDOW->info.dim.y = surfaceCaps.maxImageExtent.height;

    DFL_WINDOW->imageCount = surfaceCaps.minImageCount + 1;

    VkSwapchainCreateInfoKHR swapInfo = _dflWindowSwapchainCreateInfoGet(hWindow, DFL_WINDOW->device);
    VkSwapchainKHR dummy = NULL;
    vkCreateSwapchainKHR(((struct DflDevice_T*)DFL_WINDOW->device)->device, &swapInfo, NULL, &dummy);
    if (dummy == NULL)
    {
        DFL_WINDOW->error = DFL_VULKAN_SURFACE_NO_SWAPCHAIN_ERROR;
        return;
    }
    vkDeviceWaitIdle(((struct DflDevice_T*)DFL_WINDOW->device)->device);
    if(DFL_WINDOW->swapchain != NULL)
        vkDestroySwapchainKHR(((struct DflDevice_T*)DFL_WINDOW->device)->device, DFL_WINDOW->swapchain, NULL);
    DFL_WINDOW->swapchain = dummy;

    _dflWindowSwapchainImagesGet(hWindow);
    // we want to destroy the old image views before creating new ones
    for (int i = 0; i < DFL_WINDOW->imageCount; i++)
        if(DFL_WINDOW->imageViews[i] != NULL)
            vkDestroyImageView(((struct DflDevice_T*)DFL_WINDOW->device)->device, DFL_WINDOW->imageViews[i], NULL);
 
    _dflWindowSwapchainImageViewsCreate(hWindow);
}

// this exists just as a shorthand
inline static struct DflWindow_T* _dflWindowAlloc();
struct DflWindow_T*               _dflWindowAlloc()
{
    return calloc(1, sizeof(struct DflWindow_T));
}

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

DflWindow _dflWindowCreate(DflWindowInfo* pInfo, DflSession hSession)
{
    struct DflWindow_T* window = _dflWindowAlloc();
    if(window == NULL)
        return NULL;

    if (pInfo == NULL)
    {
        DflWindowInfo info = {
            .dim = (struct DflVec2D){ 1920, 1080 },
            .view = (struct DflVec2D){ 1920, 1080 },
            .pos = (struct DflVec2D){ 200, 200 },
            .name = "Dragonfly-App",
            .mode = DFL_WINDOWED
        };
        window->info = info;
    }
    else
        window->info = *pInfo;
    
    window->error = 0;
    window->handle = _dflWindowCall(window->info.dim, window->info.name, window->info.mode, &window->error);

    if (window->handle == NULL)
    {
        // the window will not be created for two reasons:
        // 1. glfwInit() failed
        // 2. glfwCreateWindow() failed
        // We know that if 1 didn't happen, then 2 did.
        if(window->error != DFL_GLFW_API_INIT_ERROR)
            window->error = DFL_GLFW_WINDOW_INIT_ERROR;
        return (DflWindow)window;
    }

    if (window->info.hIcon != NULL)
    {
        GLFWimage image;
        dflImageLoad((struct DflImage_T*)window->info.hIcon);
        image.pixels = ((unsigned char*)((struct DflImage_T*)window->info.hIcon)->data);
        image.height = ((struct DflImage_T*)window->info.hIcon)->size.y;
        image.width = ((struct DflImage_T*)window->info.hIcon)->size.x;
        glfwSetWindowIcon(window->handle, 1, &image);
    }

    if(window->info.pos.x != NULL || window->info.pos.y != NULL)
        glfwSetWindowPos(window->handle, window->info.pos.x, window->info.pos.y);

    return (DflWindow)window;
}

DflWindow dflWindowInit(DflWindowInfo* pWindowInfo, DflSession hSession)
{
    DflWindow hWindow = _dflWindowCreate(pWindowInfo, hSession);

    if(hWindow == NULL)
        return NULL;

    if (DFL_WINDOW->error == DFL_SUCCESS)
        DFL_WINDOW->error = _dflWindowBindToSession(hWindow, hSession);

    DFL_HANDLE(Window)->session = hSession;

    for (int i = 0; i < DFL_MAX_ITEM_COUNT; i++)
    {
        if (DFL_SESSION->windows[i] == NULL) // Dragonfly searches for the first available slot to store the window handle.
        {
            DFL_SESSION->windows[i] = hWindow;
            DFL_WINDOW->index = i;
            break;
        }
    }

    return hWindow;
}

void dflWindowBindToDevice(DflWindow hWindow, DflDevice hDevice)
{
    int formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(DFL_DEVICE->physDevice, DFL_WINDOW->surface, &formatCount, NULL);
    if(formatCount == 0)
    {
        DFL_WINDOW->error = DFL_VULKAN_SURFACE_NO_FORMATS_ERROR;
        return;
    }
    VkSurfaceFormatKHR* formats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    if(formats == NULL)
    {
        DFL_WINDOW->error = DFL_GENERIC_OOM_ERROR;
        return;
    }
    vkGetPhysicalDeviceSurfaceFormatsKHR(DFL_DEVICE->physDevice, DFL_WINDOW->surface, &formatCount, formats);

    int choice = 0;
    for (int i = 0; i < formatCount; i++)
    {
        if (DFL_WINDOW->info.colorFormat == formats[i].format)
            choice = i;
    }
    DFL_WINDOW->info.colorFormat = formats[choice].format;
    DFL_WINDOW->colorSpace = formats[choice].colorSpace;

    DFL_WINDOW->device = hDevice;
    _dflWindowSwapchainRecreate(hWindow);
}

/* -------------------- *
 *   CHANGE             *
 * -------------------- */

void dflWindowReshape(int type, struct DflVec2D rect, DflWindow hWindow)
{
    switch (type) {
    case DFL_DIMENSIONS:
        DFL_WINDOW->info.dim = rect;
        glfwSetWindowSize(DFL_WINDOW->handle, rect.x, rect.y);
        break;
    default:
        DFL_WINDOW->info.view = rect;
        break;
    }

    if (DFL_WINDOW->swapchain != NULL)
        _dflWindowSwapchainRecreate(hWindow);
}

void dflWindowReposition(struct DflVec2D pos, DflWindow hWindow)
{
    DFL_WINDOW->info.pos = pos;
    glfwSetWindowPos(DFL_HANDLE(Window)->handle, pos.x, pos.y);
}

void dflWindowChangeMode(int mode, DflWindow hWindow)
{
    DFL_WINDOW->info.mode = mode;
    switch (mode) {
    case DFL_WINDOWED:
        glfwSetWindowAttrib(DFL_WINDOW->handle, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(DFL_WINDOW->handle, NULL, DFL_WINDOW->info.pos.x, DFL_WINDOW->info.pos.y, DFL_WINDOW->info.dim.x, DFL_WINDOW->info.dim.y, GLFW_DONT_CARE);
        break;
    case DFL_FULLSCREEN:
        glfwSetWindowMonitor(DFL_WINDOW->handle, glfwGetPrimaryMonitor(), 0, 0, DFL_WINDOW->info.dim.x, DFL_WINDOW->info.dim.y, GLFW_DONT_CARE);
        break;
    case DFL_BORDERLESS:
        glfwSetWindowAttrib(DFL_WINDOW->handle, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowMonitor(DFL_WINDOW->handle, NULL, DFL_WINDOW->info.pos.x, DFL_WINDOW->info.pos.y, DFL_WINDOW->info.dim.x, DFL_WINDOW->info.dim.y, GLFW_DONT_CARE);
        break;
    default:
        glfwSetWindowAttrib(DFL_WINDOW->handle, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(DFL_WINDOW->handle, NULL, DFL_WINDOW->info.pos.x, DFL_WINDOW->info.pos.y, DFL_WINDOW->info.dim.x, DFL_WINDOW->info.dim.y, GLFW_DONT_CARE);
        break;
    }

    if(DFL_WINDOW->swapchain != NULL)
        _dflWindowSwapchainRecreate(hWindow);
}

void dflWindowRename(const char* name, DflWindow hWindow)
{
    strcpy_s(&DFL_WINDOW->info.name, DFL_MAX_CHAR_COUNT, name);
    glfwSetWindowTitle(DFL_WINDOW->handle, name);
}

void dflWindowChangeIcon(DflImage icon, DflWindow hWindow)
{
    GLFWimage image;
    dflImageLoad(&icon);
    image.pixels = ((struct DflImage_T*)icon)->data;

    glfwSetWindowIcon(DFL_WINDOW->handle, 1, &image);
    DFL_WINDOW->info.hIcon = icon;
}

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

struct DflVec2D dflWindowRectGet(int type, DflWindow hWindow)
{
    switch (type) {
    case DFL_DIMENSIONS:
        return DFL_WINDOW->info.dim;
    default:
        return DFL_WINDOW->info.view;
    }
}

struct DflVec2D dflWindowPosGet(DflWindow hWindow)
{
    struct DflVec2D pos = { .x = 0, .y = 0 };

    glfwGetWindowPos(DFL_WINDOW->handle, &pos.x, &pos.y);
    return pos;
}

DflSession dflWindowSessionGet(DflWindow hWindow)
{
    return (DFL_WINDOW->session);
}

bool dflWindowShouldCloseGet(DflWindow hWindow)
{
    return glfwWindowShouldClose(DFL_WINDOW->handle);
}

int dflWindowErrorGet(DflWindow hWindow)
{
    return DFL_WINDOW->error;
}

void dflWindowWin32AttributeSet(int attrib, int value, DflWindow hWindow)
{
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(DFL_WINDOW->handle);
    HRESULT hr = S_OK;

    if (attrib > 0)
    {
        switch (attrib)
        {
        case DFL_WINDOW_WIN32_DARK_MODE:
            BOOL dark = TRUE;
            hr = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(BOOL));
            break;
        case DFL_WINDOW_WIN32_BORDER_COLOUR:
            hr = DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &value, sizeof(DWORD));
            break;
        case DFL_WINDOW_WIN32_TITLE_TEXT_COLOUR:
            hr = DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &value, sizeof(DWORD));
            break;
        case DFL_WINDOW_WIN32_TITLE_BAR_COLOUR:
            hr = DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &value, sizeof(DWORD));
            break;
        case DFL_WINDOW_WIN32_ROUND_CORNERS:
            hr = DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &value, sizeof(DWORD));
            break;
        }
    }
    else // make whole area available for rendering by the client, if true
    {
        if (value == true)
        {
            MARGINS margins = {
                .cxLeftWidth = 0, 
                .cxRightWidth = 0, 
                .cyTopHeight = -1,
                .cyBottomHeight = 0 
            };
            hr = DwmExtendFrameIntoClientArea(hwnd, &margins);

            SetWindowPos(hwnd, NULL, DFL_HANDLE(Window)->info.pos.x, DFL_HANDLE(Window)->info.pos.y, DFL_HANDLE(Window)->info.dim.x, DFL_HANDLE(Window)->info.dim.y, SWP_FRAMECHANGED);
        }
        else
        {
            MARGINS margins = { 0, 0, 0, 0 };
            hr = DwmExtendFrameIntoClientArea(hwnd, &margins);
        }
    }

    DFL_WINDOW->error = hr;
#endif

    return;
}

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void _dflWindowDestroy(DflWindow hWindow)
{
    glfwDestroyWindow(DFL_WINDOW->handle);

    free(hWindow);
}

// the existence of hSession in the arguments is purely as a reminder that this function is to be called BEFORE the session is destroyed.
void dflWindowTerminate(DflWindow hWindow, DflSession hSession) 
{
    if (hSession != DFL_WINDOW->session) // however, we still don't want mixups between sessions
        return;

    ((struct DflSession_T*)DFL_WINDOW->session)->windows[DFL_HANDLE(Window)->index] = NULL;

    vkDestroySurfaceKHR(((struct DflSession_T*)DFL_WINDOW->session)->instance, DFL_WINDOW->surface, NULL);
    ((struct DflSession_T*)DFL_WINDOW->session)->windows[DFL_WINDOW->index] = NULL;

    _dflWindowDestroy(hWindow);
}

void dflWindowUnbindFromDevice(DflWindow hWindow, DflDevice hDevice)
{
    if (DFL_WINDOW->swapchain == NULL)
        return;

    if (DFL_WINDOW->device != hDevice)
        return; // this ensures that the user hasn't accidentally passed in the wrong device (thus a crash is avoided)

    for(int i = 0; i < DFL_WINDOW->imageCount; i++)
        vkDestroyImageView(DFL_DEVICE->device, DFL_WINDOW->imageViews[i], NULL);

    vkDestroySwapchainKHR(DFL_DEVICE->device, DFL_WINDOW->swapchain, NULL);
}