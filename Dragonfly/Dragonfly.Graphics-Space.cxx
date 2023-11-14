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

#include <fstream>

#include "vulkan/vulkan.h"

module Dragonfly.Graphics:Space;

namespace DflGr = Dfl::Graphics;

DflGr::Space::Space(const SpaceInfo& info)
{
    this->Info = info;

    this->Subpass.flags = 0;
    this->Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    this->Subpass.colorAttachmentCount = 1;
    this->Subpass.pResolveAttachments = nullptr;
    

    this->PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    this->PipelineInfo.pNext = nullptr;
    this->PipelineInfo.flags = 0;
    this->PipelineInfo.stageCount = 2;
    
    VkPipelineShaderStageCreateInfo shaderInfo;
    shaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderInfo.pNext = nullptr;
    shaderInfo.flags = 0;
}