#pragma once

#include <assimp/scene.h>

#include "keyframes.h"
#include "vertex.h"

struct Node
{
    std::string name;
    int meshCount;
    glm::mat4 transform;
    std::vector<Node> children;
};

using BoneMap = std::unordered_map<std::string, Bone>;

class Animation
{
public:
    Animation(aiAnimation* data, BoneMap& bones);
    void computeBoneTransform(
        BoneMap& bones, Node& node,
        double time, glm::mat4 parentTransform
    );

    double ticksPerSecond, duration;
    std::vector<glm::mat4> boneTransforms;
    std::vector<glm::mat4> meshTransforms;
private:
    BoneMap* bonesRef;
    std::unordered_map<std::string, Keyframes> nodeAnimations;
};

class Animator
{
public:
    void load(const aiScene* scene);
    int getBoneId(std::string name);

    // Compute the bone transforms for the current transform given the
    // time in seconds and return a pointer to the current animation.
    Animation* run(double seconds);
private:
    Node readNodeData(const aiScene* scene, aiNode* data);

    int currentAnimation;
    std::vector<Animation> animations;

    Node rootNode;
    BoneMap bones;
};
