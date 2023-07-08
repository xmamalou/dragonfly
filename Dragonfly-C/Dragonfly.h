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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define DFL_VERSION(major, minor, patch) (((unsigned int)major << 16) | ((unsigned int)minor << 8) | (unsigned int)patch)

#define DFL_ENGINE_VERSION DFL_VERSION(0, 1, 0)

/* -------------------- *
 *   ERROR CODES        *
 * -------------------- */

#define DFL_SUCCESS 0 // operation was successful

// errors are < 0. Warnings are > 0.
/*
* Errors and warnings are a 2-byte integer, laid out in hex as follows:
* 
* The first digit (from the left) is the general category of the error. For example, 0x1XXX is a generic error, 0x2XXX is a GLFW error, etc.
* The second digit is a more specified category of the error. For example, for Vulkan, there could be multiple errors concerning the instance, the device, the surface, etc.
* The third and fourth digits are the error code number.
*/

#define DFL_GENERIC_OOM_ERROR -0x1001
#define DFL_GENERIC_NO_SUCH_FILE_ERROR -0x1002
#define DFL_GENERIC_OUT_OF_BOUNDS_ERROR -0x1003 // attempted to create more items than the maximum allowed
#define DFL_GENERIC_ALREADY_INITIALIZED_ERROR -0x1004 // the item is already initialized

#define DFL_GLFW_API_INIT_ERROR -0x2101 // glfw initialization error
#define DFL_GLFW_WINDOW_INIT_ERROR -0x2201 // glfw window creation error

#define DFL_VULKAN_INSTANCE_ERROR -0x3101 // vulkan instance creation error
#define DFL_VULKAN_DEVICE_ERROR -0x3201 // vulkan device creation error
#define DFL_VULKAN_LAYER_ERROR -0x3301 // vulkan layer creation error
#define DFL_VULKAN_DEBUG_ERROR -0x3401 // vulkan debug creation error
#define DFL_VULKAN_EXTENSION_ERROR -0x3501 // vulkan extension creation error
#define DFL_VULKAN_SURFACE_ERROR -0x3601 // vulkan surface creation error
#define DFL_VULKAN_SURFACE_NO_FORMATS_ERROR -0x3602 // vulkan surface has no formats
#define DFL_VULKAN_SURFACE_NO_SWAPCHAIN_ERROR -0x3603 // vulkan swapchain couldn't be created
#define DFL_VULKAN_SURFACE_NO_VIEWS_ERROR -0x3604 // vulkan surface couldn't have its swapchain image views created
#define DFL_VULKAN_QUEUE_ERROR -0x3701 // vulkan queue creation error
#define DFL_VULKAN_QUEUES_NO_PROPERTIES_ERROR -0x3702 // queues have no properties
#define DFL_VULKAN_QUEUES_COMPOOL_ALLOC_ERROR -0x3703 // failed to allocate command pool

// other definitions

#define DFL_MAX_CHAR_COUNT 256 // the maximum number of characters that can be used in a string
#define DFL_MAX_ITEM_COUNT 64 // the maximum number of items that can be used in a list

// colours
#define DFL_COLOR_RGB(r, g, b) (((unsigned int)r) | ((unsigned int)g << 8) | (unsigned int)b << 16)

// some predefined colours

#define DFL_COLOR_BLACK DFL_COLOR_RGB(0, 0, 0)
#define DFL_COLOR_WHITE DFL_COLOR_RGB(255, 255, 255)
#define DFL_COLOR_GRAY DFL_COLOR_RGB(128, 128, 128)
#define DFL_COLOR_RED DFL_COLOR_RGB(255, 0, 0)
#define DFL_COLOR_GREEN DFL_COLOR_RGB(0, 255, 0)
#define DFL_COLOR_BLUE DFL_COLOR_RGB(0, 0, 255)
#define DFL_COLOR_YELLOW DFL_COLOR_RGB(255, 255, 0)
#define DFL_COLOR_MAGENTA DFL_COLOR_RGB(255, 0, 255)
#define DFL_COLOR_CYAN DFL_COLOR_RGB(0, 255, 255)

#define DFL_MAKE_HANDLE(type) typedef struct type##_HND* type; // a handle to a type

// opaque handle for a DflSession_T object.
DFL_MAKE_HANDLE(DflSession);
// opaque handle for a DflMemoryPool_T object.
DFL_MAKE_HANDLE(DflMemoryPool);
// opaque handle for a DflDevice_T object.
DFL_MAKE_HANDLE(DflDevice);
// opaque handle for a DflWindow_T object.
DFL_MAKE_HANDLE(DflWindow);

// abstract objects

// opaque handle for a DflImage_T object. (not the same as a Vulkan image)
DFL_MAKE_HANDLE(DflImage);

/* -------------------- *
 *  GLOBAL TYPES        *
 * -------------------- */

 // a 2-tuple of integers
struct DflVec2D
{
	int x;
	int y;
};

// a triple of integers
struct DflVec3D
{
	int x;
	int y;
	int z;
};

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
    const char* pAppName;
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
DflDevice*        dflSessionDevicesGet(int* pCount, DflSession hSession);

extern inline int dflSessionErrorGet(DflSession hSession);

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

/*
* @brief Destroy a session. Also frees the memory allocated for it.
*
* Must be called after destroying any object that was created with this session.
*/
void dflSessionDestroy(DflSession hSession);

/* ================================ *
 *             MEMORY               *
 * ================================ */

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

/*
* @brief Initialize a memory pool.
* 
* @param size: The size of the memory pool, in 4-byte words.
*/
DflMemoryPool dflMemoryPoolCreate(int size);

/* -------------------- *
 *   CHANGE             *
 * -------------------- */

void dflMemoryPoolExpand(int size, DflMemoryPool hMemoryPool);

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

/*
* @brief Get the size of a memory pool in BYTES.
*/
extern inline int dflMemoryPoolSizeGet(DflMemoryPool hMemoryPool);

extern inline int dflMemoryPoolErrorGet(DflMemoryPool hMemoryPool);

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

DflMemoryPool dflMemoryPoolDestroy(DflMemoryPool hMemoryPool);

/* ================================ *
 *             THREADS              *
 * ================================ */

/* -------------------- *
 *   TYPES              *
 * -------------------- */

DFL_MAKE_HANDLE(DflThread); // a thread handle

typedef void (*DflThreadProc)(void* parameters); // a thread function

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

/*
* @brief Initializes a thread.
* 
* @param pFuncProc: A pointer to the function that will be executed by the thread.
* @param pParams: A pointer to the parameters that will be passed to the thread function.
* @param pSession: A pointer to the session that will be used to create the thread and where its shared memory wil be.
*/
DflThread dflThreadInit(DflThreadProc pFuncProc, void* pParams, DflSession hSession);

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
DflDevice   dflDeviceInit(int GPUCriteria, int choice, DflDevice* pDevices, DflSession hSession);

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

extern inline bool        dflDeviceCanPresentGet(DflDevice hDevice);

extern inline const char* dflDeviceNameGet(DflDevice hDevice);

extern inline DflSession  dflDeviceSessionGet(DflDevice hDevice);

extern inline int         dflDeviceErrorGet(DflDevice hDevice);

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void dflDeviceTerminate(DflDevice hDevice, DflSession hSession);

/* ================================ *
 *             MONITORS             *
 * ================================ */

/* -------------------- *
*   TYPES              *
* -------------------- */

struct DflMonitorInfo {
	struct DflVec2D res; // the resolution in screen coordinates
	struct DflVec2D pos; // the position in screen coordinates
	struct DflVec2D workArea; // the work area in screen coordinates
	
	char name[DFL_MAX_CHAR_COUNT];

	int	rate;

	int 			colorDepth;
	struct DflVec3D colorDepthPerPixel; // x = red, y = green, z = blue
};

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

/*
* @brief Returns the monitors connected to the system + their number.
* 
* @param pCount: A pointer to an integer that will be set to the number of monitors connected to the system.
*/
struct DflMonitorInfo*     dflMonitorsGet(int* pCount, DflSession hSession);

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

// format numbers are taken from Vulkan

#define DFL_WINDOW_FORMAT_R8G8B8A8_UNORM 44
#define DFL_WINDOW_FORMAT_R8G8B8A8_SRGB 50
#define DFL_WINDOW_FORMAT_R16G16B16A16_UNORM 88
#define DFL_WINDOW_FORMAT_R16G16B16A16_SFLOAT 97

// swizzling modes

#define DFL_WINDOW_SWIZZLING_NONE 0
#define DFL_WINDOW_SWIZZLING_RED 1 // this means "use the red channel"
#define DFL_WINDOW_SWIZZLING_GREEN 2 // this means "use the green channel"
#define DFL_WINDOW_SWIZZLING_BLUE 4 // this means "use the blue channel"
#define DFL_WINDOW_SWIZZLING_ALPHA 8 // this means "use the alpha channel"
#define DFL_WINDOW_SWIZZLING_NORMAL (DFL_WINDOW_SWIZZLING_RED | DFL_WINDOW_SWIZZLING_GREEN | DFL_WINDOW_SWIZZLING_BLUE | DFL_WINDOW_SWIZZLING_ALPHA)


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
* 
* @remark The window stores this information. Thus, you can discard the struct after the window is created.
*/
typedef struct DflWindowInfo {
	struct DflVec2D	dim; // the dimensions of the window in SCREEN COORDINATES
	/*
	* @brief The viewport of the window in SCREEN COORDINATES. This is relevant once the window is "charted"
	* (i.e. when the device learns about the window and makes a swapchain for it).
	* 
	* The viewport specifically refers to how much of the window will be used for rendering.
	*/
	struct DflVec2D view;

	char			name[DFL_MAX_CHAR_COUNT];
	DflImage		hIcon;

	int				mode : 2; // DFL_WINDOWED, DFL_FULLSCREEN, DFL_BORDERLESS

	int	            rate; // the refresh rate of the window. Set 0 for unlimited.
	bool            vsync; // overrides rate.

	struct DflVec2D pos; // the position of the window in SCREEN COORDINATES

	int             colorFormat; // the image format of the window. Set 0 for default. If not supported, the default will be used.
	unsigned int    swizzling;
	int				layers; // how many layers the images for the window will have. Useful for VR. Set to 1 for normal windows.
} DflWindowInfo;

/* -------------------- *
*   INITIALIZE         *
* -------------------- */

/*
* @brief Creates a window and binds it to a session and a Vulkan surface.
*/
DflWindow   dflWindowInit(struct DflWindowInfo* pWindowInfo, DflSession hSession);

/*
* @brief Bind a Window to a Device (i.e. make a swapchain for it).
* 
* This function is used to bind a window to a device. You don't need to pass any parameters specifying the swapchain, as 
* DflWindowInfo already contains all the information needed to create a swapchain. This function just uses that information.
*/
void dflWindowBindToDevice(DflWindow hWindow, DflDevice hDevice);

/* -------------------- *
 *   CHANGE             *
 * -------------------- */

/*
* @brief Change the window's dimensions, viewport, or resolution.
*
* @param type: DFL_DIMENSIONS for the dimensions, DFL_VIEWPORT for the viewport, and DFL_RESOLUTION for the resolution.
* @param rect: the new dimensions, viewport, or resolution.
*/
void dflWindowReshape(int type, struct DflVec2D rect, DflWindow hWindow);

void dflWindowReposition(struct DflVec2D pos, DflWindow hWindow);
/*
* @brief Change the window's mode.
*
* @param mode: DFL_WINDOWED, DFL_FULLSCREEN, or DFL_BORDERLESS. Check their definitions for more information.
*/
void dflWindowChangeMode(int mode, DflWindow hWindow);

void dflWindowRename(const char* name, DflWindow hWindow);
/**
* @brief Change the window's icon.
*
* Note: This function keeps the old icon if the new one is invalid.
* @param icon: the new icon's path.
*/
void dflWindowChangeIcon(DflImage icon, DflWindow hWindow);

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

 /**
 * @brief Get the window's dimensions, viewport, or resolution.
 *
 * @param type: DFL_DIMENSIONS for the dimensions, DFL_VIEWPORT for the viewport, and DFL_RESOLUTION for the resolution.
 */
struct DflVec2D            dflWindowRectGet(int type, DflWindow hWindow);

struct DflVec2D            dflWindowPosGet(DflWindow hWindow);

/**
* @brief Check if the window should close.
*
* @deprecated This will probably be obsolete in the future, as rendering operations will handle such things and those will be handled by a thread.
*/
extern inline bool         dflWindowShouldCloseGet(DflWindow hWindow);

/**
 * @brief Get the session that is the parent of this window
*/
extern inline DflSession   dflWindowSessionGet(DflWindow hWindow);

/*
* @brief Get the window's error.
*/
extern inline int          dflWindowErrorGet(DflWindow hWindow);

/**
* @brief Set the window's attributes - Windows only
*
* @param attrib: The attribute to change. Check the `DFL_WINDOW_WIN32_` macros.
* @param value: The value to set the attribute to. Depends on what the attribute is.
*/
void                       dflWindowWin32AttributeSet(int attrib, int value, DflWindow hWindow);

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

void dflWindowTerminate(DflWindow hWindow, DflSession hSession);

void dflWindowUnbindFromDevice(DflWindow hWindow, DflDevice hDevice);

/* ================================ *
 *             IMAGES               *
 * ================================ */

/* -------------------- *
 *   CHANGE             *
 * -------------------- */

/**
* @breif Load an image's data. Needs an image that is referenced.
*/
void dflImageLoad(DflImage hImage);

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

/**
* @brief Get an image from a file. See stbi_load for supported formats.
*/
DflImage dflImageFromFileGet(const char* file);

/**
* @brief Get an image with no data loaded (i.e. only path).
*/
DflImage dflImageReferenceFromFileGet(const char* file);

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

/**
* @brief Free the image's data. Also frees the handle.
*/
void dflImageDestroy(DflImage hImage);

#ifdef __cplusplus
}
#endif

#endif // !DFL_DRAGONFLY_H_
