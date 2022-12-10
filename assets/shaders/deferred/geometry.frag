#version 450

layout(location = 0) in vec2 texture_coord;
layout(location = 1) in vec3 vertex_position;
layout(location = 2) in vec3 vertex_normal;

//layout(location = 3) in vec3 tangentLightPos;
//layout(location = 4) in vec3 tangentViewPos;
//layout(location = 5) in vec3 tangentFragPos;


layout(set = 1, binding = 0) uniform sampler2D albedoTexture;
layout(set = 1, binding = 1) uniform sampler2D normalTexture;
layout(set = 1, binding = 2) uniform sampler2D specularTexture;




layout(location = 0) out vec4 out_position;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_color;

void main()
{
    out_position = vec4(vertex_position, 1.0);
    out_normal = vec4(normalize(texture(normalTexture, texture_coord).rgb), 1.0);
    out_color = texture(albedoTexture, texture_coord);
}