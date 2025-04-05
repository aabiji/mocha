#pragma once
#include <glm/glm.hpp>

struct Bone
{
    // Index of the bone in the bone transforms list
    int id;
    // Converts a vertex from local space to bone space
    glm::mat4 inverseBindMatrix;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 coord;
    glm::ivec4 boneIds;
    glm::vec4 boneWeights;
};
