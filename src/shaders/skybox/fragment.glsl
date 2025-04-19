#version 460 core

in vec3 textureCoord;
out vec4 color;

uniform float exposure;
uniform samplerCube cubeTexture;

void main()
{
    float gamma = 2.2;
    vec3 hdrColor = texture(cubeTexture, textureCoord).rgb;
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure); // Exposure tone mapping
    vec3 corrected = pow(mapped, vec3(1.0 / gamma)); // Gamma correction
    color = vec4(corrected, 1.0);
}
