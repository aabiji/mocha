#include <glad.h>

#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "textures.h"

// Load the texture pixels and metadata
Texture::Texture(const aiTexture* data)
{
    id = new unsigned int;
    *id = UINT_MAX;
    vao = vbo = ebo = UINT_MAX;

    width = data->mWidth;
    height = data->mHeight;
    pixels = (unsigned char *)data->pcData;

    int channels = 4;
    if (height == 0) {
        // The texture was compressed
        pixels = stbi_load_from_memory(pixels, sizeof(aiTexel) * width, &width,
                                       &height, &channels, 0);
        if (pixels == nullptr)
            throw "Invalid texture data\n";
    }

    format = channels == 1 ? GL_RED : channels == 4 ? GL_RGBA : GL_RGB;
}

Texture::Texture(std::string path)
{
    id = new unsigned int;
    *id = UINT_MAX;
    vao = vbo = ebo = UINT_MAX;

    int channels = 0;
    pixels = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (pixels == nullptr)
        throw "Couldn't read " + path;
    format = channels == 1 ? GL_RED : channels == 4 ? GL_RGBA : GL_RGB;
}

// Create an empty 1x1 texture
Texture::Texture(unsigned char color)
{
    id = new unsigned int;
    *id = UINT_MAX;
    vao = vbo = ebo = UINT_MAX;
    format = GL_RGB;
    width = height = 1;
    pixels = new unsigned char[3]{ color, color, color };
}

Texture::Texture(int w, int h)
{
    id = new unsigned int;
    *id = UINT_MAX;
    vao = vbo = ebo = UINT_MAX;
    format = GL_RGB;
    width = w;
    height = h;
    pixels = nullptr;
}

void Texture::init()
{
    if (*id != UINT_MAX)
        return; // The texture object has already been created

    glGenTextures(1, id);
    glBindTexture(GL_TEXTURE_2D, *id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (pixels != nullptr)
        free(pixels);
}

void Texture::cleanup()
{
    if (id == nullptr)
        return; // Already been cleaned up
    glDeleteTextures(1, id);
    delete id;
    id = nullptr;

    if (vao != UINT_MAX) {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
    }
}

void Texture::write(int x, int y, unsigned char* pixels)
{
    glBindTexture(GL_TEXTURE_2D, *id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height,
                    format, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::createQuad(float windowWidth, float windowHeight)
{
    // Determine the vertex positions that'll result in
    // the texture being drawn with the correct aspect ratio
    float ratio = float(width) / float(height);
    float screenRatio = windowWidth / windowHeight;

    float scaleX = 1.0, scaleY = 1.0;
    if (ratio > screenRatio) {
        scaleY = screenRatio / ratio;
    } else {
        scaleX = ratio / screenRatio;
    }

    float vertices[] = {
        scaleX, -scaleY, 1.0, 1.0,  // top right
        scaleX,  scaleY, 1.0, 0.0,  // bottom right
       -scaleX,  scaleY, 0.0, 0.0,  // bottom left
       -scaleX, -scaleY, 0.0, 1.0   // top left
   };
   unsigned int indexes[] = { 0, 1, 3, 1, 2, 3 };

   glGenVertexArrays(1, &vao);
   glBindVertexArray(vao);

   glGenBuffers(1, &vbo);
   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                         (void*)(2 * sizeof(float)));
   glEnableVertexAttribArray(1);

   glGenBuffers(1, &ebo);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexes),
                &indexes, GL_STATIC_DRAW);

   glBindVertexArray(0);
}

void Texture::draw(Shader& shader, float x, float y, float windowWidth, float windowHeight)
{
    if (vao == UINT_MAX)
        createQuad(windowWidth, windowHeight);

    glBindVertexArray(vao);

    glm::mat4 transform = glm::mat4(1.0);
    float ndcX = (2.0 * x / windowWidth) - 1.0;
    float ndcY = 1.0 - (2.0 * y / windowHeight);
    transform = glm::translate(transform, glm::vec3(ndcX, ndcY, -1.0f));
    
    float scaleX = 2.0 * width / windowWidth;
    float scaleY = 2.0 * height / windowHeight;
    transform = glm::scale(transform, glm::vec3(scaleX, scaleY, 1.0f));
    shader.set<glm::mat4>("transform", transform);

    shader.set<int>("tex", 0);
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, *id);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

TextureLoader::TextureLoader()
{
    samplerNames[aiTextureType_DIFFUSE] = "diffuse";
    defaults["diffuse"]  = Texture(255);

    samplerNames[aiTextureType_SPECULAR] = "specular";
    defaults["specular"] = Texture(150);

    samplerNames[aiTextureType_AMBIENT] = "ambient";
    defaults["ambient"]  = Texture(35);

    samplerNames[aiTextureType_EMISSIVE] = "emission";
    defaults["emission"] = Texture((unsigned char)0);

    samplerNames[aiTextureType_NORMALS] = "normal";
}

void TextureLoader::cleanup()
{
    for (auto& pair : defaults) {
        pair.second.cleanup();
    }

    for (auto& pair : cache) {
        pair.second.cleanup();
    }

    defaults.clear();
    cache.clear();
}

// Get the textures that are defined in the material
TextureMap TextureLoader::get(
    const aiScene* scene,
    const aiMaterial* material,
    std::string basePath
) {
    TextureMap textures;
    for (auto& [type, samplerName]: samplerNames) {
        if (material->GetTextureCount(type) == 0)
            continue;

        aiString aiName;
        material->GetTexture(type, 0, &aiName);
        std::string path = std::string(aiName.C_Str());
        size_t lastBackslash = path.find_last_of("/\\");

        // Texture has been loaded before
        if (cache.count(path)) {
            textures[samplerName] = cache[path];
            continue;
        }

        const aiTexture *data = scene->GetEmbeddedTexture(path.c_str());
        if (data == nullptr && lastBackslash != std::string::npos) {
            // Load the texture from a file
            Texture t(basePath + path.substr(lastBackslash + 1));
            textures[samplerName] = t;
            cache[path] = t;
        } else if (data != nullptr) {
            // Load the embedded texture
            Texture t(data);
            textures[samplerName] = t;
            cache[path] = t;
        }
    }

    // Insert the default texture in place of a missing texture
    for (auto& [name, t] : defaults) {
        if (textures.count(name) == 0)
            textures[name] = t;
    }

    return textures;
}
