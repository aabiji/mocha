#version 460 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 textureCoord;

uniform mat4 transform;
out vec2 coord;

void main()
{
    gl_Position = transform * vec4(position.x, position.y, 0.0, 1.0);
    coord = textureCoord;
}