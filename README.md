# Dragonfly - Rendering and Physics

*** NOTE: THIS BRANCH IS OBSOLETE. It does no longer represent Dragonfly, and will no longer be updated. `master-cpp` is the default branch of the project now. ***

Dragonfly is not in a working state. 

Currently, it has reached the state where a device is ready for work and a window is ready to be rendered to. However, no rendering operations take place.

## Building
Dragonfly supports two methods for building; CMake and Visual Studio (2022). Visual Studio is the strongly tested method for building Dragonfly and it is guaranteed that it works. CMake is also tested, however it's not guaranteed that it will always work.

Dragonfly is built as an executable currently, with the `main` function being in the dummy file `Dragonfly-C/Main.c`. However, when the library reaches a satisfactory state, it will then be built as a dynamic library.

## External Dependencies
Dragonfly uses 3 libraries, one of which ([stb](https://github.com/nothings/stb)) is contained as a submodule in the repo. The other two are external and are the following:
- [GLFW](https://github.com/glfw/glfw)
- [Vulkan](https://vulkan.lunarg.com/)

GLFW is *statically* linked.
