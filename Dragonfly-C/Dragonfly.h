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

#ifndef DFL_DRAGONFLY_H_
#define DFL_DRAGONFLY_H_

#include <stdbool.h>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DFL_VERSION(major, minor, patch) VK_MAKE_API_VERSION(0, major, minor, patch)
#define DFL_ENGINE_VERSION DFL_VERSION(0, 1, 0)

/* -------------------- *
 *   ERROR CODES        *
 * -------------------- */

#define DFL_SUCCESS 0 // operation was successful

#define DFL_GENERIC_OOM_ERROR -0x00F // generic out of memory error
#define DFL_GENERIC_NO_SUCH_FILE_ERROR -0xF17E // generic file not found error
#define DFL_GENERIC_NULL_POINTER_ERROR -0xA110 // generic null pointer error
#define DFL_GENERIC_OUT_OF_BOUNDS_ERROR -0x0B0B // generic out of bounds error
#define DFL_GENERIC_ALREADY_INITIALIZED_ERROR -0x1A1A // generic already initialized error
#define DFL_GENERIC_OVERFLOW_ERROR -0x0F10 // generic overflow error
#define DFL_GENERIC_ILLEGAL_ACTION_ERROR -0x1A5E // thrown by functions that are not supposed to be called by the user (any prefixed with `_dfl` that could potentially cause dramatic issues)

#define DFL_GLFW_INIT_ERROR -0x50BAD // glfw initialization error
#define DFL_GLFW_WINDOW_ERROR -0x533 // glfw window creation error

#define DFL_VULKAN_INSTANCE_ERROR -0xDED // vulkan instance creation error
#define DFL_VULKAN_DEVICE_ERROR -0xF00C // vulkan device creation error
#define DFL_VULKAN_LAYER_ERROR -0xCA4E // vulkan layer creation error
#define DFL_VULKAN_DEBUG_ERROR -0xB0B0 // vulkan debug creation error
#define DFL_VULKAN_EXTENSION_ERROR -0xFA7 // vulkan extension creation error
#define DFL_VULKAN_SURFACE_ERROR -0xBADBED // vulkan surface creation error
#define DFL_VULKAN_QUEUE_ERROR -0x10B // vulkan queue creation error

// other definitions

#define DFL_MAX_CHAR_COUNT 256 // the maximum number of characters that can be used in a string

/* -------------------- *
 *  GLOBAL TYPES        *
 * -------------------- */

// a 2-tuple of integers
struct DflVec2D
{
	int x;
	int y;
};

#define DFL_MAKE_HANDLE(type) typedef struct type##_HND* type; // a handle to a type
#define DFL_HANDLE(type) ((struct Dfl##type##_T*)*p##type) // a shorthand for casting a handle to its type (will be used when `type` refers to a function argument in the form `ptype` (pointer to handle))
#define DFL_HANDLE_ARRAY(type, pos) ((struct Dfl##type##_T*)*(p##type##s + pos)) // a shorthand for casting a handle to its type (will be used when `type` refers to a function argument in the form `ptype` (pointer to handle))

// opaque handle for a DflSession_T object.
DFL_MAKE_HANDLE(DflSession);
// opaque handle for a DflDevice_T object.
DFL_MAKE_HANDLE(DflDevice);
// opaque handle for a DflWindow_T object.
DFL_MAKE_HANDLE(DflWindow);

/* ================================ *
 *             SESSIONS             *
 * ================================ */

/* -------------------- *
 *   TYPES              *
 * -------------------- */

#define DFL_SESSION_CRITERIA_NONE 0x00000000 // Dragonfly will operate as it sees fit. No extra criteria on how to manage the session.
#define DFL_SESSION_CRITERIA_ONLY_OFFSCREEN 0x00000001 // Dragonfly implicitly assumes that on-screen rendering is desired. Use this flag to override that assumption.
#define DFL_SESSION_CRITERIA_DO_DEBUG 0x00000002 // Dragonfly will enable validation layers and other debug features

struct DflSessionInfo
{
    const char* appName;
    uint32_t    appVersion;
    uint32_t    sessionCriteria;
};

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

/*
* @brief Initializes a Dragonfly session.
*/
DflSession  dflSessionCreate(struct DflSessionInfo* pInfo);

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

/**
* @brief Returns the number of devices that are available to the session, plus the devices themselves.
* Note that they are all uninitialized at this state.
*
* @param pCount: A pointer to an integer that will be set to the number of devices available to the session.
*/
DflDevice*        dflSessionDevicesGet(int* pCount, DflSession* pSession);

extern inline int dflSessionErrorGet(DflSession session);

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

/*
* @brief Destroy a session. Also frees the memory allocated for it.
*
* Must be called after destroying any object that was created with this session.
*/
void dflSessionDestroy(DflSession* pSession);

/* ================================ *
 *             DEVICES              *
 * ================================ */

/* -------------------- *
 *   TYPES              *
 * -------------------- */

#define DFL_GPU_RANK_N_CHOOSE -1 // Dragonfly will rank and choose the GPU, instead of the programmer picking.

#define DFL_GPU_CRITERIA_NONE 0x00000000 // Dragonfly will choose the best GPU available - no extra criteria on how to use it. It will use it as it sees fit.
#define DFL_GPU_CRITERIA_ONLY_OFFSCREEN 0x00000001 // Dragonfly will implicitly assume that on-screen rendering is desired. Use this flag to override that assumption.
#define DFL_GPU_CRITERIA_LOW_PERFORMANCE 0x00000002 // Dragonfly will try to use the least intensive GPU available
#define DFL_GPU_CRITERIA_ABUSE_MEMORY 0x00000004 // Dragonfly will normally implicitly leave a little wiggle room for GPU memory - use this flag to override that assumption
#define DFL_GPU_CRITERIA_ALL_QUEUES 0x00000008 // Dragonfly will reserve only one queue for each queue family - use this flag to override that assumption

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

/*
* @brief Initializes a Dragonfly device.
* 
* @param GPUCriteria: A bitfield of criteria that Dragonfly will use to choose the best GPU available.
* @param choice: the choice of the GPU. If you want Dragonfly to decide for itself, pass `DFL_GPU_RANK_N_CHOOSE`.
* @param pDevices: A pointer to an array of devices that are available to the session.
*/
DflDevice   dflDeviceInit(int GPUCriteria, int choice, DflDevice* pDevices, DflSession* pSession);

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

extern inline bool        dflDeviceCanPresentGet(DflDevice device);

extern inline const char* dflDeviceNameGet(DflDevice device);

extern inline int         dflDeviceErrorGet(DflDevice device);

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void dflDeviceTerminate(DflDevice* pDevice, DflSession* pSession);

/* ================================ *
 *             WINDOWS              *
 * ================================ */

 /* -------------------- *
  *   TYPES              *
  * -------------------- */

#define DFL_WINDOWED 0 
#define DFL_FULLSCREEN 1
/*
* @brief Borderless window mode. Removes decorations *completely*. If you are on Windows (or MacOS -in the future-),
* you can use `dflWindowWin32AttributeSet` to expand the window to the whole window, whilst keeping the decorations.
*/
#define DFL_BORDERLESS 2 

#define DFL_DIMENSIONS 0
#define DFL_VIEWPORT 1
#define DFL_RESOLUTION 2

// Window attributes (for Windows only -maybe MacOS too, when I am able to develop for it-)
// Since in Linux there's not a specific compositor/window manager + different display protocols,
// I don't think there's a reasonable method to change these programmatically for Linux, which is why
// I don't think I will bother implementing them for Linux.

#define DFL_WINDOW_WIN32_FULL_WINDOW_AVAILABLE 0
#define DFL_WINDOW_WIN32_DARK_MODE 1
#define DFL_WINDOW_WIN32_BORDER_COLOUR 2
#define DFL_WINDOW_WIN32_TITLE_BAR_COLOUR 3
#define DFL_WINDOW_WIN32_TITLE_TEXT_COLOUR 4
#define DFL_WINDOW_WIN32_ROUND_CORNERS 5

#define DFL_WINDOW_CORNER_NORMAL 0
#define DFL_WINDOW_CORNER_SHARP 1
#define DFL_WINDOW_CORNER_SMALL_ROUND 3

/**
* @brief Constructor info for a window.
*
* @param dim: the dimensions of the window.
* @param view: the viewport of the window.
* @param res: the resolution of the window.
* @param name: the name of the window.
* @param icon: the icon of the window.
* @param mode: the mode of the window. Check the definitions for more information.
* @param rate: the refresh rate of the window. Set 0 for unlimited.
* @param pos: the position of the window.
*/
typedef struct DflWindowInfo {
	struct DflVec2D	dim;
	struct DflVec2D view;
	struct DflVec2D res;

	char name[DFL_MAX_CHAR_COUNT];
	char icon[DFL_MAX_CHAR_COUNT];

	int				mode : 2; // DFL_WINDOWED, DFL_FULLSCREEN, DFL_BORDERLESS
	int	            rate;
	struct DflVec2D pos;
} DflWindowInfo;


/* -------------------- *
 *   CALLBACKS          *
 * -------------------- */
/*
	Most of the time, callbacks take the same parameters as the functions they are called from.
*/

typedef void (*DflWindowReshapeCLBK)(struct DflVec2D rect, int type, DflWindow* pWindow); // Called when a window is reshaped.
typedef void (*DflWindowRepositionCLBK)(struct DflVec2D pos, DflWindow* pWindow); // Called when a window is repositioned.
typedef void (*DflWindowChangeModeCLBK)(int mode, DflWindow* pWindow); // Called when a window's mode changes.
typedef void (*DflWindowRenameCLBK)(const char* name, DflWindow* pWindow); // Called when a window changes name.
typedef void (*DflWindowChangeIconCLBK)(const char* icon, DflWindow* pWindow); // Called when a window changes its icon.

typedef void (*DflWindowCloseCLBK)(DflWindow* pWindow); // Called RIGHT BEFORE a window is closed.

/* -------------------- *
*   INITIALIZE         *
* -------------------- */

/*
* @brief Creates a window and binds it to a session and a Vulkan surface.
*/
DflWindow   dflWindowInit(struct DflWindowInfo* pWindowInfo, DflSession* pSession);

/* -------------------- *
 *   CHANGE             *
 * -------------------- */

/*
* @brief Change the window's dimensions, viewport, or resolution.
*
* @param type: DFL_DIMENSIONS for the dimensions, DFL_VIEWPORT for the viewport, and DFL_RESOLUTION for the resolution.
* @param rect: the new dimensions, viewport, or resolution.
*/
void dflWindowReshape(int type, struct DflVec2D rect, DflWindow* pWindow);

void dflWindowReposition(struct DflVec2D pos, DflWindow* pWindow);
/*
* @brief Change the window's mode.
*
* @param mode: DFL_WINDOWED, DFL_FULLSCREEN, or DFL_BORDERLESS. Check their definitions for more information.
*/
void dflWindowChangeMode(int mode, DflWindow* pWindow);

void dflWindowRename(const char* name, DflWindow* pWindow);
/**
* @brief Change the window's icon.
*
* Note: This function keeps the old icon if the new one is invalid.
* @param icon: the new icon's path.
*/
void dflWindowChangeIcon(const char* icon, DflWindow* pWindow);

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

 /**
 * @brief Get the window's dimensions, viewport, or resolution.
 *
 * @param type: DFL_DIMENSIONS for the dimensions, DFL_VIEWPORT for the viewport, and DFL_RESOLUTION for the resolution.
 */
struct DflVec2D            dflWindowRectGet(int type, DflWindow window);

struct DflVec2D            dflWindowPosGet(DflWindow window);

int				           dflMonitorNumGet();

// Get the primary monitor's position.
struct DflVec2D            dflPrimaryMonitorPosGet();

/**
* @brief Check if the window should close.
*
* @deprecated This will probably be obsolete in the future, as rendering operations will handle such things and those will be handled by a thread.
*/
extern inline bool         dflWindowShouldCloseGet(DflWindow window);

/**
 * @brief Get the session that is the parent of this window
*/
extern inline DflSession   dflWindowSessionGet(DflWindow window);

/*
* @brief Get the window's error.
*/
extern inline int          dflWindowErrorGet(DflWindow window);

/**
* @brief Set the window's attributes - Windows only
*
* @param attrib: The attribute to change. Check the `DFL_WINDOW_WIN32_` macros.
* @param value: The value to set the attribute to. Depends on what the attribute is.
*/
void                       dflWindowWin32AttributeSet(int attrib, int value, DflWindow* pWindow);

/* -------------------- *
 *   CALLBACK SETTERS   *
 * -------------------- */
/*
	They consist of the callback's name + `Set`. They take the callback and the window as parameters.
*/

extern inline void dflWindowReshapeCLBKSet(DflWindowReshapeCLBK clbk, DflWindow* pWindow);
extern inline void dflWindowRepositionCLBKSet(DflWindowRepositionCLBK clbk, DflWindow* pWindow);
extern inline void dflWindowChangeModeCLBKSet(DflWindowChangeModeCLBK clbk, DflWindow* pWindow);
extern inline void dflWindowRenameCLBKSet(DflWindowRenameCLBK clbk, DflWindow* pWindow);
extern inline void dflWindowChangeIconCLBKSet(DflWindowChangeIconCLBK clbk, DflWindow* pWindow);

extern inline void dflWindowDestroyCLBKSet(DflWindowCloseCLBK clbk, DflWindow* window);

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void dflWindowTerminate(DflWindow* pWindow, DflSession* pSession);

#ifdef __cplusplus
}
#endif

#endif // !DFL_DRAGONFLY_H_
