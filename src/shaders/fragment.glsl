#version 460 core

in vec2 texCoord;
out vec4 outputColor;

uniform vec4 lightColor;
uniform vec4 objectColor;

uniform sampler2D texture1;

void main() {
  outputColor = lightColor * objectColor * texture(texture1, texCoord);
}