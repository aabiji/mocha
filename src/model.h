#pragma once

#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

#include "shader.h"

struct Texture
{
    std::string sampler;
    unsigned int id;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 coord;
};

struct Mesh
{
    void init();
    unsigned int vao, vbo, ebo;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indexes;
    std::vector<Texture> textures;
};

class Model
{
public:
    void load(const char* path);
    void draw(Shader& shader);
private:
    void processNode(const aiScene* scene, const aiNode* node);
    void processMesh(const aiScene* scene, const aiMesh* meshData);
    std::vector<Texture> getTextures(const aiScene* scene, const aiMaterial* material);

    std::vector<Mesh> meshes;
    std::unordered_map<std::string, Texture> textureCache;
};
