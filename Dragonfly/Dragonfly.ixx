#include <stdint.h>

#include "Dragonfly.h"

export module Dragonfly;

export import Dragonfly.Hardware;
export import Dragonfly.Observer;
export import Dragonfly.Graphics;

namespace Dfl
{
    export inline uint32_t MakeVersion(uint32_t major, uint32_t minor, uint32_t patch)
    {
        return (major << 16) | (minor << 8) | patch;
    };

    export const uint32_t EngineVersion{ MakeVersion(0, 1, 0) };
}