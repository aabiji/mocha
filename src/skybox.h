#pragma once

#include <string>
#include "shader.h"

class Skybox
{
public:
    void init(const char* hdrImagePath, std::string outputFolder);
    void cleanup();
private:
    Shader shader;
};
