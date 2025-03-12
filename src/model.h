#pragma once

#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <vector>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
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
    void load(const char* path);
    void draw();
private:
    void processNode(const aiScene* scene, const aiNode* node);
    void processMesh(const aiMesh* meshData);

    std::vector<Mesh> meshes;
};