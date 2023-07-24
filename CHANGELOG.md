# DRAGONFLY CHANGELOG

Starting from now, all changes in Dragonfly will be documented in this file.

## unversioned [master] - 4/7/2023
- Functions no longer require pointers to handles as arguments. Instead, they take the handles themselves as arguments.
- Removed field `struct DflVec2D res` from `struct DflWindowInfo`
- Changed field `const char* icon[DFL_MAX_CHAR_COUNT]` to `DflImage icon` in `struct DflWindowInfo`
- Added monitor information functions
- Added `DflImage` and provided functions for loading images
- Added `dflWindowBindToDevice` and `dflWindowUnbindFromDevice` functions which create and destroy a device's swapchain for a window.
- Added functionality for `DFL_GPU_CRITERIA_ALL_QUEUES`. Dragonfly will no more request all the queues by default.
- `dflWindowTerminate` and `dflDeviceTerminate` no longer need their session as an argument. They remember it themselves.
- Restructured code so functions are grouped under the main handle (or usage) they concern.
- `Dragonfly.h` now no longer has external dependencies. It can be included by itself. 
- GLFW is statically linked, so when the project is compiled as a dynamic library, only required dependency (for now) will be Vulkan.

## unversioned [master] - 24/7/2023
- Destruction functions for handles that are dependent on a DflSession handle will now require explicit passing of said handle, even if the code itself doesn't make use of the handle. This is done as a way to remind the user to not destroy objects after their dependencies have been destroyed.
- Added `DflMemoryPool` and `DflBuffer` handles.
- Added `DflMemoryPool_T`, `DflMemoryBlock_T`, `DflBuffer_T` and `DflBufferChunk_T` structs.
- Added `dflMemoryPoolCreate`, `dflMemoryPoolDestroy`, `dflMemoryPoolExpand`, `dflMemoryPoolSizeGet` and `dflMemoryPoolBlockCountGet` function declarations and definitions. Memory pools can be created using both host and GPU resources
- Added `dflAllocate` function declaration, but unfinished definition.
- Added `dflReallocate` and `dflFree` function declarations, but no definitions.
- Caching of monitor info no longer happens within `dflMonitorsGet` but happens when creating a session.
- Added error codes