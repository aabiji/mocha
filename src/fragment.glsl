#version 460 core

in vec2 texCoord;
out vec4 outputColor;

uniform sampler2D texture1;

void main() {
  outputColor = texture(texture1, texCoord);
}