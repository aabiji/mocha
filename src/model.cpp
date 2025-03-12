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

Texture::Texture(aiTexture* info)
{
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, info->mWidth, info->mHeight, 0, GL_RGBA, GL_UNSIGNED_INT, info->pcData);
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
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, coord));
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size() * sizeof(unsigned int), indexes.data(), GL_STATIC_DRAW);
}

void Model::load(const char* path)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path, aiProcessPreset_TargetRealtime_Fast);
    if (scene == nullptr)
        throw std::string(importer.GetErrorString());

    if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        throw std::string("Invalid model file");

    for (unsigned int i = 0; i < scene->mNumTextures; i++) {
        textures.push_back(Texture(scene->mTextures[i]));
    }

    processNode(scene, scene->mRootNode);
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
        glDrawElements(GL_TRIANGLES, (unsigned int)mesh.indexes.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}