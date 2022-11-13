#version 450

layout(location = 0) out vec4 final_color;

layout(location = 0) in vec2 texture_coord;
layout(location = 1) in vec3 vertex_position;
layout(location = 2) in vec3 vertex_normal;

layout(binding = 1) uniform scene_ubo
{
    float ambientStrength;
    float specularStrength;
    float specularShininess;
    vec3 cameraPosition;
    vec3 lightPosition;
    vec3 lightColor;
} scene;

layout(set = 1, binding = 0) uniform sampler2D albedoTexture;



vec3 direction_lighting(vec3 object_color, vec3 position, vec3 normal)
{
    // ambient
    vec3 ambientResult = scene.ambientStrength * object_color;

    // diffuse
    vec3 lightDir   = normalize(scene.lightPosition - position);
    float difference = max(dot(normal, lightDir), 0.0);
    vec3 diffuseResult = difference * object_color * scene.lightColor;

    // specular
    vec3 viewDir    = normalize(scene.cameraPosition - position);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec          = pow(max(dot(normal, reflectDir), 0.0), scene.specularShininess);
    vec3 specularResult = scene.specularStrength * spec * scene.lightColor;

    return ambientResult + diffuseResult + specularResult;
}



void main()
{
    vec3 norm = normalize(vertex_normal);

    vec3 albedo   = texture(albedoTexture, texture_coord).rgb;

    vec3 result = direction_lighting(albedo, vertex_position, norm);

    final_color = vec4(result, 1.0);
}