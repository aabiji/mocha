
struct Light
{
    vec3 color;
    vec3 position; // In world space
    float c, l, q; // Constant, linear, quadratic (attenuation value)
};

const int NUM_LIGHTS = 3;

layout (std140, binding = 1) uniform Lights
{
    Light lights[NUM_LIGHTS];
};
