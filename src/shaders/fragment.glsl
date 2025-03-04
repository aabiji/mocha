#version 460 core

struct Material {
  float shininess;
  // Strength of the light for the different types of lighting
  sampler2D diffuse;
  sampler2D specular;
};

struct Light {
  vec3 position;
  // Color of the light for the different types of lighting
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

in vec3 fragmentNormal;
in vec3 fragmentPosition;
in vec2 textureCoordinate;

out vec4 color;

uniform vec3 viewPosition;
uniform Material material;
uniform Light light;

void main() {
  vec3 diffusePoint = texture(material.diffuse, textureCoordinate).rgb;
  vec3 specularPoint = texture(material.specular, textureCoordinate).rgb;

  vec3 ambient = diffusePoint * light.ambient;

  // diffuse lighting:
  // the smaller the angle between the normal and the light direction vector (narrow),
  // the more of an impact the light has (obviously because the object's facing the light more).
  // the bigger the angle between the normal and the light direction vector (wide),
  // the less of an impact the light has (obviously because the object's facing away from the light)
  // we don't need to calculate the normal, it's given to us
  // why wouldn't the calculations work well when we've scaled the lightPosition? --> because
  // we weren't inputting the correct fragment position!
  vec3 normal = normalize(fragmentNormal);
  vec3 lightDirection = normalize(light.position - fragmentPosition);
  float diffuseIntensity = max(dot(normal, lightDirection), 0.0);
  vec3 diffuse = diffuseIntensity * diffusePoint * light.diffuse;

  // specular lighting:
  // take the light direction vector and reflect it around the normal
  // use the dot product between the view direction and the reflected vector as the specular intensity
  vec3 reflected = reflect(-lightDirection, normal);
  vec3 viewDirection = normalize(viewPosition - fragmentPosition);
  float specularIntensity = pow(max(dot(reflected, viewDirection), 0.0), material.shininess);
  vec3 specular = specularIntensity * specularPoint * light.specular;

  color = vec4(ambient + diffuse + specular, 1.0);
}