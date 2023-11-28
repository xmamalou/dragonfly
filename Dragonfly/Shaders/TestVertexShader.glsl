#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec3 VertexColour;

layout(location = 0) out vec3 Colour;

void main()
{
    gl_Position = vec4(VertexPosition, 1.0);
    Colour = VertexColour;
}