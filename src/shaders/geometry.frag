#version 450

layout(location = 0) out vec4 final_color;

layout(location = 1) in vec2 texture_coord;
layout(location = 2) in vec3 vertex_position;
layout(location = 3) in vec3 vertex_normal;


layout(binding = 1) uniform scene_ubo {
    float ambientStrength;
    float specularStrength;
    float specularShininess;
    vec3 cameraPosition;
    vec3 lightPosition;
    vec3 lightColor;

} scene;

layout(set = 1, binding = 0) uniform sampler2D tex;

void main() {

    // ambient
    vec3 ambient_lighting = scene.ambientStrength * scene.lightColor;


    // diffuse
    vec3 norm = normalize(vertex_normal);
    vec3 sun_dir = normalize(scene.lightPosition - vertex_position);
    float diff = max(dot(norm, sun_dir), 0.0);
    vec3 diffuse = diff * scene.lightColor;


    // specular
    vec3 view_dir = normalize(scene.cameraPosition - vertex_position);
    vec3 reflect_dir = reflect(-sun_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), scene.specularShininess);
    vec3 specular = scene.specularStrength * spec * scene.lightColor;


    // final result
    vec3 texel = texture(tex, texture_coord).xyz;
    vec3 color = (ambient_lighting + diffuse + specular) * texel;
    final_color = vec4(color, 1.0);
}