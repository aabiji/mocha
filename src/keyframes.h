#pragma once

#include <utility>
#include <vector>

#include <assimp/anim.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// The position, rotation and scaling transforms
// for each keyframe of a node's animation
class Keyframes
{
public:
    Keyframes() {}
    Keyframes(aiNodeAnim* n);
    glm::mat4 getInterpolatedTransform(double time);
private:
    // The pair groups the time at which the transform
    // happened with the transform itself
    std::vector<std::pair<double, glm::vec3>> positions;
    std::vector<std::pair<double, glm::vec3>> scalings;
    std::vector<std::pair<double, glm::quat>> rotations;
};
