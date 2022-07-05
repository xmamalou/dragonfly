
#pragma once
#ifndef RENDER_H
#define RENDER_H

/*
*   Main render functions
*/

#include <SDL/SDL.h>
#include <SDL/SDL_vulkan.h>

/*
* All functions return an error code in the form of a number.
* 0 = no error
* Any other code is unique to the error.
* The Haskell binding should store the error code in a variable
* and act accordingly.
*/

/*
* All functions in this library will start with 
* dlf (dragonfly) and will have a noun and a gerund 
* that briefly descibe the funtion.
* 
* Functions and variables will follow the Haskell paradigm
* of having a small initial letter.
* CamelCase is used for both.
* Global variables will be prefixed with dlf
* Scope specific variables won't be prefixed with that.
* You will know whether a name refers to a variable or a function
* if it ends with a gerund (then it's a function) or a noun (then it's a variable).
*
* Structures and Unions will follow the Haskell paradigm of data types
* having one initial capital letter.
* CamelCase will also be used for them.
*
* Macros will use snake_case and all capitals.
* Macros will mostly be used for error codes.
* Enums will also be written like this.
*/

// ERROR CODES - Error number depends on when the error code was defined
// during development (except for DLF_SUCCESS, obviously).
#define DLF_SUCCESS 0

#define DLF_SDLV_INIT_ERROR 1 // Couldn't initialize SDL (video)
#define DLF_SDL_WINDOW_INIT_ERROR 2 // Couldn't initialize SDL window


// GLOBAL VARIABLES
SDL_Window* dlfWindow = NULL;
SDL_Surface* dlfSurface = NULL; // is Vulkan surface the same as this?

// WINDOW MANAGEMENT - SDL
int dlfWindowIniting( // initialise the window
    const char* windowName, 
    int         width,
    int         height,
    int         fullscreen); 

#endif