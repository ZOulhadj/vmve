#version 450

layout(location = 0) in vec3 position;

layout(binding = 0) uniform light_data 
{
    mat4 lightViewMatrix;
} light;


layout(push_constant) uniform constant
{
    mat4 model;
} obj;

void main()
{
    gl_Position = light.lightViewMatrix * obj.model * vec4(position, 1.0);
}