#version 460 core

in vec3 fragmentNormal;
in vec2 textureCoordinate;

out vec4 color;

uniform int currentTexture;
uniform sampler2D textures[32];

void main()
{
    color = texture(textures[currentTexture], textureCoordinate);
}