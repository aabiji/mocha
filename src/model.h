#pragma once

#include <string>
#include <vector>

#include <assimp/anim.h>
#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>

#include "shader.h"
#include "textures.h"

struct Bone
{
    int id;
    // Converts a vertex from local space to bone space
    glm::mat4 inverseBindMatrix;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 coord;
    glm::ivec4 boneIds;
    glm::vec4 boneWeights;
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
    void draw(Shader& shader, double timeInSeconds);
    void cleanup();

    void setPosition(glm::vec3 v);
    void setSize(glm::vec3 size, bool preserveAspectRatio);
private:
    void processNode(const aiScene* scene, const aiNode* node, glm::mat4 parentTransform);
    void processMesh(const aiScene* scene, const aiMesh* meshData, glm::mat4 transform);
    void updateBoundingBox(glm::vec3 v);

    void getBoneWeights(Mesh& mesh, aiBone** bones, int numBones);
    void addBoneToVertex(Vertex& v, int boneId, float weight);

    void calculateBoneTransform(aiNode* node, double time, glm::mat4 parentTransform, int& meshIndex);
    aiNodeAnim* findNodeAnimation(aiString name);

    glm::vec3 scale;
    glm::vec3 position;

    glm::vec3 boundingBoxMin;
    glm::vec3 boundingBoxMax;

    TextureLoader textureLoader;

    int boneCount;
    std::unordered_map<std::string, Bone> boneMap;
    std::vector<glm::mat4> boneTransforms;
    glm::mat4 globalInverseTransform;

    aiNode* rootNode;
    aiAnimation** animations;
    int currentAnimation;

    std::vector<Mesh> meshes;
};
