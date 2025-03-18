#version 460 core

struct Maps
{
    sampler2D ambient;
    sampler2D diffuse;
    sampler2D specular;
    sampler2D emission;
    sampler2D normal;
};

// TODO: THIS IS DODGY! We shouldn't have to redefine this struct!
struct Light
{
    vec3 color;
    vec3 position;

    // Attenuation variables
    float constant;
    float linear;
    float quadratic;
};

in vec3 vertexNormal;
in vec3 fragmentPosition;
in vec3 viewPosition;
in vec2 textureCoordinate;
in Light lights[4];

uniform Maps material;
uniform bool hasNormalMap;

out vec4 color;

vec3 calculateLighting(Light light)
{
    // Sample the normal. Make it go from a
    // range of 0 to 1 to a range of -1 to 1
    vec3 normal = texture(material.normal, textureCoordinate).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    if (!hasNormalMap) normal = vertexNormal;

    vec3 lightDirection = normalize(vec3(light.position) - fragmentPosition);
    vec3 viewDirection = normalize(viewPosition - fragmentPosition);
    vec3 reflectDirection = reflect(-lightDirection, normal);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);

    vec3 ambientColor = texture(material.ambient, textureCoordinate).rgb;
    vec3 diffuseColor = texture(material.diffuse, textureCoordinate).rgb;
    vec3 specularColor = texture(material.specular, textureCoordinate).rgb;

    float distance = length(vec3(light.position) - fragmentPosition);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    vec3 ambient = ambientColor * light.color;
    ambient *= attenuation;

    float diff = max(dot(lightDirection, normal), 0.0);
    vec3 diffuse = diff * diffuseColor * light.color;
    diffuse *= attenuation;

    float spec = pow(max(dot(normal, halfwayDirection), 0.0), 128);
    vec3 specular = spec * specularColor * light.color;
    specular *= attenuation;

    vec3 emissive = texture(material.emission, textureCoordinate).rgb;
    emissive *= attenuation;

    return ambient + diffuse + specular + emissive;
}

void main()
{
    vec3 result = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < lights.length(); i++) {
        result += calculateLighting(lights[i]);
    }
    color = vec4(result, 1.0);
}
