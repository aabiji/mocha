#pragma once

#include <string>
#include <vector>

#include <assimp/scene.h>
#include <glm/glm.hpp>

#include "assimp/mesh.h"
#include "shader.h"
#include "textures.h"

struct Bone
{
    int id;
    glm::mat4 inverseBindMatrix;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 coord;
    int boneIds[4];
    float boneWeights[4];
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
    glm::mat4 transform;
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
    void processNode(const aiScene* scene, const aiNode* node, glm::mat4 parentTransformation);
    void processMesh(const aiScene* scene, const aiMesh* meshData, glm::mat4 transform);
    void updateBoundingBox(glm::vec3 v);

    void getBoneWeights(Mesh& mesh, aiBone** bones, int numBones);
    void addBoneToVertex(Vertex& v, int boneId, float weight);

    glm::vec3 scale;
    glm::vec3 position;

    glm::vec3 boundingBoxMin;
    glm::vec3 boundingBoxMax;

    TextureLoader textureLoader;

    int boneCount;
    std::unordered_map<std::string, Bone> boneMap;

    std::vector<Mesh> meshes;
};
