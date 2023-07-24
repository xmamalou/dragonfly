/*
   Copyright 2023 Christopher-Marios Mamaloukas

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include "../Dragonfly.h"
#include "../Internal.h"

#include <stdlib.h>
#include <string.h>

struct DflMonitorInfo* dflMonitorsGet(int* pCount)
{
    if (!glfwInit())
    {
        *pCount = 0;
        return NULL;
    }

    GLFWmonitor** monitors = glfwGetMonitors(pCount);

    if (!monitors)
        return NULL;

    struct DflMonitorInfo* infos = malloc(sizeof(struct DflMonitorInfo) * *pCount);
    if (infos == NULL)
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