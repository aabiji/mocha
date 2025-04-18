#version 460 core

layout (local_size_x = 6, local_size_y = 1, local_size_z = 1) in;

// Input HDR image
layout (std430, binding = 3) readonly buffer HDRImage { float pixels[]; };

// Output cube face
uniform layout (binding = 4, rgba32f) writeonly image2DArray cubeFaces;

void main()
{
    uint x = gl_WorkGroupID.x;
    uint y = gl_WorkGroupID.y;
    uint cubeFace = gl_LocalInvocationID.x;
    vec4 color = vec4(1, 1, 1, 1);
    imageStore(cubeFaces, ivec3(x, y, cubeFace), color);
}
