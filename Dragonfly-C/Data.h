/*
   Copyright 2023 Christopher-Marios Mamaloukas

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef DFL_DATA_H_
#define DFL_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/* -------------------- *
 *   ERROR CODES        *
 * -------------------- */

#define DFL_SUCCESS 0 // operation was successful

#define DFL_GENERIC_OOM_ERROR -0x00F // generic out of memory error
#define DFL_GENERIC_NO_SUCH_FILE_ERROR -0xF17E // generic file not found error
#define DFL_GENERIC_NULL_POINTER_ERROR -0xA110 // generic null pointer error
#define DFL_GENERIC_OUT_OF_BOUNDS_ERROR -0x0B0B // generic out of bounds error
#define DFL_GENERIC_ALREADY_INITIALIZED_ERROR -0x1A1A // generic already initialized error
#define DFL_GENERIC_OVERFLOW_ERROR -0x0F10 // generic overflow error
#define DFL_GENERIC_ILLEGAL_ACTION_ERROR -0x1A5E // thrown by functions that are not supposed to be called by the user (any prefixed with `_dfl` that could potentially cause dramatic issues)

#define DFL_GLFW_INIT_ERROR -0x50BAD // glfw initialization error
#define DFL_GLFW_WINDOW_ERROR -0x533 // glfw window creation error

#define DFL_VULKAN_INSTANCE_ERROR -0xDED // vulkan instance creation error
#define DFL_VULKAN_DEVICE_ERROR -0xF00C // vulkan device creation error
#define DFL_VULKAN_LAYER_ERROR -0xCA4E // vulkan layer creation error
#define DFL_VULKAN_DEBUG_ERROR -0xB0B0 // vulkan debug creation error
#define DFL_VULKAN_EXTENSION_ERROR -0xFA7 // vulkan extension creation error
#define DFL_VULKAN_SURFACE_ERROR -0xBADBED // vulkan surface creation error
#define DFL_VULKAN_QUEUE_ERROR -0x10B // vulkan queue creation error

// other definitions

#define DFL_MAX_CHAR_COUNT 256 // the maximum number of characters that can be used in a string

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