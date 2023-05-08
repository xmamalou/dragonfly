#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// ERROR CODES

#define DFL_SUCCESS 0 // operation was successful
#define DFL_FILE_NOT_FOUND 1 // Specified file was not found
#define DFL_VULKAN_NOT_INITIALIZED 2 // Vulkan was not initialized
#define DFL_ALLOC_ERROR 4 // Memory allocation error
#define DFL_NO_LAYERS_FOUND 5 // No layers were found (only applicable if debug is desired but the host cannot provide tools for it)

#include <stdbool.h>

struct DflVec2D
{
	int x;
	int y;
};

#define DFL_MAKE_HANDLE(type) typedef struct type##_HND* type; // a handle to a type

#ifdef __cplusplus
}
#endif