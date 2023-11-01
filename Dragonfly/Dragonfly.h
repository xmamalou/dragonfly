#pragma once

#ifdef DRAGONFLY_EXPORTS
#define DFL_API __declspec(dllexport)
#else
#define DFL_API __declspec(dllimport)
#endif

#define DFL_CALL __stdcall