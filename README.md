# Dragonfly - Rendering and Physics

Dragonfly is not in a working state. 

Currently, it can do the following:
- Create a Vulkan instance
- Initiate a Vulkan device
- Create a window, bind it to a Vulkan surface and change its properties (only if the instance it's bound to supports presentation)

## Building
Currently, the repo is structured in such a way to be built using Visual Studio (2022). However, since the project (and the libraries it uses) are OS-agnostic, building in other OSes or using other build systems is possible, but currently, you'd need to do it manually. When Dragonfly is in a working state, more build methods will be worked on.

## External Dependencies
Dragonfly uses 3 libraries, one of which ([stb](https://github.com/nothings/stb)) is contained as a submodule in the repo. The other two are external and are the following:
- [GLFW](https://github.com/glfw/glfw)
- [Vulkan](https://vulkan.lunarg.com/)

Both are dynamically linked
