#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <assimp/scene.h>
#include <glm/glm.hpp>

#include "shader.h"

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 coord;
};

class Texture
{
public:
    // Load the pixel data
    Texture(const aiTexture* data);
    Texture(unsigned char color);
    Texture() {}

    // Initialize the OpenGL texture object
    void init();

    unsigned int id;
private:
    int width, height, format;
    std::shared_ptr<unsigned char[]> pixels;
};

using TextureMap = std::unordered_map<std::string, Texture>;

struct Mesh
{
    // Initialize/clenaup the OpenGL objects
    void init();
    void cleanup();

    // These will be set when the vertices are loaded
    bool initialized;
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
    Model(std::string path);

    void draw(Shader& shader);
    void cleanup();

    void setPosition(glm::vec3 v);
    void setSize(glm::vec3 size, bool preserveAspectRatio);
private:
    void processNode(const aiScene* scene, const aiNode* node);
    void processMesh(const aiScene* scene, const aiMesh* meshData);
    TextureMap getTextures(const aiScene* scene, const aiMaterial* material);

    std::vector<Mesh> meshes;

    TextureMap textureCache;
    TextureMap defaultTextures;

    glm::vec3 scale;
    glm::vec3 position;
    BoundingBox globalBox;
};