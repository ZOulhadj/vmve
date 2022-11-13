#version 450

layout(location = 0) in vec2 texture_coord;

layout(location = 0) out vec4 final_color;

layout(set = 1, binding = 0) uniform sampler2D tex;

void main()
{
    vec3 texel = texture(tex, texture_coord).xyz;
    final_color = vec4(texel, 1.0);
}