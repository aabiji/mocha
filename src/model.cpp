#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "convert.h"
#include "model.h"

struct ModelTransforms
{
    glm::mat4 model;
    glm::mat4 meshTransform;
    glm::mat4 boneTransforms[];
};

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
    glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIds));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));
    glEnableVertexAttribArray(5);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexes.size() * sizeof(unsigned int), indexes.data(), GL_STATIC_DRAW);

    for (auto& [sampler, texture] : textures) {
        texture.init();
    }

    vertices.clear(); // Won't need this anymore
    glBindVertexArray(0);
}

void Mesh::draw(Shader& shader)
{
    if (!initialized) {
        initialized = true;
        init();
    }
    glBindVertexArray(vao);

    // Bind the texture samplers
    int index = 0;
    for (auto& [sampler, texture] : textures) {
        glActiveTexture(GL_TEXTURE0 + index);
        shader.set<int>("material." + sampler, index);
        glBindTexture(GL_TEXTURE_2D, texture.id);
        index++;
    }

    // Draw
    shader.set<int>("material.hasNormal", textures.count("normal") > 0);
    glDrawElements(GL_TRIANGLES, indexes.size(), GL_UNSIGNED_INT, 0);
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

Model::Model(std::string id, std::string path, std::string textureBasePath)
{
    name = id;
    scale = glm::vec3(1.0);
    position = glm::vec3(0.0);

    Assimp::Importer importer;
    unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                         aiProcess_CalcTangentSpace | aiProcess_FlipUVs |
                         aiProcess_GenBoundingBoxes;
    const aiScene* scene = importer.ReadFile(path, flags);
    if (scene == nullptr)
        throw std::string(importer.GetErrorString());

    if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        throw std::string("Invalid model file");

    textureLoader.setBasePath(textureBasePath);
    animator.load(scene);

    processNode(scene, scene->mRootNode);
}

void Model::cleanup()
{
    for (Mesh& mesh : meshes) {
        mesh.cleanup();
    }
}

void Model::setPosition(glm::vec3 v) { position = v; }

int Model::getCurrentAnimation() { return animator.currentAnimation; }

void Model::setCurrentAnimation(int index) { animator.currentAnimation = index; }

void Model::toggleAnimation() { animator.playing = !animator.playing; }

bool Model::animationPlaying() { return animator.playing; }

std::vector<std::string> Model::animationNames() { return animator.animationNames(); }

glm::mat4 Model::getTransformationMatrix()
{
    glm::mat4 transform = glm::mat4(1.0);
    transform = glm::translate(transform, position);
    transform = glm::scale(transform, scale);
    return transform;
}

void Model::setSize(glm::vec3 size, bool preserveAspectRatio)
{
    // TODO: use worldSpaceBoundingBox() instead
    glm::mat4 mat = getTransformationMatrix();
    glm::vec3 scaledBoxMin = mat * glm::vec4(box.min, 1.0);
    glm::vec3 scaledBoxMax = mat * glm::vec4(box.max, 1.0);
    glm::vec3 boxSize = scaledBoxMax - scaledBoxMin;

    if (FLOAT_EQUAL(boxSize.x, size.x) ||
        FLOAT_EQUAL(boxSize.y, size.y) ||
        FLOAT_EQUAL(boxSize.z, size.z)) {
        return; // Already resized to the size
    }

    glm::vec3 factor = size / boxSize;
    if (preserveAspectRatio) {
        float uniformScale = std::max({ factor.x, factor.y, factor.z });
        scale = glm::vec3(uniformScale);
    } else {
        scale = factor;
    }
}

void Model::processNode(const aiScene* scene, const aiNode* node)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        processMesh(scene, scene->mMeshes[node->mMeshes[i]]);
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(scene, node->mChildren[i]);
    }
}

void Model::addBoneToVertex(Vertex& v, int boneId, float weight)
{
    // Each bone has at most 4 bones that can influence it
    for (int i = 0; i < 4; i++) {
        if (v.boneWeights[i] == 0.0) {
            v.boneIds[i] = boneId;
            v.boneWeights[i] = weight;
            return;
        }
    }
}

void Model::getBoneWeights(aiMesh* data, Mesh& mesh)
{
    for (unsigned int i = 0; i < data->mNumBones; i++) {
        aiBone* bone = data->mBones[i];
        std::string name = std::string(bone->mName.C_Str());
        int boneId = animator.getBoneId(name);

        for (unsigned int j = 0; j < bone->mNumWeights; j++) {
            float weight = bone->mWeights[j].mWeight;
            int vertexIndex = bone->mWeights[j].mVertexId;
            addBoneToVertex(mesh.vertices[vertexIndex], boneId, weight);
        }
    }
}

void Model::processMesh(const aiScene* scene, aiMesh* data)
{
    Mesh mesh;
    mesh.initialized = false;
    mesh.textures = textureLoader.get(scene, scene->mMaterials[data->mMaterialIndex]);

    for (unsigned int i = 0; i < data->mNumFaces; i++) {
        for (unsigned int j = 0; j < data->mFaces[i].mNumIndices; j++) {
            mesh.indexes.push_back(data->mFaces[i].mIndices[j]);
        }
    }

    for (unsigned int i = 0; i < data->mNumVertices; i++) {
        Vertex v;
        v.boneIds = glm::ivec4(-1.0);
        v.boneWeights = glm::vec4(0.0);
        v.position = toVec3(data->mVertices[i]);
        if (data->HasNormals())
            v.normal = toVec3(data->mNormals[i]);

        // The tangent is derived from the texture coordinate, so if there's
        // no texture coordinate, there can't be a tangent. If the tangent is
        // made to default to (0, 0, 0), the TBN matrix will be singular, which
        // would fuck up the lighting calculations. So, the tangent is better
        // off as randomly initialized.
        if (data->mTextureCoords[0]) {
            v.coord = toVec2(data->mTextureCoords[0][i]);
            v.tangent = toVec3(data->mTangents[i]);
        } else {
            v.coord = glm::vec2(0, 0);
            v.tangent = glm::vec3(rand(), rand(), rand());
        }

        mesh.vertices.push_back(v);
    }

    box.update(toVec3(data->mAABB.mMin));
    box.update(toVec3(data->mAABB.mMax));
    getBoneWeights(data, mesh);

    meshes.push_back(std::move(mesh));
}

void Model::draw(Shader& shader, double timeInSeconds)
{
    // Initial the shader storage buffer object
    if (!shader.haveBuffer(name)) {
        int maxPossibleSize =
            sizeof(glm::mat4) * (animator.getNumBoneTransforms() + 2);
        shader.createBuffer(name, 1, maxPossibleSize);
    }
    shader.bindBuffer(name);

    glm::mat4 t = getTransformationMatrix();
    shader.writeBuffer(name, glm::value_ptr(t), offsetof(ModelTransforms, model), sizeof(t));

    Animation* animation = animator.run(timeInSeconds);

    for (size_t i = 0; i < meshes.size(); i++) {
        Mesh& mesh = meshes[i];

        if (animation != nullptr) {
            for (size_t j = 0; j < animation->boneTransforms.size(); j++) {
                glm::mat4& t = animation->boneTransforms[j];
                int offset = offsetof(ModelTransforms, boneTransforms) + j * sizeof(glm::mat4);
                shader.writeBuffer(name, glm::value_ptr(t), offset, sizeof(t));
            }

            glm::mat4& t = animation->meshTransforms[i];
            shader.writeBuffer(name, glm::value_ptr(t), offsetof(ModelTransforms, meshTransform), sizeof(t));
        } else {
            glm::mat4 t = glm::mat4(1.0);
            shader.writeBuffer(name, glm::value_ptr(t), offsetof(ModelTransforms, meshTransform), sizeof(t));
        }

        mesh.draw(shader);
    }

    textureLoader.cleanup(); // Don't need the texture pixels anymore
}
