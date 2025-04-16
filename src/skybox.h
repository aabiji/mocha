#pragma once

#include <string>
#include "shader.h"

class Skybox
{
public:
    void cleanup();
    void init(std::vector<std::string> texturePaths);
    void draw(glm::mat4 projection, glm::mat4 viewWithoutTranslation);
private:
    Shader shader;
    unsigned int vao, vbo, textureId;
};