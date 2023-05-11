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

#include "GLFW.h"

#include <stdlib.h>

#include "../Data.h"
#include "../StbDummy.h"
#include "Vulkan.h"

struct DflWindow_T { // A Dragonfly window
    GLFWwindow* handle;
    DflWindowInfo	info;

    /* ------------------- *
    *   VULKAN SPECIFIC    *
    *  ------------------- */

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
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        return glfwCreateWindow(dim.x, dim.y, name, NULL, NULL);
    case DFL_FULLSCREEN:
        return glfwCreateWindow(dim.x, dim.y, name, glfwGetPrimaryMonitor(), NULL);
    case DFL_BORDERLESS:
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        return glfwCreateWindow(dim.x, dim.y, name, NULL, NULL);
    default:
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

DflWindow dflWindowCreate(DflWindowInfo* pInfo, DflSession* session)
{
    if(session == NULL)
        return NULL;
    if(*session == NULL)
        return NULL;
    if (dflSessionCanPresentGet(*session) == false)
        return NULL;

    if(pInfo == NULL)
    {
        pInfo = calloc(1, sizeof(DflWindowInfo));

        pInfo->dim = (struct DflVec2D){ 1920, 1080 };
        pInfo->view = (struct DflVec2D){ 1920, 1080 };
        pInfo->res = (struct DflVec2D){ 1920, 1080 };
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

void dflWindowReshape(struct DflVec2D rect, int type, DflWindow* pWindow)
{
    if(pWindow == NULL)
        return;

    switch (type) {
    case DFL_DIMENSIONS:
        ((struct DflWindow_T*)*pWindow)->info.dim = rect;
        glfwSetWindowSize(((struct DflWindow_T*)*pWindow)->handle, rect.x, rect.y);
        break;
    case DFL_VIEWPORT:
        ((struct DflWindow_T*)*pWindow)->info.view = rect;
        break;
    default:
        ((struct DflWindow_T*)*pWindow)->info.res = rect;
        break;
    }

    if (((struct DflWindow_T*)*pWindow)->reshapeCLBK != NULL)
        ((struct DflWindow_T*)*pWindow)->reshapeCLBK(rect, type, pWindow);
}

void dflWindowReposition(struct DflVec2D pos, DflWindow* pWindow)
{
    if(pWindow == NULL)
        return;

    ((struct DflWindow_T*)*pWindow)->info.pos = pos;
    glfwSetWindowPos(((struct DflWindow_T*)*pWindow)->handle, pos.x, pos.y);

    if (((struct DflWindow_T*)*pWindow)->repositionCLBK != NULL)
        ((struct DflWindow_T*)*pWindow)->repositionCLBK(pos, pWindow);
}

void dflWindowChangeMode(int mode, DflWindow* pWindow)
{
    if(pWindow == NULL)
        return;

    ((struct DflWindow_T*)*pWindow)->info.mode = mode;
    switch (mode) {
    case DFL_WINDOWED:
        glfwSetWindowAttrib(((struct DflWindow_T*)*pWindow)->handle, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(((struct DflWindow_T*)*pWindow)->handle, NULL, 0, 0, ((struct DflWindow_T*)*pWindow)->info.dim.x, ((struct DflWindow_T*)*pWindow)->info.dim.y, GLFW_DONT_CARE);
        break;
    case DFL_FULLSCREEN:
        glfwSetWindowMonitor(((struct DflWindow_T*)*pWindow)->handle, glfwGetPrimaryMonitor(), 0, 0, ((struct DflWindow_T*)*pWindow)->info.dim.x, ((struct DflWindow_T*)*pWindow)->info.dim.y, GLFW_DONT_CARE);
        break;
    case DFL_BORDERLESS:
        glfwSetWindowAttrib(((struct DflWindow_T*)*pWindow)->handle, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowMonitor(((struct DflWindow_T*)*pWindow)->handle, NULL, 0, 0, ((struct DflWindow_T*)*pWindow)->info.dim.x, ((struct DflWindow_T*)*pWindow)->info.dim.y, GLFW_DONT_CARE);
        break;
    default:
        glfwSetWindowAttrib(((struct DflWindow_T*)*pWindow)->handle, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(((struct DflWindow_T*)*pWindow)->handle, NULL, 0, 0, ((struct DflWindow_T*)*pWindow)->info.dim.x, ((struct DflWindow_T*)*pWindow)->info.dim.y, GLFW_DONT_CARE);
        break;
    }

    if (((struct DflWindow_T*)*pWindow)->modeCLBK != NULL)
        ((struct DflWindow_T*)*pWindow)->modeCLBK(mode, pWindow);
}

void dflWindowRename(const char* name, DflWindow* pWindow)
{
    if(pWindow == NULL)
        return;

    ((struct DflWindow_T*)*pWindow)->info.name = name;
    glfwSetWindowTitle(((struct DflWindow_T*)*pWindow)->handle, name);

    if (((struct DflWindow_T*)*pWindow)->renameCLBK != NULL)
        ((struct DflWindow_T*)*pWindow)->renameCLBK(name, pWindow);
}

void dflWindowChangeIcon(const char* icon, DflWindow* pWindow)
{
    if(pWindow == NULL)
        return;

    GLFWimage image;
    image.pixels = stbi_load(icon, &image.width, &image.height, 0, 4); //rgba channels 
    if (image.pixels != NULL)
    {
        glfwSetWindowIcon(((struct DflWindow_T*)*pWindow)->handle, 1, &image);
        ((struct DflWindow_T*)*pWindow)->info.icon = icon;
    }
    stbi_image_free(image.pixels);

    if (((struct DflWindow_T*)*pWindow)->iconCLBK != NULL)
        ((struct DflWindow_T*)*pWindow)->iconCLBK(icon, pWindow);
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
    glfwGetMonitorPos(glfwGetPrimaryMonitor(), &pos.x, &pos.y);
    return pos;
}

/* -------------------- *
 *   CALLBACK SETTERS   *
 * -------------------- */

void dflWindowReshapeCLBKSet(DflWindowReshapeCLBK clbk, DflWindow* pWindow)
{
    ((struct DflWindow_T*)*pWindow)->reshapeCLBK = clbk;
}

void dflWindowRepositionCLBKSet(DflWindowRepositionCLBK clbk, DflWindow* pWindow)
{
    ((struct DflWindow_T*)*pWindow)->repositionCLBK = clbk;
}

void dflWindowChangeModeCLBKSet(DflWindowChangeModeCLBK clbk, DflWindow* pWindow)
{
    ((struct DflWindow_T*)*pWindow)->modeCLBK = clbk;
}

void dflWindowRenameCLBKSet(DflWindowRenameCLBK clbk, DflWindow* pWindow)
{
    ((struct DflWindow_T*)*pWindow)->renameCLBK = clbk;
}

void dflWindowChangeIconCLBKSet(DflWindowChangeIconCLBK clbk, DflWindow* pWindow)
{
    ((struct DflWindow_T*)*pWindow)->iconCLBK = clbk;
}

void dflWindowDestroyCLBKSet(DflWindowCloseCLBK clbk, DflWindow* pWindow)
{
    ((struct DflWindow_T*)*pWindow)->destroyCLBK = clbk;
}

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void dflWindowDestroy(DflWindow* pWindow)
{
    if (((struct DflWindow_T*)*pWindow)->destroyCLBK != NULL)
        ((struct DflWindow_T*)*pWindow)->destroyCLBK(pWindow);

    glfwDestroyWindow(((struct DflWindow_T*)*pWindow)->handle);
    glfwTerminate();

    free(*pWindow);
}