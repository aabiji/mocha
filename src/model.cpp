#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "model.h"

#define toVec3(v) glm::vec3(v.x, v.y, v.z)
#define toVec2(v) glm::vec2(v.x, v.y)

void BoundingBox::update(glm::vec3 v)
{
    bool smaller = v.x < min.x && v.y < min.y && v.z < min.z;
    bool bigger = v.x > max.x && v.y > max.y && v.z > max.z;
    min = smaller ? v : min;
    max = bigger ? v : max;
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

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size() * sizeof(unsigned int), indexes.data(), GL_STATIC_DRAW);

    for (auto& [sampler, texture] : textures) {
        texture.init();
    }

    vertices.clear(); // Won't need this anymore
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

    Assimp::Importer importer;
    int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                aiProcess_CalcTangentSpace | aiProcess_FlipUVs |
                aiProcess_GenBoundingBoxes;
    const aiScene* scene = importer.ReadFile(path, flags);
    if (scene == nullptr)
        throw std::string(importer.GetErrorString());

    if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        throw std::string("Invalid model file");

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

void Model::processMesh(const aiScene* scene, const aiMesh* data)
{
    Mesh mesh;
    mesh.textures = textureLoader.get(scene, scene->mMaterials[data->mMaterialIndex]);
    mesh.initialized = false;

    globalBox.update(toVec3(data->mAABB.mMin));
    globalBox.update(toVec3(data->mAABB.mMax));

    for (unsigned int i = 0; i < data->mNumFaces; i++) {
        for (unsigned int j = 0; j < data->mFaces[i].mNumIndices; j++) {
            mesh.indexes.push_back(data->mFaces[i].mIndices[j]);
        }
    }

    for (unsigned int i = 0; i < data->mNumVertices; i++) {
        Vertex v;
        v.position = toVec3(data->mVertices[i]);
        v.normal = data->HasNormals()
            ? toVec3(data->mNormals[i])
            : glm::vec3(0, 0, 0);
        v.coord = data->mTextureCoords[0]
            ? toVec2(data->mTextureCoords[0][i])
            : glm::vec2(0, 0);
        v.tangent = data->HasTangentsAndBitangents()
            ? toVec3(data->mTangents[i])
            : glm::vec3(0, 0, 0);
        mesh.vertices.push_back(v);
    }

    meshes.push_back(std::move(mesh));
}

void Model::setPosition(glm::vec3 v) { position = v; }

void Model::setSize(glm::vec3 size, bool preserveAspectRatio)
{
    float xScale = size.x / (globalBox.max.x - globalBox.min.x);
    float yScale = size.y / (globalBox.max.y - globalBox.min.y);
    float zScale = size.z / (globalBox.max.z - globalBox.min.z);

    if (preserveAspectRatio) {
        float uniformScale = std::max({ xScale, yScale, zScale });
        scale = glm::vec3(uniformScale);
    } else {
        scale = glm::vec3(xScale, yScale, zScale);
    }

    globalBox.min *= scale;
    globalBox.max *= scale;
}

void Model::draw(Shader& shader)
{
    // translate, rotate, scale (so that it's actually done in reverse)
    glm::mat4 transform = glm::mat4(1.0);
    transform = glm::translate(transform, position);
    transform = glm::scale(transform, scale);

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
            shader.setInt(("material." + sampler).c_str(), index);
            glBindTexture(GL_TEXTURE_2D, texture.id);
            index++;
        }

        shader.setMatrix("model", transform);
        shader.setInt("hasNormalMap", mesh.textures.count("normal") > 0);
        glDrawElements(GL_TRIANGLES, mesh.indexes.size(), GL_UNSIGNED_INT, 0);
    }

    textureLoader.cleanup(); // Don't need the texture pixels anymore
}