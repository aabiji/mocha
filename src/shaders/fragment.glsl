#version 460 core

in vec3 fragmentNormal;
in vec3 fragmentPosition;

out vec4 color;

uniform vec3 lightPosition;
uniform vec3 viewPosition;
uniform vec3 lightColor;
uniform vec3 objectColor;

void main() {
  float ambientStrength = 0.1;
  vec3 ambient = ambientStrength * lightColor;

  // diffuse lighting:
  // the smaller the angle between the normal and the light direction vector (narrow),
  // the more of an impact the light has (obviously because the object's facing the light more).
  // the bigger the angle between the normal and the light direction vector (wide),
  // the less of an impact the light has (obviously because the object's facing away from the light)
  // we don't need to calculate the normal, it's given to us
  // why wouldn't the calculations work well when we've scaled the lightPosition? --> because
  // we weren't inputting the correct fragment position!
  vec3 normal = normalize(fragmentNormal);
  vec3 lightDirection = normalize(lightPosition - fragmentPosition);
  float diffuseIntensity = max(dot(normal, lightDirection), 0.0);
  vec3 diffuse = diffuseIntensity * lightColor;

  // specular lighting:
  // take the light direction vector and reflect it around the normal
  // use the dot product between the view direction and the reflected vector as the specular intensity
  float shininess = 32, strength = 0.5;
  vec3 reflected = reflect(-lightDirection, normal);
  vec3 viewDirection = normalize(viewPosition - fragmentPosition);
  float specularIntensity = pow(max(dot(reflected, viewDirection), 0.0), shininess);
  vec3 specular = strength * specularIntensity * lightColor;

  color = vec4((ambient + diffuse + specular) * objectColor, 1.0);
}