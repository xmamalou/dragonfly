# Dragonfly

>[!WARNING]
> Dragonfly is not in a working state! Everything is subject to change and may break!

## About the project

Dragonfly is (planned to be) a renderer and physics simulation engine, using Vulkan as its graphics/GPU API.

It is meant to be simple to use, without abstracting too much over Vulkan, whilst also reducing the amount of boilerplate needed to write a Vulkan-based app.

## Building

Dragonfly supports and will support purely Visual Studio as its method of building.

Itself, it is build as a dynamic library (.dll). One sample project (`Testing`) is also built along with it.

Dragonfly uses C++20 features (namely modules), hence C++20 features *need* to be enabled in Visual Studio's properties.

>[!IMPORTANT]
> The same applies, obviously, when using Dragonfly.

## External Dependencies

Dragonfly uses 2 libraries:

- [GLFW](https://github.com/glfw/glfw)
- [Vulkan](https://vulkan.lunarg.com/)

GLFW is *statically* linked.

>[!IMPORTANT]
> The above dependencies **need** to be included even when not building Dragonfly and just using it, as the import modules actively include them
