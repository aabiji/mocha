#version 460 core

layout(location = 0) in vec3 position;
layout(location = 4) in ivec4 boneIds;
layout(location = 5) in vec4 boneWeights;
// TODO: these are unsused -- can we just not pass them in?
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 coord;

// TODO: these should be in some sort of buffer, instead of being uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPosition;

uniform mat4 meshTransform; // Fallback transform when the vertex has no associated bone
uniform mat4 boneTransforms[100]; // TODO: size???

uniform uint modelId;
flat out uvec4 modelIdInput;

void main()
{
    modelIdInput = uvec4(modelId, modelId, modelId, modelId);

    vec4 basePosition = meshTransform * vec4(position, 1.0);
    vec4 updatedPosition = vec4(0.0);

    // Transform the vertex with the given bone transformations
    for (int i = 0; i < 4; i++) {
        // Bone has no influence
        if (boneIds[i] == -1 || boneIds[i] > boneTransforms.length())
            break;

        vec4 p = boneTransforms[boneIds[i]] * basePosition;
        updatedPosition += p * boneWeights[i];
    }

    if (boneIds == vec4(-1)) // Has no bone influence
        updatedPosition = basePosition;
    gl_Position = projection * view * model * updatedPosition;
}
