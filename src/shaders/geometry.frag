#version 450

layout(location = 0) out vec4 final_color;

layout(location = 1) in vec2 texture_coord;
layout(location = 2) in vec3 vertex_position;
layout(location = 3) in vec3 vertex_normal;


layout(binding = 1) uniform scene_ubo {
    vec3 cam_pos;
    vec3 sun_pos;
    vec3 sun_color;
} scene;

layout(set = 1, binding = 0) uniform sampler2D tex;

void main() {
    // phong lighting = ambient + diffuse + specular

    float ambient_intensity = 0.05f;
    vec3 ambient_lighting = ambient_intensity * scene.sun_color;

    vec3 norm = normalize(vertex_normal);
    vec3 sun_dir = normalize(scene.sun_pos - vertex_position);
    float diff = max(dot(norm, sun_dir), 0.0);
    vec3 diffuse = diff * scene.sun_color;

    float specular_intensity = 0.5f;
    vec3 view_dir = normalize(scene.cam_pos - vertex_position);
    vec3 reflect_dir = reflect(-sun_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
    vec3 specular = specular_intensity * spec * scene.sun_color;

    vec3 texel = texture(tex, texture_coord).xyz;
    vec3 color = (ambient_lighting + diffuse + specular) * texel;
    final_color = vec4(color, 1.0);
}