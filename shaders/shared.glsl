
struct Light
{
    vec3 color;
    vec3 position;

    // Attenuation variables
    float constant;
    float linear;
    float quadratic;
};

const int NUM_LIGHTS = 4;

layout (std140, binding = 1) uniform Lights
{
    Light lights[NUM_LIGHTS];
};
