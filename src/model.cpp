#include <format>

#include <assimp/cimport.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/quaternion.h>

#include <glad.h>

#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "model.h"

#define toVec3(v) glm::vec3(v.x, v.y, v.z)
#define toVec2(v) glm::vec2(v.x, v.y)
#define toQuat(q) glm::quat(q.w, q.x, q.y, q.z)

glm::mat4 assimpToGlmMatrix(const aiMatrix4x4& matrix)
{
    glm::mat4 m;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m[j][i] = matrix[i][j];
        }
    }
    return m;
}

void Mesh::init()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, coord));
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIds));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));
    glEnableVertexAttribArray(5);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size() * sizeof(unsigned int), indexes.data(), GL_STATIC_DRAW);

    for (auto& [sampler, texture] : textures) {
        texture.init();
    }

    vertices.clear(); // Won't need this anymore
    glBindVertexArray(0);
}

void Mesh::cleanup()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    for (auto &t : textures) {
        glDeleteTextures(1, &t.second.id);
    }
}

thread_local Assimp::Importer importer;

Model::Model(std::string path, std::string textureBasePath)
    : textureLoader(textureBasePath)
{
    scale = glm::vec3(1.0);
    position = glm::vec3(0.0);

    boundingBoxMin = glm::vec3(std::numeric_limits<float>::max());
    boundingBoxMax = glm::vec3(std::numeric_limits<float>::min());

    unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                aiProcess_CalcTangentSpace | aiProcess_FlipUVs;
    const aiScene* scene = importer.ReadFile(path, flags);
    if (scene == nullptr)
        throw std::string(importer.GetErrorString());

    if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        throw std::string("Invalid model file");

    boneCount = 0;
    processNode(scene, scene->mRootNode);

    currentAnimation = 0;
    rootNode = scene->mRootNode;
    animations = scene->mAnimations;
    globalInverseTransform = assimpToGlmMatrix(rootNode->mTransformation.Inverse());
    boneTransforms.resize(boneMap.size());
}

void Model::cleanup()
{
    for (Mesh& mesh : meshes) {
        mesh.cleanup();
    }
}

void Model::processNode(const aiScene* scene, const aiNode* node)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        processMesh(scene, scene->mMeshes[node->mMeshes[i]]);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(scene, node->mChildren[i]);
    }
}

void Model::addBoneToVertex(Vertex& v, int boneId, float weight)
{
    // Each bone has at most 4 bones that can influence it
    for (int i = 0; i < 4; i++) {
        if (v.boneWeights[i] == 0.0) {
            v.boneIds[i] = boneId;
            v.boneWeights[i] = weight;
            break;
        }
    }
}

void Model::getBoneWeights(Mesh& mesh, aiBone** bones, int numBones)
{
    for (int i = 0; i < numBones; i++) {
        int boneId = -1;
        std::string name = bones[i]->mName.C_Str();
        if (boneMap.count(name))
            boneId = boneMap[name].id;
        else {
            Bone bone;
            bone.id = boneCount++;
            bone.inverseBindMatrix = assimpToGlmMatrix(bones[i]->mOffsetMatrix);
            boneMap[name] = bone;
            boneId = bone.id;
        }
        assert(boneId != -1);

        for (unsigned int j = 0; j < bones[i]->mNumWeights; j++) {
            float weight = bones[i]->mWeights[j].mWeight;
            int vertexIndex = bones[i]->mWeights[j].mVertexId;
            addBoneToVertex(mesh.vertices[vertexIndex], boneId, weight);
        }
    }
}

void Model::processMesh(const aiScene* scene, const aiMesh* data)
{
    Mesh mesh;
    mesh.textures = textureLoader.get(scene, scene->mMaterials[data->mMaterialIndex]);
    mesh.initialized = false;

    for (unsigned int i = 0; i < data->mNumFaces; i++) {
        for (unsigned int j = 0; j < data->mFaces[i].mNumIndices; j++) {
            mesh.indexes.push_back(data->mFaces[i].mIndices[j]);
        }
    }

    for (unsigned int i = 0; i < data->mNumVertices; i++) {
        Vertex v;
        v.boneIds = glm::ivec4(0.0);
        v.boneWeights = glm::vec4(0.0);
        v.position = toVec3(data->mVertices[i]);
        if (data->HasNormals())
            v.normal = toVec3(data->mNormals[i]);

        updateBoundingBox(v.position); // TODO: account for node transformation

        // The tangent is derived from the texture coordinate, so if there's
        // no texture coordinate, there can't be a tangent. If the tangent is
        // made to default to (0, 0, 0), the TBN matrix will be singular, which
        // would fuck up the lighting calculations. So, the tangent is better
        // off as randomly initialized.
        if (data->mTextureCoords[0]) {
            v.coord = toVec2(data->mTextureCoords[0][i]);
            v.tangent = toVec3(data->mTangents[i]);
        } else {
            v.coord = glm::vec2(0, 0);
            v.tangent = glm::vec3(rand(), rand(), rand());
        }

        mesh.vertices.push_back(v);
        updateBoundingBox(toVec3(data->mVertices[i]));
    }

    getBoneWeights(mesh, data->mBones, data->mNumBones);
    meshes.push_back(std::move(mesh));
}

void Model::updateBoundingBox(glm::vec3 v)
{
    for (int i = 0; i < 3; i++) {
        boundingBoxMin[i] = std::min(boundingBoxMin[i], v[i]);
        boundingBoxMax[i] = std::max(boundingBoxMax[i], v[i]);
    }
}

void Model::setPosition(glm::vec3 v) { position = v; }

void Model::setSize(glm::vec3 size, bool preserveAspectRatio)
{
    float xScale = size.x / (boundingBoxMax.x - boundingBoxMin.x);
    float yScale = size.y / (boundingBoxMax.y - boundingBoxMin.y);
    float zScale = size.z / (boundingBoxMax.z - boundingBoxMin.z);

    if (preserveAspectRatio) {
        float uniformScale = std::max({ xScale, yScale, zScale });
        scale = glm::vec3(uniformScale);
    } else {
        scale = glm::vec3(xScale, yScale, zScale);
    }

    boundingBoxMin *= scale;
    boundingBoxMax *= scale;
}

// Get the animation associated to the node
aiNodeAnim* Model::findNodeAnimation(aiString name)
{
    aiAnimation* a = animations[currentAnimation];
    for (unsigned int i = 0; i < a->mNumChannels; i++) {
        if (name == a->mChannels[i]->mNodeName)
            return a->mChannels[i];
    }
    return nullptr;
}

glm::quat interpolateRotationKeyframes(aiNodeAnim* animation, double time)
{
    // In order to interpolate we need at least 2 values
    if (animation->mNumRotationKeys == 1)
        return toQuat(animation->mRotationKeys[0].mValue);

    // Get the current rotation keyframe
    unsigned int index = 0;
    for (unsigned int i = 0; i < animation->mNumRotationKeys; i++) {
        if (animation->mRotationKeys[i].mTime > time) {
            index = i - 1;
            break;
        }
    }
    assert(index + 1 < animation->mNumRotationKeys);

    // Calculate the percentage of the animation that has ran
    aiQuatKey a = animation->mRotationKeys[index];
    aiQuatKey b = animation->mRotationKeys[index + 1];
    double percentage = (time - a.mTime) / (b.mTime - a.mTime);
    assert(percentage >= 0.0 && percentage <= 1.0);

    aiQuaternion result;
    aiQuaternionInterpolate(&result, &a.mValue, &b.mValue, percentage);
    return toQuat(result.Normalize());
}

glm::vec3 interpolateVectorKeyframes(aiVectorKey* keyframes, int numKeyframes, double time)
{
    // In order to interpolate we need at least 2 values
    if (numKeyframes == 1)
        return toVec3(keyframes[0].mValue);

    // Get the current rotation keyframe
    int index = 0;
    for (int i = 0; i < numKeyframes; i++) {
        if (keyframes[i].mTime > time) {
            index = i - 1;
            break;
        }
    }
    assert(index + 1 < numKeyframes);

    // Calculate the percentage of the animation that has ran
    aiVectorKey a = keyframes[index];
    aiVectorKey b = keyframes[index + 1];
    float percentage = (time - a.mTime) / (b.mTime - a.mTime);
    assert(percentage >= 0.0 && percentage <= 1.0);

    // Linearly interpolate the two positions
    glm::vec3 result = toVec3(a.mValue) * (1.0f - percentage) + toVec3(b.mValue) * percentage;
    return glm::normalize(result);
}

void Model::calculateBoneTransform(aiNode* node, double animationTime, glm::mat4 parentTransform)
{
    aiString name = node->mName;
    std::string nameStr = std::string(name.C_Str());

    glm::mat4 transform = assimpToGlmMatrix(node->mTransformation);
    aiNodeAnim* animation = findNodeAnimation(node->mName);

    // Get the node transform by interpolating the position, scaling and rotation keyframes
    if (animation) {
        glm::quat quaternion = interpolateRotationKeyframes(animation, animationTime);
        glm::mat4 rotation = glm::mat4(quaternion);

        glm::vec3 pos = interpolateVectorKeyframes(animation->mPositionKeys, animation->mNumPositionKeys, animationTime);
        glm::mat4 translation = glm::translate(glm::mat4(1.0), pos);

        glm::vec3 scale = interpolateVectorKeyframes(animation->mScalingKeys, animation->mNumScalingKeys, animationTime);
        glm::mat4 scaling = glm::scale(glm::mat4(1.0), scale);

        transform = translation * rotation * scaling;
    }

    glm::mat4 globalTransform = parentTransform * transform;
    if (boneMap.count(nameStr)) {
        Bone& bone = boneMap[nameStr];
        boneTransforms[bone.id] = globalInverseTransform * globalTransform * bone.inverseBindMatrix;
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        calculateBoneTransform(node->mChildren[i], animationTime, globalTransform);
    }
}

void Model::draw(Shader& shader, double timeInSeconds)
{
    // translate, rotate, scale (so that it's actually done in reverse)
    glm::mat4 transform = glm::mat4(1.0);
    transform = glm::translate(transform, position);
    transform = glm::scale(transform, scale);

    // Calculate the bone transforms for the current animation
    if (animations) {
        aiAnimation* animation = animations[currentAnimation];
        double tps = animation->mTicksPerSecond == 0 ? 25.0 : animation->mTicksPerSecond;
        double time = fmod(timeInSeconds * tps, animation->mDuration);
        calculateBoneTransform(rootNode, time, glm::mat4(1.0));
    }

    for (Mesh& mesh : meshes) {
        if (!mesh.initialized) {
            mesh.initialized = true;
            mesh.init();
        }
        glBindVertexArray(mesh.vao);

        // Bind the texture samplers
        int index = 0;
        for (auto& [sampler, texture] : mesh.textures) {
            glActiveTexture(GL_TEXTURE0 + index);
            shader.set<int>(("material." + sampler).c_str(), index);
            glBindTexture(GL_TEXTURE_2D, texture.id);
            index++;
        }

        // Set the bone transforms
        for (size_t i = 0; i < boneTransforms.size(); i++) {
            shader.set<glm::mat4>(std::format("boneTransforms[{}]", i).c_str(), boneTransforms[i]);
        }

        shader.set<glm::mat4>("model", transform);
        shader.set<int>("hasNormalMap", mesh.textures.count("normal") > 0);
        glDrawElements(GL_TRIANGLES, mesh.indexes.size(), GL_UNSIGNED_INT, 0);
    }

    textureLoader.cleanup(); // Don't need the texture pixels anymore
}
