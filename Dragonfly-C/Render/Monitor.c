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

struct DflMonitorInfo* dflMonitorsGet(int* pCount, DflSession hSession)
{
    if ((DFL_SESSION == NULL) || (DFL_SESSION->monitorCount == 0))
    {
        if (!glfwInit())
            return NULL;

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

        DFL_SESSION->monitorCount = *pCount;
        for(int i = 0; i < *pCount; i++) // cache them to the session
        {
            DFL_SESSION->monitors[i] = infos[i];
        }

        return infos;
    }
    else // if we already have them cached
    {
        *pCount = DFL_SESSION->monitorCount;
        return &DFL_SESSION->monitors;
    }
}