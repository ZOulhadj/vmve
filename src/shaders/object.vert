#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec2 texture_coord;
layout(location = 1) out vec3 vertex_position;
layout(location = 2) out vec3 vertex_normal;

layout(binding = 0) uniform model_view_projection {
    mat4 view;
    mat4 proj;
} mvp;


layout(push_constant) uniform constant
{
    mat4 model;
} obj;

void main()
{
    texture_coord = uv;
    vertex_position = vec3(obj.model * vec4(position, 1.0));
    vertex_normal = normal;

    gl_Position = mvp.proj * mvp.view * obj.model * vec4(position, 1.0);
}