#version 460 core
#include "buffers.glsl"

in FragmentInfo
{
    vec3 vertexPos;
    vec3 vertexNormal;
    vec2 textureCoord;
    mat3 TBN;
} fragIn;

uniform struct PhongMaps
{
    sampler2D ambient;
    sampler2D diffuse;
    sampler2D specular;
    sampler2D emission;
    sampler2D normal;
    bool hasNormal;
} material;

uniform bool isFramebuffer;
uniform float modelId;

out vec4 color;

// Calculate phong lighting given a light source
vec3 computePhongLighting(Light light, vec3 pos, vec3 viewPos)
{
    // Light position in tangent space
    vec3 lightPos = fragIn.TBN * vec3(model * vec4(light.position, 1.0));

    // Sample the normal. Make it go from a
    // range of 0 to 1 to a range of -1 to 1
    vec3 normal = texture(material.normal, fragIn.textureCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    if (!material.hasNormal) normal = fragIn.vertexNormal;

    vec3 lightDirection = normalize(vec3(lightPos) - pos);
    vec3 viewDirection = normalize(viewPos - pos);
    vec3 reflectDirection = reflect(-lightDirection, normal);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);

    vec3 ambientColor = texture(material.ambient, fragIn.textureCoord).rgb;
    vec3 diffuseColor = texture(material.diffuse, fragIn.textureCoord).rgb;
    vec3 specularColor = texture(material.specular, fragIn.textureCoord).rgb;

    float d = length(lightPos - pos);
    float attenuation = 1.0 / (light.c + light.l * d + light.q * (d * d));

    vec3 ambient = ambientColor * light.color;
    vec3 emissive = texture(material.emission, fragIn.textureCoord).rgb;

    float diff = max(dot(lightDirection, normal), 0.0);
    vec3 diffuse = diff * diffuseColor * light.color;

    float spec = pow(max(dot(normal, halfwayDirection), 0.0), 128);
    vec3 specular = spec * specularColor * light.color;

    return attenuation * (ambient + diffuse + specular + emissive);
}

void main()
{
    // Output the normalized model id
    if (isFramebuffer) {
        float n = 1.0 / modelId;
        color = vec4(n, n, n, 1.0);
        return;
    }

    // Convert the view position and vertex position to tangent space
    vec3 viewPos = fragIn.TBN * viewPosition;
    vec3 pos = fragIn.TBN * vec3(model * vec4(fragIn.vertexPos, 1.0));

    vec3 result = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < lights.length(); i++) {
        result += computePhongLighting(lights[i], pos, viewPos);
    }
    color = vec4(result, 1.0);
}
