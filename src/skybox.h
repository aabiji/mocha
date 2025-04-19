#pragma once

#include <string>
#include "shader.h"

class Skybox
{
public:
    void init(const char* hdrImagePath, std::string outputFolder);
    void draw(glm::mat4 projection, glm::mat4 viewWithoutTranslation);
    void cleanup();
private:
    void setupBuffers();
    void setupCubemap(std::string imageFolder);
    void createCubemapImages(const char* hdrImagePath, std::string outputFolder);

    Shader shader;
    Shader computeShader;

    unsigned int vao, vbo, ebo;
    unsigned int cubemapTexture;
};
