#pragma once

#include <assimp/quaternion.h>
#include <assimp/vector3.h>
#include <assimp/matrix4x4.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define FLOAT_EQUAL(x, y) ((abs(x - y)) < FLT_EPSILON)

inline glm::vec2 toVec2(aiVector3D v)   { return glm::vec2(v.x, v.y); }
inline glm::vec3 toVec3(aiVector3D v)   { return glm::vec3(v.x, v.y, v.z); }
inline glm::quat toQuat(aiQuaternion q) { return glm::quat(q.w, q.x, q.y, q.z); }

// Convert from assimp's row major ordering to glm's column major ordering
inline glm::mat4 assimpToGlmMatrix(const aiMatrix4x4& matrix)
{
    glm::mat4 m;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m[j][i] = matrix[i][j];
        }
    }
    return m;
}
