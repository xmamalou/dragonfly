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

// ERROR CODES

#define DFL_SUCCESS 0 // operation was successful
#define DFL_FILE_NOT_FOUND 1 // Specified file was not found
#define DFL_VULKAN_NOT_INITIALIZED 2 // Vulkan was not initialized
#define DFL_ALLOC_ERROR 4 // Memory allocation error
#define DFL_NO_LAYERS_FOUND 5 // No layers were found (only applicable if debug is desired but the host cannot provide tools for it)
#define DFL_NO_GLFW_EXTENSIONS_LOADED 6 // GLFW extensions were not loaded
#define DFL_NO_QUEUES_FOUND 7 // No queues were found

#include <stdbool.h>

// A generic 2D vector that can refer to anything that needs 2 coordinates.
struct DflVec2D
{
	int x;
	int y;
};

#define DFL_MAKE_HANDLE(type) typedef struct type##_HND* type; // a handle to a type

#ifdef __cplusplus
}
#endif

#endif // !DFL_DATA_H_