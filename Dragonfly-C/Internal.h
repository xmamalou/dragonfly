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

#define DFL_HANDLE(type)            ((struct Dfl##type##_T*)h##type) // a shorthand for casting a handle to its type (will be used when `type` refers to a function argument in the form `ptype` (pointer to handle))
#define DFL_HANDLE_ARRAY(type, pos) ((struct Dfl##type##_T*)*(p##type##s + pos)) // a shorthand for casting a handle to its type (will be used when `type` refers to a function argument in the form `ptype` (pointer to handle))

#define DFL_SESSION     DFL_HANDLE(Session)

#define DFL_MEMORY_POOL DFL_HANDLE(MemoryPool)
#define DFL_BUFFER      DFL_HANDLE(Buffer)

#define DFL_WINDOW      DFL_HANDLE(Window)
#define DFL_DEVICE      DFL_HANDLE(Device)

#define DFL_IMAGE       DFL_HANDLE(Image)

/* ================================ *
 *             SESSION              *
 * ================================ */

struct DflSession_T { // A Dragonfly session
    struct DflSessionInfo       info;
    VkInstance                  instance;
    VkDebugUtilsMessengerEXT    messenger;

    int                         processorCount;
    unsigned long long          processorSpeed;
    int                         threadCount;

    unsigned long long          memorySize;

    int                         deviceCount;

    int                         monitorCount;
    struct DflMonitorInfo*      monitors;

    VkSurfaceKHR  surface; // this exists purely to make device creation work, because it requires to check a surface for presentation support.

    DflWindow    windows[DFL_MAX_ITEM_COUNT]; // the windows that are currently open in this session
    DflDevice    devices[DFL_MAX_ITEM_COUNT]; // the devices that are currently open in this session

    int flags;

    int                error;
    unsigned long long warning;

    DflMemoryPool sharedMemory; // will be used for threads
};

/* ================================ *
 *             MEMORY               *
 * ================================ */

struct DflMemoryBlock_T
{
    int size; // in BYTES, not in 4-byte words like in dflMemoryBlockInit
    int used; // in BYTES, not in 4-byte words like in dflMemoryBlockInit
    void* startingAddress;

    int firstAvailableSlot; // the first available slot in a memory block. Must, obviously, be smaller than size/4.
    // firstAvailableSlot *isn't* the same as used, because used just tells how much memory is
    // used and not where the free memory is. 

    VkDeviceMemory hMemoryHandle;
    DflDevice      hDevice;
    void*          miniBuff; // if memory is unmappable, then this will be a small (4 bytes) buffer that will be used to copy data to the vulkan buffer.

    struct DflMemoryBlock_T* next; // the next memory block
};

struct DflMemoryPool_T
{
    int size; // in BYTES, not in 4-byte words like in dflMemoryPoolInit
    int used; // in BYTES, not in 4-byte words like in dflMemoryPoolInit

    struct DflMemoryBlock_T* firstBlock; // the first memory block

    int error;
};

struct DflBufferChunk_T // chunks concern individual memory blocks and are part of a buffer
{
    struct DflMemoryBlock_T* associatedBlock; // the memory block that this chunk is part of
    int size; // the size of the chunk
    int offset; // the offset of the chunk in the memory block

    struct DflBufferChunk_T* next; // the next chunk in the buffer

    VkBuffer        buffer;
    VkCommandBuffer transferOp; // the tranfer operation used for unmapped buffers responsible for copying the data from the starting address to the vulkan buffer. 
    VkEvent         hostCanGetNextPart; // since the small host buffer for unmapped buffer is just 4 bytes, we need to signal to the host each time 4 bytes are transferred to the vulkan buffer so the host can copy the next 4 bytes to the small host buffer.
    VkFence         isTransferDone; // the fence will be signaled when the transfer operation is done.
};

struct DflBuffer_T
{
    int size; 

    // NOTE: if (size of buffer) + (offset) > (size of memory block), then the buffer is split across multiple memory blocks, UNLESS
    // there is no other memory block (pNext is NULL), where an OOM error will be thrown. Automatic pool extension won't be done.
    struct DflBufferChunk_T* firstChunk;

    // DflBuffers won't have an error field, the memory pool will have buffer related errors reported in its error field instead.
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


struct DflMem_T {
    VkDeviceSize    size;
    uint32_t        heapIndex;
    bool            isHostVisible;
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

    VkSurfaceKHR   surface; // The surface of the window. If NULL, then the window doesn't have a surface.
    VkSwapchainKHR swapchain; // The swapchain is included here so it can change when the window is resized.
    int            imageCount;
    VkImage*       images;
    VkImageView*   imageViews;
    int            colorSpace;

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
