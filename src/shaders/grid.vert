#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout (location = 0) out vec2 uv;
layout (location = 1) out float grid_size;

layout(binding = 0) uniform model_view_projection {
    mat4 view;
    mat4 proj;
} mvp;

void main()
{
    grid_size = 500;
    vec3 position = position * grid_size;

    gl_Position = mvp.proj * mvp.view * vec4(position, 1.0);
    uv = position.xz;

}