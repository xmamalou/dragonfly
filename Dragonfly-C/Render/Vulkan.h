#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan.h>

#include "../Data.h" 
#include "../Render/GLFW.h"

#define DFL_VERSION(major, minor, patch) VK_MAKE_API_VERSION(0, major, minor, patch)

// TYPES

#define DFL_SESSION_CRITERIA_NONE 0 // Dragonfly will operate as it sees fit. No extra criteria on how to manage the session.
#define DFL_SESSION_CRITERIA_ENABLE_LAYERS 1 // Dragonfly will enable Vulkan layers -if available. This is not recommended for production use.

#define DFL_GPU_CRITERIA_NONE 0 // Dragonfly will choose the best GPU available - no extra criteria on how to use it. It will use it as it sees fit.
#define DFL_GPU_CRITERIA_HASTY 1 // Dragonfly will choose the first GPU available - no extra criteria on how to use it. It will use it as it sees fit.
#define DFL_GPU_CRITERIA_INTEGRATED 2 // Dragonfly implicitly assumes that the desired GPU is discrete - use this flag to override that assumption
#define DFL_GPU_CRITERIA_LOW_PERFORMANCE 4 // Dragonfly will try to use the least intensive GPU available
#define DFL_GPU_CRITERIA_UNDERWORK 8 // Dragonfly will pick the best GPU available, but it will omit features for the sake of performance.
#define DFL_GPU_CRITERIA_ABUSE_MEMORY 16 // Dragonfly implicitly leaves a little wiggle room for GPU memory - use this flag to override that assumption

struct DflSessionInfo
{
    const char* appName;
    uint32_t    appVersion;
    int         debug;
    bool        offscreenWorkOnly; // If set, Dragonfly will not render to a window.
};
// opaque handle for a DflSession_T object.
DFL_MAKE_HANDLE(DflSession);

// FUNCTIONS

// Initializes Vulkan + [to be added]. 
// SessionCrit: Session criteria. Form by using the DFL_SESSION_CRITERIA_* flags.
// GPUCrit: GPU criteria. Form by using the DFL_GPU_CRITERIA_* flags.
DflSession dflSessionInit(struct DflSessionInfo* info, int sessionCrit, int GPUCrit);

void dflSessionEnd(DflSession* session);

#ifdef __cplusplus
}
#endif