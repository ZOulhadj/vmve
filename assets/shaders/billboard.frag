#version 450

layout(location = 0) out vec4 final_color;

layout(location = 0) in vec2 texture_coord;
layout(location = 1) in vec3 vertex_position;

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;

void main()
{
    vec4 albedo = texture(albedoTexture, texture_coord).rgba;

    if (albedo.a < 0.2) {
        discard;
    }

    final_color = vec4(albedo.rgb, 1.0);
}