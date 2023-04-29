#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// ERROR CODES

#define DFL_SUCCESS 0 // operation was successful
#define DFL_FILE_NOT_FOUND 1 // Specified file was not found
#define DFL_VULKAN_NOT_INITIALIZED 2 // Vulkan was not initialized

// BOOLEAN MACROS

#define DFL_TRUE 1
#define DFL_FALSE 0

struct DflVec2D
{
	int x;
	int y;
};

#define DFL_MAKE_HANDLE(type) typedef struct type##_HND* type; // a handle to a type

#ifdef __cplusplus
}
#endif