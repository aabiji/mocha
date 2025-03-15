#version 460 core

struct Maps
{
    sampler2D ambient;
    sampler2D diffuse;
    sampler2D specular;
    sampler2D normal;
};

in vec3 vertexNormal;
in vec3 fragmentPosition;
in vec3 lightPosition;
in vec3 viewPosition;
in vec2 textureCoordinate;

uniform vec3 lightColor;
uniform Maps material;
uniform bool hasNormalMap;

out vec4 color;

void main()
{
    vec3 lightDirection = normalize(lightPosition - fragmentPosition);
    vec3 viewDirection = normalize(viewPosition - fragmentPosition);

    vec3 normal = vertexNormal;
    if (hasNormalMap) {
        normal = texture(material.normal, textureCoordinate).rgb;
        normal = normalize(normal * 2.0 - 1.0); // map from -1 to 1
    }
    vec3 norm = normalize(normal);

    vec3 ambient = lightColor * texture(material.ambient, textureCoordinate).rgb;

    float diff = max(0.0, dot(norm, lightDirection));
    vec3 diffuse =
        lightColor * (diff * texture(material.diffuse, textureCoordinate).rgb);

    vec3 reflectDirection = reflect(-lightDirection, norm);
    float spec = pow(max(0.0, dot(viewDirection, reflectDirection)), 32);
    vec3 specular =
        lightColor * (spec * texture(material.specular, textureCoordinate).rgb);

    color = vec4(ambient + diffuse + specular, 1.0);
}
