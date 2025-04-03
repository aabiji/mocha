#version 460 core
#include "shared.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 coord;
layout(location = 4) in ivec4 boneIds;
layout(location = 5) in vec4 boneWeights;

out FragmentInfo
{
    vec3 viewPos;
    vec3 fragmentPos;
    vec2 textureCoord;
    vec3 vertexNormal;
    vec3 lightPos[NUM_LIGHTS]; // Light positions in tangent space
} fragOut;

// TODO: these should be in some sort of buffer, instead of being uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPosition;

uniform mat4 meshTransform; // Fallback transform when the vertex has no associated bone
uniform mat4 boneTransforms[41]; // TODO: size???

void main()
{
    vec4 updatedPosition = vec4(0.0);
    vec3 updatedNormal = vec3(0.0);

    // Transform the vertex with the given bone transformations
    for (int i = 0; i < 4; i++) {
        // Bone has no influence
        if (boneIds[i] == -1) continue;

        vec4 p = boneTransforms[boneIds[i]] * vec4(position, 1.0);
        updatedPosition += p * boneWeights[i];

        vec3 n = mat3(boneTransforms[boneIds[i]]) * normal;
        updatedNormal += n * boneWeights[i];
    }

    if (boneIds == vec4(-1)) { // Vertex has no associated bones
        updatedPosition = meshTransform * vec4(position, 1.0);
        updatedNormal = mat3(meshTransform) * normal;
    }

    // Scale the vertex normal and tangent properly
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vec3 T = normalize(normalMatrix * tangent);
    vec3 N = normalize(normalMatrix * updatedNormal);
    // Make sure the tangent is perpendicular to the normal
    T = normalize(T - dot(T, N) * N);
    // Derive the bitangent
    vec3 B = cross(N, T);
    // Now we have the tangent-bitangent-normal matrix
    mat3 TBN = mat3(T, B, N);

    // Map the vectors from world space to tangent space so that
    // all the lighting calculations can be done in tangent space
    fragOut.vertexNormal = N;
    fragOut.viewPos = TBN * viewPosition;
    fragOut.fragmentPos = TBN * vec3(model * updatedPosition);
    for (int i = 0; i < NUM_LIGHTS; i++) {
        fragOut.lightPos[i] =
            TBN * vec3(model * vec4(lights[i].position, 1.0));
    }

    gl_Position = projection * view * model * updatedPosition;
    fragOut.textureCoord = coord;
}
