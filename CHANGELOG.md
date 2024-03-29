# DRAGONFLY CHANGELOG

## unversioned [master-cpp] - 14/11/2023

- Whole project is rewritten from C to C++.
  - Handles are replaced by objects and the structs being pointed to by those handles by private class fields.
  - Macro definitions have been replaced by enum classes.
  - Naming scheme changed
    - Every function, class, struct and enum class of Dragonfly now belongs under the `Dfl` namespace, with various sub-namespaces being used, depending on specific purposes.
    - Getters are now just represented by the noun of the property they get.
    - Casing follows PascalCase. Exception is when fields referred are handles, where the initial letter is h (of small case), or pointers, where the initial letter is p (of small case).
  - Using C++20 feature of **modules**, instead of header files, to structure Dragonfly. This means that, in order to successfully compile Dragonfly, you need a C++ compiler that supports C++20 features, and specifically modules
- Dragonfly is now limited to scope from platform agnostic to supporting Windows.
- Dragonfly is now compiled as a dynamic library (.dll). Project is supplied with a test executable.
- ***Window***:
  - Windows are now represented by the class `Dfl::Observer::Window`.
  - `Dfl::Observer::WindowInfo` (former `DflWindowInfo`) has the following changes:
    - `dim` is renamed to `Resolution` and is represented by an `std::tuple` type.
    - `View` is now represented by an `std::tuple` type.
    - Likewise for `Position`
    - `mode` is replaced by `DoFullscreen` of type `bool`.
    - `colorFormat`, `swizzling`, `layers` (temporarily) removed.
    - `vsync` is renamed `DoVsync`.
    - `name` is renamed to `WindowTitle` and is represented by an `std::string_view` type of arbitrary length
    - Introduced field `Processes`, of type `std::vector`, that contains pointers to objects of class `Dfl::Observer::WindowProcess`. More on that class below.
    - Introduced fields `IsVisible` and `hWindow`, meant to handle offscreen rendering and child windows (to other windows).
  - Windows are now launched on their own thread. GFLW initialization and termination is handled per window thread.
  - GLFW is limited to window handling only. Attempts to manage displays through Vulkan (or Windows-native methods) are going to be attempted later in development
  - Introduced pure abstract class `Dfl::Observer::WindowProcess` with overloaded operator `()` with parameter a reference to an object of struct `Dfl::Observer::WindowProcessArgs`. This class is meant to be inherited from by user-defined classes and is meant to be executed by the window thread it belongs to.
  - Introduced struct `Dfl::Observer::WindowProcessArgs` which contains arguments passed to processes executed in window threads. Currently, it is empty.
  - `dflWindowInit` is now broken into two stages; the `Dfl::Observer::Window` constructor and `Dfl::Observer::Window::OpenWindow()`. The latter stage launches the thread and opens the window.
  - Temporarily removed any specific window management functions. They will be readded later in development.
  - `dflWindowTerminate` is noww replaced by the `Dfl::Observer::Window` destructor. That also means that Window destruction needs not (and should not) be called explicitly.
  - `glfwPollEvents` is now called by the window thread. The user needs not call it to poll window events.
  - User can now get the `HWND` of the window with `Dfl::Observer::Window::WindowHandle()`.
  - User can push new `Dfl::Observer::WindowProcess` processes to a window thread by calling `Dfl::Observer::PushProcess`. Synchonization is ensured by Dragonfly, and the user needs not handle it externally. Calling thread is blocked until it has successfully pushed a process to a window thread.
- ***Session***:
  - Sessions are now represented by the class `Dfl::Hardware::Session`.
  - `Dfl::Hardware::SessionInfo` (former `DflSessionInfo`) has the following changes:
    - `sessionCriteria` is now replaced by bools `DoDebug` and `EnableRTRendering`.
    - Added `EnableRaytracing` field, of type `bool`. Enabling it loads raytracing extensions into a session.
  - `dflSessionCreate` has been broken into two stages, the constructor of `Dfl::Hardware::Session` and `Dfl::Hardware::Session::InitVulkan()`. The latter creates a Vulkan instance.
  - `dflSessionLoadDevices` is now replaced by `Dfl::Hardware::Session::LoadDevices()`. The functionality has remained largerly the same.
  - `dflSessionInitDevice` is now replaced by `Dfl::Hardware::Session::InitDevice()`:
    - device index and GPU criteria are included under a new struc, `GPUInfo`.
    - The same struct also includes pointers to `Dfl::Observer::Window` windows that the device will use to render to.
      - This means that `dflWindowBindToDevice` and `dflWindowUnbindFromDevice` are now obsolete.
      - This also means that windows cannot be added dynamically to a device to be handled by.
  - The way queues are claimed by Dragonfly is changed. Now, Dragonfly claims one queue per Window and Simulation, and one additional one, for a generic use.
  - Surface and swapchain creation are not yet ported over.

## unversioned [master] - 1/10/2023

- Flag `DFL_GPU_CRITERIA_LOW_PERFORMANCE` is now effective.
- Changed the way Dragonfly manages queues; 
  - `DflQueueCollection_T` is now `DflQueueFamily_T` and concerns *solely* queue families.
  - Enabled queues are now contained on a 4-array, which distinguishes them based on 4 types (`DflSingleTypeQueueArray_T`).
  - Each of the members of the afore mentioned array contains a count of the enabled queues per device and the handles of the queues themselves, along with their queue family index
  - Each family has now only one command pool initialized and only if it has a queue activated.
  - Each family now records the amount of queues per family and their type.
  - `DFL_QUEUE_TYPE_*` macros changed to flags.
- Fixed memory access issues where Dragonfly would attempt to allocate memory far, far greater than it needed.
- Types are now more consistent
  - Fields that didn't make sense to be signed are now signed.
  - `DFL_CHOOSE_ON_RANK` is now defined as the biggest integer possible, instead of as a negative number.
  - Any function that refers to a device index changed the device index type from `int` to `uint32_t`.
- Added `dflSessionActivatedDeviceIndicesGet` function.

## unversioned [master] - 19/9/2023

- Fixed issue [#1](https://github.com/xmamalou/dragonfly/issues/1), where Dragonfly was unable to create a Vulkan device due to malformed device extension names. The reason was that the passed variable was out of scope at the moment of the creation of the device.
- Related to this is the addition of the fields `extensionsCount` and `extentionNames` in `struct DflDevice_T`.
- Fixed type issues in queue priority setting, that would cause garbage values to be set as priorities for queues.
- Made explicit type conversions when using `calloc` in places where said conversions were left implicit.
- Changed error `DFL_GENERIC_ALREADY_INITIALIZED_ERROR` to `DFL_GENERIC_ALREADY_INITIALIZED_WARNING` (from value -0x1004 to value 0x1001).
- Readded `DFL_GPU_CRITERIA_ONLY_OFFSCREEN` criterion.
- Removed any reference to obsolete handles (`DflDevice`).
- Removed `surface` field from `struct DflSession_T`, as presentation support checking now happens using the windows directly.
- Changed certain `int` types to `uint32_t`, as the variables they concerned were never going to have negative values stored in them.

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
