#pragma once

#include <glm/glm.hpp>

class Shader
{
public:
    void use();
    void assemble();
    void cleanup();
    void load(int type, const char *path);

    void setInt(const char* name, int value);
    void setFloat(const char* name, float value);
    void setMatrix(const char* name, glm::mat4 value);
    void setVec3(const char* name, glm::vec3 value);
    void setVec4(const char* name, glm::vec4 value);

private:
    int vertexShader = -1;
    int fragmentShader = -1;
    int geometryShader = -1;
    int program = -1;
};
