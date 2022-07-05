#include "../include/render.h"

int dlfWindowIniting(const char* windowName, int width, int height, int fullscreen)
{    
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
        return DLF_SDLV_INIT_ERROR;

    dlfWindow = SDL_CreateWindow(windowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);

    if(dlfWindow == NULL)
        return DLF_SDL_WINDOW_INIT_ERROR;

    dlfSurface = SDL_GetWindowSurface(dlfWindow);
    return DLF_SUCCESS;
}