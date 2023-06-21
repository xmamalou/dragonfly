#ifndef DFL_INTERNAL_H
#define DFL_INTERNAL_H

/*
* @brief Everything here is intended for internal use only.
*/

#include "Dragonfly.h"

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

    VkSurfaceKHR  surface; // this exists purely to make device creation work, because it requires to check a surface for presentation support.

    int flags;

    int error;
};

/* ================================ *
 *             DEVICES              *
 * ================================ */

#define DFL_QUEUE_TYPE_GRAPHICS 0
#define DFL_QUEUE_TYPE_COMPUTE 1
#define DFL_QUEUE_TYPE_TRANSFER 2
#define DFL_QUEUE_TYPE_PRESENTATION 3

struct DflQueueCollection_T { // the order of the queues is: graphics, compute, transfer, presentation (see up)
    VkQueue* handles[4];

    uint32_t count[4];

    uint32_t index[4];
};


struct DflLocalMem_T {
    uint64_t size;
    uint32_t heapIndex;
};

// this is some data about the device that will be used.
// TODO: More fields to be added.
struct DflDevice_T {
    VkDevice                    device;
    int                         queueFamilyCount;
    struct DflQueueCollection_T queues;
    VkPhysicalDevice            physDevice;

    DflSession                  session;

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
 *   INITIALIZE         *
 * -------------------- */

 /*
* @brief Xreate a window. Not bound to a session or surface.
*/
DflWindow _dflWindowCreate(DflWindowInfo* pInfo);

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

/*
* @brief Destroy a window. Also frees the memory allocated for it.
*/
void _dflWindowDestroy(DflWindow* pWindow);

#endif // !DFL_INTERNAL_H
