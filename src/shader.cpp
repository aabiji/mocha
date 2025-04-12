#include <filesystem>
#include <fstream>
#include <string>
#include <type_traits>

#include <glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"

std::string readFile(std::string path)
{
    std::ifstream file(path);
    if (!file.good())
        throw "Couldn't read " + path;

    std::string contents = "";
    std::string line = "";
    while (getline(file, line))
        contents += line + "\n";
    return contents;
}

// A very basic preprocessor...
std::string preprocess(std::string path, std::string& basePath)
{
    std::string source = readFile(path);
    std::string pattern = "#include";
    size_t start = 0;

    // Find each occurence of the #include directive
    while (((start = source.find(pattern, start)) != std::string::npos)) {
        // Extract the include path
        int strStart = source.find("\"", start) + 1;
        int strEnd = source.find("\"", strStart) + 1;
        std::string p = source.substr(strStart, strEnd - strStart - 1);

        // Replace the #include directive with the contents of the included file
        source.insert(strEnd, preprocess(basePath + p, basePath));
        source.erase(start, strEnd - start);
    }

    source = source.substr(0, source.length() - 1); // Remove the trailing newline
    return source;
}

void Shader::cleanup() { glDeleteProgram(program); }
void Shader::use() { glUseProgram(program); }

void Shader::load(int type, const char* path)
{
    std::string base = std::filesystem::path(path).parent_path() / "";
    std::string source = preprocess(path, base);
    const char *c_str = source.c_str();

    int shader = glCreateShader(type);
    glShaderSource(shader, 1, &c_str, nullptr);
    glCompileShader(shader);

    char log[512];
    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        throw "SHADER ERROR: (" + std::string(path) + "): " + log;
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
        throw "LINKER ERROR: " + std::string(log);
    }
}

template void Shader::set<int>(const char* name, int value);
template void Shader::set<unsigned int>(const char* name, unsigned int value);
template void Shader::set<glm::mat4>(const char* name, glm::mat4 value);
template void Shader::set<glm::vec3>(const char* name, glm::vec3 value);

template <typename T>
void Shader::set(const char* name, T value)
{
    int address = glGetUniformLocation(program, name);
    if constexpr (std::is_same<T, glm::vec3>::value)
        glUniform3fv(address, 1, glm::value_ptr(value));
    if constexpr (std::is_same<T, glm::mat4>::value)
        glUniformMatrix4fv(address, 1, GL_FALSE, glm::value_ptr(value));
    if constexpr (std::is_same<T, int>::value)
        glUniform1i(address, value);
    if constexpr (std::is_same<T, unsigned int>::value)
        glUniform1ui(address, value);
}

unsigned int Shader::createBuffer(int port, int dataSize)
{
    unsigned int id;
    glGenBuffers(1, &id);
    glBindBuffer(GL_UNIFORM_BUFFER, id);
    glBufferData(GL_UNIFORM_BUFFER, dataSize, NULL, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, port, id);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    return id;
}

void Shader::updateBuffer(unsigned int id, int dataSize, void* data)
{
    glBindBuffer(GL_UNIFORM_BUFFER, id);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, dataSize, data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
