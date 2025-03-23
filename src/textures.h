#pragma once

#include <assimp/scene.h>
#include <unordered_map>

struct Texture
{
    // Load the pixel data
    Texture(const aiTexture* data);
    Texture(unsigned char color);
    Texture(std::string path);
    Texture() {}

    // Initialize the OpenGL texture object
    void init();

    unsigned int id;
    int width, height, format;
    unsigned char* pixels;
};

using TextureMap = std::unordered_map<std::string, Texture>;

class TextureLoader
{
public:
    TextureLoader(std::string base);
    TextureMap get(const aiScene* scene, const aiMaterial* material);
    void cleanup();
private:
    std::string basePath;
    TextureMap cache;
    TextureMap defaults;
    std::unordered_map<aiTextureType, std::string> samplerNames;
};