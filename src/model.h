#pragma once

#include <assimp/mesh.h>

#include "animator.h"
#include "box.h"
#include "shader.h"
#include "textures.h"

struct Mesh
{
    void init();
    void cleanup();
    void draw(Shader& shader);

    // Vertex array object, vertex buffer object, element buffer object
    unsigned int vao, vbo, ebo;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indexes;

    bool initialized;
    TextureMap textures;
};

class Model
{
public:
    Model(std::string id, std::string path, std::string textureBasePath);
    void draw(Shader& shader, double timeInSeconds);
    void cleanup();

    void setPosition(glm::vec3 v);
    void setSize(glm::vec3 size, bool preserveAspectRatio);

    void toggleAnimation();
    bool animationPlaying();

    int getCurrentAnimation();
    void setCurrentAnimation(int index);
    std::vector<std::string> animationNames();

    bool isCalled(std::string s) { return name == s; }
private:
    void processNode(const aiScene* scene, const aiNode* node);
    void processMesh(const aiScene* scene, aiMesh* meshData);

    void getBoneWeights(aiMesh* data, Mesh& mesh);
    void addBoneToVertex(Vertex& v, int boneId, float weight);

    std::string name;

    glm::vec3 scale;
    glm::vec3 position;
    BoundingBox box;

    Animator animator;
    std::vector<Mesh> meshes;
    TextureLoader textureLoader;
};
