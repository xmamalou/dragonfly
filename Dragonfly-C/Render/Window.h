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
#ifndef DFL_RENDER_WINDOW_H
#define DFL_RENDER_WINDOW_H

/*
* @file Window.h
* 
* @brief Defines several Window related structures and functions.
* 
* Note: not all functions related to windows are defined here. Check the documentation for a more concise list.
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "../Data.h"

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

// Avoid using this function. It doesn't create a window bound to a Vulkan surface. It's used internally. Prefer `dflWindowInit`.
DflWindow _dflWindowCreate(DflWindowInfo* pInfo); 

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
// You cannot use this. It is protected from outside use.
extern inline GLFWwindow* _dflWindowHandleGet(DflWindow window); 

/**
* @brief Set the window's attributes - Windows only
* 
* @param attrib: The attribute to change. Check the `DFL_WINDOW_WIN32_` macros.
* @param value: The value to set the attribute to. Depends on what the attribute is.
*/
void                       dflWindowWin32AttributeSet(int attrib, int value, DflWindow* pWindow);
// You cannot use this. It is protected from outside use.
void                      _dflWindowErrorSet(int error, DflWindow* pWindow); 
// You cannot use this. It is protected from outside use.
void                      _dflWindowSessionSet(VkSurfaceKHR surface, DflSession session, DflWindow* pWindow); 

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

/*
* @brief Destroy a window. Also frees the memory allocated for it.
*/
void _dflWindowDestroy(DflWindow* pWindow);

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

#ifdef __cplusplus
}
#endif

#endif // !DFL_RENDER_WINDOW_H