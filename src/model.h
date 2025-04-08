#pragma once

#include <assimp/mesh.h>

#include "animator.h"
#include "box.h"
#include "shader.h"
#include "textures.h"

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

class Model
{
public:
    Model(std::string path, std::string textureBasePath);
    void draw(Shader& shader, double timeInSeconds);
    void cleanup();

    void setPosition(glm::vec3 v);
    void setSize(glm::vec3 size, bool preserveAspectRatio);

    void toggleAnimation();
    bool animationPlaying();

    int getCurrentAnimation();
    void setCurrentAnimation(int index);
    std::vector<std::string> animationNames();

    std::string name;
    BoundingBox box;
private:
    void processNode(const aiScene* scene, const aiNode* node);
    void processMesh(const aiScene* scene, aiMesh* meshData);

    void getBoneWeights(aiMesh* data, Mesh& mesh);
    void addBoneToVertex(Vertex& v, int boneId, float weight);

    glm::vec3 scale;
    glm::vec3 position;

    Animator animator;
    std::vector<Mesh> meshes;
    TextureLoader textureLoader;
};
