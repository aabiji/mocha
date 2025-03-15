#version 460 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 coord;

out vec3 vertexNormal;
out vec3 fragmentPosition;
out vec3 lightPosition;
out vec3 viewPosition;
out vec2 textureCoordinate;

uniform vec3 viewPos;
uniform vec3 lightPos;

uniform mat4 view;
uniform mat4 model;
uniform mat4 projection;

void main()
{
    textureCoordinate = coord;
    vertexNormal = normal;

    // Multiply all the light vectors by the tangent-normal-bitangent
    // matrix so all the lighting calculations can be done in tangent space
    vec3 T = normalize(vec3(model * vec4(tangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(normal, 0.0)));
    vec3 B = cross(N, T); // bitangent
    mat3 TBN = transpose(mat3(T, B, N));
    fragmentPosition = TBN * vec3(model * vec4(position, 1.0));
    lightPosition = TBN * lightPos;
    viewPosition = TBN * viewPos;
    vertexNormal = TBN * normal; // TODO: should we multiply by the normal matrix????

    gl_Position = projection * view * model * vec4(position, 1.0);
}
