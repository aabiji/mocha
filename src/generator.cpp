#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image_write.h"

#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>

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

    // 4 bytes * 3 = 12 bytes per pixel
    // 512 * 512 * 12 = 3145728 bytes = 3.145728 MB per image
    // 3.145728 * 6 = 18.874368 MB, so about 19 MB allocation for the cubemap
    //
    // 4096 * 2048 * 3 = 25165824 bytes = 25 MB allocation for the hdr image
    //
    // 25 + 19 = 44 MB minimum allocation

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

                // TODO: bilinear interpolation -- how are we going
                //       to get the 4 corners from the hdrX and hdrY???
                // the fractional part of hdrX is tx, the fractional part of hdrY is ty
                // the left is the interger part of hdrX, the right is the integer part of hdrX + 1 (same idea for y)
                // edge case where the left is on right edge or bottom is on bottom edge
                // then plug into this equation: https://uploads-cdn.omnicalculator.com/images/bilin_form4.png?width=425&enlarge=0&format=webp

                // Write the corresponding pixel unto the cube face
                int _x = std::clamp(int(hdrX), 0, hdrWidth  - 1);
                int _y = std::clamp(int(hdrY), 0, hdrHeight - 1);
                for (int c = 0; c < channels; c++) {
                    int imgIndex =   x * channels + cubemapSize * y * channels + c;
                    int hdrIndex =  _x * channels + hdrWidth *   _y * channels + c;
                    newImage[imgIndex] = pixels[hdrIndex];
                }
            }
        }

        stbi_write_hdr(facePaths[face], cubemapSize, cubemapSize, channels, newImage);
    }

    free(pixels);
}
