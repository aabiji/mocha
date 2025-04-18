#include <filesystem>
#include <iostream>

#include <glad.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include "skybox.h"

const char* cubemapFiles[6] = {
    "right.hdr", "left.hdr",
    "top.hdr", "bottom.hdr",
    "front.hdr", "back.hdr"
};

bool cubemapFilesExists(std::string base)
{
    for (int i = 0; i < 6; i++) {
        if (!std::filesystem::exists(base + cubemapFiles[i]))
            return false;
    }
    return true;
}

// Load the cubemap images associated to the HDR image,
// if there aren't any, create the cubemap images from the HDR image
void Skybox::init(const char* hdrImagePath, std::string outputFolder)
{
    (void)hdrImagePath;
    if (cubemapFilesExists(outputFolder))
        throw "TODO!";

    if (!stbi_is_hdr(hdrImagePath))
        throw "The image must be an HDR image";
    std::filesystem::create_directories(outputFolder);

    shader.load(GL_COMPUTE_SHADER, "../shaders/compute.glsl");
    shader.assemble();
    shader.use();

    // Load the HDR image into the shader storage buffer object
    int w, h, channels;
    float* pixels = stbi_loadf(hdrImagePath, &w, &h, &channels, 0);
    if (pixels == nullptr)
        throw std::string("Couldn't read ") + hdrImagePath;
    int numBytes = w * h * channels;
    shader.createBuffer("hdrImage", 3, numBytes);
    shader.writeBuffer("hdrImage", pixels, 0, numBytes);
    free(pixels);

    // Setup the cube faces texture array
    int cubemapSize = 512, numFaces = 6;
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
        std::string path = outputFolder + cubemapFiles[i];
        int result = stbi_write_hdr(path.c_str(), cubemapSize, cubemapSize, channels, p);
        if (result == 0) throw "Couldn't write " + path;
    }
    free(textureArrayPixels);
}

void Skybox::cleanup()
{
    shader.cleanup();
}
