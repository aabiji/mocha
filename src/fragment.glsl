#version 460 core

in vec3 fragmentColor;
in vec2 texCoord;
out vec4 outputColor;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main() {
  outputColor = texture(texture2, texCoord);
}