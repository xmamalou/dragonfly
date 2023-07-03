/*
    Copyright 2022 Christopher-Marios Mamaloukas

    Redistribution and use in source and binary forms, with or without modification,
    are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation and/or
    other materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its contributors
    may be used to endorse or promote products derived from this software without
    specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
    BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
    EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

static void _dflWindowSwapchainRecreate(DflWindow hWindow);
void _dflWindowSwapchainRecreate(DflWindow hWindow)
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(((struct DflDevice_T*)DFL_HANDLE(Window)->device)->physDevice, DFL_HANDLE(Window)->surface, &surfaceCaps);

    DFL_HANDLE(Window)->minRes.x = surfaceCaps.minImageExtent.width;
    DFL_HANDLE(Window)->minRes.y = surfaceCaps.minImageExtent.height;

    DFL_HANDLE(Window)->maxRes.x = surfaceCaps.maxImageExtent.width;
    DFL_HANDLE(Window)->maxRes.y = surfaceCaps.maxImageExtent.height;

    // the below make sure that the resolution is within the bounds that are supported
    if ((DFL_HANDLE(Window)->minRes.x > DFL_HANDLE(Window)->info.dim.x) || (DFL_HANDLE(Window)->info.dim.x == NULL))
        DFL_HANDLE(Window)->info.dim.x = DFL_HANDLE(Window)->minRes.x;
    if ((DFL_HANDLE(Window)->minRes.y > DFL_HANDLE(Window)->info.dim.y) || (DFL_HANDLE(Window)->info.dim.y == NULL))
        DFL_HANDLE(Window)->info.dim.y = DFL_HANDLE(Window)->minRes.y;

    if ((DFL_HANDLE(Window)->maxRes.x < DFL_HANDLE(Window)->info.dim.x) || (DFL_HANDLE(Window)->info.dim.x == NULL))
        DFL_HANDLE(Window)->info.dim.x = DFL_HANDLE(Window)->maxRes.x;
    if ((DFL_HANDLE(Window)->maxRes.y < DFL_HANDLE(Window)->info.dim.y) || (DFL_HANDLE(Window)->info.dim.y == NULL))
        DFL_HANDLE(Window)->info.dim.y = DFL_HANDLE(Window)->maxRes.y;

    VkSwapchainCreateInfoKHR swapInfo = _dflWindowSwapchainCreateInfoGet(hWindow, DFL_HANDLE(Window)->device);
    VkSwapchainKHR dummy = NULL;
    vkCreateSwapchainKHR(((struct DflDevice_T*)DFL_HANDLE(Window)->device)->device, &swapInfo, NULL, &dummy);
    if (dummy == NULL)
    {
        DFL_HANDLE(Window)->error = DFL_VULKAN_SURFACE_NO_SWAPCHAIN_ERROR;
        return;
    }
    vkDestroySwapchainKHR(((struct DflDevice_T*)DFL_HANDLE(Window)->device)->device, DFL_HANDLE(Window)->swapchain, NULL);
    DFL_HANDLE(Window)->swapchain = dummy;
}

// this exists just as a shorthand
inline static struct DflWindow_T* _dflWindowAlloc();
static struct DflWindow_T* _dflWindowAlloc()
{
    return calloc(1, sizeof(struct DflWindow_T));
}

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

DflWindow _dflWindowCreate(DflWindowInfo* pInfo)
{
    struct DflWindow_T* window = _dflWindowAlloc();

    if (pInfo == NULL)
    {
        int count = 0;
        struct DflMonitorInfo *monitors = dflMonitorsGet(&count);


        DflWindowInfo info = {
            .dim = (struct DflVec2D){ 1920, 1080 },
            .view = (struct DflVec2D){ 1920, 1080 },
            .pos = (struct DflVec2D){ 200, 200 },
            .name = "Dragonfly-App",
            .mode = DFL_WINDOWED
        };
        window->info = info;
        
        free(monitors);
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

    if (window->info.icon != NULL)
    {
        GLFWimage image;
        dflImageLoad((struct DflImage_T*)window->info.icon);
        image.pixels = ((unsigned char*)((struct DflImage_T*)window->info.icon)->data);
        image.height = ((struct DflImage_T*)window->info.icon)->size.y;
        image.width = ((struct DflImage_T*)window->info.icon)->size.x;
        glfwSetWindowIcon(window->handle, 1, &image);
    }

    if(window->info.pos.x != NULL || window->info.pos.y != NULL)
        glfwSetWindowPos(window->handle, window->info.pos.x, window->info.pos.y);

    return (DflWindow)window;
}

DflWindow dflWindowInit(DflWindowInfo* pWindowInfo, DflSession hSession)
{
    DflWindow hWindow = _dflWindowCreate(pWindowInfo);

    if (DFL_HANDLE(Window)->error == DFL_SUCCESS)
    {
        DFL_HANDLE(Window)->error = _dflWindowBindToSession(hWindow, hSession);
    }

    DFL_HANDLE(Window)->session = hSession;

    for (int i = 0; i < DFL_MAX_ITEM_COUNT; i++)
    {
        if (DFL_HANDLE(Session)->windows[i] == NULL) // Dragonfly searches for the first available slot to store the window handle.
        {
            DFL_HANDLE(Session)->windows[i] = hWindow;
            DFL_HANDLE(Window)->index = i;
            break;
        }
    }

    return hWindow;
}

void dflWindowBindToDevice(DflWindow hWindow, DflDevice hDevice)
{
    VkSurfaceCapabilitiesKHR surfaceCaps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(DFL_HANDLE(Device)->physDevice, DFL_HANDLE(Window)->surface, &surfaceCaps);

    int formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(DFL_HANDLE(Device)->physDevice, DFL_HANDLE(Window)->surface, &formatCount, NULL);
    if(formatCount == 0)
    {
        DFL_HANDLE(Window)->error = DFL_VULKAN_SURFACE_NO_FORMATS_ERROR;
        return;
    }
    VkSurfaceFormatKHR* formats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    if(formats == NULL)
    {
        DFL_HANDLE(Window)->error = DFL_GENERIC_OOM_ERROR;
        return;
    }
    vkGetPhysicalDeviceSurfaceFormatsKHR(DFL_HANDLE(Device)->physDevice, DFL_HANDLE(Window)->surface, &formatCount, formats);

    for (int i = 0; i < formatCount; i++)
    {
        if (DFL_HANDLE(Window)->info.colorFormat == formats[i].format)
        {
            DFL_HANDLE(Window)->colorSpace = formats[i].colorSpace;
            break;
        }
        else // else just prefer the first format
        {
            DFL_HANDLE(Window)->colorSpace = formats[0].colorSpace;
            DFL_HANDLE(Window)->info.colorFormat = formats[0].format;
        }
    }

    DFL_HANDLE(Window)->minRes.x = surfaceCaps.minImageExtent.width;
    DFL_HANDLE(Window)->minRes.y = surfaceCaps.minImageExtent.height;
    
    DFL_HANDLE(Window)->maxRes.x = surfaceCaps.maxImageExtent.width;
    DFL_HANDLE(Window)->maxRes.y = surfaceCaps.maxImageExtent.height;
    
    // the below make sure that the resolution is within the bounds that are supported
    if ((DFL_HANDLE(Window)->minRes.x > DFL_HANDLE(Window)->info.dim.x) || (DFL_HANDLE(Window)->info.dim.x == NULL))
        DFL_HANDLE(Window)->info.dim.x = DFL_HANDLE(Window)->minRes.x;
    if ((DFL_HANDLE(Window)->minRes.y > DFL_HANDLE(Window)->info.dim.y) || (DFL_HANDLE(Window)->info.dim.y == NULL))
        DFL_HANDLE(Window)->info.dim.y = DFL_HANDLE(Window)->minRes.y;

    if ((DFL_HANDLE(Window)->maxRes.x < DFL_HANDLE(Window)->info.dim.x) || (DFL_HANDLE(Window)->info.dim.x == NULL))
        DFL_HANDLE(Window)->info.dim.x = DFL_HANDLE(Window)->maxRes.x;
    if ((DFL_HANDLE(Window)->maxRes.y < DFL_HANDLE(Window)->info.dim.y) || (DFL_HANDLE(Window)->info.dim.y == NULL))
        DFL_HANDLE(Window)->info.dim.y = DFL_HANDLE(Window)->maxRes.y;

    DFL_HANDLE(Window)->imageCount = surfaceCaps.minImageCount + 1;

    VkSwapchainCreateInfoKHR swapInfo = _dflWindowSwapchainCreateInfoGet(hWindow, hDevice);

    vkCreateSwapchainKHR(DFL_HANDLE(Device)->device, &swapInfo, NULL, &DFL_HANDLE(Window)->swapchain);
    if(DFL_HANDLE(Window)->swapchain == NULL)
    {
        DFL_HANDLE(Window)->error = DFL_VULKAN_SURFACE_NO_SWAPCHAIN_ERROR;
        return;
    }

    DFL_HANDLE(Window)->device = hDevice;
}

/* -------------------- *
 *   CHANGE             *
 * -------------------- */

void dflWindowReshape(int type, struct DflVec2D rect, DflWindow hWindow)
{
    switch (type) {
    case DFL_DIMENSIONS:
        DFL_HANDLE(Window)->info.dim = rect;
        glfwSetWindowSize(DFL_HANDLE(Window)->handle, rect.x, rect.y);
        break;
    default:
        DFL_HANDLE(Window)->info.view = rect;
        break;
    }

    if (DFL_HANDLE(Window)->swapchain != NULL)
        _dflWindowSwapchainRecreate(hWindow);

    if (DFL_HANDLE(Window)->reshapeCLBK != NULL)
        DFL_HANDLE(Window)->reshapeCLBK(rect, type, hWindow);
}

void dflWindowReposition(struct DflVec2D pos, DflWindow hWindow)
{
    DFL_HANDLE(Window)->info.pos = pos;
    glfwSetWindowPos(DFL_HANDLE(Window)->handle, pos.x, pos.y);

    if (DFL_HANDLE(Window)->repositionCLBK != NULL)
        DFL_HANDLE(Window)->repositionCLBK(pos, hWindow);
}

void dflWindowChangeMode(int mode, DflWindow hWindow)
{
    DFL_HANDLE(Window)->info.mode = mode;
    switch (mode) {
    case DFL_WINDOWED:
        glfwSetWindowAttrib(DFL_HANDLE(Window)->handle, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(DFL_HANDLE(Window)->handle, NULL, DFL_HANDLE(Window)->info.pos.x, DFL_HANDLE(Window)->info.pos.y, DFL_HANDLE(Window)->info.dim.x, DFL_HANDLE(Window)->info.dim.y, GLFW_DONT_CARE);
        break;
    case DFL_FULLSCREEN:
        glfwSetWindowMonitor(DFL_HANDLE(Window)->handle, glfwGetPrimaryMonitor(), 0, 0, DFL_HANDLE(Window)->info.dim.x, DFL_HANDLE(Window)->info.dim.y, GLFW_DONT_CARE);
        break;
    case DFL_BORDERLESS:
        glfwSetWindowAttrib(DFL_HANDLE(Window)->handle, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowMonitor(DFL_HANDLE(Window)->handle, NULL, DFL_HANDLE(Window)->info.pos.x, DFL_HANDLE(Window)->info.pos.y, DFL_HANDLE(Window)->info.dim.x, DFL_HANDLE(Window)->info.dim.y, GLFW_DONT_CARE);
        break;
    default:
        glfwSetWindowAttrib(DFL_HANDLE(Window)->handle, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(DFL_HANDLE(Window)->handle, NULL, DFL_HANDLE(Window)->info.pos.x, DFL_HANDLE(Window)->info.pos.y, DFL_HANDLE(Window)->info.dim.x, DFL_HANDLE(Window)->info.dim.y, GLFW_DONT_CARE);
        break;
    }

    if(DFL_HANDLE(Window)->swapchain != NULL)
        _dflWindowSwapchainRecreate(hWindow);

    if (DFL_HANDLE(Window)->modeCLBK != NULL)
        DFL_HANDLE(Window)->modeCLBK(mode, hWindow);
}

void dflWindowRename(const char* name, DflWindow hWindow)
{
    strcpy_s(&DFL_HANDLE(Window)->info.name, DFL_MAX_CHAR_COUNT, name);
    glfwSetWindowTitle(DFL_HANDLE(Window)->handle, name);

    if (DFL_HANDLE(Window)->renameCLBK != NULL)
        DFL_HANDLE(Window)->renameCLBK(name, hWindow);
}

void dflWindowChangeIcon(DflImage icon, DflWindow hWindow)
{
    GLFWimage image;
    dflImageLoad(&icon);
    image.pixels = ((struct DflImage_T*)icon)->data;

    glfwSetWindowIcon(DFL_HANDLE(Window)->handle, 1, &image);
    DFL_HANDLE(Window)->info.icon = icon;

    if (DFL_HANDLE(Window)->iconCLBK != NULL)
        DFL_HANDLE(Window)->iconCLBK(icon, hWindow);
}

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

struct DflVec2D dflWindowRectGet(int type, DflWindow hWindow)
{
    switch (type) {
    case DFL_DIMENSIONS:
        return DFL_HANDLE(Window)->info.dim;
    default:
        return DFL_HANDLE(Window)->info.view;
    }
}

struct DflVec2D dflWindowPosGet(DflWindow hWindow)
{
    struct DflVec2D pos = { .x = 0, .y = 0 };

    glfwGetWindowPos(DFL_HANDLE(Window)->handle, &pos.x, &pos.y);
    return pos;
}

DflSession dflWindowSessionGet(DflWindow hWindow)
{
    return (DFL_HANDLE(Window)->session);
}

bool dflWindowShouldCloseGet(DflWindow hWindow)
{
    return glfwWindowShouldClose(DFL_HANDLE(Window)->handle);
}

int dflWindowErrorGet(DflWindow hWindow)
{
    return DFL_HANDLE(Window)->error;
}

void dflWindowWin32AttributeSet(int attrib, int value, DflWindow hWindow)
{
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(DFL_HANDLE(Window)->handle);
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
                .cyTopHeight = -50,
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

    DFL_HANDLE(Window)->error = hr;
#endif

    return;
}

void _dflWindowErrorSet(int error, DflWindow hWindow)
{
    DFL_HANDLE(Window)->error = error;
}

void _dflWindowSessionSet(VkSurfaceKHR surface, DflSession session, DflWindow hWindow)
{
    DFL_HANDLE(Window)->session = session;
    DFL_HANDLE(Window)->surface = surface;
}

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void _dflWindowDestroy(DflWindow hWindow)
{
    if (DFL_HANDLE(Window)->destroyCLBK != NULL)
        DFL_HANDLE(Window)->destroyCLBK(hWindow);

    glfwDestroyWindow(DFL_HANDLE(Window)->handle);

    free(hWindow);
}

void dflWindowTerminate(DflWindow hWindow)
{
    ((struct DflSession_T*)DFL_HANDLE(Window)->session)->windows[DFL_HANDLE(Window)->index] = NULL;

    vkDestroySurfaceKHR(((struct DflSession_T*)DFL_HANDLE(Window)->session)->instance, DFL_HANDLE(Window)->surface, NULL);
    ((struct DflSession_T*)DFL_HANDLE(Window)->session)->windows[DFL_HANDLE(Window)->index] = NULL;

    _dflWindowDestroy(hWindow);
}

void dflWindowUnbindFromDevice(DflWindow hWindow, DflDevice hDevice)
{
    if (DFL_HANDLE(Window)->swapchain == NULL)
        return;

    vkDestroySwapchainKHR(((struct DflDevice_T*)hDevice)->device, DFL_HANDLE(Window)->swapchain, NULL);
}

/* -------------------- *
 *   CALLBACK SETTERS   *
 * -------------------- */

void dflWindowReshapeCLBKSet(DflWindowReshapeCLBK clbk, DflWindow hWindow)
{
    // TODO: add a way to set the GLFW callback as well
    DFL_HANDLE(Window)->reshapeCLBK = clbk;
}

void dflWindowRepositionCLBKSet(DflWindowRepositionCLBK clbk, DflWindow hWindow)
{
    // TODO: add a way to set the GLFW callback as well
    DFL_HANDLE(Window)->repositionCLBK = clbk;
}

void dflWindowChangeModeCLBKSet(DflWindowChangeModeCLBK clbk, DflWindow hWindow)
{
    DFL_HANDLE(Window)->modeCLBK = clbk;
}

void dflWindowRenameCLBKSet(DflWindowRenameCLBK clbk, DflWindow hWindow)
{
    DFL_HANDLE(Window)->renameCLBK = clbk;
}

void dflWindowChangeIconCLBKSet(DflWindowChangeIconCLBK clbk, DflWindow hWindow)
{
    DFL_HANDLE(Window)->iconCLBK = clbk;
}

void dflWindowDestroyCLBKSet(DflWindowCloseCLBK clbk, DflWindow hWindow)
{
    DFL_HANDLE(Window)->destroyCLBK = clbk;
}