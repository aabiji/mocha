#version 460 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 coordinate;

out vec3 fragmentColor;
out vec2 texCoord;

void main() {
  fragmentColor = color;
  texCoord = coordinate;
  gl_Position = vec4(position.x, position.y, position.z, 1);
}
