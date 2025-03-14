#version 460 core

in vec3 vertexNormal;
in vec2 textureCoordinate;

out vec4 color;

uniform sampler2D ambient;
uniform sampler2D diffuse;
uniform sampler2D specular;
uniform sampler2D normals;

void main()
{
    color = texture(diffuse, textureCoordinate);
}
