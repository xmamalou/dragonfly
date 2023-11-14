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

#include <string>

module Dragonfly.Graphics:Image;

namespace DflGr = Dfl::Graphics;

DflGr::Image::Image(const std::string_view& file)
{
    this->Path = file;
}

void DflGr::Image::Load()
{
    // TODO: Implement image loading
    return;
}