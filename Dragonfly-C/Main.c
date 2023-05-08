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

	{
		struct DflVec2D dim = { .x = 1920, .y = 1080 };
		struct DflVec2D view = { .x = 800, .y = 600 };
		struct DflVec2D res = { .x = 800, .y = 600 };

		struct DflVec2D place = { .x = 100, .y = 100 };

		DflWindowInfo info =
		{
			.dim = dim,
			.view = view,
			.res = res,
			.name = "Dragonfly says: Hello World!",
			.mode = DFL_WINDOWED,
			.rate = 165,
			.pos = place
		};

		struct DflSessionInfo sessionInfo =
		{
			.appName = "Dragonfly",
			.appVersion = DFL_VERSION(0, 1, 0),
			.debug = true
		};

		session = dflSessionInit(&sessionInfo, DFL_SESSION_CRITERIA_NONE, DFL_GPU_CRITERIA_NONE);
		if (session == NULL)
			return 1;

		window = dflWindowCreate(&info);
		if (window == NULL)
			return 1;

	}
	Sleep(1000);

	dflWindowDestroy(&window);
	dflSessionEnd(&session);

	return 0;
}