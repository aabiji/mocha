#include <format>
#include <fstream>
#include <string>

#include <glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"

std::string readFile(std::string path)
{
    std::ifstream file(path);
    if (!file.good())
        return "";

    std::string contents = "";
    std::string line = "";
    while (getline(file, line))
        contents += line + "\n";
    return contents;
}

void Shader::cleanup() { glDeleteProgram(program); }

void Shader::use() { glUseProgram(program); }

void Shader::load(int type, const char* path)
{
    std::string source = readFile(path);
    if (source.empty())
        throw std::format("ERROR: Failed to read {}", path);

    const char *c_str = source.c_str();
    int shader = glCreateShader(type);
    glShaderSource(shader, 1, &c_str, nullptr);
    glCompileShader(shader);

    char log[512];
    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        throw std::format("ERROR ({}): {}", path, log);
    }

    if (type == GL_VERTEX_SHADER)
        vertexShader = shader;
    if (type == GL_FRAGMENT_SHADER)
        fragmentShader = shader;
    if (type == GL_GEOMETRY_SHADER)
        geometryShader = shader;
}

void Shader::assemble()
{
    program = glCreateProgram();

    if (vertexShader != -1) {
        glAttachShader(program, vertexShader);
        glDeleteShader(vertexShader);
    }

    if (fragmentShader != -1) {
        glAttachShader(program, fragmentShader);
        glDeleteShader(fragmentShader);
    }

    if (geometryShader != -1) {
        glAttachShader(program, geometryShader);
        glDeleteShader(geometryShader);
    }

    glLinkProgram(program);

    int success = 0;
    char log[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        throw std::format("LINKER ERROR: {}", log);
    }
}

void Shader::setInt(const char* name, int value)
{
    glUniform1i(glGetUniformLocation(program, name), value);
}

void Shader::setFloat(const char* name, float value)
{
    glUniform1f(glGetUniformLocation(program, name), value);
}

void Shader::setMatrix(const char* name, glm::mat4 value)
{
    glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setVec3(const char* name, glm::vec3 value)
{
    glUniform3fv(glGetUniformLocation(program, name), 1, glm::value_ptr(value));
}

void Shader::setVec4(const char* name, glm::vec4 value)
{
    glUniform4fv(glGetUniformLocation(program, name), 1, glm::value_ptr(value));
}
