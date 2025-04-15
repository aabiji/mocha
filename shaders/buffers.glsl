
struct Light
{
    vec3 color;
    vec3 position; // In world space
    float c, l, q; // Constant, linear, quadratic (attenuation value)
};

layout (std430, binding = 0) readonly buffer Lights
{
    Light lights[];
};

layout(std430, binding = 1) readonly buffer ModelTransforms
{
    mat4 model;
    mat4 meshTransform;
    mat4 boneTransforms[];
};

layout(std430, binding = 2) readonly buffer MVPTransforms
{
    mat4 view;
    mat4 projection;
    vec3 viewPosition;
};
