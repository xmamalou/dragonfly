# FILEPATH: ${DRAGONFLY_ROOT}/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "" FORCE)
set(CMAKE_C_STANDARD 17) 

set(HOME $ENV{HOME})
set(VULKAN_SDK $ENV{VULKAN_SDK})

# project info
project(Dragonfly VERSION 0.1.0 LANGUAGES C)

add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION ${HOME}/glfw/lib/libglfw3.a
)
add_library(vulkan SHARED IMPORTED)
set_target_properties(vulkan PROPERTIES
    IMPORTED_LOCATION ${VULKAN_SDK}/lib/libvulkan.so
)

include_directories(Dragonfly-C)
include_directories(Dragonfly-C/Render)

# source files
set(SOURCES
    Dragonfly-C/Render/Image.c
    Dragonfly-C/Render/Monitor.c
    Dragonfly-C/Render/Session.c
    Dragonfly-C/Render/Window.c
    Dragonfly-C/Main.c
)

# this is only temporary, when in a finished state, Dragonfly will be a (dynamically linked) library
add_executable(dragonfly ${SOURCES})

target_link_libraries(dragonfly
    glfw
    vulkan
    -lm
    -lXrandr
    -lX11
    -lXext
)
target_include_directories(dragonfly
    PUBLIC
        ${HOME}/glfw/include
        ${VULKAN_SDK}/include
)