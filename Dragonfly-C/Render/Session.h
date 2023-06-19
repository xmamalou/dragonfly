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

#ifndef DFL_RENDER_SESSION_H
#define DFL_RENDER_SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan.h>

#include "../Data.h" 
#include "Window.h"

#define DFL_VERSION(major, minor, patch) VK_MAKE_API_VERSION(0, major, minor, patch)

/* -------------------- *
 *   TYPES              *
 * -------------------- */

#define DFL_SESSION_CRITERIA_NONE 0x00000000 // Dragonfly will operate as it sees fit. No extra criteria on how to manage the session.
#define DFL_SESSION_CRITERIA_ONLY_OFFSCREEN 0x00000001 // Dragonfly implicitly assumes that on-screen rendering is desired. Use this flag to override that assumption.
#define DFL_SESSION_CRITERIA_DO_DEBUG 0x00000002 // Dragonfly will enable validation layers and other debug features

#define DFL_GPU_CRITERIA_NONE 0x00000000 // Dragonfly will choose the best GPU available - no extra criteria on how to use it. It will use it as it sees fit.
#define DFL_GPU_CRITERIA_ONLY_OFFSCREEN 0x00000001 // Dragonfly will implicitly assume that on-screen rendering is desired. Use this flag to override that assumption.
#define DFL_GPU_CRITERIA_LOW_PERFORMANCE 0x00000002 // Dragonfly will try to use the least intensive GPU available
#define DFL_GPU_CRITERIA_ABUSE_MEMORY 0x00000004 // Dragonfly will normally implicitly leave a little wiggle room for GPU memory - use this flag to override that assumption
#define DFL_GPU_CRITERIA_ALL_QUEUES 0x00000008 // Dragonfly will reserve only one queue for each queue family - use this flag to override that assumption

struct DflSessionInfo
{
    const char* appName;
    uint32_t    appVersion;
    uint32_t    sessionCriteria;
};

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

// initializes Vulkan -- implicitly assumes that on-screen rendering is desired.
// Use DFL_SESSION_CRITERIA_ONLY_OFFSCREEN to override this assumption.
DflSession  dflSessionCreate(struct DflSessionInfo* pInfo);
// unlike `dflWindowCreate`, this also binds the window to a surface.
DflWindow   dflWindowInit(struct DflWindowInfo* pWindowInfo, DflSession* pSession);
DflDevice   dflDeviceInit(int GPUCriteria, int choice, DflDevice* pDevices, DflSession* pSession);

/* -------------------- *
 *   GET & SET          *
 * -------------------- */
/**
* @brief Returns the number of devices that are available to the session, plus the devices themselves.
* Note that they are all uninitialized at this state.
* 
* @param pCount: A pointer to an integer that will be set to the number of devices available to the session.
*/
DflDevice*                 dflSessionDevicesGet(int* pCount, DflSession* pSession);
extern inline int          dflSessionErrorGet(DflSession session);
extern inline bool         dflDeviceCanPresentGet(DflDevice device);
extern inline const char*  dflDeviceNameGet(DflDevice device);
extern inline int          dflDeviceErrorGet(DflDevice device);
/**
 * @brief This is an internal function used to check whether other internal functions can be used
 * The flag this function checks for is set only by the library itself and cannot be set by the user.
*/
extern inline bool        _dflSessionIsLegalGet(DflSession session);
extern inline VkInstance  _dflSessionInstanceGet(DflSession session);

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

 /*
 * @brief Destroy a session. Also frees the memory allocated for it.
 * 
 * Must be called after destroying any object that was created with this session.
 */
void dflSessionDestroy(DflSession* pSession);

void dflDeviceTerminate(DflDevice* pDevice, DflSession* pSession);
void dflWindowTerminate(DflWindow* pWindow, DflSession* pSession);


#ifdef __cplusplus
}
#endif

#endif // !DFL_RENDER_SESSION_H