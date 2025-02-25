#version 460 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 coordinate;

out vec3 fragmentColor;
out vec2 texCoord;

uniform mat4 transform;

void main() {
  texCoord = coordinate;
  gl_Position = transform * vec4(position.x, position.y, position.z, 1);
}
