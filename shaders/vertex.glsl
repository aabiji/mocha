#version 460 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tangent;
layout (location = 3) in vec2 coord;

// TODO: hardcoding the array sizes like that is dodgy 
struct Light
{
    vec3 color;
    vec3 position;

    // Attenuation variables
    float constant;
    float linear;
    float quadratic;
};

uniform vec3 viewPos;
uniform mat4 view;
uniform mat4 model;
uniform mat4 projection;
uniform Light lightSources[4];

out vec3 fragmentPosition;
out vec3 viewPosition;
out vec3 vertexNormal;
out vec2 textureCoordinate;
out Light lights[4];

void main()
{
    // Scale the vertex normal and tangent properly
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vec3 T = normalize(normalMatrix * tangent);
    vec3 N = normalize(normalMatrix * normal);
    // Make sure the tangent is perpendicular to the normal
    T = normalize(T - dot(T, N) * N);
    // Derive the bitangent
    vec3 B = cross(N, T);
    // Now we have the tangent-bitangent-normal matrix
    mat3 TBN = mat3(T, B, N);
 
    // Map the vectors from world space to tangent space so that
    // all the lighting calculations can be done in tangent space
    vertexNormal     = N;
    viewPosition     = TBN * viewPos;
    fragmentPosition = TBN * vec3(model * vec4(position, 1.0));
    for (int i = 0; i < lights.length(); i++) {
        vec3 p = TBN * vec3(model * vec4(lightSources[i].position, 1.0));
        lights[i] = lightSources[i];
        lights[i].position = p;
    }

    gl_Position = projection * view * model * vec4(position, 1.0);
    textureCoordinate = coord;
}
