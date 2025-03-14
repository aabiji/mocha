#version 460 core

in vec3 fragmentNormal;
in vec2 textureCoordinate;

out vec4 color;

uniform sampler2D ambient;
uniform sampler2D diffuse;
uniform sampler2D specular;

void main()
{
    color = texture(diffuse, textureCoordinate);
}
