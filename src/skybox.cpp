#include <glad.h>
#include <stb_image.h>

#include <iostream>

#include "skybox.h"

// TODO:
// - Create a C++ script to convert the
/*
for each cube face, for each y and for each x:
    - Get the direction vector that'll point to the (x, y) on the specific face of the cube
    - Derive spherical coordinates from the direction vector
    - Convert the spherical coordinates into (x, y) coordinates on the HDR image
    - Write the binearly interpolated value as the pixel value
*/
//   equirectangular HDR images into the 6 cubemap faces on the CPU 
// - Research compute shaders in order to port the script to the GPU
// - Reduce the amount of skyboxVertices and add an EBO
// - Load each image using Texture(), instead of calling init(),
//   define a CubeMap class that handles the cubemap initialization
// - Change the texture formats depending on whether the texture is HDR
//   and if it is, apply gamma correction in the fragment shader
// - Environmental mapping
// - Refactor, refactor, refactor
// - Done the engine, start researching pose estimation!

const float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
};

void Skybox::init(std::vector<std::string> texturePaths)
{
    // Init the shader
    shader.load(GL_VERTEX_SHADER, "../shaders/skybox_vertex.glsl");
    shader.load(GL_FRAGMENT_SHADER, "../shaders/skybox_fragment.glsl");
    shader.assemble();
    shader.use();

    // Init the VAO and VBO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Init the cubemap texture
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);

    for (size_t i = 0; i < texturePaths.size(); i++) {
        std::string path = texturePaths[i];
        int width, height, channels;
        unsigned char* pixels = stbi_load(texturePaths[i].c_str(), &width, &height, &channels, 0);
        std::cout << "Width: " << width << " Height: " << height << "\n";
        if (pixels == nullptr)
            throw "Couldn't read " + texturePaths[i];
        int format = channels == 1 ? GL_RED : channels == 4 ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, pixels);
        free(pixels);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Skybox::cleanup()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteTextures(1, &textureId);
}

void Skybox::draw(glm::mat4 projection, glm::mat4 viewWithoutTranslation)
{
    // Make depth test so that the depth test passes when values are equal
    // to the depth buffer's content. (don't overwrite existing pixels)
    glDepthFunc(GL_LEQUAL);

    shader.use();
    shader.set<glm::mat4>("projection", projection);
    shader.set<glm::mat4>("view", viewWithoutTranslation);

    // Draw
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    // Reset to default
    glDepthFunc(GL_LESS);
}
