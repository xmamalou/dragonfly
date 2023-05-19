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

#include "../Data.h"
#include "../StbDummy.h"

struct DflWindow_T { // A Dragonfly window
    GLFWwindow* handle;
    DflWindowInfo	info;

    /* ------------------- *
    *   VULKAN SPECIFIC    *
    *  ------------------- */

    int surfaceIndex; // The index of the window in the surface array

    // CALLBACKS 

    DflWindowReshapeCLBK    reshapeCLBK;
    DflWindowRepositionCLBK repositionCLBK;
    DflWindowChangeModeCLBK modeCLBK;
    DflWindowRenameCLBK     renameCLBK;
    DflWindowChangeIconCLBK iconCLBK;

    DflWindowCloseCLBK      destroyCLBK;
};

/* -------------------- *
 *   INTERNAL           *
 * -------------------- */

static GLFWwindow* dflWindowCallHIDN(struct DflVec2D dim, struct DflVec2D view, struct DflVec2D res, const char* name, int mode);
static GLFWwindow* dflWindowCallHIDN(struct DflVec2D dim, struct DflVec2D view, struct DflVec2D res, const char* name, int mode)
{
    if (name == NULL)
        name = "Dragonfly-App";

    if (dim.x == NULL || dim.y == NULL)
        dim = (struct DflVec2D){ 1920, 1080 };

    if (view.x == NULL || view.y == NULL)
        view = (struct DflVec2D){ 1920, 1080 };

    if (res.x == NULL || res.y == NULL)
        res = (struct DflVec2D){ 1920, 1080 };

    if (!glfwInit())
        return NULL;

    switch (mode) {
    case DFL_WINDOWED:
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

inline static struct DflWindow_T* dflWindowAllocHIDN();
inline static struct DflWindow_T* dflWindowAllocHIDN()
{
    return calloc(1, sizeof(struct DflWindow_T));
}

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

DflWindow dflWindowCreate(DflWindowInfo* pInfo)
{
    if(pInfo == NULL)
    {
        pInfo = calloc(1, sizeof(DflWindowInfo));

        pInfo->dim = (struct DflVec2D){ 1920, 1080 };
        pInfo->view = (struct DflVec2D){ 1920, 1080 };
        pInfo->res = (struct DflVec2D){ 1920, 1080 };
        pInfo->pos = (struct DflVec2D){ 200, 200 };
        pInfo->name = "Dragonfly-App";
        pInfo->mode = DFL_WINDOWED;
    }

    struct DflWindow_T* window = dflWindowAllocHIDN();

    window->info = *pInfo;
    window->handle = dflWindowCallHIDN(pInfo->dim, pInfo->view, pInfo->res, pInfo->name, pInfo->mode);

    if (window->handle == NULL)
        return NULL;

    if (pInfo->icon != NULL)
    {
        GLFWimage image;
        image.pixels = stbi_load(window->info.icon, &image.width, &image.height, 0, 4); //rgba channels 
        if (image.pixels != NULL)
            glfwSetWindowIcon(window->handle, 1, &image);
        stbi_image_free(image.pixels);
    }

    if(pInfo->pos.x != NULL || pInfo->pos.y != NULL)
        glfwSetWindowPos(window->handle, window->info.pos.x, window->info.pos.y);

    return (DflWindow)window;
}

/* -------------------- *
 *   CHANGE             *
 * -------------------- */

void dflWindowReshape(int type, struct DflVec2D rect, DflWindow* pWindow)
{
    if (pWindow == NULL)
        return;
    if (*pWindow == NULL)
        return;

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
    if (pWindow == NULL)
        return;
    if (*pWindow == NULL)
        return;

    DFL_HANDLE(Window)->info.pos = pos;
    glfwSetWindowPos(DFL_HANDLE(Window)->handle, pos.x, pos.y);

    if (DFL_HANDLE(Window)->repositionCLBK != NULL)
        DFL_HANDLE(Window)->repositionCLBK(pos, pWindow);
}

void dflWindowChangeMode(int mode, DflWindow* pWindow)
{
    if (pWindow == NULL)
        return;
    if (*pWindow == NULL)
        return;

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
    if (pWindow == NULL)
        return;
    if (*pWindow == NULL)
        return;

    DFL_HANDLE(Window)->info.name = name;
    glfwSetWindowTitle(((struct DflWindow_T*)*pWindow)->handle, name);

    if (DFL_HANDLE(Window)->renameCLBK != NULL)
        DFL_HANDLE(Window)->renameCLBK(name, pWindow);
}

void dflWindowChangeIcon(const char* icon, DflWindow* pWindow)
{
    if (pWindow == NULL)
        return;
    if (*pWindow == NULL)
        return;

    GLFWimage image;
    image.pixels = stbi_load(icon, &image.width, &image.height, 0, 4); //rgba channels 
    if (image.pixels != NULL)
    {
        glfwSetWindowIcon(DFL_HANDLE(Window)->handle, 1, &image);
        DFL_HANDLE(Window)->info.icon = icon;
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
    if(window == NULL)
        return (struct DflVec2D){ .x = 0, .y = 0 };

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
    if(window != NULL)
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

/* -------------------- *
 *   INTERNAL ONLY      *
 *struct DflWindow_T--- */
size_t dflWindowSizeRequirementsGet()
{
    return sizeof(struct DflWindow_T);
}

GLFWwindow* dflWindowHandleGet(DflWindow window)
{
    return (((struct DflWindow_T*)window)->handle);
}

void dflWindowSurfaceIndexSet(int index, DflWindow* pWindow)
{
    if (pWindow == NULL)
        return;
    if (*pWindow == NULL)
        return;

    DFL_HANDLE(Window)->surfaceIndex = index;
}

/* -------------------- *
 *   CALLBACK SETTERS   *
 * -------------------- */

void dflWindowReshapeCLBKSet(DflWindowReshapeCLBK clbk, DflWindow* pWindow)
{
    if (pWindow == NULL)
        return;
    if (*pWindow == NULL)
        return;

    DFL_HANDLE(Window)->reshapeCLBK = clbk;
}

void dflWindowRepositionCLBKSet(DflWindowRepositionCLBK clbk, DflWindow* pWindow)
{
    if (pWindow == NULL)
        return;
    if (*pWindow == NULL)
        return;

    DFL_HANDLE(Window)->repositionCLBK = clbk;
}

void dflWindowChangeModeCLBKSet(DflWindowChangeModeCLBK clbk, DflWindow* pWindow)
{
    if (pWindow == NULL)
        return;
    if (*pWindow == NULL)
        return;

    DFL_HANDLE(Window)->modeCLBK = clbk;
}

void dflWindowRenameCLBKSet(DflWindowRenameCLBK clbk, DflWindow* pWindow)
{
    if (pWindow == NULL)
        return;
    if (*pWindow == NULL)
        return;

    DFL_HANDLE(Window)->renameCLBK = clbk;
}

void dflWindowChangeIconCLBKSet(DflWindowChangeIconCLBK clbk, DflWindow* pWindow)
{
    if (pWindow == NULL)
        return;
    if (*pWindow == NULL)
        return;

    DFL_HANDLE(Window)->iconCLBK = clbk;
}

void dflWindowDestroyCLBKSet(DflWindowCloseCLBK clbk, DflWindow* pWindow)
{
    if(pWindow == NULL)
        return;
    if(*pWindow == NULL)
        return;

    DFL_HANDLE(Window)->destroyCLBK = clbk;
}

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void dflWindowDestroy(DflWindow* pWindow)
{
    if(pWindow == NULL)
        return;
    if(*pWindow == NULL)
        return;

    if (DFL_HANDLE(Window)->destroyCLBK != NULL)
        DFL_HANDLE(Window)->destroyCLBK(pWindow);

    glfwDestroyWindow(DFL_HANDLE(Window)->handle);

    free(*pWindow);
}