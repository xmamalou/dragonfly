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
	DflDevice*	devices = NULL;

	struct DflSessionInfo info = { "Dragonfly", DFL_VERSION(0, 1, 0), DFL_SESSION_CRITERIA_DO_DEBUG};

	session = dflSessionCreate(&info);
	if (session == NULL)
		return 1;

	dflSessionInitWindow(NULL, &window, &session);
	if (window == NULL)
		return 1;

	int choice = 0;
	devices = dflSessionDevicesGet(&choice, session);
	if (devices == NULL)
        return 1;

	if (dflDeviceInit(DFL_GPU_CRITERIA_NONE, 0, devices, &session) != DFL_SUCCESS)
		return 1;

	while (dflWindowShouldCloseGet(window) == false)
	{
        glfwPollEvents();
    }

	dflDeviceDestroy(0, devices);
	dflWindowDestroy(&window);
	dflSessionEnd(&session);

	glfwTerminate();

	return 0;
}