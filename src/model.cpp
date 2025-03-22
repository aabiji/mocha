#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glad.h>
#include <glm/gtc/type_ptr.hpp>

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

// Create an empty 1x1 texture
unsigned int emptyTexture(unsigned char color)
{
    unsigned int id;
    unsigned char data[] = { color, color, color };
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    return id;
}

// FIXME:
// So now our app is multithreaded and we're having some trouble
// using assimp over multiple threads. What can we do?
// Genuinely don't understand how assimp can't read the node tree properly.
#include <iostream>
Model::Model(std::string path)
{
    scale = glm::vec3(1.0);
    position = glm::vec3(1.0);

    defaultTextures["ambient"] = emptyTexture(35);
    defaultTextures["diffuse"] = emptyTexture(255);
    defaultTextures["specular"] = emptyTexture(128);
    defaultTextures["emission"] = emptyTexture(0);

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

void Model::processNode(const aiScene* scene, const aiNode* node)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        processMesh(scene, scene->mMeshes[node->mMeshes[i]]);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(scene, node->mChildren[i]);
    }
}

// Setup a texture object and return its id
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

// Get the textures that are defined in the material
TextureMap Model::getTextures(const aiScene* scene, const aiMaterial* material) {
    // Map the texture types to their corresponding names in the fragment shader
    std::unordered_map<aiTextureType, const char*> types = {
        { aiTextureType_DIFFUSE,  "diffuse" },
        { aiTextureType_SPECULAR, "specular" },
        { aiTextureType_AMBIENT, "ambient" },
        { aiTextureType_NORMALS, "normal" },
        { aiTextureType_EMISSIVE, "emission" }
    };

    TextureMap textures;
    for (auto& [type, samplerName]: types) {
        if (material->GetTextureCount(type) == 0)
            continue;

        aiString aiName;
        material->GetTexture(type, 0, &aiName);
        std::string assimpName = std::string(aiName.C_Str());

        if (textureCache.count(assimpName)) {
            textures[samplerName] = textureCache[assimpName];
        } else {
            const aiTexture* texture = scene->GetEmbeddedTexture(assimpName.c_str());
            if (texture == nullptr)
                continue;
            unsigned int id = loadTexture(texture);
            textureCache[assimpName] = id;
            textures[samplerName] = id;
        }
    }

    // Insert the default texture in place of a missing texture
    for (auto& [name, t] : defaultTextures) {
        if (textures.count(name) == 0)
            textures[name] = t;
    }

    return textures;
}

void Model::processMesh(const aiScene* scene, const aiMesh* data)
{
    Mesh mesh;
    mesh.textures = getTextures(scene, scene->mMaterials[data->mMaterialIndex]);

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

    mesh.init();
    meshes.push_back(mesh);
}

void Model::setPosition(glm::vec3 v) { position = v; }

void Model::setSize(glm::vec3 size, bool preserveAspectRatio)
{
    float xScale = size.x / (globalBox.max.x - globalBox.min.x);
    float yScale = size.y / (globalBox.max.y - globalBox.min.y);
    float zScale = size.z / (globalBox.max.z - globalBox.min.z);

    // TODO: we may or may not be scaling the normals properly???

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
        glBindVertexArray(mesh.vao);

        // Bind the texture samplers
        int index = 0;
        for (auto& [sampler, id] : mesh.textures) {
            glActiveTexture(GL_TEXTURE0 + index);
            shader.setInt(("material." + sampler).c_str(), index);
            glBindTexture(GL_TEXTURE_2D, id);
            index++;
        }

        shader.setInt("hasNormalMap", mesh.textures.count("normal") > 0);
        shader.setMatrix("model", transform);
        glDrawElements(GL_TRIANGLES, mesh.indexes.size(), GL_UNSIGNED_INT, 0);
    }
}

void Model::cleanup()
{
    for (Mesh& mesh : meshes) {
        glDeleteVertexArrays(1, &mesh.vao);
        glDeleteBuffers(1, &mesh.vbo);
        glDeleteBuffers(1, &mesh.ebo);
        for (auto& t : mesh.textures) {
            glDeleteTextures(1, &t.second);
        }
    }
}
