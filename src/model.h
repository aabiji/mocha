#pragma once

#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

#include "shader.h"

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 coord;
};

using TextureMap = std::unordered_map<std::string, unsigned int>;

struct Mesh
{
    void init();
    unsigned int vao, vbo, ebo;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indexes;
    TextureMap textures;
};

class Model
{
public:
    void load(const char* path);
    void draw(Shader& shader);
    void cleanup();
private:
    void processNode(const aiScene* scene, const aiNode* node);
    void processMesh(const aiScene* scene, const aiMesh* meshData);
    TextureMap getTextures(const aiScene* scene, const aiMaterial* material);

    std::vector<Mesh> meshes;
    TextureMap textureCache;
    TextureMap defaultTextures;
};
