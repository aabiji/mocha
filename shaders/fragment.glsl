#version 460 core
#include "shared.glsl"

in FragmentInfo
{
    vec3 vertexNormal;
    vec3 viewPos;
    vec3 fragmentPos;
    vec3 lightPos[NUM_LIGHTS];
    vec2 textureCoord;
} fragOut;

out vec4 color;

uniform Maps
{
    sampler2D ambient;
    sampler2D diffuse;
    sampler2D specular;
    sampler2D emission;
    sampler2D normal;
} material;

uniform bool hasNormalMap;


// Calculate phong lighting (ambiant, diffuse,
// specular, emissive) given a light source
vec3 calculateLighting(Light light, vec3 lightPosition)
{
    // Sample the normal. Make it go from a
    // range of 0 to 1 to a range of -1 to 1
    vec3 normal = texture(material.normal, fragOut.textureCoord).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    if (!hasNormalMap) normal = fragOut.vertexNormal;

    vec3 lightDirection = normalize(vec3(lightPosition) - fragOut.fragmentPos);
    vec3 viewDirection = normalize(fragOut.viewPos - fragOut.fragmentPos);
    vec3 reflectDirection = reflect(-lightDirection, normal);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);

    vec3 ambientColor = texture(material.ambient, fragOut.textureCoord).rgb;
    vec3 diffuseColor = texture(material.diffuse, fragOut.textureCoord).rgb;
    vec3 specularColor = texture(material.specular, fragOut.textureCoord).rgb;

    float distance = length(vec3(lightPosition) - fragOut.fragmentPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 ambient = ambientColor * light.color;
    ambient *= attenuation;

    float diff = max(dot(lightDirection, normal), 0.0);
    vec3 diffuse = diff * diffuseColor * light.color;
    diffuse *= attenuation;

    float spec = pow(max(dot(normal, halfwayDirection), 0.0), 128);
    vec3 specular = spec * specularColor * light.color;
    specular *= attenuation;

    vec3 emissive = texture(material.emission, fragOut.textureCoord).rgb;
    emissive *= attenuation;

    return ambient + diffuse + specular + emissive;
}

void main()
{
    vec3 result = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < lights.length(); i++) {
        result += calculateLighting(lights[i], fragOut.lightPos[i]);
    }
    color = vec4(result, 1.0);
}
