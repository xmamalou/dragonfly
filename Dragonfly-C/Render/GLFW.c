#include "GLFW.h"

#include <stdlib.h>

#include "../StbDummy.h"

typedef struct DflWindow_T { // A Dragonfly window
    GLFWwindow*     window;
    DflWindowInfo	info;

    // CALLBACKS 

    DflWindowReshapeCLBK    reshapeCLBK;
    DflWindowRepositionCLBK repositionCLBK;
    DflWindowChangeModeCLBK modeCLBK;
    DflWindowRenameCLBK     renameCLBK;
    DflWindowChangeIconCLBK iconCLBK;

    DflWindowCloseCLBK      closeCLBK;
} DflWindow_T;

static GLFWwindow* dflWindowCallHIDN(struct DflVec2D dim, struct DflVec2D view, struct DflVec2D res, const char* name, const char* icon, int mode);
static GLFWwindow* dflWindowCallHIDN(struct DflVec2D dim, struct DflVec2D view, struct DflVec2D res, const char* name, const char* icon, int mode)
{
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

// HEADER FUNCTIONS

DflWindow dflWindowCreate(DflWindowInfo* info)
{
    DflWindow_T* window = dflWindowAllocHIDN();

    window->info = *info;
    window->window = dflWindowCallHIDN(info->dim, info->view, info->res, info->name, info->icon, info->mode);

    if (window->window == NULL)
        return NULL;

    GLFWimage image;
    image.pixels = stbi_load(window->info.icon, &image.width, &image.height, 0, 4); //rgba channels 
    if(image.pixels != NULL)
        glfwSetWindowIcon(window->window, 1, &image);
    stbi_image_free(image.pixels);

    glfwSetWindowPos(window->window, window->info.pos.x, window->info.pos.y);

    return (DflWindow)window;
}

void dflWindowReshape(struct DflVec2D rect, int type, DflWindow* window)
{
    switch (type) {
    case DFL_DIMENSIONS:
        ((struct DflWindow_T*)*window)->info.dim = rect;
        glfwSetWindowSize(((struct DflWindow_T*)*window)->window, rect.x, rect.y);
        break;
    case DFL_VIEWPORT:
        ((struct DflWindow_T*)*window)->info.view = rect;
        break;
    default:
        ((struct DflWindow_T*)*window)->info.res = rect;
        break;
    }

    if (((struct DflWindow_T*)*window)->reshapeCLBK != NULL)
        ((struct DflWindow_T*)*window)->reshapeCLBK(rect, type, window);
}

void dflWindowReposition(struct DflVec2D pos, DflWindow* window)
{
    glfwSetWindowPos(((struct DflWindow_T*)*window)->window, pos.x, pos.y);

    if (((struct DflWindow_T*)*window)->repositionCLBK != NULL)
        ((struct DflWindow_T*)*window)->repositionCLBK(pos, window);
}

void dflWindowChangeMode(int mode, DflWindow* window)
{
    switch (mode) {
    case DFL_WINDOWED:
        glfwSetWindowAttrib(((struct DflWindow_T*)*window)->window, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(((struct DflWindow_T*)*window)->window, NULL, 0, 0, ((struct DflWindow_T*)*window)->info.dim.x, ((struct DflWindow_T*)*window)->info.dim.y, GLFW_DONT_CARE);
        break;
    case DFL_FULLSCREEN:
        glfwSetWindowMonitor(((struct DflWindow_T*)*window)->window, glfwGetPrimaryMonitor(), 0, 0, ((struct DflWindow_T*)*window)->info.dim.x, ((struct DflWindow_T*)*window)->info.dim.y, GLFW_DONT_CARE);
        break;
    case DFL_BORDERLESS:
        glfwSetWindowAttrib(((struct DflWindow_T*)*window)->window, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowMonitor(((struct DflWindow_T*)*window)->window, NULL, 0, 0, ((struct DflWindow_T*)*window)->info.dim.x, ((struct DflWindow_T*)*window)->info.dim.y, GLFW_DONT_CARE);
        break;
    default:
        glfwSetWindowAttrib(((struct DflWindow_T*)*window)->window, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(((struct DflWindow_T*)*window)->window, NULL, 0, 0, ((struct DflWindow_T*)*window)->info.dim.x, ((struct DflWindow_T*)*window)->info.dim.y, GLFW_DONT_CARE);
        break;
    }

    if (((struct DflWindow_T*)*window)->modeCLBK != NULL)
        ((struct DflWindow_T*)*window)->modeCLBK(mode, window);
}

void dflWindowRename(const char* name, DflWindow* window)
{
    ((struct DflWindow_T*)*window)->info.name = name;
    glfwSetWindowTitle(((struct DflWindow_T*)*window)->window, name);

    if (((struct DflWindow_T*)*window)->renameCLBK != NULL)
        ((struct DflWindow_T*)*window)->renameCLBK(name, window);
}

void dflWindowChangeIcon(const char* icon, DflWindow* window)
{
    GLFWimage image;
    image.pixels = stbi_load(icon, &image.width, &image.height, 0, 4); //rgba channels 
    if (image.pixels != NULL)
    {
        glfwSetWindowIcon(((struct DflWindow_T*)*window)->window, 1, &image);
        ((struct DflWindow_T*)*window)->info.icon = icon;
    }
    stbi_image_free(image.pixels);

    if (((struct DflWindow_T*)*window)->iconCLBK != NULL)
        ((struct DflWindow_T*)*window)->iconCLBK(icon, window);
}

// GETTERS

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
    glfwGetWindowPos(((struct DflWindow_T*)window)->window, &pos.x, &pos.y);
    return pos;
}

struct DflVec2D dflPrimaryMonitorPosGet()
{
    struct DflVec2D pos = { .x = 0, .y = 0 };
    glfwGetMonitorPos(glfwGetPrimaryMonitor(), &pos.x, &pos.y);
    return pos;
}

// OTHER

void dflWindowDestroy(DflWindow* window)
{
    if (((struct DflWindow_T*)*window)->closeCLBK != NULL)
        ((struct DflWindow_T*)*window)->closeCLBK(window);

    glfwDestroyWindow(((struct DflWindow_T*)*window)->window);
    glfwTerminate();

    free(*window);
}

// CALLBACKS

void dflWindowReshapeCLBKSet(DflWindowReshapeCLBK clbk, DflWindow* window)
{
    ((struct DflWindow_T*)*window)->reshapeCLBK = clbk;
}

void dflWindowRepositionCLBKSet(DflWindowRepositionCLBK clbk, DflWindow* window)
{
    ((struct DflWindow_T*)*window)->repositionCLBK = clbk;
}

void dflWindowChangeModeCLBKSet(DflWindowChangeModeCLBK clbk, DflWindow* window)
{
    ((struct DflWindow_T*)*window)->modeCLBK = clbk;
}

void dflWindowRenameCLBKSet(DflWindowRenameCLBK clbk, DflWindow* window)
{
    ((struct DflWindow_T*)*window)->renameCLBK = clbk;
}

void dflWindowChangeIconCLBKSet(DflWindowChangeIconCLBK clbk, DflWindow* window)
{
    ((struct DflWindow_T*)*window)->iconCLBK = clbk;
}

void dflWindowCloseCLBKSet(DflWindowCloseCLBK clbk, DflWindow* window)
{
    ((struct DflWindow_T*)*window)->closeCLBK = clbk;
}


