#include <filesystem>

#include <glad.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include "skybox.h"

const float cubeVertices[] = {
    -1.0, -1.0, -1.0,  1.0, -1.0, -1.0,
     1.0,  1.0, -1.0, -1.0,  1.0, -1.0,
    -1.0, -1.0,  1.0,  1.0, -1.0,  1.0,
     1.0,  1.0,  1.0, -1.0,  1.0,  1.0
};

const unsigned int cubeIndexes[] = {
    0, 1, 2, 2, 3, 0,
    4, 0, 3, 3, 7, 4,
    1, 5, 6, 6, 2, 1,
    3, 2, 6, 6, 7, 3,
    4, 5, 1, 1, 0, 4,
    5, 4, 7, 7, 6, 5
};

const char* cubemapImages[6] = {
    "right.hdr", "left.hdr",
    "top.hdr", "bottom.hdr",
    "front.hdr", "back.hdr"
};

bool cubemapImagesExists(std::string base)
{
    for (int i = 0; i < 6; i++) {
        if (!std::filesystem::exists(base + cubemapImages[i]))
            return false;
    }
    return true;
}

// Load the cubemap images associated to the HDR image,
// if there aren't any, create the cubemap images from the HDR image
void Skybox::init(const char* hdrImagePath, std::string outputFolder)
{
    if (!stbi_is_hdr(hdrImagePath))
        throw "The image must be an HDR image";

    if (!cubemapImagesExists(outputFolder)) {
        computeShader.load(GL_COMPUTE_SHADER, "../src/shaders/skybox/compute.glsl");
        computeShader.assemble();
        computeShader.use();

        std::filesystem::create_directories(outputFolder);
        createCubemapImages(hdrImagePath, outputFolder);

        computeShader.cleanup();
    }

    shader.load(GL_VERTEX_SHADER, "../src/shaders/skybox/vertex.glsl");
    shader.load(GL_FRAGMENT_SHADER, "../src/shaders/skybox/fragment.glsl");
    shader.assemble();
    shader.use();

    setupBuffers();
    setupCubemap(outputFolder);
}

void Skybox::setupBuffers()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndexes),
            cubeIndexes, GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void Skybox::setupCubemap(std::string imageFolder)
{
    glGenTextures(1, &cubemapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load the pixels for each face of the cubemap
    for (int i = 0; i < 6; i++) {
        std::string path = imageFolder + cubemapImages[i];

        int w, h, channels;
        float* pixels = stbi_loadf(path.c_str(), &w, &h, &channels, 0);
        if (pixels == nullptr)
            throw "Couldn't read " + path;

        int target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;
        int format = channels == 3 ? GL_RGB : GL_RGBA;
        glTexImage2D(target, 0, GL_RGBA32F, w, h, 0, format, GL_FLOAT, pixels);
        free(pixels);
    }
}

// Generate images for the 6 cubemap textures using an equirectangular HDR image
void Skybox::createCubemapImages(const char* hdrImagePath, std::string outputFolder)
{
    // Load the HDR image as the input image2d
    int w, h, channels;
    float* pixels = stbi_loadf(hdrImagePath, &w, &h, &channels, 0);
    if (pixels == nullptr)
        throw std::string("Couldn't read ") + hdrImagePath;
    int format = channels == 3 ? GL_RGB : GL_RGBA;

    unsigned int hdrTexture;
    glGenTextures(1, &hdrTexture);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, format, GL_FLOAT, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindImageTexture(3, hdrTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    free(pixels);

    // Setup the cube faces texture array
    int cubemapSize = 1024, numFaces = 6;
    unsigned int textureArray;
    glGenTextures(1, &textureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, cubemapSize, cubemapSize, numFaces);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glBindImageTexture(4, textureArray, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // Run the compute shader
    glDispatchCompute(cubemapSize, cubemapSize, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Get the cube face texture pixels
    int stride = cubemapSize * cubemapSize * channels;
    float* textureArrayPixels = (float*)malloc(numFaces * stride * sizeof(float));
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
    glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, GL_FLOAT, textureArrayPixels);

    // Write the cube face textures to disk
    for (int i = 0; i < numFaces; i++) {
        float* p = textureArrayPixels + i * stride;
        std::string path = outputFolder + cubemapImages[i];
        stbi_write_hdr(path.c_str(), cubemapSize, cubemapSize, channels, p);
    }

    glDeleteTextures(1, &hdrTexture);
    glDeleteTextures(1, &textureArray);
    free(textureArrayPixels);
}

void Skybox::draw(glm::mat4 projection, glm::mat4 viewWithoutTranslation)
{
    int size = sizeof(cubeIndexes) / sizeof(unsigned int);

    glDepthFunc(GL_LEQUAL);

    shader.use();
    shader.set<glm::mat4>("projection", projection);
    shader.set<glm::mat4>("view", viewWithoutTranslation);
    shader.set<float>("exposure", 1.0);

    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawElements(GL_TRIANGLES, size, GL_UNSIGNED_INT, 0);

    glDepthFunc(GL_LESS);
}

void Skybox::cleanup()
{
    shader.cleanup();
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(1, &cubemapTexture);
}
