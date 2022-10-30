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

void main()
{

    // Get pixel colors for each texture map
    vec3 albedo   = texture(albedoTexture, texture_coord).rgb;


    // Global properties
    vec3 lightDir   = normalize(scene.lightPosition - vertex_position);
    vec3 viewDir    = normalize(scene.cameraPosition - vertex_position);
    vec3 reflectDir = reflect(-lightDir, vertex_normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    // ambient
    vec3 ambientResult = scene.ambientStrength * albedo;

    // diffuse
    vec3 diffuseResult = max(dot(lightDir, vertex_normal), 0.0) * albedo;

    // specular
    float spec          = pow(max(dot(vertex_normal, halfwayDir), 0.0), scene.specularShininess);
    vec3 specularResult = scene.specularStrength * spec * scene.lightColor;


    // final result
    vec3 result = (ambientResult + diffuseResult + specularResult);
    final_color = vec4(result, 1.0);
}