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

#ifdef __cplusplus
extern "C" {
#endif

#include <GLFW/glfw3.h>

#include "../Data.h"

/* -------------------- *
 *   TYPES              *
 * -------------------- */

#define DFL_WINDOWED 0
#define DFL_FULLSCREEN 1
#define DFL_BORDERLESS 2

#define DFL_DIMENSIONS 0
#define DFL_VIEWPORT 1
#define DFL_RESOLUTION 2

typedef struct DflWindowInfo { 
	struct DflVec2D	dim;
	struct DflVec2D view; // refers to how much of the window will be used for rendering
	struct DflVec2D res; // refers to the resolution of the view

	const char*		name;
	const char*		icon;

	int				mode : 2; // Windowed, Fullscreen, Borderless
	int	            rate; // refresh rate, set 0 for unlimited
	struct DflVec2D pos;
} DflWindowInfo;


/* -------------------- *
 *   CALLBACKS          *
 * -------------------- */
// They take -most of the time- the same parameters as the functions they are called after. But they all return nothing.
// See their related functions for argument information. If there's an extra argument, it will be explained.

typedef void (*DflWindowReshapeCLBK)(struct DflVec2D rect, int type, DflWindow* pWindow); // Called when a window is reshaped.
typedef void (*DflWindowRepositionCLBK)(struct DflVec2D pos, DflWindow* pWindow); // Called when a window is repositioned.
typedef void (*DflWindowChangeModeCLBK)(int mode, DflWindow* pWindow); // Called when a window's mode changes.
typedef void (*DflWindowRenameCLBK)(const char* name, DflWindow* pWindow); // Called when a window changes name.
typedef void (*DflWindowChangeIconCLBK)(const char* icon, DflWindow* pWindow); // Called when a window changes its icon.

typedef void (*DflWindowCloseCLBK)(DflWindow* pWindow); // Called RIGHT BEFORE a window is closed.

/* -------------------- *
 *   INITIALIZE         *
 * -------------------- */

// Returns NULL if unsuccessful.
// NOTE: you can omit members of the struct. Dragonfly will set default values if NULL.
DflWindow dflWindowCreate(DflWindowInfo* pInfo); 

/* -------------------- *
 *   CHANGE             *
 * -------------------- */

// Changes the dimensions, viewport, or resolution of the window.
// `type`: DFL_DIMENSIONS for the dimensions, DFL_VIEWPORT for the viewport, and DFL_RESOLUTION for the resolution.
void dflWindowReshape(int type, struct DflVec2D rect, DflWindow* pWindow);
void dflWindowReposition(struct DflVec2D pos, DflWindow* pWindow);
// Change window mode.
// `mode`: DFL_WINDOWED, DFL_FULLSCREEN, or DFL_BORDERLESS.
void dflWindowChangeMode(int mode, DflWindow* pWindow);
void dflWindowRename(const char* name, DflWindow* pWindow);
// Will keep old icon if the path is invalid.
// `icon`: path to the icon. NOT ITS DATA.
void dflWindowChangeIcon(const char* icon, DflWindow* pWindow);

/* -------------------- *
 *   GET & SET          *
 * -------------------- */

// Get the dimensions, viewport, or resolution of the window.
// `type`: DFL_DIMENSIONS for the dimensions, DFL_VIEWPORT for the viewport, and DFL_RESOLUTION for the resolution.
struct DflVec2D dflWindowRectGet(int type, DflWindow window);
struct DflVec2D dflWindowPosGet(DflWindow window);
int				dflMonitorNumGet();
// Get the primary monitor's position.
struct DflVec2D dflPrimaryMonitorPosGet();
// check if the window is supposed to close.
bool            dflWindowShouldCloseGet(DflWindow window);

/* -------------------- *
 *   DESTROY            *
 * -------------------- */

 // It frees the `window` pointer.
void dflWindowDestroy(DflWindow* pWindow);

/* -------------------- *
 *   ONLY INTERNAL USE  *
 * -------------------- */

size_t      dflWindowSizeRequirementsGet(); // not protected since its results are trivial and cannot cause any harm.
GLFWwindow* dflWindowHandleGet(DflWindow window, DflSession session); // protected from outside use.
int         dflWindowSurfaceIndexGet(DflWindow window); // not protected since its results are trivial and cannot cause any harm.

void        dflWindowSurfaceIndexSet(int index, DflWindow* pWindow, DflSession session); // protected from outside use.


/* -------------------- *
 *   CALLBACK SETTERS   *
 * -------------------- */

void dflWindowReshapeCLBKSet(DflWindowReshapeCLBK clbk, DflWindow* pWindow);
void dflWindowRepositionCLBKSet(DflWindowRepositionCLBK clbk, DflWindow* pWindow);
void dflWindowChangeModeCLBKSet(DflWindowChangeModeCLBK clbk, DflWindow* pWindow);
void dflWindowRenameCLBKSet(DflWindowRenameCLBK clbk, DflWindow* pWindow);
void dflWindowChangeIconCLBKSet(DflWindowChangeIconCLBK clbk, DflWindow* pWindow);

void dflWindowDestroyCLBKSet(DflWindowCloseCLBK clbk, DflWindow* window);

#ifdef __cplusplus
}
#endif

#endif // !DFL_RENDER_WINDOW_H