#version 450

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_normal;

layout (location = 0) out vec3 position;
layout (location = 1) out vec3 normal;
layout (location = 2) out vec2 uv;

layout (location = 3) out float grid_size;

layout(binding = 0) uniform model_view_projection {
    mat4 view;
    mat4 proj;
} mvp;

void main()
{
    grid_size = 1000;

    position = vertex_position * grid_size;
    uv = position.xz;
    normal = vertex_normal;


    gl_Position = mvp.proj * mvp.view * vec4(position, 1.0);
}