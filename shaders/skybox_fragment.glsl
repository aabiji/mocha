#version 460 core

in vec3 textureCoord;
out vec4 color;
uniform samplerCube cubeTexture;

void main()
{
    color = texture(cubeTexture, textureCoord);
}