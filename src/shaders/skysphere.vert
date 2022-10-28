#version 450

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 uv;

layout(location = 1) out vec2 texture_coord;

layout(binding = 0) uniform model_view_projection
{
    mat4 view;
    mat4 proj;
} mvp;

void main()
{
    gl_Position = mvp.proj * mat4(mat3(mvp.view)) * vec4(position, 1.0);
    texture_coord = uv;
}