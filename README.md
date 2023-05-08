# Dragonfly-C

Dragonfly is not in a working state. 

Currently, it can do the following:
- Open a window and change its properties
- Create a Vulkan instance (with no layers or extensions enabled)
- Sort some device properties (only if the machine contains one device or `DFL_GPU_CRITERIA_HASTY` flag is enabled)

This currently compiles for Windows, using Visual Studio. Other compilation methods will be worked on after Dragonfly reaches a working state

## External Dependencies
Dragonfly uses 3 libraries, one of which ([stb](https://github.com/nothings/stb)) is contained as a submodule in the repo. The other two are external and are the following:
- [GLFW](https://github.com/glfw/glfw)
- [Vulkan](https://vulkan.lunarg.com/)
