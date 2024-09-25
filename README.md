# Dragonfly

>[!WARNING]
> Dragonfly is frozen in development! It may resume development in the future under a new goal, but currently it's not being worked on.

## About the project (see warning above)

~~Dragonfly is (planned to be) a renderer and physics simulation engine, using Vulkan as its graphics/GPU API.~~

~~It is meant to be simple to use, without abstracting too much over Vulkan, whilst also reducing the amount of boilerplate needed to write a Vulkan-based app.~~

## Building

Dragonfly supports and will support purely Visual Studio as its method of building.

Itself, it is build as a dynamic library (.dll). One sample project (`Testing`) is also built along with it.

Dragonfly uses C++20 features (for example, concepts and coroutines), hence C++20 features *need* to be enabled in Visual Studio's properties.

>[!IMPORTANT]
> The same applies, obviously, when using Dragonfly.

## External Dependencies

Dragonfly uses 1 library:
- [Vulkan](https://vulkan.lunarg.com/)

