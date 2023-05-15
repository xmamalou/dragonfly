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

	session = dflSessionInit(DFL_SESSION_CRITERIA_DO_DEBUG, NULL);
	if (session == NULL)
		return 1;

	window = dflWindowCreate(NULL);
	if (window == NULL)
		return 1;

	int error = dflSessionBindWindow(&window, &session);
	if (error != DFL_SUCCESS)
		return error;

	device = dflDeviceInit(DFL_GPU_CRITERIA_NONE, NULL, NULL, &session);
	if (device == NULL)
        return 1;

	while (glfwWindowShouldClose(dflWindowHandleGet(window)) == GLFW_FALSE)
	{
        glfwPollEvents();
    }

	dflWindowDestroy(&window);
	dflSessionEnd(&session);

	return 0;
}