#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;

layout(location = 0) out vec2 texture_coord;
layout(location = 1) out vec3 vertex_position;
layout(location = 2) out vec3 vertex_normal;
layout(location = 3) out vec3 vertex_tangent;

//layout(location = 3) out vec3 tangentLightPos;
//layout(location = 4) out vec3 tangentViewPos;
//layout(location = 5) out vec3 tangentFragPos;
//

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
    // Transform vertex from model to projection space
    // Model space -> World Space -> View Space -> Projection Space
    gl_Position = mvp.proj * mvp.view * obj.model * vec4(position, 1.0);
   
    vertex_position = vec3(obj.model * vec4(position, 1.0));

    texture_coord = uv;

    // Calculate TBN matrix required for normal mapping
    mat3 M = transpose(inverse(mat3(obj.model)));
    vertex_tangent = M * normalize(tangent);
    vertex_normal = M * normalize(normal);
}