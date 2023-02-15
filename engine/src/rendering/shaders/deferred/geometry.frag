#version 450

layout(location = 0) in vec2 texture_coord;
layout(location = 1) in vec3 vertex_position;
layout(location = 2) in vec3 vertex_normal;
layout(location = 3) in vec3 vertex_tangent;

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;
layout(set = 1, binding = 1) uniform sampler2D normalTexture;
layout(set = 1, binding = 2) uniform sampler2D specularTexture;

layout(location = 0) out vec4 out_position;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_color;
layout(location = 3) out vec4 out_specular;

void main()
{
    out_position = vec4(vertex_position, 1.0);

    out_color = texture(albedoTexture, texture_coord);
    out_specular = texture(specularTexture, texture_coord);   

    // Create the bitangent
    vec3 T = normalize(vertex_tangent);
    vec3 N = normalize(vertex_normal);
    vec3 B = cross(N, T);
    mat3 TBN = mat3(T, B, N);

    vec3 normal_tangent = TBN * normalize(texture(normalTexture, texture_coord).xyz * 2.0 - vec3(1.0));
	out_normal = vec4(N, 1.0);
}