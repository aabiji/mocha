#version 460 core
#include "shared.glsl"

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 coord;

out FragmentInfo
{
    vec3 viewPos;
    vec3 fragmentPos;
    vec2 textureCoord;
    vec3 vertexNormal;
    vec3 lightPos[NUM_LIGHTS]; // Light positions in tangent space
} fragOut;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPosition;

void main()
{
    // Scale the vertex normal and tangent properly
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vec3 T = normalize(normalMatrix * tangent);
    vec3 N = normalize(normalMatrix * normal);
    // Make sure the tangent is perpendicular to the normal
    T = normalize(T - dot(T, N) * N);
    // Derive the bitangent
    vec3 B = cross(N, T);
    // Now we have the tangent-bitangent-normal matrix
    mat3 TBN = mat3(T, B, N);

    // Map the vectors from world space to tangent space so that
    // all the lighting calculations can be done in tangent space
    fragOut.vertexNormal = N;
    fragOut.viewPos      = TBN * viewPosition;
    fragOut.fragmentPos  = TBN * vec3(model * vec4(position, 1.0));
    for (int i = 0; i < NUM_LIGHTS; i++) {
        fragOut.lightPos[i] =
            TBN * vec3(model * vec4(lights[i].position, 1.0));
    }

    gl_Position = projection * view * model * vec4(position, 1.0);
    fragOut.textureCoord = coord;
}
