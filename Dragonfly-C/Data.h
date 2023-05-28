/*
    Copyright 2022 Christopher-Marios Mamaloukas

    Redistribution and use in source and binary forms, with or without modification,
    are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation and/or
    other materials provided with the distribution.

    3. Neither the name of the copyright holder nor the names of its contributors
    may be used to endorse or promote products derived from this software without
    specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
    BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
    EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef DFL_DATA_H_
#define DFL_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------- *
 *   ERROR CODES        *
 * -------------------- */

#define DFL_SUCCESS 0 // operation was successful

#define DFL_GENERIC_OOM_ERROR -0x00F // generic out of memory error
#define DFL_GENERIC_NO_SUCH_FILE_ERROR -0xF17E // generic file not found error
#define DFL_GENERIC_NULL_POINTER_ERROR -0xA110 // generic null pointer error
#define DFL_GENERIC_OUT_OF_BOUNDS_ERROR -0x0B0B // generic out of bounds error

#define DFL_GLFW_INIT_ERROR -0x50BAD // glfw initialization error
#define DFL_GLFW_WINDOW_ERROR -0x533 // glfw window creation error

#define DFL_VULKAN_INSTANCE_ERROR -0xDED // vulkan instance creation error
#define DFL_VULKAN_DEVICE_ERROR -0xF00C // vulkan device creation error
#define DFL_VULKAN_LAYER_ERROR -0xCA4E // vulkan layer creation error
#define DFL_VULKAN_DEBUG_ERROR -0xB0B0 // vulkan debug creation error
#define DFL_VULKAN_EXTENSION_ERROR -0xFA7 // vulkan extension creation error
#define DFL_VULKAN_SURFACE_ERROR -0xBADBED // vulkan surface creation error
#define DFL_VULKAN_QUEUE_ERROR -0x10B // vulkan queue creation error

#include <stdbool.h>

// A generic 2D vector that can refer to anything that needs 2 coordinates.
struct DflVec2D
{
	int x;
	int y;
};

#define DFL_MAKE_HANDLE(type) typedef struct type##_HND* type; // a handle to a type
#define DFL_HANDLE(type) ((struct Dfl##type##_T*)*p##type) // a shorthand for casting a handle to its type (will be used when `type` refers to a function argument in the form `ptype` (pointer to handle))
#define DFL_HANDLE_ARRAY(type, pos) ((struct Dfl##type##_T*)*(p##type##s + pos)) // a shorthand for casting a handle to its type (will be used when `type` refers to a function argument in the form `ptype` (pointer to handle))

// opaque handle for a DflSession_T object.
DFL_MAKE_HANDLE(DflSession);
// opaque handle for a DflDevice_T object.
DFL_MAKE_HANDLE(DflDevice);
// opaque handle for a DflWindow_T object.
DFL_MAKE_HANDLE(DflWindow);


#ifdef __cplusplus
}
#endif

#endif // !DFL_DATA_H_