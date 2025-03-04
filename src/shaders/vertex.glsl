#version 460 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 coordinate;

out vec3 fragmentNormal;
out vec3 fragmentPosition;
out vec2 textureCoordinate;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 normalMatrix;

void main() {
  vec4 pos = vec4(position, 1.0);

  fragmentNormal = vec3(normalMatrix * vec4(normal, 1.0));
  fragmentPosition = vec3(model * pos);
  textureCoordinate = coordinate;

  gl_Position = projection * view * model * pos;
}