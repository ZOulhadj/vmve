#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 biTangent;

layout(location = 1) out vec2 texture_coord;
layout(location = 2) out vec3 vertex_position;
//layout(location = 3) out vec3 vertex_normal;

layout(location = 4) out vec3 tangentLightPos;
layout(location = 5) out vec3 tangentViewPos;
layout(location = 6) out vec3 tangentFragPos;

layout(binding = 0) uniform model_view_projection {
    mat4 view;
    mat4 proj;
} mvp;

layout(binding = 1) uniform scene_ubo
{
    float ambientStrength;
    float specularStrength;
    float specularShininess;
    vec3 cameraPosition;
    vec3 lightPosition;
    vec3 lightColor;
} scene;

layout(push_constant) uniform constant
{
    mat4 model;
} obj;

void main() {

    texture_coord = uv;
    vertex_position = vec3(obj.model * vec4(position, 1.0));
    mat3 normalMatrix = transpose(inverse(mat3(obj.model)));
    vec3 T = normalize(normalMatrix * tangent);
    vec3 N = normalize(normalMatrix * normal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    mat3 TBN = transpose(mat3(T, B, N));
    tangentLightPos = TBN * scene.lightPosition;
    tangentViewPos  = TBN * scene.cameraPosition;
    tangentFragPos = TBN * vertex_position;

    gl_Position = mvp.proj * mvp.view * obj.model * vec4(position, 1.0);
}