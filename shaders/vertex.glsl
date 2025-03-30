#version 460 core
#include "shared.glsl"

layout (location = 0) in vec3 _position;
layout (location = 1) in vec3 _normal;
layout (location = 2) in vec3 _tangent;
layout (location = 3) in vec2 coord;
layout (location = 4) in ivec4 boneIds;
layout (location = 5) in vec4 boneWeights;

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
uniform mat4 boneTransforms[41]; // TODO: size???

void main()
{
    // Transform the vertex with the given bone transformations
    mat4 boneTransform = mat4(1.0);
    for (int i = 0; i < 4; i++) {
        boneTransform += boneTransforms[boneIds[i]] * boneWeights[i];
    }
    vec3 position = vec3(boneTransform * vec4(_position, 1.0));
    vec3 normal   = vec3(boneTransform * vec4(_normal, 0.0));
    vec3 tangent  = vec3(boneTransform * vec4(_tangent, 0.0));

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
