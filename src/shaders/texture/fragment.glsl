#version 460 core

in vec2 coord;
out vec4 color;
uniform sampler2D tex;

void main()
{
    color = texture(tex, coord);
}