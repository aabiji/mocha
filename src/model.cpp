#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glad.h>

#include "model.h"

#define toVec3(v) glm::vec3(v.x, v.y, v.z)
#define toVec2(v) glm::vec2(v.x, v.y)

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
}

void Model::load(const char* path)
{
    Assimp::Importer importer;

    int flags = aiProcessPreset_TargetRealtime_Fast | aiProcess_FlipUVs;
    const aiScene* scene = importer.ReadFile(path, flags);
    if (scene == nullptr)
        throw std::string(importer.GetErrorString());

    if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        throw std::string("Invalid model file");

    processNode(scene, scene->mRootNode);
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

int loadTexture(const aiTexture* texture)
{
    unsigned int id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width = texture->mWidth;
    int height = texture->mHeight;
    int internalFormat = GL_RGBA;
    int channels = 4;

    unsigned char* data = (unsigned char*)texture->pcData;
    bool compressed = height == 0;

    if (compressed) {
        data = stbi_load_from_memory(data, sizeof(aiTexel) * width, &width, &height, &channels, 0);
        internalFormat = channels == 1 ? GL_RED : channels == 4 ? GL_RGBA : GL_RGB;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, internalFormat, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (compressed)
        free(data);
    return id;
}

std::vector<Texture> Model::getTextures(const aiScene* scene, const aiMaterial* material) {
    // Map the texture types to their corresponding names in the fragment shader
    std::unordered_map<aiTextureType, const char*> types = {
        { aiTextureType_DIFFUSE, "diffuse" },
        { aiTextureType_SPECULAR, "specular" },
        { aiTextureType_AMBIENT, "ambient" },
        { aiTextureType_NORMALS, "normals" },
    };

    std::vector<Texture> textures;
    for (auto& [type, name]: types) {
        if (material->GetTextureCount(type) == 0)
            continue;

        aiString aiName;
        material->GetTexture(type, 0, &aiName);
        std::string assimpName = std::string(aiName.C_Str());

        if (textureCache.count(assimpName)) {
            textures.push_back(textureCache[assimpName]);
        } else {
            Texture t;
            t.sampler = name;
            t.id = loadTexture(scene->GetEmbeddedTexture(assimpName.c_str()));
            textureCache[assimpName] = t;
            textures.push_back(t);
        }
    }

    return textures;
}

void Model::processMesh(const aiScene* scene, const aiMesh* data)
{
    Mesh mesh;
    mesh.textures = getTextures(scene, scene->mMaterials[data->mMaterialIndex]);

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

    mesh.init();
    meshes.push_back(mesh);
}

void Model::draw(Shader& shader)
{
    for (Mesh& mesh : meshes) {
        glBindVertexArray(mesh.vao);

        // Initialize the texture samplers
        for (size_t i = 0; i < mesh.textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            shader.setInt(mesh.textures[i].sampler.c_str(), i);
            glBindTexture(GL_TEXTURE_2D, mesh.textures[i].id);
        }

        glDrawElements(GL_TRIANGLES, mesh.indexes.size(), GL_UNSIGNED_INT, 0);
    }
}
