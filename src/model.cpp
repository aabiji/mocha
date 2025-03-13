#include <format>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glad.h>

#include "model.h"

#define toVec3(v) glm::vec3(v.x, v.y, v.z)
#define toVec2(v) glm::vec2(v.x, v.y)

// TODO:
// Draw meshes with their respective textures
// Load materials and add some basic directional lighting
// Start researching skeletal animation

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
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, coord));
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size() * sizeof(unsigned int), indexes.data(), GL_STATIC_DRAW);
}

void Model::load(Shader* shader, const char* path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path, aiProcessPreset_TargetRealtime_Fast);
    if (scene == nullptr)
        throw std::string(importer.GetErrorString());

    if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        throw std::string("Invalid model file");

    shaderPtr = shader;
    loadTextures(scene);
    processNode(scene, scene->mRootNode);
}

void Model::loadTextures(const aiScene* scene)
{
    textureIds.clear();
    textureIds.reserve(scene->mNumTextures);
    glGenTextures(scene->mNumTextures, textureIds.data());

    for (unsigned int i = 0; i < scene->mNumTextures; i++) {
        unsigned char* data = (unsigned char*)scene->mTextures[i]->pcData;
        int width = scene->mTextures[i]->mWidth;
        int height = scene->mTextures[i]->mHeight;
        int channels = 4, internalFormat = GL_RGBA;

        bool wasCompressed = false;
        if (height == 0) { // Decompress the embedded texture
            data = stbi_load_from_memory(data, sizeof(aiTexel) * width, &width, &height, &channels, 0);
            internalFormat = channels == 1 ? GL_RED : channels == 4 ? GL_RGBA : GL_RGB;
            wasCompressed = true;
        }

        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textureIds[i]);
        shaderPtr->setInt(std::format("textures[{}]", i).c_str(), i);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        if (wasCompressed)
            free(data);
    }
}

void Model::processNode(const aiScene* scene, const aiNode* node)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        processMesh(scene->mMeshes[node->mMeshes[i]]);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(scene, node->mChildren[i]);
    }
}

void Model::processMesh(const aiMesh* meshData)
{
    Mesh mesh;

    for (unsigned int i = 0; i < meshData->mNumFaces; i++) {
        for (unsigned int j = 0; j < meshData->mFaces[i].mNumIndices; j++) {
            mesh.indexes.push_back(meshData->mFaces[i].mIndices[j]);
        }
    }

    for (unsigned int i = 0; i < meshData->mNumVertices; i++) {
        Vertex v;
        v.position = toVec3(meshData->mVertices[i]);
        v.normal = toVec3(meshData->mNormals[i]);
        v.coord = toVec2(meshData->mTextureCoords[0][i]);
        mesh.vertices.push_back(v);
    }

    mesh.init();
    meshes.push_back(mesh);
}

void Model::draw()
{
    for (Mesh& mesh : meshes) {
        glBindVertexArray(mesh.vao);
        for (size_t i = 0; i < textureIds.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, textureIds[i]);
        }
        shaderPtr->setInt("currentTexture", 0);
        glDrawElements(GL_TRIANGLES, (unsigned int)mesh.indexes.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}