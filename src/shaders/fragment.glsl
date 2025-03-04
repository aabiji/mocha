#version 460 core

struct Material {
  float shininess;
  // Strength of the light for the different types of lighting
  sampler2D diffuse;
  sampler2D specular;
};

struct Light {
  // Directional lights illuminate the entire scene.
  // it's pointing AWAY from the light source
  vec3 direction;
  vec3 position;

  // Attenuation equation values for point light
  float constant;
  float linear;
  float quadratic;

  // Spotlight
  vec3 spotlightPosition;
  vec3 spotlightDirection;
  float cutoff;
  float outerCutoff;

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
  vec3 lightDirection = normalize(-light.direction); // make the vector point TOWARDS the light source

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
  float diffuseIntensity = max(dot(normal, lightDirection), 0.0);
  vec3 diffuse = diffuseIntensity * diffusePoint * light.diffuse;

  // specular lighting:
  // take the light direction vector and reflect it around the normal
  // use the dot product between the view direction and the reflected vector as the specular intensity
  vec3 reflected = reflect(-lightDirection, normal);
  vec3 viewDirection = normalize(viewPosition - fragmentPosition);
  float specularIntensity = pow(max(dot(reflected, viewDirection), 0.0), material.shininess);
  vec3 specular = specularIntensity * specularPoint * light.specular;

  // Point light: A light that gets dimmer the further away you are from it
  // There's an equation you can use to get pretty goo dresults -- you just need to play around with the constants
  float dist = length(light.position - fragmentPosition);
  float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * (dist * dist));
  ambient *= attenuation;
  diffuse *= attenuation;
  specular *= attenuation;

/* // TODO: add spotlight when adding multiple light sources
  // Spotlight: A point light with a fixed illumination radius
  // Objects that are not inside the spotlight radius are not affected by it.
  // using < not > since for example, cos(180) < cos(0) -- so the cutoff angle
  // should still be bigger than theta, the comparison order is just swapped
  float theta = dot(lightDirection, normalize(-light.direction));
  // Interpolating between the outer cutoff and inner cutoff to get a softness around the borders of the spotlight
  float epsilon = light.cutoff - light.outerCutoff;
  float intensity = min(max(0.0, (theta - light.outerCutoff) / epsilon), 1.0);
  if (light.cutoff < theta) {
    diffuse *= intensity;
    specular *= intensity;
  }
*/

  color = vec4(ambient + diffuse + specular, 1.0);
}