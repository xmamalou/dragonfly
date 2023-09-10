/*
* NOTE: THIS EXISTS ONLY AS A DUMMY FILE! 
* WHEN DRAGONFLY REACHES A DESIRED WORKING STATE, THIS FILE WILL BE DELETED!
* WHEN THAT HAPPENS, DRAGONFLY WILL BE SET TO COMPILE AS A DYNAMIC LIBRARY!
*/

#include <stdio.h>
#include <stdint.h>

#ifdef _WIN32
	#include <windows.h>
	#include <VersionHelpers.h>
	#include <shellapi.h>
#endif

#ifdef _WIN32
	#pragma comment (lib, "shell32.lib")
	#pragma comment (lib, "user32.lib")
    #pragma comment (lib, "gdi32.lib")
    #pragma comment (lib, "dwmapi.lib")
#endif

#include "Dragonfly.h"
#include <GLFW/glfw3.h>

int main()
{
	int error = 0;

	DflWindow	window = NULL;
	DflSession	session = NULL;
	DflDevice	device = NULL;
	
	struct DflSessionInfo info = { "Dragonfly", DFL_VERSION(0, 1, 0), DFL_SESSION_CRITERIA_NONE};
	struct DflWindowInfo winfo = {
		.dim = {1820, 1080},
		.view = {1820, 1080},
		.vsync = true,
		.name = "Dragonfly", 
		.hIcon = dflImageReferenceFromFileGet("Resources/bugs.png"), 
		.mode = DFL_WINDOWED, 
		.rate = 165, 
		.pos = {200, 200},
	    .colorFormat = DFL_WINDOW_FORMAT_R8G8B8A8_SRGB,
		.swizzling = DFL_WINDOW_SWIZZLING_NORMAL
	};

	session = dflSessionCreate(&info);
	error = dflSessionErrorGet(session);
	if (error < DFL_SUCCESS)
	{
		printf("Error: %x\n", error);
		return 1;
	}

	window = dflWindowInit(&winfo, session);
	error = dflWindowErrorGet(window);
	if (error < DFL_SUCCESS)
	{
		printf("Error: %x\n", error);
		return 1;
	}

	dflImageDestroy(winfo.hIcon);

#ifdef _WIN32
	dflWindowWin32AttributeSet(DFL_WINDOW_WIN32_BORDER_COLOUR, DFL_COLOR_GRAY, window);
	dflWindowWin32AttributeSet(DFL_WINDOW_WIN32_TITLE_TEXT_COLOUR, DFL_COLOR_WHITE, window);
	dflWindowWin32AttributeSet(DFL_WINDOW_WIN32_DARK_MODE, true, window);
#endif
	
	dflSessionLoadDevices(session);
	if (dflSessionDeviceCountGet(session) == NULL)
		return 1;

	dflSessionInitDevice(DFL_GPU_CRITERIA_NONE, DFL_CHOOSE_ON_RANK, session);
	error = dflSessionErrorGet(session);
	if (error < DFL_SUCCESS)
	{
		printf("Error: %x\n", error);
		return 1;
	}

	printf("Rank: %d", dflSessionDeviceRankGet(0, session));

	dflWindowBindToDevice(window, 0, session);
	error = dflWindowErrorGet(window);
	if (error < DFL_SUCCESS)
	{
		printf("Error: %x\n", error);
		return 1;
	}

	struct DflVec2D newDim = { 800, 600 };
	dflWindowReshape(DFL_DIMENSIONS, newDim, window);

#ifdef _WIN32
	dflWindowWin32AttributeSet(DFL_WINDOW_WIN32_FULL_WINDOW_AVAILABLE, true, window);
#endif

	while ((dflWindowShouldCloseGet(window) == false))
		glfwPollEvents();

	dflWindowUnbindFromDevice(window);
	dflWindowTerminate(window, session);

	dflSessionTerminateDevice(0, session);
	dflSessionDestroy(session);

	glfwTerminate();

	return 0;
}