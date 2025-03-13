#version 460 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 coord;

out vec3 fragmentNormal;
out vec2 textureCoordinate;

uniform mat4 transform; // model, view, projection matrix
uniform mat4 normalMatrix; // to scale the normals properly

void main()
{
    gl_Position = transform * vec4(position, 1.0);
    fragmentNormal = vec3(normalMatrix * vec4(normal, 1.0));
    textureCoordinate = coord;
}