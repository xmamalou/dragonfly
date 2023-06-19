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

#include "Window.h"

#include <stdlib.h>

#ifdef _WIN32
#include <Windows.h>
#include <uxtheme.h>
#include <dwmapi.h>
#pragma comment (lib, "Dwmapi")
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include <GLFW/glfw3native.h>

#include "../Data.h"
#include "../StbDummy.h"
#include "Session.h"

struct DflWindow_T { // A Dragonfly window
    GLFWwindow*     handle;
    DflWindowInfo	info;

    DflSession      session;

    /* ------------------- *
    *   VULKAN SPECIFIC    *
    *  ------------------- */

    VkSurfaceKHR surface; // The surface of the window. If NULL, then the window doesn't have a surface.

    // CALLBACKS 

    DflWindowReshapeCLBK    reshapeCLBK;
    DflWindowRepositionCLBK repositionCLBK;
    DflWindowChangeModeCLBK modeCLBK;
    DflWindowRenameCLBK     renameCLBK;
    DflWindowChangeIconCLBK iconCLBK;

    DflWindowCloseCLBK      destroyCLBK;

    // error
    int error;
};

/* -------------------- *
 *   INTERNAL           *
 * -------------------- */

// make the window
static GLFWwindow* _dflWindowCall(struct DflVec2D dim, struct DflVec2D view, struct DflVec2D res, const char* name, int mode, int* error);
static GLFWwindow* _dflWindowCall(struct DflVec2D dim, struct DflVec2D view, struct DflVec2D res, const char* name, int mode, int* error)
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

    if (view.x == NULL || view.y == NULL)
        view = (struct DflVec2D){ 1920, 1080 };

    if (res.x == NULL || res.y == NULL)
        res = (struct DflVec2D){ 1920, 1080 };

    if (!glfwInit())
    {
        *error = DFL_GLFW_INIT_ERROR;
        return NULL;
    }

    switch (mode) {
    case DFL_WINDOWED:
        //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // TODO: Decide if making the window resizable is a good idea.
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

// this exists just as a shorthand
inline static struct DflWindow_T* _dflWindowAlloc();
inline static struct DflWindow_T* _dflWindowAlloc()
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
        DflWindowInfo info = {
            .dim = (struct DflVec2D){ 1920, 1080 },
            .view = (struct DflVec2D){ 1920, 1080 },
            .res = (struct DflVec2D){ 1920, 1080 },
            .pos = (struct DflVec2D){ 200, 200 },
            .name = "Dragonfly-App",
            .mode = DFL_WINDOWED
        };
        window->info = info;
    }
    else
        window->info = *pInfo;
    
    window->error = 0;
    window->handle = _dflWindowCall(window->info.dim, window->info.view, window->info.res, window->info.name, window->info.mode, &window->error);

    if (window->handle == NULL)
    {
        // the window will not be created for two reasons:
        // 1. glfwInit() failed
        // 2. glfwCreateWindow() failed
        // We know that if 1 didn't happen, then 2 did.
        if(window->error != DFL_GLFW_INIT_ERROR)
            window->error = DFL_GLFW_WINDOW_ERROR;
        return (DflWindow)window;
    }

    if (window->info.icon != NULL)
    {
        GLFWimage image;
        image.pixels = stbi_load(window->info.icon, &image.width, &image.height, 0, 4); //rgba channels 
        if (image.pixels != NULL)
            glfwSetWindowIcon(window->handle, 1, &image);
        stbi_image_free(image.pixels);
    }

    if(window->info.pos.x != NULL || window->info.pos.y != NULL)
        glfwSetWindowPos(window->handle, window->info.pos.x, window->info.pos.y);

    return (DflWindow)window;
}

/* -------------------- *
 *   CHANGE             *
 * -------------------- */

void dflWindowReshape(int type, struct DflVec2D rect, DflWindow* pWindow)
{
    switch (type) {
    case DFL_DIMENSIONS:
        DFL_HANDLE(Window)->info.dim = rect;
        glfwSetWindowSize(DFL_HANDLE(Window)->handle, rect.x, rect.y);
        break;
    case DFL_VIEWPORT:
        DFL_HANDLE(Window)->info.view = rect;
        break;
    default:
        DFL_HANDLE(Window)->info.res = rect;
        break;
    }

    if (DFL_HANDLE(Window)->reshapeCLBK != NULL)
        DFL_HANDLE(Window)->reshapeCLBK(rect, type, pWindow);
}

void dflWindowReposition(struct DflVec2D pos, DflWindow* pWindow)
{
    DFL_HANDLE(Window)->info.pos = pos;
    glfwSetWindowPos(DFL_HANDLE(Window)->handle, pos.x, pos.y);

    if (DFL_HANDLE(Window)->repositionCLBK != NULL)
        DFL_HANDLE(Window)->repositionCLBK(pos, pWindow);
}

void dflWindowChangeMode(int mode, DflWindow* pWindow)
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

    if (DFL_HANDLE(Window)->modeCLBK != NULL)
        DFL_HANDLE(Window)->modeCLBK(mode, pWindow);
}

void dflWindowRename(const char* name, DflWindow* pWindow)
{
    strcpy_s(&DFL_HANDLE(Window)->info.name, DFL_MAX_CHAR_COUNT, name);
    glfwSetWindowTitle(((struct DflWindow_T*)*pWindow)->handle, name);

    if (DFL_HANDLE(Window)->renameCLBK != NULL)
        DFL_HANDLE(Window)->renameCLBK(name, pWindow);
}

void dflWindowChangeIcon(const char* icon, DflWindow* pWindow)
{
    GLFWimage image;
    image.pixels = stbi_load(icon, &image.width, &image.height, 0, 4); //rgba channels 
    if (image.pixels != NULL)
    {
        glfwSetWindowIcon(DFL_HANDLE(Window)->handle, 1, &image);
        strcpy_s(&DFL_HANDLE(Window)->info.icon, DFL_MAX_CHAR_COUNT, icon);
        stbi_image_free(image.pixels);
    }

    if (DFL_HANDLE(Window)->iconCLBK != NULL)
        DFL_HANDLE(Window)->iconCLBK(icon, pWindow);
}

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

struct DflVec2D dflWindowRectGet(int type, DflWindow window)
{
    switch (type) {
    case DFL_DIMENSIONS:
        return ((struct DflWindow_T*)window)->info.dim;
    case DFL_VIEWPORT:
        return ((struct DflWindow_T*)window)->info.view;
    default:
        return ((struct DflWindow_T*)window)->info.res;
    }
}

struct DflVec2D dflWindowPosGet(DflWindow window)
{
    struct DflVec2D pos = { .x = 0, .y = 0 };

    glfwGetWindowPos(((struct DflWindow_T*)window)->handle, &pos.x, &pos.y);
    return pos;
}

struct DflVec2D dflPrimaryMonitorPosGet()
{
    struct DflVec2D pos = { .x = 0, .y = 0 };

    if(glfwInit() == GLFW_TRUE)
        glfwGetMonitorPos(glfwGetPrimaryMonitor(), &pos.x, &pos.y);
    
    return pos;
}

DflSession dflWindowSessionGet(DflWindow window)
{
    return (((struct DflWindow_T*)window)->session);
}

bool dflWindowShouldCloseGet(DflWindow window)
{
    return glfwWindowShouldClose(((struct DflWindow_T*)window)->handle);
}

int dflWindowErrorGet(DflWindow window)
{
    return ((struct DflWindow_T*)window)->error;
}

GLFWwindow* _dflWindowHandleGet(DflWindow window)
{
    if (_dflSessionIsLegalGet(((struct DflWindow_T*)window)->session) == false)
        return NULL;

    return (((struct DflWindow_T*)window)->handle);
}

void dflWindowWin32AttributeSet(int attrib, int value, DflWindow* pWindow)
{
#ifdef _WIN32
    HWND hwnd = glfwGetWin32Window(DFL_HANDLE(Window)->handle);

    if (attrib > 0)
    {
        switch (attrib)
        {
        case DFL_WINDOW_WIN32_DARK_MODE:
            BOOL dark = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(BOOL));
            break;
        case DFL_WINDOW_WIN32_BORDER_COLOUR:
            DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &value, sizeof(DWORD));
            break;
        case DFL_WINDOW_WIN32_TITLE_TEXT_COLOUR:
            DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &value, sizeof(DWORD));
            break;
        case DFL_WINDOW_WIN32_TITLE_BAR_COLOUR:
            DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &value, sizeof(DWORD));
            break;
        case DFL_WINDOW_WIN32_ROUND_CORNERS:
            DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &value, sizeof(DWORD));
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
            DwmExtendFrameIntoClientArea(hwnd, &margins);

            SetWindowPos(hwnd, NULL, DFL_HANDLE(Window)->info.pos.x, DFL_HANDLE(Window)->info.pos.y, DFL_HANDLE(Window)->info.dim.x, DFL_HANDLE(Window)->info.dim.y, SWP_FRAMECHANGED);
        }
        else
        {
            MARGINS margins = { 0, 0, 0, 0 };
            DwmExtendFrameIntoClientArea(hwnd, &margins);
        }
    }
#endif

    return;
}

void _dflWindowErrorSet(int error, DflWindow* pWindow)
{
    if (_dflSessionIsLegalGet(DFL_HANDLE(Window)->session) == false)
        return;

    DFL_HANDLE(Window)->error = error;
}

void _dflWindowSessionSet(VkSurfaceKHR surface, DflSession session, DflWindow* pWindow)
{
    if (_dflSessionIsLegalGet(session) == false)
        return;

    DFL_HANDLE(Window)->session = session;
    DFL_HANDLE(Window)->surface = surface;
}

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void _dflWindowDestroy(DflWindow* pWindow)
{
    if (_dflSessionIsLegalGet(DFL_HANDLE(Window)->session) == false)
        return;

    if (DFL_HANDLE(Window)->destroyCLBK != NULL)
        DFL_HANDLE(Window)->destroyCLBK(pWindow);

    vkDestroySurfaceKHR(_dflSessionInstanceGet(DFL_HANDLE(Window)->session), DFL_HANDLE(Window)->surface, NULL);
    glfwDestroyWindow(DFL_HANDLE(Window)->handle);

    free(*pWindow);
}

/* -------------------- *
 *   CALLBACK SETTERS   *
 * -------------------- */

void dflWindowReshapeCLBKSet(DflWindowReshapeCLBK clbk, DflWindow* pWindow)
{
    DFL_HANDLE(Window)->reshapeCLBK = clbk;
}

void dflWindowRepositionCLBKSet(DflWindowRepositionCLBK clbk, DflWindow* pWindow)
{
    DFL_HANDLE(Window)->repositionCLBK = clbk;
}

void dflWindowChangeModeCLBKSet(DflWindowChangeModeCLBK clbk, DflWindow* pWindow)
{
    DFL_HANDLE(Window)->modeCLBK = clbk;
}

void dflWindowRenameCLBKSet(DflWindowRenameCLBK clbk, DflWindow* pWindow)
{
    DFL_HANDLE(Window)->renameCLBK = clbk;
}

void dflWindowChangeIconCLBKSet(DflWindowChangeIconCLBK clbk, DflWindow* pWindow)
{
    DFL_HANDLE(Window)->iconCLBK = clbk;
}

void dflWindowDestroyCLBKSet(DflWindowCloseCLBK clbk, DflWindow* pWindow)
{
    DFL_HANDLE(Window)->destroyCLBK = clbk;
}