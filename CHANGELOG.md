# DRAGONFLY CHANGELOG

## unversioned [master] - 11/9/2023
- ~~Fixed bug with memory pool creation and expansion, where Dragonfly would keep trying to allocate memory from device heaps even though allocation was already successful.~~
- ~~Buffers are now created to span the memory of the block that is assigned to them, instead of being created when a Dragonfly buffer is allocated~~
- NOTE: All memory management and thread management related functions will now be moved to a new project, called "Termite" as to keep the scope of Dragonfly focused and to make development of memory and thread management much more substantial.
- Added CMake building method (not fully functional).
- Any piece of code that mentioned `strcpy_s` has it now replaced with the macro `STRCPY`, since `strcpy_s` is Windows specific. The macro points to the Windows specific (and memory safe) version of `strcpy` if the library is being built on Windows (or for Windows). Otherwise, to regular `strcpy`.
- Devices are now unified with sessions.
    - Removed `DflDevice` handle and any associated macros
    - Removed `error` field from `struct DflDevice_T`
    - Removed `device` field from `struct DflWindow_T`
    - Added `deviceIndex` field in `struct DflWindow_T`
    - Replaced `dflDeviceInit` with `dflSessionInitDevice`
    - Replaced `dflSessionDevicesGet` with `dflSessionLoadDevices`
       - Both of these are of type `void`
    - Added `dflSessionDeviceCountGet`
    - Added `dflSessionDeviceNameGet`
    - Added `dflSessionDeviceTerminate`
    - All internal functions in `Dragonfly-C/Device.c` are now moved in `Dragonfly-C/Session.c`
    - Any function requiring a device handle now instead requires a session handle and the device index.
- Removed `surface` field from `struct DflSession_T`. Dragonfly now checks the first available window surface to perform presentation checks.
- `dflWindowUnbindFromDevice` now requires only the window handle as an argument. 
- Added `rank` field in `struct DflDevice_T`, of type `uint32_t`. 
- Added `dflSessionDeviceRankGet`.
- Added `DFL_CHOOSE_ON_RANK`. Dragonfly is now able to choose a GPU based on its rank.

## unversioned [master] - 24/7/2023
- Destruction functions for handles that are dependent on a DflSession handle will now require explicit passing of said handle, even if the code itself doesn't make use of the handle. This is done as a way to remind the user to not destroy objects after their dependencies have been destroyed.
- Added `DflMemoryPool` and `DflBuffer` handles.
- Added `DflMemoryPool_T`, `DflMemoryBlock_T`, `DflBuffer_T` and `DflBufferChunk_T` structs.
- Added `dflMemoryPoolCreate`, `dflMemoryPoolDestroy`, `dflMemoryPoolExpand`, `dflMemoryPoolSizeGet` and `dflMemoryPoolBlockCountGet` function declarations and definitions. Memory pools can be created using both host and GPU resources
- Added `dflAllocate` function declaration, but unfinished definition.
- Added `dflReallocate` and `dflFree` function declarations, but no definitions.
- Caching of monitor info no longer happens within `dflMonitorsGet` but happens when creating a session.
- Added more error codes

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
- `Dragonfly.h` now no longer has external dependencies. It can be included by just itself. 
- GLFW is statically linked, so when the project is compiled as a dynamic library, only required dependency (for now) will be Vulkan.

Starting from now, all changes in Dragonfly will be documented in this file.