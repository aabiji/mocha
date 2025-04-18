#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image_write.h"

#include <cmath>
#include <vector>

struct vec3 { float x, y, z; };

int main()
{
    // Start point corresponding to the left bottom pixel, the right direction, the up direction
    std::vector<std::vector<vec3>> faceInfo = {{
        {{{ 1.0f, -1.0f, -1.0f}, { 0.0f, 0.0f,  1.0f}, {0.0f, 1.0f,  0.0f}}}, // right
        {{{-1.0f, -1.0f,  1.0f}, { 0.0f, 0.0f, -1.0f}, {0.0f, 1.0f,  0.0f}}}, // left
        {{{-1.0f,  1.0f, -1.0f}, { 1.0f, 0.0f,  0.0f}, {0.0f, 0.0f,  1.0f}}}, // top
        {{{-1.0f, -1.0f,  1.0f}, { 1.0f, 0.0f,  0.0f}, {0.0f, 0.0f, -1.0f}}}, // bottom
        {{{-1.0f, -1.0f, -1.0f}, { 1.0f, 0.0f,  0.0f}, {0.0f, 1.0f,  0.0f}}}, // front
        {{{ 1.0f, -1.0f,  1.0f}, {-1.0f, 0.0f,  0.0f}, {0.0f, 1.0f,  0.0f}}}  // back 
    }};
    std::vector<const char*> facePaths = {
        "right.hdr", "left.hdr", "top.hdr", "bottom.hdr", "front.hdr", "back.hdr"
    };

    int cubemapSize = 512;
    const char* path = "../assets/hdr/brown_photostudio_01_4k.hdr";
    // stbi_is_hdr(path);
    int hdrWidth, hdrHeight, channels;
    float* pixels = stbi_loadf(path, &hdrWidth, &hdrHeight, &channels, 0);

    for (int face = 0; face < 6; face++) {
        float* newImage = new float[cubemapSize * cubemapSize * channels];

        for (int y = 0; y < cubemapSize; y++) {
            for (int x = 0; x < cubemapSize; x++) {
                // Normalize x and y to a range of -1 to 1
                float normX = (float(x) * 2.0 + 0.5) / cubemapSize;
                float normY = (float(y) * 2.0 + 0.5) / cubemapSize;

                // Get the direction vector that points towards the xy coordinate on the cube face
                auto info = faceInfo[face];
                vec3 direction = {
                    info[0].x + info[1].x * normX + info[2].x * normY,
                    info[0].y + info[1].y * normX + info[2].y * normY,
                    info[0].z + info[1].z * normX + info[2].z * normY
                };

                // Convert the direction vector into polar coordinates
                // This gives us the corresponding position on the sphere
                // that the hdr texture is wrapped around
                float azimuth   = atan2f(direction.x, -direction.z);
                float elevation = atanf(direction.y / sqrt(direction.x * direction.x + direction.z * direction.z));

                // Convert from a range of -PI to PI to a range of 0 to 2PI
                azimuth   += M_PI;
                // Convert from a range of -PI/2 to PI/2 to a range of 0 to PI
                elevation += M_PI / 2;

                // Map the polar coordinates into an xy position on the hdr texture
                float hdrX = (azimuth / 2 / M_PI) * hdrWidth;
                float hdrY = (elevation   / M_PI) * hdrHeight;

                // The fractional part of the hdr x and y are the factors
                // The integer part is the actual hdr x and y
                float leftX, rightX, topY, bottomY;
                float factorX = modf(hdrX, &leftX);
                float factorY = modf(hdrY, &topY);

                // The right and top wrap around if they're out of bounds
                rightX  = leftX == hdrWidth - 1 ? 0 : leftX + 1;
                bottomY = topY == hdrHeight - 1 ? 0 : topY + 1;

                // Determine the weights of the 4 surrounding pixels
                // The equation comes from here: https://uploads-cdn.omnicalculator.com/images/bilin_form4.png?width=425&enlarge=0&format=webp
                // Since the distance between the 4 points is always 1, the equation can be simplified down
                float w1 = (1.0 - factorX) * (1.0 - factorY);
                float w2 = factorX * (1.0 - factorY);
                float w3 = (1.0 - factorX) * factorY;
                float w4 = factorX * factorY;

                // Write the interpolated pixel unto the cube face
                for (int c = 0; c < channels; c++) {
                    // Get the indexes for the 4 weights
                    int w1index = leftX  * channels + hdrWidth * bottomY * channels + c;
                    int w2index = rightX * channels + hdrWidth * bottomY * channels + c;
                    int w3index = leftX  * channels + hdrWidth * topY    * channels + c;
                    int w4index = rightX * channels + hdrWidth * topY    * channels + c;

                    int imgIndex =   x * channels + cubemapSize * y * channels + c;
                    newImage[imgIndex] = pixels[w1index] * w1 + pixels[w2index] * w2 + pixels[w3index] * w3 + pixels[w4index] * w4;
                }
            }
        }

        stbi_write_hdr(facePaths[face], cubemapSize, cubemapSize, channels, newImage);
    }

    free(pixels);
}
