#pragma once

#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <vector>

#include "shader.h"

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 coord;
};

struct Mesh
{
    unsigned int vao, vbo, ebo;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indexes;
    void init();
};

class Model
{
public:
    void load(Shader* shader, const char* path);
    void draw();
private:
    void processNode(const aiScene* scene, const aiNode* node);
    void processMesh(const aiMesh* meshData);
    void loadTextures(const aiScene* scene);

    std::vector<Mesh> meshes;
    std::vector<unsigned int> textureIds;
    Shader* shaderPtr;
};