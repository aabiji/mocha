#pragma once

#include <glm/glm.hpp>
#include <unordered_map>

// Shader storage buffer object
struct StorageBuffer
{
    unsigned int id;
    unsigned int binding;
};

class Shader
{
public:
    void use(); // Set the shader program as the curent one
    void assemble(); // Link all the separate shader programs into one
    void cleanup(); // Delete this shader program

    // Load a type (GL_FRAGMENT_SHADER, GL_VERTEX_SHADER, etc)
    // of shader and link it to the shader program
    void load(int type, const char *path);

    // Set a uniform value
    template <typename T> void set(std::string name, T value);

    // Handle shader storage buffer objects
    void createBuffer(std::string name, int binding, int allocationSize);
    void writeBuffer(std::string name, void* data, int offset, int size);
    void bindBuffer(std::string name);
    void deleteBuffer(std::string name);
private:
    // Ids of the different shaders
    int vertexShader = -1;
    int fragmentShader = -1;
    int geometryShader = -1;
    int program = -1;

    // Map shader storage objects to their given names
    std::unordered_map<std::string, StorageBuffer> buffers;
};
