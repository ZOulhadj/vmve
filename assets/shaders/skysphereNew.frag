#version 450

layout(location = 0) in vec2 texture_coord;

layout(location = 0) out vec4 final_color;

layout(binding = 1) uniform sampler2D tex;

void main()
{
    final_color = vec4(texture(tex, texture_coord).rgb, 1.0);
}