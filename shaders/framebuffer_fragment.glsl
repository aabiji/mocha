#version 460 core

flat in uvec4 modelIdInput;

layout(location = 0) out uvec4 modelIdOutput;

void main()
{
    modelIdOutput = modelIdInput;
}