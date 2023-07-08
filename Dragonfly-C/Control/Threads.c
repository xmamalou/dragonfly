#include "../Dragonfly.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

DflThread dflThreadInit(DflThreadProc pFuncProc, void* pParams, DflSession hSession)
{
    struct DflThread_T* thread = 1;

    return (DflThread)thread;
}
