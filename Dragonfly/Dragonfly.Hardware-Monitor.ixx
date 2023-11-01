#include <tuple>
#include <string>
#include <vector>

#include "Dragonfly.h"

export module Dragonfly.Hardware:Monitor;

namespace Dfl
{
    namespace Hardware
    {
        export struct MonitorInfo
        {
            std::tuple<uint32_t, uint32_t> Resolution;
            std::tuple<uint32_t, uint32_t> Position; // position of the specific monitor in the screen space
            std::tuple<uint32_t, uint32_t> WorkArea; // how much of the specific moitor is usable

            std::string Name;

            uint32_t    Rate;

            uint32_t    Depth;
            std::tuple<uint32_t, uint32_t, uint32_t> DepthPerPixel;
        };

        export DFL_API std::vector<MonitorInfo> DFL_CALL Monitors();
    }
}