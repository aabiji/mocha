#include "convert.h"
#include "keyframes.h"

Keyframes::Keyframes(aiNodeAnim* n)
{
    for (unsigned int i = 0; i < n->mNumPositionKeys; i++) {
        aiVectorKey key = n->mPositionKeys[i];
        positions.push_back({ key.mTime, toVec3(key.mValue) });
    }

    for (unsigned int i = 0; i < n->mNumScalingKeys; i++) {
        aiVectorKey key = n->mScalingKeys[i];
        scalings.push_back({ key.mTime, toVec3(key.mValue) });
    }

    for (unsigned int i = 0; i < n->mNumRotationKeys; i++) {
        aiQuatKey key = n->mRotationKeys[i];
        rotations.push_back({ key.mTime, toQuat(key.mValue) });
    }
}

// Interpolate between 2 vectors or quaternions
template <typename T>
T interpolate(std::vector<std::pair<double, T>>& keyframes, double time) {
    // In order to interpolate we need at least 2 values
    if (keyframes.size() == 1)
        return keyframes[0].second;

    // Get the current keyframe
    size_t index = 0;
    for (size_t i = 0; i < keyframes.size(); i++) {
        if (keyframes[i].first > time) {
            index = i - 1;
            break;
        }
    }

    assert(index + 1 < keyframes.size());
    auto current = keyframes[index];
    auto next = keyframes[index + 1];

    // Calculate the percentage of the animation that has ran
    float factor = (time - current.first) / (next.first - current.first);
    assert(factor >= 0.0 && factor <= 1.0);

    // Spherically interpolate quaternions, linearly interpolate vectors
    if constexpr (std::is_same<T, glm::quat>::value)
        return glm::slerp(current.second, next.second, factor);
    return glm::mix(current.second, next.second, factor);
}

glm::mat4 Keyframes::getInterpolatedTransform(double time)
{
    if (positions.size() == 0)
        return glm::mat4(1.0);

    glm::vec3 position = interpolate<glm::vec3>(positions, time);
    glm::vec3 scaling = interpolate<glm::vec3>(scalings, time);
    glm::quat rotation = interpolate<glm::quat>(rotations, time);

    glm::mat4 t = glm::translate(glm::mat4(1.0), position);
    glm::mat4 s = glm::scale(glm::mat4(1.0), scaling);
    glm::mat4 r = glm::mat4(glm::normalize(rotation));

    return t * r * s;
}
