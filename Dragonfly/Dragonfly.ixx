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

module;

#include <stdint.h>

#include "Dragonfly.h"

export module Dragonfly;

export import Dragonfly.Generics;
export import Dragonfly.Hardware;
export import Dragonfly.Memory;
export import Dragonfly.Observer;
export import Dragonfly.Graphics;

namespace Dfl{
    export inline consteval uint32_t MakeVersion(uint32_t major, uint32_t minor, uint32_t patch){ return (major << 16) | (minor << 8) | patch; };
    export        constexpr uint32_t EngineVersion{ MakeVersion(0, 1, 0) };

    export inline consteval uint32_t MakeBinaryPower(uint32_t power) { uint32_t value{ 1 }; while(power > 0) { value *= 2; power--; } return value; }

    export        constexpr uint32_t NoOptions{ 0 };
}