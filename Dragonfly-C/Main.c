/*
* NOTE: THIS EXISTS ONLY AS A DUMMY FILE! 
* WHEN DRAGONFLY REACHES A DESIRED WORKING STATE, THIS FILE WILL BE DELETED!
* WHEN THAT HAPPENS, DRAGONFLY WILL BE SET TO COMPILE AS A DYNAMIC LIBRARY!
*/


#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef _DEBUG
#define MAIN WinMain
#else
#define MAIN main
#endif

#include "Dragonfly.h"

int MAIN()
{
	DflWindow	window = NULL;
	DflSession	session = NULL;
	DflDevice	device = NULL;

	struct DflWindowInfo winfo = { .dim = {1920, 1080}, .view = {1920, 1080}, .res = {1920, 1080}, .name = "Dragonfly", .icon = NULL, .mode = 0, .rate = 144, .pos = {200, 200} };
	struct DflSessionInfo info = { "Dragonfly", DFL_VERSION(0, 1, 0), DFL_SESSION_CRITERIA_DO_DEBUG};

	session = dflSessionCreate(&info);
	if (dflSessionErrorGet(session) != DFL_SUCCESS)
		return 1;

	window = dflWindowInit(&winfo, &session);
	if(dflWindowErrorGet(window) != DFL_SUCCESS)
        return 1;

	int choice = 0;
	DflDevice* devices = dflSessionDevicesGet(&choice, &session);
	if (devices == NULL)
        return 1;

	device = dflDeviceInit(DFL_GPU_CRITERIA_NONE, 0, devices, &session);
	if (dflDeviceErrorGet(device) != DFL_SUCCESS)
		return 1;
	free(devices);

	while (dflWindowShouldCloseGet(window) == false)
	{
        glfwPollEvents();
    }

	dflDeviceTerminate(&device, &session);
	dflWindowTerminate(&window, &session);
	dflSessionDestroy(&session);

	glfwTerminate();

	return 0;
}