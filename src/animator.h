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
        glm::mat4 parentTransform,
        double time, bool playing
    );

    std::string name;
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
    std::vector<std::string> animationNames();

    // Compute the bone transforms for the current transform given the
    // time in seconds and return a pointer to the current animation.
    Animation* run(double seconds);

    int getNumBoneTransforms();

    bool playing;
    size_t currentAnimation;
private:
    Node readNodeData(const aiScene* scene, aiNode* data);

    Node rootNode;
    BoneMap bones;
    std::vector<Animation> animations;
};
