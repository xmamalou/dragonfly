﻿/*
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

#ifdef _RELEASE && _WIN32
	#define MAIN WinMain
#else
	#define MAIN main
#endif

#ifdef _WIN32
	#pragma comment (lib, "shell32.lib")
	#pragma comment (lib, "user32.lib")
    #pragma comment (lib, "gdi32.lib")
    #pragma comment (lib, "dwmapi.lib")
#endif

#include "Dragonfly.h"
#include <GLFW/glfw3.h>

void func()
{

}

int MAIN()
{
	DflWindow	window = NULL;
	DflSession	session = NULL;
	DflDevice	device = NULL;
	
	struct DflSessionInfo info = { "Dragonfly", DFL_VERSION(0, 1, 0), DFL_SESSION_CRITERIA_DO_DEBUG};
	struct DflWindowInfo winfo = {
		.dim = {1820, 1080},
		.view = {1820, 1080},
		.vsync = true,
		.name = "Dragonfly", 
		.icon = dflImageReferenceFromFileGet("Resources/bugs.png"), 
		.mode = DFL_WINDOWED, 
		.rate = 165, 
		.pos = {200, 200},
	    .colorFormat = DFL_WINDOW_FORMAT_R8G8B8A8_SRGB
	};

	session = dflSessionCreate(&info);
	if (dflSessionErrorGet(session) < DFL_SUCCESS)
		return 1;

	window = dflWindowInit(&winfo, session);
	if (dflWindowErrorGet(window) < DFL_SUCCESS)
		return 1;

	dflImageDestroy(winfo.icon);

#ifdef _WIN32
	dflWindowWin32AttributeSet(DFL_WINDOW_WIN32_BORDER_COLOUR, DFL_COLOR_GRAY, window);
	dflWindowWin32AttributeSet(DFL_WINDOW_WIN32_TITLE_TEXT_COLOUR, DFL_COLOR_WHITE, window);
	dflWindowWin32AttributeSet(DFL_WINDOW_WIN32_DARK_MODE, true, window);
#endif

	int choice = 0;
	DflDevice* devices = dflSessionDevicesGet(&choice, session);
	if (devices == NULL)
		return 1;

	device = dflDeviceInit(DFL_GPU_CRITERIA_NONE, 0, devices, session);
	if (dflDeviceErrorGet(device) < DFL_SUCCESS)
		return 1;
	free(devices);

	dflWindowBindToDevice(window, device);
	if(dflWindowErrorGet(window) < DFL_SUCCESS)
		return 1;

	struct DflVec2D newDim = { 2560, 1440 };
	dflWindowReshape(DFL_DIMENSIONS, newDim, window);
	dflWindowChangeMode(DFL_FULLSCREEN, window);

	while ((dflWindowShouldCloseGet(window) == false))
	{
		glfwPollEvents();
	}

	struct DflVec2D pos = { 0, 0 };

	dflWindowUnbindFromDevice(window, device);

	dflWindowTerminate(window);
	dflDeviceTerminate(device);
	dflSessionDestroy(session);

	glfwTerminate();

	return 0;
}