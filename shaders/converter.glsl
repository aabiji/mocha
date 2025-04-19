// Compute shader to convert an equirectangular HDR image into 6 cube map textures
#version 460 core

layout(local_size_x = 6, local_size_y = 1, local_size_z = 1) in;

// Input HDR image
uniform layout(binding = 3, rgba32f) readonly image2D hdrTexture;

// Output cube face
uniform layout(binding = 4, rgba32f) writeonly image2DArray cubeFaces;

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
    ivec2 hdrSize = imageSize(hdrTexture);

    // Get the direction vector that points towards the xy coordinate on the cube face
    vec2 norm = (pos * 2.0 + 0.5) / dimensions;
    vec3 direction = startVectors[i] + rightVectors[i] * norm.x + upVectors[i] * norm.y;
    direction = normalize(direction);

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

    // Map the polar coordinates into an xy position on the hdr texture
    vec2 hdrPos = vec2((azimuth / 2 / PI) * hdrSize.x, (elevation / PI) * hdrSize.y);

    // Interpolation factor for the x and y
    vec2 factor = vec2(hdrPos.x - floor(hdrPos.x), hdrPos.y - floor(hdrPos.y));

    // left x and top y
    ivec2 left = ivec2(floor(hdrPos.x), floor(hdrPos.y));
    // right x and bottom y (they wrap around if they're out of bounds)
    ivec2 right = ivec2(
        left.x == hdrSize.x - 1 ? 0 : left.x + 1,
        left.y == hdrSize.y - 1 ? 0 : left.y + 1
    );

    // Determine the weights of the 4 surrounding pixels
    // The equation comes from here:
    // https://uploads-cdn.omnicalculator.com/images/bilin_form4.png
    // Since the distance between the 4 points is always 1, the equations are simplified
    float w1 = (1.0 - factor.x) * (1.0 - factor.y);
    float w2 = factor.x * (1.0 - factor.y);
    float w3 = (1.0 - factor.x ) * factor.y;
    float w4 = factor.x * factor.y;

    // Write the interpolated pixel value
    vec4 interpolated =
        imageLoad(hdrTexture, left)  * w1 +
        imageLoad(hdrTexture, ivec2(right.x, left.y)) * w2 +
        imageLoad(hdrTexture, ivec2(left.x, right.y)) * w3 +
        imageLoad(hdrTexture, right) * w4;

    imageStore(cubeFaces, ivec3(pos.x, pos.y, i), interpolated);
}
