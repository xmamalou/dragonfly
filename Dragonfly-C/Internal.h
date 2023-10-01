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

#ifdef _WIN32
    #define STRCPY(dst, size, src) strcpy_s(dst, size, src)
#else 
    #define STRCPY(dst, size, src) strcpy(dst, src)
#endif

#define DFL_HANDLE(type)            ((struct Dfl##type##_T*)h##type) // a shorthand for casting a handle to its type (will be used when `type` refers to a function argument in the form `ptype` (pointer to handle))

#define DFL_SESSION     DFL_HANDLE(Session)

#define DFL_WINDOW      DFL_HANDLE(Window)

#define DFL_IMAGE       DFL_HANDLE(Image)

/* ================================ *
 *             SESSION              *
 * ================================ */

struct DflSession_T { // A Dragonfly session
    struct DflSessionInfo       info;
    VkInstance                  instance;
    VkDebugUtilsMessengerEXT    messenger;

    int      processorCount;
    uint64_t processorSpeed;
    int      threadCount;

    uint64_t memorySize;

    uint32_t            deviceCount;
    struct DflDevice_T* devices;

    uint32_t               monitorCount;
    struct DflMonitorInfo* monitors;

    DflWindow    windows[DFL_MAX_ITEM_COUNT]; // the windows that are currently open in this session

    int flags;
    int error;
};

/* ================================ *
 *             DEVICES              *
 * ================================ */

#define DFL_QUEUE_TYPE_GRAPHICS 0x0001
#define DFL_QUEUE_TYPE_COMPUTE 0x0002
#define DFL_QUEUE_TYPE_TRANSFER 0x0004
#define DFL_QUEUE_TYPE_PRESENTATION 0x0008

struct DflQueue_T {
    VkQueue queue;
    uint32_t index;
};

struct DflSingleTypeQueueArray_T {
    uint32_t            count;
    struct DflQueue_T*  pQueues;
};

struct DflQueueFamily_T { 
    VkCommandPool comPool;
    uint32_t      queueCount;
    uint32_t      queueType;
    bool          canPresent;

    bool          isUsed;
};


struct DflMem_T {
    VkDeviceSize    size;
    uint32_t        heapIndex;
    bool            isHostVisible;
};

// this is some data about the device that will be used.
// TODO: More fields to be added.
struct DflDevice_T {
    VkDevice                    device;

    /* -------------------- *
     *        QUEUES        *
     * -------------------- */

    VkPhysicalDevice            physDevice;
    uint32_t                    queueFamilyCount;
    struct DflQueueFamily_T*    pQueueFamilies;

    struct DflSingleTypeQueueArray_T queues[4]; // 0: graphics, 1: compute, 2: transfer, 3: presentation

    /* -------------------- *
     *   GPU PROPERTIES     *
     * -------------------- */

    char name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];

    uint32_t         localHeaps;
    struct DflMem_T  localMem[DFL_MAX_ITEM_COUNT];

    uint32_t         nonLocalHeaps;
    struct DflMem_T  nonLocalMem[DFL_MAX_ITEM_COUNT];

    uint32_t maxDim1D;
    uint32_t maxDim2D;
    uint32_t maxDim3D;

    bool canDoGeomShade;
    bool canDoTessShade;

    bool doLowPerf;
    bool doAbuseMemory;

    bool canPresent;

    uint32_t extensionsCount;
    char**   extensionNames;

    uint32_t rank; // a number that represents how good the device is. The higher the better. It is calculated based on some of the above fields. (Not yet decided how)
};


/* ================================ *
 *             WINDOWS              *
 * ================================ */

struct DflWindow_T { // A Dragonfly window
    GLFWwindow* handle;
    DflWindowInfo	info;

    DflSession      session;
    uint32_t        deviceIndex;

    /* ------------------- *
    *   VULKAN SPECIFIC    *
    *  ------------------- */

    VkSurfaceKHR   surface; // The surface of the window. If NULL, then the window doesn't have a surface.
    VkSwapchainKHR swapchain; // The swapchain is included here so it can change when the window is resized.
    int            imageCount;
    VkImage*       images;
    VkImageView*   imageViews;
    int            colorSpace;

    int      error;
    uint32_t index; // the index of the window in the session's window array
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
extern inline void _dflWindowDestroy(DflWindow hWindow);

/* ================================ *
 *             IMAGES               *
 * ================================ */

struct DflImage_T {
    unsigned char* data; // RGB and Alpha

    struct DflVec2D size;
    char            path[DFL_MAX_CHAR_COUNT]; // relative path to the image
};

#endif // !DFL_INTERNAL_H
