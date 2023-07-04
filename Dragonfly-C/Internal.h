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

#ifndef DFL_INTERNAL_H
#define DFL_INTERNAL_H

/*
* @brief Everything here is intended for internal use only.
*/

#include "Dragonfly.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#define DFL_HANDLE(type) ((struct Dfl##type##_T*)h##type) // a shorthand for casting a handle to its type (will be used when `type` refers to a function argument in the form `ptype` (pointer to handle))
#define DFL_HANDLE_ARRAY(type, pos) ((struct Dfl##type##_T*)*(p##type##s + pos)) // a shorthand for casting a handle to its type (will be used when `type` refers to a function argument in the form `ptype` (pointer to handle))

#define DFL_SESSION DFL_HANDLE(Session)
#define DFL_WINDOW  DFL_HANDLE(Window)
#define DFL_DEVICE  DFL_HANDLE(Device)
#define DFL_IMAGE   DFL_HANDLE(Image)

/* ================================ *
 *             SESSION              *
 * ================================ */

struct DflSession_T { // A Dragonfly session
    struct DflSessionInfo       info;
    VkInstance                  instance;
    VkDebugUtilsMessengerEXT    messenger;

    const char                  processorName[DFL_MAX_CHAR_COUNT];
    int                         processorCount;
    int                         processorSpeed;

    int                         deviceCount;

    int                         monitorCount;
    struct DflMonitorInfo       monitors[DFL_MAX_ITEM_COUNT];

    VkSurfaceKHR  surface; // this exists purely to make device creation work, because it requires to check a surface for presentation support.

    DflWindow    windows[DFL_MAX_ITEM_COUNT]; // the windows that are currently open in this session
    DflDevice    devices[DFL_MAX_ITEM_COUNT]; // the devices that are currently open in this session

    int flags;

    int error;

    //TODO: Add memory blocks for shared memory between threads.
};

/* ================================ *
 *             THREADS              *
 * ================================ */

struct DflThread_T
{
    void* handle;

    DflSession    session; // the session the thread is running on - where the shared memory is

    DflThreadProc process;
    int           stackSize;
};

/* ================================ *
 *             DEVICES              *
 * ================================ */

#define DFL_QUEUE_TYPE_GRAPHICS 0
#define DFL_QUEUE_TYPE_COMPUTE 1
#define DFL_QUEUE_TYPE_TRANSFER 2
#define DFL_QUEUE_TYPE_PRESENTATION 3

struct DflQueueCollection_T { // the order of the queues is: graphics, compute, transfer, presentation (see up)
    VkQueue*      handles[4];

    VkCommandPool comPool[4];

    uint32_t      count[4];

    uint32_t      index[4];
};


struct DflLocalMem_T {
    uint64_t size;
    uint32_t heapIndex;
};

// this is some data about the device that will be used.
// TODO: More fields to be added.
struct DflDevice_T {
    VkDevice                    device;

    VkPhysicalDevice            physDevice;
    int                         queueFamilyCount;
    struct DflQueueCollection_T queues;

    DflSession                  session;
    int                         index; // the index of the device in the session's window array

    /* -------------------- *
     *   GPU PROPERTIES     *
     * -------------------- */

    char name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];

    uint32_t              localHeaps;
    struct DflLocalMem_T* localMem;

    uint32_t maxDim1D;
    uint32_t maxDim2D;
    uint32_t maxDim3D;

    bool canDoGeomShade;
    bool canDoTessShade;

    bool doLowPerf;
    bool doAbuseMemory;

    bool canPresent;

    int error;
};


/* ================================ *
 *             WINDOWS              *
 * ================================ */

struct DflWindow_T { // A Dragonfly window
    GLFWwindow* handle;
    DflWindowInfo	info;

    DflSession      session;
    DflDevice       device;

    /* ------------------- *
    *   VULKAN SPECIFIC    *
    *  ------------------- */

    VkSurfaceKHR surface; // The surface of the window. If NULL, then the window doesn't have a surface.
    VkSwapchainKHR swapchain; // The swapchain is included here so it can change when the window is resized.
    int imageCount;
    int colorSpace;

    struct DflVec2D minRes;
    struct DflVec2D maxRes;

    DflWindowReshapeCLBK    reshapeCLBK;
    DflWindowRepositionCLBK repositionCLBK;
    DflWindowChangeModeCLBK modeCLBK;
    DflWindowRenameCLBK     renameCLBK;
    DflWindowChangeIconCLBK iconCLBK;

    DflWindowCloseCLBK      destroyCLBK;

    int error;
    int index; // the index of the window in the session's window array
};

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

 /*
* @brief Xreate a window. Not bound to a session or surface.
*/
DflWindow _dflWindowCreate(DflWindowInfo* pInfo, DflSession hSession);

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

/*
* @brief Destroy a window. Also frees the memory allocated for it.
*/
void _dflWindowDestroy(DflWindow hWindow);

/* ================================ *
 *             IMAGES               *
 * ================================ */

struct DflImage_T {
    unsigned char* data; // RGB and Alpha

    struct DflVec2D size;
    char            path[DFL_MAX_CHAR_COUNT]; // relative path to the image
};

#endif // !DFL_INTERNAL_H
