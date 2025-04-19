// Compute shader to convert an equirectangular HDR image into 6 cube map textures
#version 460 core

layout (local_size_x = 6, local_size_y = 1, local_size_z = 1) in;

// Input HDR image
layout (std430, binding = 3) readonly buffer HDRImage { float pixels[]; };

// Size of the HDR image (width, height, hdrSize.z)
uniform vec3 hdrSize;

// Output cube face
uniform layout (binding = 4, rgba32f) writeonly image2DArray cubeFaces;

const vec3 startVectors[6] = {
    vec3( 1.0, -1.0, -1.0), vec3(-1.0, -1.0, 1.0),
    vec3(-1.0,  1.0, -1.0), vec3(-1.0, -1.0, 1.0),
    vec3(-1.0, -1.0, -1.0), vec3( 1.0, -1.0, 1.0),
};

const vec3 rightVectors[6] = {
    vec3(0.0, 0.0, 1.0), vec3( 0.0, 0.0,-1.0),
    vec3(1.0, 0.0, 0.0), vec3( 1.0, 0.0, 0.0),
    vec3(1.0, 0.0, 0.0), vec3(-1.0, 0.0, 0.0),
};

const vec3 upVectors[6] = {
    vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0,-1.0),
    vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 0.0)
};

const float PI = 3.141592653589793;

void main()
{
    vec2 pos = vec2(gl_WorkGroupID.xy);
    vec2 dimensions = vec2(gl_NumWorkGroups.xy);
    uint i = gl_LocalInvocationID.x; // Index of the cube face

    // Get the direction vector that points towards the xy coordinate on the cube face
    //vec2 norm = (pos * 2.0 + 0.5) / dimensions;
    vec2 norm = pos * 2.0 / dimensions;
    vec3 direction = startVectors[i] + rightVectors[i] * norm.x + upVectors[i] * norm.y;

    // Convert the direction vector into polar coordinates
    // This gives us the corresponding position on the sphere
    // that the hdr texture is wrapped around
    float azimuth   = atan(direction.x, -direction.z);
    float elevation =
        atan(direction.y / sqrt(direction.x * direction.x + direction.z * direction.z));

    // Convert from a range of -PI to PI to a range of 0 to 2PI
    azimuth   += PI;
    // Convert from a range of -PI/2 to PI/2 to a range of 0 to PI
    elevation += PI / 2;

    // FIXME: the interpolation is not ideal

    // Map the polar coordinates into an xy position on the hdr texture
    float hdrX = (azimuth / 2 / PI) * hdrSize.x;
    float hdrY = (elevation   / PI) * hdrSize.y;

    // The fractional part of the hdr x and y are the factors
    // The integer part is the actual hdr x and y
    float leftX, rightX, topY, bottomY;
    float factorX = modf(hdrX, leftX);
    float factorY = modf(hdrY, topY);

    // The right and top wrap around if they're out of bounds
    rightX  = leftX == hdrSize.x - 1 ? 0 : leftX + 1;
    bottomY = topY == hdrSize.y - 1 ? 0 : topY + 1;

    // Determine the weights of the 4 surrounding pixels
    // The equation comes from here: https://uploads-cdn.omnicalculator.com/images/bilin_form4.png?width=425&enlarge=0&format=webp
    // Since the distance between the 4 points is always 1, the equation can be simplified down
    float w1 = (1.0 - factorX) * (1.0 - factorY);
    float w2 = factorX * (1.0 - factorY);
    float w3 = (1.0 - factorX) * factorY;
    float w4 = factorX * factorY;

    // Compute the interpolated hdr pixel value
    vec3 interpolated = vec3(0, 0, 0);
    for (int c = 0; c < hdrSize.z; c++) {
        // Get the indexes for the 4 weights
        int w1index = int(leftX  * hdrSize.z + hdrSize.x * bottomY * hdrSize.z + c);
        int w2index = int(rightX * hdrSize.z + hdrSize.x * bottomY * hdrSize.z + c);
        int w3index = int(leftX  * hdrSize.z + hdrSize.x * topY    * hdrSize.z + c);
        int w4index = int(rightX * hdrSize.z + hdrSize.x * topY    * hdrSize.z + c);
        interpolated[c] =
            pixels[w1index] * w1 +
            pixels[w2index] * w2 +
            pixels[w3index] * w3 +
            pixels[w4index] * w4;
    }

    imageStore(cubeFaces, ivec3(pos.x, pos.y, i), vec4(interpolated, 1.0));
}
