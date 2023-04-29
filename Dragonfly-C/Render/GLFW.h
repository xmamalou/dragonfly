#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <GLFW/glfw3.h>

#include "../Data.h"

// TYPES

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
// opaque handle for a DflWindow_T object.
DFL_MAKE_HANDLE(DflWindow);

// CALLBACKS -- They take -most of the time- the same parameters as the functions they are called after. But they all return nothing.

typedef void (*DflWindowReshapeCLBK)(struct DflVec2D rect, int type, DflWindow* window); // Called when a window is reshaped.
typedef void (*DflWindowRepositionCLBK)(struct DflVec2D pos, DflWindow* window); // Called when a window is repositioned.
typedef void (*DflWindowChangeModeCLBK)(int mode, DflWindow* window); // Called when a window's mode changes.
typedef void (*DflWindowRenameCLBK)(const char* name, DflWindow* window); // Called when a window changes name.
typedef void (*DflWindowChangeIconCLBK)(const char* icon, DflWindow* window); // Called when a window changes its icon.

typedef void (*DflWindowCloseCLBK)(DflWindow* window); // Called RIGHT BEFORE a window is closed.

// FUNCTIONS

/*
*	Callbacks get called *after* an action has been performed. 
*/

// Returns NULL if unsuccessful. 
DflWindow dflWindowCreate(DflWindowInfo* info); 

// type: DFL_DIMENSIONS for the dimensions, DFL_VIEWPORT for the viewport, and DFL_RESOLUTION for the resolution.
// Changes the given Rect of the window.
void dflWindowReshape(struct DflVec2D rect, int type, DflWindow* window);
void dflWindowReposition(struct DflVec2D pos, DflWindow* window);
// mode: DFL_WINDOWED, DFL_FULLSCREEN, or DFL_BORDERLESS.
void dflWindowChangeMode(int mode, DflWindow* window);
void dflWindowRename(const char* name, DflWindow* window);
// Will keep old icon if the path is invalid.
void dflWindowChangeIcon(const char* icon, DflWindow* window);

// GETTERS

// type: DFL_DIMENSIONS for the dimensions, DFL_VIEWPORT for the viewport, and DFL_RESOLUTION for the resolution.
struct DflVec2D dflWindowRectGet(int type, DflWindow window);
struct DflVec2D dflWindowPosGet(DflWindow window);
int				dflMonitorNumGet();
// Gives it in terms of the virtual screen space.
struct DflVec2D dflPrimaryMonitorPosGet();

// Does not free WINDOW pointer.
void dflWindowDestroy(DflWindow* window);

// CALLBACK SETTERS -- Add callbacks to be executed when window functions are called. None return anything.

void dflWindowReshapeCLBKSet(DflWindowReshapeCLBK clbk, DflWindow* window);
void dflWindowRepositionCLBKSet(DflWindowRepositionCLBK clbk, DflWindow* window);
void dflWindowChangeModeCLBKSet(DflWindowChangeModeCLBK clbk, DflWindow* window);
void dflWindowRenameCLBKSet(DflWindowRenameCLBK clbk, DflWindow* window);
void dflWindowChangeIconCLBKSet(DflWindowChangeIconCLBK clbk, DflWindow* window);

void dflWindowCloseCLBKSet(DflWindowCloseCLBK clbk, DflWindow* window);

#ifdef __cplusplus
}
#endif