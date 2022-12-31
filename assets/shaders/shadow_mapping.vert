#version 450

layout(location = 0) in vec3 position;

layout(binding = 0) uniform SunData
{
    mat4 viewProj;
} sun;


layout(push_constant) uniform constant
{
    mat4 model;
} obj;

void main()
{
    gl_Position = sun.viewProj * obj.model * vec4(position, 1.0);
}