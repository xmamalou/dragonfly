/*
   Copyright 2024 Christopher-Marios Mamaloukas

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

#pragma once

// Dragonfly.hxx

#include <stdint.h>

#include "Dragonfly.h"

#include "Dragonfly.Error.hxx"
#include "Dragonfly.Generics.hxx"
// Dfl::Hardware
#include "Dragonfly.Hardware.Session.hxx"
#include "Dragonfly.Hardware.Device.hxx"
// Dfl::Memory
#include "Dragonfly.Memory.Block.hxx"
#include "Dragonfly.Memory.Buffer.hxx"
// Dfl::Graphics
#include "Dragonfly.Graphics.Renderer.hxx"
// Dfl::UI
#include "Dragonfly.UI.Window.hxx"

#include "../WinRT/Dragonfly.WinRT.hxx"

namespace Dfl{
    consteval uint32_t          MakeVersion(
                                    uint32_t major, 
                                    uint32_t minor, 
                                    uint32_t patch){ 
                                        return (major << 16) | (minor << 8) | patch; };
    constexpr uint32_t          EngineVersion{ MakeVersion(0, 1, 0) };

    consteval uint64_t          MakeBinaryPower(uint64_t power) { 
                                    uint64_t value{ 1 }; 
                                    while(power > 0) { value *= 2; power--; } 
                                    return value; }
    constexpr uint64_t          Kilo { MakeBinaryPower(10) };
    constexpr uint64_t          Mega { MakeBinaryPower(20) };
    constexpr uint64_t          Giga { MakeBinaryPower(30) };
    constexpr uint64_t          Tera { MakeBinaryPower(40) };
    constexpr uint64_t          Peta { MakeBinaryPower(50) };

    constexpr uint32_t          NoOptions{ 0 };

    using Session = Hardware::Session;
    using SessionInfo = Session::Info;

    using Device = Hardware::Device;
    using DevInfo = Device::Info;
    using DevOptions = Device::RenderOptions;
}