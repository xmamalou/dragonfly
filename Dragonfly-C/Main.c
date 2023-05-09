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

	session = dflSessionInit(NULL, DFL_SESSION_CRITERIA_NONE, DFL_GPU_CRITERIA_NONE);
	if (session == NULL)
		return 1;

	window = dflWindowCreate(NULL, &session);
	if (window == NULL)
		return 1;

	Sleep(1000);

	dflWindowDestroy(&window);
	dflSessionEnd(&session);

	return 0;
}