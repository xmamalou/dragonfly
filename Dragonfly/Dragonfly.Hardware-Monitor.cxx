module;

#include <vector>
#include <memory>
#include <string>
#include <GLFW/glfw3.h>

module Dragonfly.Hardware:Monitor;

namespace DflHW = Dfl::Hardware;

std::vector<DflHW::MonitorInfo> DflHW::Monitors()
{
    if (!glfwInit())
        return {};

    int count = 0;
    GLFWmonitor** pMonitor(glfwGetMonitors(&count));

    if (pMonitor == nullptr)
        return {};

    std::vector<DflHW::MonitorInfo> monitors(count);

    for (int i = 0; i < count; i++)
    {
        const GLFWvidmode* mode = glfwGetVideoMode(pMonitor[i]);

        monitors[i].Name = std::string(glfwGetMonitorName(pMonitor[i]));

        monitors[i].Resolution = std::make_tuple(mode->width, mode->height);
        monitors[i].Rate = mode->refreshRate;

        monitors[i].DepthPerPixel = std::make_tuple(mode->redBits, mode->greenBits, mode->blueBits);
        monitors[i].Depth = mode->redBits + mode->greenBits + mode->blueBits;
    }

    return monitors;
}