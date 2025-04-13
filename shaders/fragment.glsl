#version 460 core
#include "shared.glsl"

uniform struct Maps
{
    sampler2D ambient;
    sampler2D diffuse;
    sampler2D specular;
    sampler2D emission;
    sampler2D normal;
} material;
uniform bool hasNormalMap;

in FragmentInfo
{
    vec3 viewPos;
    vec3 fragmentPos;
    vec2 textureCoord;
    vec3 vertexNormal;
    vec3 lightPos[NUM_LIGHTS]; // Light positions in tangent space
} fragIn;

uniform bool isFramebuffer;
uniform uint modelId;

out vec4 color;

// Calculate phong lighting (ambiant, diffuse,
// specular, emissive) given a light source
vec3 calculateLighting(Light light, vec3 lightPosition)
{
    // Sample the normal. Make it go from a
    // range of 0 to 1 to a range of -1 to 1
    vec3 normal = texture(material.normal, fragIn.textureCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    if (!hasNormalMap) normal = fragIn.vertexNormal;

    vec3 lightDirection = normalize(vec3(lightPosition) - fragIn.fragmentPos);
    vec3 viewDirection = normalize(fragIn.viewPos - fragIn.fragmentPos);
    vec3 reflectDirection = reflect(-lightDirection, normal);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);

    vec3 ambientColor = texture(material.ambient, fragIn.textureCoord).rgb;
    vec3 diffuseColor = texture(material.diffuse, fragIn.textureCoord).rgb;
    vec3 specularColor = texture(material.specular, fragIn.textureCoord).rgb;

    float d = length(lightPosition - fragIn.fragmentPos);
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
    if (isFramebuffer) {
        // Map the modelId to a range of 0 to 1
        float n = 1.0 / float(modelId);
        color = vec4(n, n, n, 1.0);
        return;
    }

    vec3 result = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < NUM_LIGHTS; i++) {
        result += calculateLighting(lights[i], fragIn.lightPos[i]);
    }
    color = vec4(result, 1.0);
}