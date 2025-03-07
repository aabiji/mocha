#version 460 core

struct Material {
  float shininess;
  // Strength of the light for the different types of lighting
  sampler2D diffuse;
  sampler2D specular;
};

struct DirectionalLight {
  // Direction all the light rays are pointing towards
  vec3 direction;
  // Color of the light for the different types of lighting
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

struct PointLight {
  vec3 position;

  // Color of the light for the different types of lighting
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

  // Values for the attenuation equation
  float constant;
  float linear;
  float quadratic;
};

struct SpotLight {
  vec3 position;
  vec3 direction; // pointing towards
  float cutoff; // inner illumination area
  float outerCutoff; // outer illumination area

  // Values for the attenuation equation
  float constant;
  float linear;
  float quadratic;

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
uniform DirectionalLight dirlight;
uniform PointLight pointlight;
uniform SpotLight spotlight;

vec3 computeDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDirection) {
  vec3 diffusePoint = texture(material.diffuse, textureCoordinate).rgb;
  vec3 specularPoint = texture(material.specular, textureCoordinate).rgb;

  vec3 lightDirection = normalize(-light.direction); // make the vector point TOWARDS the light source
  vec3 ambient = light.ambient * diffusePoint;

  float diffuseIntensity = max(dot(normal, lightDirection), 0.0);
  vec3 diffuse = diffuseIntensity * diffusePoint * light.diffuse;

  vec3 reflected = reflect(-lightDirection, normal);
  float specularIntensity = pow(max(dot(reflected, viewDirection), 0.0), material.shininess);
  vec3 specular = specularIntensity * specularPoint * light.specular;

  return ambient + diffuse + specular;
}

vec3 computePointLight(PointLight light, vec3 normal, vec3 viewDirection) {
  vec3 diffusePoint = texture(material.diffuse, textureCoordinate).rgb;
  vec3 specularPoint = texture(material.specular, textureCoordinate).rgb;

  vec3 lightDirection = normalize(light.position - fragmentPosition);
  vec3 ambient = light.ambient * diffusePoint;

  float diffuseIntensity = max(dot(normal, lightDirection), 0.0);
  vec3 diffuse = diffuseIntensity * diffusePoint * light.diffuse;

  vec3 reflected = reflect(-lightDirection, normal);
  float specularIntensity = pow(max(dot(reflected, viewDirection), 0.0), material.shininess);
  vec3 specular = specularIntensity * specularPoint * light.specular;

  float dist = length(light.position - fragmentPosition);
  float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist));
  ambient *= attenuation;
  diffuse *= attenuation;
  specular *= attenuation;

  return ambient + diffuse + specular;
}

vec3 computeSpotLight(SpotLight light, vec3 normal, vec3 viewDirection) {
  vec3 diffusePoint = texture(material.diffuse, textureCoordinate).rgb;
  vec3 specularPoint = texture(material.specular, textureCoordinate).rgb;

  vec3 lightDirection = normalize(light.position - fragmentPosition);
  vec3 ambient = light.ambient * diffusePoint;

  float diffuseIntensity = max(dot(normal, lightDirection), 0.0);
  vec3 diffuse = diffuseIntensity * diffusePoint * light.diffuse;

  vec3 reflected = reflect(-lightDirection, normal);
  float specularIntensity = pow(max(dot(reflected, viewDirection), 0.0), material.shininess);
  vec3 specular = specularIntensity * specularPoint * light.specular;

  float angle = dot(normalize(light.direction), light.direction);
  float intensity = (angle - light.outerCutoff) / (light.outerCutoff - light.cutoff);
  intensity = clamp(intensity, 0.0, 1.0);

  float dist = length(light.position - fragmentPosition);
  float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist));
  ambient *= attenuation * intensity;
  diffuse *= attenuation * intensity;
  specular *= attenuation * intensity;

  return ambient + diffuse + specular;
}

void main() {
  vec3 normal = normalize(fragmentNormal);
  vec3 viewDirection = normalize(viewPosition - fragmentPosition);

  // basic example
  vec3 directional = computeDirectionalLight(dirlight, normal, viewDirection);
  vec3 point = computePointLight(pointlight, normal, viewDirection);
  vec3 spot = computeSpotLight(spotlight, normal, viewDirection);
  color = vec4(directional + point + spot, 1.0);
}
