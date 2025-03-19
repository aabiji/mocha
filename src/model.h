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

struct BoundingBox
{
    glm::vec3 min;
    glm::vec3 max;

    BoundingBox() : min(1.0), max(0.0) {}

    void update(glm::vec3 v)
    {
        bool smaller = v.x < min.x && v.y < min.y && v.z < min.z;
        bool bigger = v.x > max.x && v.y > max.y && v.z > max.z;
        min = smaller ? v : min;
        max = bigger ? v : max;
    }
};

class Model
{
public:
    void load(const char* path, glm::mat4 matrix);
    void draw(Shader& shader);
    void cleanup();
private:
    void processNode(const aiScene* scene, const aiNode* node);
    void processMesh(const aiScene* scene, const aiMesh* meshData);
    TextureMap getTextures(const aiScene* scene, const aiMaterial* material);

    std::vector<Mesh> meshes;
    TextureMap textureCache;
    TextureMap defaultTextures;
    BoundingBox globalBox;
    glm::mat4 transform;
};
