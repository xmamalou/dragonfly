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
			.icon = "Resources/charliefav.png",
			.mode = DFL_WINDOWED,
			.rate = 165,
			.pos = place 
		};

		struct DflSessionInfo sessionInfo = 
		{
			.appName = "Dragonfly",
			.appVersion = DFL_VERSION(0, 0, 1),
			.window = &window,
			.debug = DFL_TRUE
		};

		session = dflSessionInit(&sessionInfo, DFL_SESSION_CRITERIA_NONE, DFL_GPU_CRITERIA_NONE);
		window = dflWindowCreate(&info);
	}

	if (session == NULL)
		return 1;

	if(window == NULL)
		return 1;

	Sleep(1000);

	dflWindowDestroy(&window);
	dflSessionEnd(&session);

	return 0;
}