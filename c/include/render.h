/*Copyright 2022 Christopher-Marios Mamaloukas

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation and/or other materials 
provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors 
may be used to endorse or promote products derived from this software without 
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

/*! \file render.h
    \brief Window management and rendering.

    render.h (in dragonfly_engine/c/include) contains function declarations that handle anything window management
    and rendering related. Currently, it contains SDL initialization functions, but it will also 
    contain functions concerning Vulkan initialization and any loop and event related functions.
*/

#pragma once
#ifndef RENDER_H
#define RENDER_H

//#define DFL_NO_DEBUG // uncomment to disable debug messages from validation layers

/*
*   Main render functions
*/

#include <SDL/SDL.h>
#include <SDL/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "sets.h"

/*
* Many functions return an error code in the form of a number.
* 0 = no error
* Any other code is unique to the error.
* The Haskell binding should store the error code in a variable
* and act accordingly.
*
* Void functions won't return anything.
*/

/*
* All functions in this library will start with 
* dfl (from dragonfly) and will have a noun and a gerund 
* that briefly descibe the funtion.
* 
* Functions and variables will follow the Haskell paradigm
* of having a small initial letter.
* CamelCase is used for both.
* Global variables will be prefixed with dfl
* Scope specific variables won't be prefixed with that.
* You will know whether a name refers to a variable or a function
* if it ends with a gerund (then it's a function) or a noun (then it's a variable).
* Boolean variables will be a participle and always of the type 'char'.
*
* Structures and Unions will follow the Haskell paradigm of data types
* having one initial capital letter.
* CamelCase will also be used for them.
*
* Macros will use snake_case and all capitals.
* Macros will mostly be used for error codes.
* Enums will also be written like this.
*/

/* ERROR CODES - Error number depends on when the error code was defined
* during development (except for DFL_SUCCESS, obviously).
*
* Engine-side related errors will always be a natural number.
* Application-side errors, at least those created by me, will always
* be a negative integer.
*/
#define DFL_SUCCESS 0

#define DFL_DUMMY_ERROR 42069

#define DFL_SDLV_INIT_ERROR 1 // Couldn't initialize SDL (video)
#define DFL_SDL_WINDOW_INIT_ERROR 2 // Couldn't initialize SDL window
#define DFL_SDL_MONITOR_INFO_ERROR 3 // Couldn't get monitor info
#define DFL_SDL_VULKEXTENS_ENUM_ERROR 4 // Couldn't get SDL Vulkan instance extension number
#define DFL_SDL_VULKEXTENS_ERROR 5 // Couldn't get SDL extensions for Vulkan
#define DFL_VULKINSTANCE_ERROR 6 // Couldn't create Vulkan instance
#define DFL_VULKLAYERS_ERROR 7 // Couldn't find Vulkan validation layers
#define DFL_VULKDEBUG_MESSENGER_ERROR 8 // Couldn't create Vulkan debug messenger
#define DFL_VULKPHYSICAL_UNAVAILABLE_ERROR 9 // Couldn't find a Vulkan-capable physical device
#define DFL_VULKPHYSICAL_INCAPABLE_ERROR 10 // While Vulkan-capable, device cannot support required features
#define DFL_VULKQFAM_UNAVAILABLE_ERROR 11 // Couldn't find queue families for the specified device
#define DFL_VULKQFAM_INCAPABLE_ERROR 12 // While queue families are available, they don't support required features
#define DFL_VULKDEVICE_ERROR 13 // Couldn't create Vulkan device (logical)

// GLOBAL VARIABLES - SDL
const char*                     dflWindowName; // Doubles as application name
// GLOBAL VARIABLES - VULKAN
VkInstance                      dflVulkInstance; // Vulkan instance
DflSet                          dflVulkExtensions; // Vulkan extensions
#ifndef DFL_NO_DEBUG
    // VALIDATION LAYERS - VULKAN
    DflSet                      dflVulkLayers; // Vulkan Layers
    VkDebugUtilsMessengerEXT    dflDebugMessenger;
#endif
VkPhysicalDevice                dflRealGPU; // Real GPU (physical)
VkDevice                        dflGPU; // Vulkan logical device (refered as GPU hereafter)
VkQueue                         dflGraphicsQueue; // Graphics queue
VkQueue                         dflComputeQueue; // Compute queue

// WINDOW MANAGEMENT - SDL
/**
* @brief Initialize the SDL window. Executing this function 
* gives value to pointers dFLWindow, dFLDisplay and dFLSurface.
*
* \param windowName Name of the window, will appear on title bar.
* \param width Width of the window.
* \param height Height of the window.
* \param fullscreen Whether the window should be fullscreen. If 1, the
* window will inherit the resolution of the monitor (currently broken), 
* ignoring width and height given.
* \returns 0 if no error, otherwise a natural number. 
*
* \since This function exists since Dragonfly 0.0.1
*
* \sa dflWindowKilling
*/
int dflWindowIniting(
    const char* windowName, 
    int         width,
    int         height,
    int         fullscreen); 
/**
 * @brief 
 * Kill the window and clean up SDL variables.
 * 
 * Doesn't return anything.
 * 
 * \since This function exists since Dragonfly 0.0.1
 * 
 * \sa dflWindowIniting
*/
void dflWindowKilling();


// VULKAN MANAGEMENT
/**
 * @brief Initialize Vulkan. Executing this function executes 
 * various other functions specific to each necessary part that
 * makes Vulkan tick.
 * 
 * Currently, it initializes:
 * > a Vulkan instance (broken),
 * 
 * > the Vulkan Validation layers (set DFL_NO_DEBUG flag to skip)
 * 
 * It doesn't have any specific parameters now, those will be added
 * as development progresses.
 * 
 * \since This function exists since Dragonfly 0.0.1
 * 
 * \sa dflVulkanKilling
 */
int dflVulkanIniting();
/**
 * @brief 
 * Free Vulkan resources.
 * May also clean up Vulkan later.
 * 
 * Doesn't return anything.
 * 
 * \since This function exists since Dragonfly 0.0.1
 * 
 * \sa dflVulkanIniting
*/
void dflVulkanKilling();

#endif