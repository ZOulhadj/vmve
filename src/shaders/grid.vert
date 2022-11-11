#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout (location=0) out vec2 uv;


layout(binding = 0) uniform model_view_projection {
    mat4 view;
    mat4 proj;
} mvp;

//const vec3 pos[4] = vec3[4](
//    vec3( 1.0, 0.0, -1.0),
//    vec3(-1.0, 0.0, -1.0),
//    vec3( 1.0, 0.0,  1.0),
//    vec3(-1.0, 0.0,  1.0)
//);
//
//const int indices[6] = int[6](0, 1, 2, 3, 2, 1);

// extents of grid in world coordinates
float gridSize = 200.0;

void main()
{
    vec3 position = position * gridSize;

    gl_Position = mvp.proj * mvp.view * vec4(position, 1.0);
    uv = position.xz;
}