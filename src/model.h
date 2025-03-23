#pragma once

#include <string>
#include <vector>

#include <assimp/scene.h>
#include <glm/glm.hpp>

#include "shader.h"
#include "textures.h"

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 coord;
};

struct Mesh
{
    // Initialize/clenaup the OpenGL objects
    void init();
    void cleanup();
    bool initialized;

    // These will be set when the vertices are loaded
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
    void update(glm::vec3 v);
};

class Model
{
public:
    Model(std::string path, std::string textureBasePath);
    void draw(Shader& shader);
    void cleanup();

    void setPosition(glm::vec3 v);
    void setSize(glm::vec3 size, bool preserveAspectRatio);
private:
    void processNode(const aiScene* scene, const aiNode* node);
    void processMesh(const aiScene* scene, const aiMesh* meshData);

    glm::vec3 scale;
    glm::vec3 position;
    BoundingBox globalBox;
    std::vector<Mesh> meshes;
    TextureLoader textureLoader;
};