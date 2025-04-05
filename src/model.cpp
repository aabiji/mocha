#include <format>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <glad.h>

#include "convert.h"
#include "model.h"

/*
Changes:
- TODO: when finally works, add an animation selector gui
- TODO: the lighting needs help
*/

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


Model::Model(std::string path, std::string textureBasePath)
    : textureLoader(textureBasePath)
{
    scale = glm::vec3(1.0);
    position = glm::vec3(0.0);

    boundingBoxMin = glm::vec3(std::numeric_limits<float>::max());
    boundingBoxMax = glm::vec3(std::numeric_limits<float>::min());

    Assimp::Importer importer;
    unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                aiProcess_CalcTangentSpace | aiProcess_FlipUVs | aiProcess_GenBoundingBoxes;
    const aiScene* scene = importer.ReadFile(path, flags);
    if (scene == nullptr)
        throw std::string(importer.GetErrorString());

    if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        throw std::string("Invalid model file");

    animator.load(scene);
    processNode(scene, scene->mRootNode);
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
            return;
        }
    }
}

void Model::getBoneWeights(aiMesh* data, Mesh& mesh)
{
    for (unsigned int i = 0; i < data->mNumBones; i++) {
        aiBone* bone = data->mBones[i];
        std::string name = bone->mName.C_Str();
        int boneId = animator.getBoneId(name);

        for (unsigned int j = 0; j < bone->mNumWeights; j++) {
            float weight = bone->mWeights[j].mWeight;
            int vertexIndex = bone->mWeights[j].mVertexId;
            addBoneToVertex(mesh.vertices[vertexIndex], boneId, weight);
        }
    }
}

void Model::processMesh(const aiScene* scene, aiMesh* data)
{
    Mesh mesh;
    mesh.initialized = false;
    mesh.textures = textureLoader.get(scene, scene->mMaterials[data->mMaterialIndex]);

    for (unsigned int i = 0; i < data->mNumFaces; i++) {
        for (unsigned int j = 0; j < data->mFaces[i].mNumIndices; j++) {
            mesh.indexes.push_back(data->mFaces[i].mIndices[j]);
        }
    }

    for (unsigned int i = 0; i < data->mNumVertices; i++) {
        Vertex v;
        v.boneIds = glm::ivec4(-1.0);
        v.boneWeights = glm::vec4(0.0);
        v.position = toVec3(data->mVertices[i]);
        if (data->HasNormals())
            v.normal = toVec3(data->mNormals[i]);

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
    }

    updateBoundingBox(toVec3(data->mAABB.mMin));
    updateBoundingBox(toVec3(data->mAABB.mMax));
    getBoneWeights(data, mesh);

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

void Model::draw(Shader& shader, double timeInSeconds)
{
    glm::mat4 transform = glm::mat4(1.0);
    transform = glm::translate(transform, position);
    transform = glm::scale(transform, scale);

    Animation* animation = animator.run(timeInSeconds);

    for (size_t i = 0; i < meshes.size(); i++) {
        Mesh& mesh = meshes[i];
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
        if (animation != nullptr) {
            for (size_t j = 0; j < animation->boneTransforms.size(); j++) {
                shader.set<glm::mat4>(std::format("boneTransforms[{}]", j).c_str(), animation->boneTransforms[j]);
            }
            shader.set<glm::mat4>("meshTransform", animation->meshTransforms[i]);
        } else {
            shader.set<glm::mat4>("meshTransform", glm::mat4(1.0));
        }

        shader.set<glm::mat4>("model", transform);
        shader.set<int>("hasNormalMap", mesh.textures.count("normal") > 0);
        glDrawElements(GL_TRIANGLES, mesh.indexes.size(), GL_UNSIGNED_INT, 0);
    }

    textureLoader.cleanup(); // Don't need the texture pixels anymore
}
