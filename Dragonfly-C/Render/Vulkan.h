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

#ifndef DFL_RENDER_VULKAN_H
#define DFL_RENDER_VULKAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan.h>

#include "../Data.h" 

#define DFL_VERSION(major, minor, patch) VK_MAKE_API_VERSION(0, major, minor, patch)

/* -------------------- *
 *   TYPES              *
 * -------------------- */

#define DFL_SESSION_CRITERIA_NONE 0 // Dragonfly will operate as it sees fit. No extra criteria on how to manage the session.
#define DFL_SESSION_CRITERIA_ONLY_OFFSCREEN 1 // Dragonfly implicitly assumes that on-screen rendering is desired. Use this flag to override that assumption.
#define DFL_SESSION_CRITERIA_DO_DEBUG 2 // Dragonfly will enable validation layers and other debug features

#define DFL_GPU_CRITERIA_NONE 0 // Dragonfly will choose the best GPU available - no extra criteria on how to use it. It will use it as it sees fit.
#define DFL_GPU_CRITERIA_HASTY 1 // Dragonfly will choose the first GPU available - no extra criteria on how to use it. It will use it as it sees fit.
#define DFL_GPU_CRITERIA_INTEGRATED 2 // Dragonfly will normally implicitly assume that the desired GPU is discrete - use this flag to override that assumption. Won't fail if no integrated GPU is found.
#define DFL_GPU_CRITERIA_LOW_PERFORMANCE 4 // Dragonfly will try to use the least intensive GPU available
#define DFL_GPU_CRITERIA_ABUSE_MEMORY 16 // Dragonfly will normally implicitly leave a little wiggle room for GPU memory - use this flag to override that assumption
#define DFL_GPU_CRITERIA_ALL_QUEUES 32 // Dragonfly will reserve only one queue for each queue family - use this flag to override that assumption

struct DflSessionInfo
{
    const char* appName;
    uint32_t    appVersion;
};
// opaque handle for a DflSession_T object.
DFL_MAKE_HANDLE(DflSession);

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

// initializes Vulkan -- implicitly assumes that on-screen rendering is desired.
// Use DFL_SESSION_CRITERIA_ONLY_OFFSCREEN to override this assumption.
// `sessionCriteria` is a bitfield of `DFL_SESSION_CRITERIA_*` flags.
// `GPUCriteria` is a bitfield of `DFL_GPU_CRITERIA_*` flags.
DflSession dflSessionInit(struct DflSessionInfo* pInfo, int sessionCriteria, int GPUCriteria);

// initializes the rest of the session (the device). This is a separate function so it can be called either during
// `dflSessionInit` (if onscreen rendering is disabled) or during `dflWindowCreate` (if onscreen rendering is enabled).
// this isn't meant to be called by the user, but rather by the window creation function (if needed), which is in Render/GLFW.h
void dflSessionDeviceInit(VkSurfaceKHR* surface, DflSession* pSession);

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

bool dflSessionCanPresentGet(DflSession session);

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

// Also frees the session pointer. Does not destroy any associated windows. Any on-screen specific resources are not freed from this function.
// Use `dflWindowDestroy` (see `Render/GLFW.h`) to destroy any associated windows and on-screen rendering specific resources.
void dflSessionEnd(DflSession* pSession);

#ifdef __cplusplus
}
#endif

#endif // !DFL_RENDER_VULKAN_H