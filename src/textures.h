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

    void init();
    void cleanup();

    int width, height, format;
    unsigned int* id;
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
