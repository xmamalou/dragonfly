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

#include "Dragonfly.h"

#include <vector>

#include <vulkan/vulkan.h>

export module Dragonfly.Graphics:Space;

import Dragonfly.Observer;

namespace DflOb = Dfl::Observer;

namespace Dfl
{
    namespace Graphics
    {
        // denotes what the Space is intended to be used for
        export enum class SpacePurpose {
            Backdrop, // Space is drawn before another Space, but does not export its depth buffer
            Layer, // Space is drawn before another Space, but also exports its depth buffer for the next space.
            Texture, // Space is used by another space as a texture
            Out // Space directly outputs to a Window
        };

        export struct SpaceInfo {
            bool              CustomVertexShader{ false };
            std::vector<char> VertexShader{ {} };
            
            bool              CustomGeomShader{ false };
            std::vector<char> GeomShader{ {} };

            bool              CustomFragShader{ false };
            std::vector<char> FragShader{ {} };

            SpacePurpose      Purpose{ SpacePurpose::Out };
        };

        export class Space {
        protected:
            SpaceInfo                    Info;
            VkGraphicsPipelineCreateInfo PipelineInfo;
            VkPipeline                   GraphicsPipeline;
            VkSubpassDescription         Subpass;
        public:
            Space(const SpaceInfo& info);
        };       
    }
}