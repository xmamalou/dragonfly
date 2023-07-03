#include "../Dragonfly.h"
#include "../Internal.h"

#include <stdlib.h>
#include <string.h>

struct DflMonitorInfo* dflMonitorsGet(int* pCount)
{
    if (!glfwInit())
        return NULL;

    GLFWmonitor** monitors = glfwGetMonitors(pCount);

    if (!monitors)
        return NULL;

    struct DflMonitorInfo* infos = malloc(sizeof(struct DflMonitorInfo) * *pCount);
    if(infos == NULL)
        return NULL;

    for (int i = 0; i < *pCount; i++)
    {
        GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);

        strcpy_s(infos[i].name, DFL_MAX_CHAR_COUNT, glfwGetMonitorName(monitors[i]));

        infos[i].res.x = mode->width;
        infos[i].res.y = mode->height;

        infos[i].rate = mode->refreshRate;

        infos[i].colorDepthPerPixel.x = mode->redBits;
        infos[i].colorDepthPerPixel.y = mode->greenBits;
        infos[i].colorDepthPerPixel.z = mode->blueBits;

        infos[i].colorDepth = mode->redBits + mode->greenBits + mode->blueBits;
    }

    return infos;
}