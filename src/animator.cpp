#include "animator.h"
#include "convert.h"

Animation::Animation(aiAnimation* data, BoneMap& ref)
{
    duration = data->mDuration;
    ticksPerSecond = data->mTicksPerSecond;
    boneTransforms.resize(ref.size());

    for (unsigned i = 0; i < data->mNumChannels; i++) {
        aiNodeAnim* n = data->mChannels[i];
        std::string name = n->mNodeName.C_Str();
        nodeAnimations.insert({ name, Keyframes(n) });
    }
}

void Animation::computeBoneTransform(
    BoneMap& bones, Node& node,
    double time, glm::mat4 parentTransform
)
{
    glm::mat4 transform = node.transform;
    if (nodeAnimations.count(node.name))
        transform = nodeAnimations[node.name].getInterpolatedTransform(time);

    // If we have a bone, set its transformation matrix,
    // else set the transform of the mesh directly
    glm::mat4 globalTransform = parentTransform * transform;
    if (bones.count(node.name)) {
        Bone& bone = bones[node.name];
        boneTransforms[bone.id] = globalTransform * bone.inverseBindMatrix;
    } else {
        for (int i = 0; i < node.meshCount; i++) {
            meshTransforms.push_back(globalTransform);
        }
    }

    for (Node& child : node.children) {
        computeBoneTransform(bones, child, time, globalTransform);
    }
}

void Animator::load(const aiScene* scene)
{
    currentAnimation = 0;
    rootNode = readNodeData(scene, scene->mRootNode);

    for (unsigned i = 0; i < scene->mNumAnimations; i++) {
        aiAnimation* a = scene->mAnimations[i];
        animations.push_back(Animation(a, bones));
    }
}

int Animator::getBoneId(std::string name) { return bones[name].id; }

// Read the necessary bone and node data
Node Animator::readNodeData(const aiScene* scene, aiNode* data)
{
    // Read bones that have not already been read
    for (unsigned i = 0; i < data->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[data->mMeshes[i]];
        for (unsigned int j = 0; j < mesh->mNumBones; j++) {
            aiBone* boneData = mesh->mBones[j];
            std::string name = boneData->mName.C_Str();
            if (bones.count(name)) continue;

            Bone bone;
            bone.id = bones.size();
            bone.inverseBindMatrix = assimpToGlmMatrix(boneData->mOffsetMatrix);
            bones[name] = bone;
        }
    }

    // Recursively copy node data
    Node node;
    node.meshCount = data->mNumMeshes;
    node.name = data->mName.C_Str();
    node.transform = assimpToGlmMatrix(data->mTransformation);
    for (unsigned int i = 0; i < data->mNumChildren; i++) {
        Node n = readNodeData(scene, data->mChildren[i]);
        node.children.push_back(n);
    }
    return node;
}

Animation* Animator::run(double seconds)
{
    if (animations.size() == 0) return nullptr;
    Animation* a = &animations[currentAnimation];
    double time = fmod(seconds * a->ticksPerSecond, a->duration);

    if (!a->meshTransforms.empty())
        a->meshTransforms.clear();

    a->computeBoneTransform(bones, rootNode, time, glm::mat4(1.0));
    return a;
}
