#version 460 core
#include "buffers.glsl"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 coord;
layout(location = 4) in ivec4 boneIds;
layout(location = 5) in vec4 boneWeights;

out FragmentInfo
{
    vec3 vertexPos;
    vec3 vertexNormal;
    vec2 textureCoord;
    mat3 TBN;
} fragOut;

void main()
{
    vec4 basePosition = meshTransform * vec4(position, 1.0);
    vec3 baseNormal = mat3(meshTransform) * normal;

    vec4 updatedPosition = vec4(0.0);
    vec3 updatedNormal = vec3(0.0);

    // Transform the vertex with the given bone transformations
    for (int i = 0; i < 4; i++) {
        // Bone has no influence
        if (boneIds[i] == -1 || boneIds[i] > boneTransforms.length())
            break;

        vec4 p = boneTransforms[boneIds[i]] * basePosition;
        updatedPosition += p * boneWeights[i];

        vec3 n = mat3(boneTransforms[boneIds[i]]) * baseNormal;
        updatedNormal += n * boneWeights[i];
    }

    if (boneIds == vec4(-1)) { // Has no bone influence
        updatedPosition = basePosition;
        updatedNormal = baseNormal;
    }

    // Calculate the tangent-bitangent-normal matrix
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vec3 T = normalize(normalMatrix * tangent);
    vec3 N = normalize(normalMatrix * updatedNormal);
    // Make sure the tangent is perpendicular to the normal
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T); // Derive the bitangent
    mat3 TBN = mat3(T, B, N);

    // Output
    fragOut.textureCoord = coord;
    fragOut.vertexPos = vec3(updatedPosition);
    fragOut.vertexNormal = N;
    fragOut.TBN = TBN;

    gl_Position = projection * view * model * updatedPosition;
}
