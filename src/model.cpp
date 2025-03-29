#include <limits>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "model.h"

#define toVec3(v) glm::vec3(v.x, v.y, v.z)
#define toVec2(v) glm::vec2(v.x, v.y)

/*
Notes from here:
https://lisyarus.github.io/blog/posts/gltf-animation.html

affline transformation matrix:
[ a11, a21, a31, t1 ]
[ a21, a22, a32, t2 ]
[ a31, a32, a33, t3 ]
[   0,   0,   0,  0 ]
Where the inner 3x3 matrix is rotation, scaling, shearing and t is translation
If we just add the bottom row when doing the math, we actually get away
with having a 3x4 matrix

Each vertex can be connected to at most 4 bones
Each bone has a specific weight on the vertex. So
when passing data into the vertex shader, we'd need to specify
    `vec4 boneIds, boneWeights;`
We'd also need to specify a list of global bone
transforms that's indexed with the bone id

we should store the global bone transforms in a *shader storage buffer object*
not a uniform buffer object

globalBoneTransform(bone) =
    globalBoneTransform(parent) * localBoneTransform(bone)

when computing the global bone transforms, it's efficient
to compute parent's global bone transform (of course we
can use DFS to make sure that parents are evaluated before their children)

invertToParentCS(node) = inverseBindMatrix(parent) * inverseBindMatrix(node)^-1

the inverse bind matrix maps a vertex from world space
into the bone's local coordinate system the bone has a
local coordinate system because it makes the math more convenient
before that we'd need to transform the vertex into bind pose,
but most models are already in bind pose, so that step is skipped

so really, the final transformation looks like so:
vertexInModelSpace =
    globalBoneTransform * inverseBindMatrix * vertexInModelSpace

1. Convert it to the model bind pose (if not already in bind pose)
2. vertexInModelSpace =
    globalBoneTransform * inverseBindMatrix * vertexInModelSpace
3. Repeat for parent bone
*/

// TODO:
// - the cube is black because it has no textures -- so it must have a color
// - start parsing the data needed for animations
// - use a shader storage object instead of a uniform buffer object
// - perform animation in vertex shader???

void Mesh::init()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, coord));
    glEnableVertexAttribArray(3);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size() * sizeof(unsigned int), indexes.data(), GL_STATIC_DRAW);

    for (auto& [sampler, texture] : textures) {
        texture.init();
    }

    vertices.clear(); // Won't need this anymore
    glBindVertexArray(0);
}

void Mesh::cleanup()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    for (auto &t : textures) {
        glDeleteTextures(1, &t.second.id);
    }
}

Model::Model(std::string path, std::string textureBasePath)
    : textureLoader(textureBasePath)
{
    scale = glm::vec3(1.0);
    position = glm::vec3(0.0);

    boundingBoxMin = glm::vec3(std::numeric_limits<float>::max());
    boundingBoxMax = glm::vec3(std::numeric_limits<float>::min());

    Assimp::Importer importer;
    int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                aiProcess_CalcTangentSpace | aiProcess_FlipUVs;
    const aiScene* scene = importer.ReadFile(path, flags);
    if (scene == nullptr)
        throw std::string(importer.GetErrorString());

    if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        throw std::string("Invalid model file");

    processNode(scene, scene->mRootNode, glm::mat4(1.0));
}

void Model::cleanup()
{
    for (Mesh& mesh : meshes) {
        mesh.cleanup();
    }
}

glm::mat4 assimpToGlmMatrix(const aiMatrix4x4& matrix)
{
    glm::mat4 m;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m[j][i] = matrix[i][j];
        }
    }
    return m;
}

void Model::processNode(const aiScene* scene, const aiNode* node, glm::mat4 parentTransformation)
{
    glm::mat4 transformation = parentTransformation * assimpToGlmMatrix(node->mTransformation);

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        processMesh(scene, scene->mMeshes[node->mMeshes[i]], transformation);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(scene, node->mChildren[i], transformation);
    }
}

void Model::processMesh(const aiScene* scene, const aiMesh* data, glm::mat4 transform)
{
    Mesh mesh;
    mesh.textures = textureLoader.get(scene, scene->mMaterials[data->mMaterialIndex]);
    mesh.transform = transform;
    mesh.initialized = false;

    for (unsigned int i = 0; i < data->mNumFaces; i++) {
        for (unsigned int j = 0; j < data->mFaces[i].mNumIndices; j++) {
            mesh.indexes.push_back(data->mFaces[i].mIndices[j]);
        }
    }

    for (unsigned int i = 0; i < data->mNumVertices; i++) {
        glm::vec3 p = glm::vec4(toVec3(data->mVertices[i]), 1.0) * transform;
        updateBoundingBox(p);

        Vertex v;
        v.position = toVec3(data->mVertices[i]);
        v.normal = data->HasNormals()
            ? toVec3(data->mNormals[i])
            : glm::vec3(0, 0, 0);
        v.coord = data->mTextureCoords[0]
            ? toVec2(data->mTextureCoords[0][i])
            : glm::vec2(0, 0);
        v.tangent = data->HasTangentsAndBitangents()
            ? toVec3(data->mTangents[i])
            : glm::vec3(0, 0, 0);
        mesh.vertices.push_back(v);
        updateBoundingBox(toVec3(data->mVertices[i]));
    }

    meshes.push_back(std::move(mesh));
}

void Model::updateBoundingBox(glm::vec3 v)
{
    for (int i = 0; i < 3; i++) {
        boundingBoxMin[i] = std::min(boundingBoxMin[i], v[i]);
        boundingBoxMax[i] = std::max(boundingBoxMax[i], v[i]);
    }
}

void Model::setPosition(glm::vec3 v) { position = v; }

void Model::setSize(glm::vec3 size, bool preserveAspectRatio)
{
    float xScale = size.x / (boundingBoxMax.x - boundingBoxMin.x);
    float yScale = size.y / (boundingBoxMax.y - boundingBoxMin.y);
    float zScale = size.z / (boundingBoxMax.z - boundingBoxMin.z);

    if (preserveAspectRatio) {
        float uniformScale = std::max({ xScale, yScale, zScale });
        scale = glm::vec3(uniformScale);
    } else {
        scale = glm::vec3(xScale, yScale, zScale);
    }

    boundingBoxMin *= scale;
    boundingBoxMax *= scale;
}

void Model::draw(Shader& shader)
{
    // translate, rotate, scale (so that it's actually done in reverse)
    glm::mat4 transform = glm::mat4(1.0);
    transform = glm::translate(transform, position);
    transform = glm::scale(transform, scale);

    for (Mesh& mesh : meshes) {
        if (!mesh.initialized) {
            mesh.initialized = true;
            mesh.init();
        }
        glBindVertexArray(mesh.vao);

        // Bind the texture samplers
        int index = 0;
        for (auto& [sampler, texture] : mesh.textures) {
            glActiveTexture(GL_TEXTURE0 + index);
            shader.set<int>(("material." + sampler).c_str(), index);
            glBindTexture(GL_TEXTURE_2D, texture.id);
            index++;
        }

        shader.set<glm::mat4>("model", transform * mesh.transform);
        shader.set<int>("hasNormalMap", mesh.textures.count("normal") > 0);
        glDrawElements(GL_TRIANGLES, mesh.indexes.size(), GL_UNSIGNED_INT, 0);
    }

    textureLoader.cleanup(); // Don't need the texture pixels anymore
}
