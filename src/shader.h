#pragma once

#include <glm/glm.hpp>

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
    template <typename T> void set(const char* name, T value);

    // Create a uniform buffer object bound to a
    // specific binding port and return its id
    int createBuffer(int port, int dataSize);

    // Update the data in the uniform buffer object
    void updateBuffer(int id, int dataSize, void* data);

private:
    // Ids of the different shaders
    int vertexShader = -1;
    int fragmentShader = -1;
    int geometryShader = -1;
    int program = -1;
};
