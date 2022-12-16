#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 biTangent;

layout(location = 0) out vec2 texture_coord;
layout(location = 1) out vec3 vertex_position;
layout(location = 2) out vec3 vertex_normal;
layout(location = 3) out vec3 vertex_tangent;

layout(binding = 0) uniform model_view_projection
{
    mat4 view;
    mat4 proj;
} mvp;

void main()
{

    vertex_position = vec3(mat4(1.0) * vec4(position, 1.0));
    texture_coord   = uv;

    mat3 M = transpose(inverse(mat3(mat4(1.0))));
    vertex_normal = M * normalize(normal);
    vertex_tangent = M * normalize(tangent);

    gl_Position = mvp.proj * mat4(mat3(mvp.view)) * vec4(position, 1.0);
}