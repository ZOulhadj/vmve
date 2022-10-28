#version 450

layout(location = 0) out vec4 final_color;

layout(location = 1) in vec2 texture_coord;
layout(location = 2) in vec3 vertex_position;
layout(location = 3) in vec3 vertex_normal;


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
layout(set = 1, binding = 1) uniform sampler2D normalTexture;
layout(set = 1, binding = 2) uniform sampler2D specularTexture;

void main()
{

    // Get pixel colors for each texture map
    vec3 albedo   = texture(albedoTexture, texture_coord).rgb;
    vec3 normal   = texture(normalTexture, texture_coord).rgb;
    vec3 specular = texture(specularTexture, texture_coord).rgb;


    // Global properties
    vec3 normalized_normal = normalize(normal);
    vec3 lightDir   = normalize(scene.lightPosition - vertex_position);



    // ambient
    vec3 ambientResult = albedo * scene.lightColor * 0.05;

    // diffuse
    float diff = max(dot(normalized_normal, lightDir), 0.0);
    vec3 diffuseResult = albedo * diff * scene.lightColor;


    // specular
    vec3 view_dir  = normalize(scene.cameraPosition - vertex_position);
    vec3 reflectDir = reflect(-lightDir, normalized_normal);
    float spec     = pow(max(dot(view_dir, reflectDir), 0.0), scene.specularShininess);
    vec3 specularResult = spec * specular * scene.lightColor;


    // final result
    vec3 result = (ambientResult + diffuseResult + specularResult);
    final_color = vec4(result, 1.0);
}