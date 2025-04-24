#pragma once

#include <assimp/scene.h>
#include <unordered_map>

#include "shader.h"

class Texture
{
public:
    // Load the pixel data
    Texture(const aiTexture* data);
    Texture(unsigned char color);
    Texture(std::string path);
    Texture(int w, int h);
    Texture() {}

    void init();
    void cleanup();

    void draw(Shader& shader, float x, float y, float windowWidth, float windowHeight);
    void write(int x, int y, unsigned char* pixels);
    
    unsigned int* id;
private:
    void createQuad(float windowWidth, float windowHeight);

    unsigned int vao, vbo, ebo;
    int width, height, format;
    unsigned char* pixels;
};

using TextureMap = std::unordered_map<std::string, Texture>;

class TextureLoader
{
public:
    TextureLoader();
    TextureMap get(
        const aiScene* scene,
        const aiMaterial* material,
        std::string basePath);
    void cleanup();
private:
    TextureMap cache;
    TextureMap defaults;
    std::unordered_map<aiTextureType, std::string> samplerNames;
};
